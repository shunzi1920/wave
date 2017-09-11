// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "RHI.h"
#include "RHICommandList.h"

DECLARE_CYCLE_STAT(TEXT("Nonimmed. Command List Execute"), STAT_NonImmedCmdListExecuteTime, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nonimmed. Command List memory"), STAT_NonImmedCmdListMemory, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nonimmed. Command count"), STAT_NonImmedCmdListCount, STATGROUP_RHICMDLIST);

DECLARE_CYCLE_STAT(TEXT("All Command List Execute"), STAT_ImmedCmdListExecuteTime, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Immed. Command List memory"), STAT_ImmedCmdListMemory, STATGROUP_RHICMDLIST);
DECLARE_DWORD_COUNTER_STAT(TEXT("Immed. Command count"), STAT_ImmedCmdListCount, STATGROUP_RHICMDLIST);

#if !PLATFORM_USES_FIXED_RHI_CLASS
#include "RHICommandListCommandExecutes.inl"
#endif


static TAutoConsoleVariable<int32> CVarRHICmdBypass(
	TEXT("r.RHICmdBypass"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("Whether to bypass the rhi command list and send the rhi commands immediately.\n")
	TEXT("0: Disable (required for the multithreaded renderer)\n")
	TEXT("1: Enable (convenient for debugging low level graphics API calls, can supress artifacts from multithreaded renderer code)"));

static TAutoConsoleVariable<int32> CVarRHICmdUseParallelAlgorithms(
	TEXT("r.RHICmdUseParallelAlgorithms"),
	1,
	TEXT("True to use parallel algorithms. Ignored if r.RHICmdBypass is 1."));

TAutoConsoleVariable<int32> CVarRHICmdWidth(
	TEXT("r.RHICmdWidth"), 
	8,
	TEXT("Controls the task granularity of a great number of things in the parallel renderer."));

static TAutoConsoleVariable<int32> CVarRHICmdUseDeferredContexts(
	TEXT("r.RHICmdUseDeferredContexts"),
	1,
	TEXT("True to use deferred contexts to parallelize command list execution. Only available on some RHIs."));

TAutoConsoleVariable<int32> CVarRHICmdFlushRenderThreadTasks(
	TEXT("r.RHICmdFlushRenderThreadTasks"),
	0,
	TEXT("If true, then we flush the render thread tasks every pass. For issue diagnosis. This is a master switch for more granular cvars."));

TAutoConsoleVariable<int32> CVarRHICmdFlushUpdateTextureReference(
	TEXT("r.RHICmdFlushUpdateTextureReference"),
	0,
	TEXT("If true, then we flush the rhi thread when we do RHIUpdateTextureReference, otherwise this is deferred. For issue diagnosis."));

static TAutoConsoleVariable<int32> CVarRHICmdFlushOnQueueParallelSubmit(
	TEXT("r.RHICmdFlushOnQueueParallelSubmit"),
	0,
	TEXT("Wait for completion of parallel commandlists immediately after submitting. For issue diagnosis. Only available on some RHIs."));

static TAutoConsoleVariable<int32> CVarRHICmdMergeSmallDeferredContexts(
	TEXT("r.RHICmdMergeSmallDeferredContexts"),
	1,
	TEXT("When it can be determined, merge small parallel translate tasks based on r.RHICmdMinDrawsPerParallelCmdList."));

static TAutoConsoleVariable<int32> CVarRHICmdBufferWriteLocks(
	TEXT("r.RHICmdBufferWriteLocks"),
	1,
	TEXT("Only relevant with an RHI thread. Debugging option to diagnose problems with buffered locks."));

static TAutoConsoleVariable<int32> CVarRHICmdAsyncRHIThreadDispatch(
	TEXT("r.RHICmdAsyncRHIThreadDispatch"),
	0,
	TEXT("Experiemental option to do RHI dispatches async. Currently deadlocks. This causes a greater 'wait for rhi thread' penalty, but keeps data flowing to the RHI thread faster."));

static TAutoConsoleVariable<int32> CVarRHICmdCollectRHIThreadStatsFromHighLevel(
	TEXT("r.RHICmdCollectRHIThreadStatsFromHighLevel"),
	1,
	TEXT("This pushes stats on the RHI thread executes so you can determine which high level pass they came from. This has an adverse effect on framerate. This is on by default."));

static TAutoConsoleVariable<int32> CVarRHICmdUseThread(
	TEXT("r.RHICmdUseThread"),
	1,
	TEXT("Uses the RHI thread. For issue diagnosis."));

static TAutoConsoleVariable<int32> CVarRHICmdForceRHIFlush(
	TEXT("r.RHICmdForceRHIFlush"),
	0,
	TEXT("Force a flush for every task sent to the RHI thread. For issue diagnosis."));

static TAutoConsoleVariable<int32> CVarRHICmdBalanceTranslatesAfterTasks(
	TEXT("r.RHICmdBalanceTranslatesAfterTasks"),
	0,
	TEXT("Experimental option to balance the parallel translates after the render tasks are complete. This minimizes the number of deferred contexts, but adds latency to starting the translates. r.RHICmdBalanceParallelLists should be off with this option."));

static TAutoConsoleVariable<int32> CVarRHICmdMinCmdlistForParallelTranslate(
	TEXT("r.RHICmdMinCmdlistForParallelTranslate"),
	2,
	TEXT("If there are fewer than this number of parallel translates, they just run on the RHI thread and immediate context. Only relevant if r.RHICmdBalanceTranslatesAfterTasks is on."));

static TAutoConsoleVariable<int32> CVarRHICmdMinCmdlistSizeForParallelTranslate(
	TEXT("r.RHICmdMinCmdlistSizeForParallelTranslate"),
	32,
	TEXT("In kilobytes. Cmdlists are merged into one parallel translate until we have at least this much memory to process. For a given pass, we won't do more translates than we have task threads. Only relevant if r.RHICmdBalanceTranslatesAfterTasks is on."));


RHI_API FRHICommandListExecutor GRHICommandList;

static FGraphEventArray AllOutstandingTasks;
static FGraphEventArray WaitOutstandingTasks;
static FGraphEventRef RHIThreadTask;
static FGraphEventRef RenderThreadSublistDispatchTask;
static FGraphEventRef RHIThreadBufferLockFence;

// Used by AsyncCompute
RHI_API FRHICommandListFenceAllocator GRHIFenceAllocator;

DECLARE_CYCLE_STAT(TEXT("RHI Thread Execute"), STAT_RHIThreadExecute, STATGROUP_RHICMDLIST);

static TStatId GCurrentExecuteStat;

struct FRHICommandStat : public FRHICommand<FRHICommandStat>
{
	TStatId CurrentExecuteStat;
	FORCEINLINE_DEBUGGABLE FRHICommandStat(TStatId InCurrentExecuteStat)
		: CurrentExecuteStat(InCurrentExecuteStat)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		GCurrentExecuteStat = CurrentExecuteStat;
	}
};

void FRHICommandListImmediate::SetCurrentStat(TStatId Stat)
{
	new (AllocCommand<FRHICommandStat>()) FRHICommandStat(Stat);
}

DECLARE_CYCLE_STAT(TEXT("FNullGraphTask.RenderThreadTaskFence"), STAT_RenderThreadTaskFence, STATGROUP_TaskGraphTasks);
DECLARE_CYCLE_STAT(TEXT("Render thread task fence wait"), STAT_RenderThreadTaskFenceWait, STATGROUP_TaskGraphTasks);
FGraphEventRef FRHICommandListImmediate::RenderThreadTaskFence()
{
	FGraphEventRef Result;
	check(IsInRenderingThread());
	//@todo optimize, if there is only one outstanding, then return that instead
	if (WaitOutstandingTasks.Num())
	{
		Result = TGraphTask<FNullGraphTask>::CreateTask(&WaitOutstandingTasks, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(GET_STATID(STAT_RenderThreadTaskFence), ENamedThreads::RenderThread_Local);
	}
	return Result;
}
void FRHICommandListImmediate::WaitOnRenderThreadTaskFence(FGraphEventRef& Fence)
{
	if (Fence.GetReference() && !Fence->IsComplete())
	{
		SCOPE_CYCLE_COUNTER(STAT_RenderThreadTaskFenceWait);
		check(IsInRenderingThread() && !FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local));
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(Fence, ENamedThreads::RenderThread_Local);
	}
}

bool FRHICommandListImmediate::AnyRenderThreadTasksOutstanding()
{
	return !!WaitOutstandingTasks.Num();
}


FRHICommandBase* GCurrentCommand = nullptr;

void FRHICommandListExecutor::ExecuteInner_DoExecute(FRHICommandListBase& CmdList)
{
	CmdList.bExecuting = true;
	check(CmdList.Context);

	FRHICommandListIterator Iter(CmdList);
#if STATS
	bool bDoStats =  CVarRHICmdCollectRHIThreadStatsFromHighLevel.GetValueOnRenderThread() > 0 && FThreadStats::IsCollectingData() && (IsInRenderingThread() || IsInRHIThread());
	if (bDoStats)
	{
		while (Iter.HasCommandsLeft())
		{
			TStatIdData const* Stat = GCurrentExecuteStat.GetRawPointer();
			FScopeCycleCounter Scope(GCurrentExecuteStat);
			while (Iter.HasCommandsLeft() && Stat == GCurrentExecuteStat.GetRawPointer())
			{
			FRHICommandBase* Cmd = Iter.NextCommand();
			//FPlatformMisc::Prefetch(Cmd->Next);
			Cmd->CallExecuteAndDestruct(CmdList);
		}
	}
	}
	else
#endif
	{
		while (Iter.HasCommandsLeft())
		{
			FRHICommandBase* Cmd = Iter.NextCommand();
			GCurrentCommand = Cmd;
			//FPlatformMisc::Prefetch(Cmd->Next);
			Cmd->CallExecuteAndDestruct(CmdList);
		}
	}
	CmdList.Reset();
}



class FExecuteRHIThreadTask
{
	FRHICommandListBase* RHICmdList;

public:

	FExecuteRHIThreadTask(FRHICommandListBase* InRHICmdList)
		: RHICmdList(InRHICmdList)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FExecuteRHIThreadTask, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		check(GRHIThread); // this should never be used on a platform that doesn't support the RHI thread
		return ENamedThreads::RHIThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SCOPE_CYCLE_COUNTER(STAT_RHIThreadExecute);
		FRHICommandListExecutor::ExecuteInner_DoExecute(*RHICmdList);
		delete RHICmdList;
	}
};

class FDispatchRHIThreadTask
{
	FRHICommandListBase* RHICmdList;
	bool bAnyThread;

public:

	FDispatchRHIThreadTask(FRHICommandListBase* InRHICmdList, bool bInAnyThread)
		: RHICmdList(InRHICmdList)
		, bAnyThread(bInAnyThread)
	{		
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FDispatchRHIThreadTask, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		check(GRHIThread); // this should never be used on a platform that doesn't support the RHI thread
		return bAnyThread ? ENamedThreads::AnyThread : ENamedThreads::RenderThread_Local;
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		check(bAnyThread || IsInRenderingThread());
		FGraphEventArray Prereq;
		if (RHIThreadTask.GetReference())
		{
			Prereq.Add(RHIThreadTask);
		}
		RHIThreadTask = TGraphTask<FExecuteRHIThreadTask>::CreateTask(&Prereq, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(RHICmdList);
	}
};



void FRHICommandListExecutor::ExecuteInner(FRHICommandListBase& CmdList)
{
	check(CmdList.RTTasks.Num() == 0 && CmdList.HasCommands()); 

	bool bIsInRenderingThread = IsInRenderingThread();
	bool bIsInGameThread = IsInGameThread();
	if (GRHIThread)
	{
		bool bAsyncSubmit = false;
		if (bIsInRenderingThread)
		{
			if (!bIsInGameThread && !FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandListExecutor_ExecuteInner_DoTasksBeforeDispatch);
				// move anything down the pipe that needs to go
				FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::RenderThread_Local);
			}
			bAsyncSubmit = CVarRHICmdAsyncRHIThreadDispatch.GetValueOnRenderThread() > 0;
			if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
			{
				RenderThreadSublistDispatchTask = nullptr;
				if (bAsyncSubmit && RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
			{
				RHIThreadTask = nullptr;
			}
			}
			if (!bAsyncSubmit && RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
			{
				RHIThreadTask = nullptr;
			}
		}
		if (CVarRHICmdUseThread.GetValueOnRenderThread() > 0 && bIsInRenderingThread && !bIsInGameThread)
		{
			FRHICommandList* SwapCmdList;
		{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandListExecutor_SwapCmdLists);
				SwapCmdList = new FRHICommandList;

			// Super scary stuff here, but we just want the swap command list to inherit everything and leave the immediate command list wiped.
			// we should make command lists virtual and transfer ownership rather than this devious approach
			static_assert(sizeof(FRHICommandList) == sizeof(FRHICommandListImmediate), "We are memswapping FRHICommandList and FRHICommandListImmediate; they need to be swappable.");
			SwapCmdList->ExchangeCmdList(CmdList);
			}
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandListExecutor_SubmitTasks);

			//if we use a FDispatchRHIThreadTask, we must have it pass an event along to the FExecuteRHIThreadTask it will spawn so that fences can know which event to wait on for execution completion
			//before the dispatch completes.
			//if we use a FExecuteRHIThreadTask directly we pass the same event just to keep things consistent.
			if (AllOutstandingTasks.Num() || RenderThreadSublistDispatchTask.GetReference())
			{
				FGraphEventArray Prereq(AllOutstandingTasks);
				AllOutstandingTasks.Reset();
				if (RenderThreadSublistDispatchTask.GetReference())
				{
					Prereq.Add(RenderThreadSublistDispatchTask);
				}
				RenderThreadSublistDispatchTask = TGraphTask<FDispatchRHIThreadTask>::CreateTask(&Prereq, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(SwapCmdList, bAsyncSubmit);
			}
			else
			{
				check(!RenderThreadSublistDispatchTask.GetReference()); // if we are doing submits, there better not be any of these in flight since then the RHIThreadTask would get out of order.
				FGraphEventArray Prereq;
				if (RHIThreadTask.GetReference())
				{
					Prereq.Add(RHIThreadTask);
				}
				RHIThreadTask = TGraphTask<FExecuteRHIThreadTask>::CreateTask(&Prereq, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(SwapCmdList);
			}
			if (CVarRHICmdForceRHIFlush.GetValueOnRenderThread() > 0 )
			{
				if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
				{
					// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
					UE_LOG(LogRHI, Fatal, TEXT("Deadlock in FRHICommandListExecutor::ExecuteInner 2."));
				}
				if (RenderThreadSublistDispatchTask.GetReference())
				{
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(RenderThreadSublistDispatchTask, ENamedThreads::RenderThread_Local);
					RenderThreadSublistDispatchTask = nullptr;
				}
				while (RHIThreadTask.GetReference())
				{
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(RHIThreadTask, ENamedThreads::RenderThread_Local);
					if (RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
					{
						RHIThreadTask = nullptr;
					}
				}
			}
			return;
		}
		if (bIsInRenderingThread)
		{
			if (RenderThreadSublistDispatchTask.GetReference())
			{
				if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
				{
					// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
					UE_LOG(LogRHI, Fatal, TEXT("Deadlock in FRHICommandListExecutor::ExecuteInner 3."));
				}
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(RenderThreadSublistDispatchTask, ENamedThreads::RenderThread_Local);
				RenderThreadSublistDispatchTask = nullptr;
			}
			while (RHIThreadTask.GetReference())
			{
				if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
				{
					// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
					UE_LOG(LogRHI, Fatal, TEXT("Deadlock in FRHICommandListExecutor::ExecuteInner 3."));
				}
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(RHIThreadTask, ENamedThreads::RenderThread_Local);
				if (RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
				{
					RHIThreadTask = nullptr;
				}
			}
		}
	}

	ExecuteInner_DoExecute(CmdList);
}


static FORCEINLINE bool IsInRenderingOrRHIThread()
{
	return IsInRenderingThread() || IsInRHIThread();
}

void FRHICommandListExecutor::ExecuteList(FRHICommandListBase& CmdList)
{
	check(&CmdList != &GetImmediateCommandList() && (GRHISupportsParallelRHIExecute || IsInRenderingOrRHIThread()));

	if (IsInRenderingThread() && !GetImmediateCommandList().IsExecuting()) // don't flush if this is a recursive call and we are already executing the immediate command list
	{
		GetImmediateCommandList().Flush();
	}

	INC_MEMORY_STAT_BY(STAT_NonImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_NonImmedCmdListCount, CmdList.NumCommands);

	SCOPE_CYCLE_COUNTER(STAT_NonImmedCmdListExecuteTime);
	ExecuteInner(CmdList);
}

void FRHICommandListExecutor::ExecuteList(FRHICommandListImmediate& CmdList)
{
	check(IsInRenderingOrRHIThread() && &CmdList == &GetImmediateCommandList());

	INC_MEMORY_STAT_BY(STAT_ImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_ImmedCmdListCount, CmdList.NumCommands);
#if 0
	static TAutoConsoleVariable<int32> CVarRHICmdMemDump(
		TEXT("r.RHICmdMemDump"),
		0,
		TEXT("dumps callstacks and sizes of the immediate command lists to the console.\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Cheat);
	if (CVarRHICmdMemDump.GetValueOnRenderThread() > 0)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Mem %d\n"), CmdList.GetUsedMemory());
		if (CmdList.GetUsedMemory() > 300)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("big\n"));
		}
	}
#endif
	{
		SCOPE_CYCLE_COUNTER(STAT_ImmedCmdListExecuteTime);
		ExecuteInner(CmdList);
	}
}


void FRHICommandListExecutor::LatchBypass()
{
#if !UE_BUILD_SHIPPING
	if (GRHIThread)
	{
		if (bLatchedBypass)
		{
			check((GRHICommandList.OutstandingCmdListCount.GetValue() == 1 && !GRHICommandList.GetImmediateCommandList().HasCommands()));
			bLatchedBypass = false;
		}
	}
	else
	{
		GRHICommandList.GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);		

		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			if (FParse::Param(FCommandLine::Get(),TEXT("parallelrendering")) && CVarRHICmdBypass.GetValueOnRenderThread() >= 1)
			{
				IConsoleVariable* BypassVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHICmdBypass"));
				BypassVar->Set(0, ECVF_SetByCommandline);
			}
		}

		check((GRHICommandList.OutstandingCmdListCount.GetValue() == 1 && !GRHICommandList.GetImmediateCommandList().HasCommands()));
		bool NewBypass = (CVarRHICmdBypass.GetValueOnAnyThread() >= 1);

		if (NewBypass && !bLatchedBypass)
		{
			FRHIResource::FlushPendingDeletes();
		}
		bLatchedBypass = NewBypass;
	}
#endif
	if (!bLatchedBypass)
	{
		bLatchedUseParallelAlgorithms = FApp::ShouldUseThreadingForPerformance() 
#if !UE_BUILD_SHIPPING
			&& (CVarRHICmdUseParallelAlgorithms.GetValueOnAnyThread() >= 1)
#endif
			;
	}
	else
	{
		bLatchedUseParallelAlgorithms = false;
	}
}

void FRHICommandListExecutor::CheckNoOutstandingCmdLists()
{
	check(GRHICommandList.OutstandingCmdListCount.GetValue() == 1); // else we are attempting to delete resources while there is still a live cmdlist (other than the immediate cmd list) somewhere.
}

bool FRHICommandListExecutor::IsRHIThreadActive()
{
	checkSlow(IsInRenderingThread());
	bool bAsyncSubmit = CVarRHICmdAsyncRHIThreadDispatch.GetValueOnRenderThread() > 0;
	if (bAsyncSubmit)
	{
		if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
		{
			RenderThreadSublistDispatchTask = nullptr;
		}
		if (RenderThreadSublistDispatchTask.GetReference())
		{
			return true; // it might become active at any time
		}
		// otherwise we can safely look at RHIThreadTask
	}

	if (RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
	{
		RHIThreadTask = nullptr;
	}
	return !!RHIThreadTask.GetReference();
}

bool FRHICommandListExecutor::IsRHIThreadCompletelyFlushed()
{
	if (IsRHIThreadActive() || GetImmediateCommandList().HasCommands())
	{
		return false;
	}
	if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
	{
		RenderThreadSublistDispatchTask = nullptr;
	}
	return !RenderThreadSublistDispatchTask;
}

struct FRHICommandRHIThreadFence : public FRHICommand<FRHICommandRHIThreadFence>
{
	FGraphEventRef Fence;
	FORCEINLINE_DEBUGGABLE FRHICommandRHIThreadFence()
		: Fence(FGraphEvent::CreateGraphEvent())
{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		Fence->DispatchSubsequents();
		Fence = nullptr;
	}
};


FGraphEventRef FRHICommandListImmediate::RHIThreadFence(bool bSetLockFence)
{
	check(IsInRenderingThread() && GRHIThread);
	FRHICommandRHIThreadFence* Cmd = new (AllocCommand<FRHICommandRHIThreadFence>()) FRHICommandRHIThreadFence();
	if (bSetLockFence)
	{
		RHIThreadBufferLockFence = Cmd->Fence;
	}
	return Cmd->Fence;
}		
	
void FRHICommandListExecutor::WaitOnRHIThreadFence(FGraphEventRef& Fence)
{
	check(IsInRenderingThread());
	if (Fence.GetReference() && !Fence->IsComplete())
	{
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_WaitOnRHIThreadFence_Dispatch);
			GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // necessary to prevent deadlock
		}
		check(GRHIThread);
		QUICK_SCOPE_CYCLE_COUNTER(STAT_WaitOnRHIThreadFence_Wait);
		if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
		{
			// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
			UE_LOG(LogRHI, Fatal, TEXT("Deadlock in WaitOnRHIThreadFence."));
		}
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(Fence, ENamedThreads::RenderThread_Local);
	}
}




FRHICommandListBase::FRHICommandListBase()
	: MemManager(0)
{
	GRHICommandList.OutstandingCmdListCount.Increment();
	Reset();
}

FRHICommandListBase::~FRHICommandListBase()
{
	Flush();
	GRHICommandList.OutstandingCmdListCount.Decrement();
}

const int32 FRHICommandListBase::GetUsedMemory() const
{
	return MemManager.GetByteCount();
}

void FRHICommandListBase::Reset()
{
	bExecuting = false;
	check(!RTTasks.Num());
	MemManager.Flush();
	NumCommands = 0;
	Root = nullptr;
	CommandLink = &Root;
	Context = GDynamicRHI ? RHIGetDefaultContext() : nullptr;
	UID = GRHICommandList.UIDCounter.Increment();
	for (int32 Index = 0; ERenderThreadContext(Index) < ERenderThreadContext::Num; Index++)
	{
		RenderThreadContexts[Index] = nullptr;
	}
}


DECLARE_CYCLE_STAT(TEXT("Parallel Async Chain Translate"), STAT_ParallelChainTranslate, STATGROUP_RHICMDLIST);

class FParallelTranslateCommandList
{
	FRHICommandListBase** RHICmdLists;
	int32 NumCommandLists;
	IRHICommandContextContainer* ContextContainer;
public:

	FParallelTranslateCommandList(FRHICommandListBase** InRHICmdLists, int32 InNumCommandLists, IRHICommandContextContainer* InContextContainer)
		: RHICmdLists(InRHICmdLists)
		, NumCommandLists(InNumCommandLists)
		, ContextContainer(InContextContainer)
	{
		check(RHICmdLists && ContextContainer && NumCommandLists);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FParallelTranslateCommandList, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SCOPE_CYCLE_COUNTER(STAT_ParallelChainTranslate);
		check(ContextContainer && RHICmdLists);
		IRHICommandContext* Context = ContextContainer->GetContext();
		check(Context);
		for (int32 Index = 0; Index < NumCommandLists; Index++)
		{
			RHICmdLists[Index]->SetContext(Context);
			delete RHICmdLists[Index];
		}
		ContextContainer->FinishContext();
	}
};

DECLARE_DWORD_COUNTER_STAT(TEXT("Num Parallel Async Chains Links"), STAT_ParallelChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Wait for Parallel Async CmdList"), STAT_ParallelChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Parallel Async Chain Execute"), STAT_ParallelChainExecute, STATGROUP_RHICMDLIST);

struct FRHICommandWaitForAndSubmitSubListParallel : public FRHICommand<FRHICommandWaitForAndSubmitSubListParallel>
{
	FGraphEventRef TranslateCompletionEvent;
	IRHICommandContextContainer* ContextContainer;
	int32 Num;
	int32 Index;

	FORCEINLINE_DEBUGGABLE FRHICommandWaitForAndSubmitSubListParallel(FGraphEventRef& InTranslateCompletionEvent, IRHICommandContextContainer* InContextContainer, int32 InNum, int32 InIndex)
		: TranslateCompletionEvent(InTranslateCompletionEvent)
		, ContextContainer(InContextContainer)
		, Num(InNum)
		, Index(InIndex)
	{
		check(ContextContainer && Num);
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		check(ContextContainer && Num && IsInRHIThread());
		INC_DWORD_STAT_BY(STAT_ParallelChainLinkCount, 1);

		if (TranslateCompletionEvent.GetReference() && !TranslateCompletionEvent->IsComplete())
		{
			SCOPE_CYCLE_COUNTER(STAT_ParallelChainWait);
			if (IsInRenderingThread())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(TranslateCompletionEvent, ENamedThreads::RenderThread_Local);
			}
			else if (IsInRHIThread())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(TranslateCompletionEvent, ENamedThreads::RHIThread);
			}
			else
			{
				check(0);
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_ParallelChainExecute);
			ContextContainer->SubmitAndFreeContextContainer(Index, Num);
		}
	}
};



DECLARE_DWORD_COUNTER_STAT(TEXT("Num Async Chains Links"), STAT_ChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Wait for Async CmdList"), STAT_ChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Async Chain Execute"), STAT_ChainExecute, STATGROUP_RHICMDLIST);

struct FRHICommandWaitForAndSubmitSubList : public FRHICommand<FRHICommandWaitForAndSubmitSubList>
{
	FGraphEventRef EventToWaitFor;
	FRHICommandListBase* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandWaitForAndSubmitSubList(FGraphEventRef& InEventToWaitFor, FRHICommandListBase* InRHICmdList)
		: EventToWaitFor(InEventToWaitFor)
		, RHICmdList(InRHICmdList)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		INC_DWORD_STAT_BY(STAT_ChainLinkCount, 1);
		if (EventToWaitFor.GetReference() && !EventToWaitFor->IsComplete())
		{
			check(!GRHIThread || !IsInRHIThread()); // things should not be dispatched if they can't complete without further waits
			SCOPE_CYCLE_COUNTER(STAT_ChainWait);
			if (IsInRenderingThread())
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventToWaitFor, ENamedThreads::RenderThread_Local);
			}
			else
			{
				check(0);
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_ChainExecute);
			RHICmdList->CopyContext(CmdList);
			delete RHICmdList;
		}
	}
};


DECLARE_CYCLE_STAT(TEXT("Parallel Setup Translate"), STAT_ParallelSetupTranslate, STATGROUP_RHICMDLIST);

class FParallelTranslateSetupCommandList
{
	FRHICommandList* RHICmdList;
	FRHICommandListBase** RHICmdLists;
	int32 NumCommandLists;
	int32 MinSize;
	int32 MinCount;
public:

	FParallelTranslateSetupCommandList(FRHICommandList* InRHICmdList, FRHICommandListBase** InRHICmdLists, int32 InNumCommandLists)
		: RHICmdList(InRHICmdList)
		, RHICmdLists(InRHICmdLists)
		, NumCommandLists(InNumCommandLists)
	{
		check(RHICmdList && RHICmdLists && NumCommandLists);
		MinSize = CVarRHICmdMinCmdlistSizeForParallelTranslate.GetValueOnRenderThread() * 1024;
		MinCount = CVarRHICmdMinCmdlistForParallelTranslate.GetValueOnRenderThread();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FParallelTranslateSetupCommandList, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SCOPE_CYCLE_COUNTER(STAT_ParallelSetupTranslate);

		TArray<int32, TInlineAllocator<64> > Sizes;
		Sizes.Reserve(NumCommandLists);
		for (int32 Index = 0; Index < NumCommandLists; Index++)
		{
			Sizes.Add(RHICmdLists[Index]->GetUsedMemory());
		}

		int32 EffectiveThreads = 0;
		int32 Start = 0;
		// this is pretty silly but we need to know the number of jobs in advance, so we run the merge logic twice
		while (Start < NumCommandLists)
		{
			int32 Last = Start;
			int32 DrawCnt = Sizes[Start];

			while (Last < NumCommandLists - 1 && DrawCnt + Sizes[Last + 1] <= MinSize)
			{
				Last++;
				DrawCnt += Sizes[Last];
			}
			check(Last >= Start);
			Start = Last + 1;
			EffectiveThreads++;
		} 

		if (EffectiveThreads < MinCount)
		{
			FGraphEventRef Nothing;
			for (int32 Index = 0; Index < NumCommandLists; Index++)
			{
				FRHICommandListBase* CmdList = RHICmdLists[Index];
				new (RHICmdList->AllocCommand<FRHICommandWaitForAndSubmitSubList>()) FRHICommandWaitForAndSubmitSubList(Nothing, CmdList);
			}
		}
		else
		{
			Start = 0;
			int32 ThreadIndex = 0;

			while (Start < NumCommandLists)
			{
				int32 Last = Start;
				int32 DrawCnt = Sizes[Start];

				while (Last < NumCommandLists - 1 && DrawCnt + Sizes[Last + 1] <= MinSize)
				{
					Last++;
					DrawCnt += Sizes[Last];
				}
				check(Last >= Start);

				IRHICommandContextContainer* ContextContainer =  RHIGetCommandContextContainer();
				check(ContextContainer);

				FGraphEventRef TranslateCompletionEvent = TGraphTask<FParallelTranslateCommandList>::CreateTask(nullptr, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(&RHICmdLists[Start], 1 + Last - Start, ContextContainer);
				MyCompletionGraphEvent->DontCompleteUntil(TranslateCompletionEvent);
				new (RHICmdList->AllocCommand<FRHICommandWaitForAndSubmitSubListParallel>()) FRHICommandWaitForAndSubmitSubListParallel(TranslateCompletionEvent, ContextContainer, EffectiveThreads, ThreadIndex++);
				Start = Last + 1;
			}
			check(EffectiveThreads == ThreadIndex);
		}
	}
};

void FRHICommandListBase::QueueParallelAsyncCommandListSubmit(FGraphEventRef* AnyThreadCompletionEvents, FRHICommandList** CmdLists, int32* NumDrawsIfKnown, int32 Num, int32 MinDrawsPerTranslate, bool bSpewMerge)
{
	check(IsInRenderingThread() && IsImmediate() && Num);
	if (GRHIThread)
	{
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we should start on the stuff before this async list

		// as good a place as any to clear this
		if (RHIThreadBufferLockFence.GetReference() && RHIThreadBufferLockFence->IsComplete())
		{
			RHIThreadBufferLockFence = nullptr;
		}
	}
#if !UE_BUILD_SHIPPING
	// do a flush before hand so we can tell if it was this parallel set that broke something, or what came before.
	if (CVarRHICmdFlushOnQueueParallelSubmit.GetValueOnRenderThread())
	{
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
#endif

	if (Num && GRHIThread)
	{
		if (CVarRHICmdBalanceTranslatesAfterTasks.GetValueOnRenderThread() > 0 && GRHISupportsParallelRHIExecute && CVarRHICmdUseDeferredContexts.GetValueOnAnyThread() > 0)
		{
			FGraphEventArray Prereq;
			FRHICommandListBase** RHICmdLists = (FRHICommandListBase**)Alloc(sizeof(FRHICommandListBase*) * Num, ALIGNOF(FRHICommandListBase*));
			for (int32 Index = 0; Index < Num; Index++)
			{
				FGraphEventRef& AnyThreadCompletionEvent = AnyThreadCompletionEvents[Index];
				FRHICommandList* CmdList = CmdLists[Index];
				RHICmdLists[Index] = CmdList;
				if (AnyThreadCompletionEvent.GetReference())
				{
					Prereq.Add(AnyThreadCompletionEvent);
					WaitOutstandingTasks.Add(AnyThreadCompletionEvent);
				}
			}
			// this is used to ensure that any old buffer locks are completed before we start any parallel translates
			if (RHIThreadBufferLockFence.GetReference())
			{
				Prereq.Add(RHIThreadBufferLockFence);
			}
			FRHICommandList* CmdList = new FRHICommandList;
			CmdList->CopyRenderThreadContexts(*this);
			FGraphEventRef TranslateSetupCompletionEvent = TGraphTask<FParallelTranslateSetupCommandList>::CreateTask(&Prereq, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(CmdList, &RHICmdLists[0], Num);
			QueueCommandListSubmit(CmdList);
			AllOutstandingTasks.Add(TranslateSetupCompletionEvent);
			if (GRHIThread)
			{
				FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we don't want stuff after the async cmd list to be bundled with it
			}
		}
		else
		{
			IRHICommandContextContainer* ContextContainer = nullptr;
			if (GRHISupportsParallelRHIExecute && CVarRHICmdUseDeferredContexts.GetValueOnAnyThread() > 0)
			{
				ContextContainer = RHIGetCommandContextContainer();
			}
			if (ContextContainer)
			{
				bool bMerge = !!CVarRHICmdMergeSmallDeferredContexts.GetValueOnRenderThread();

				int32 EffectiveThreads = 0;
				int32 Start = 0;
				// this is pretty silly but we need to know the number of jobs in advance, so we run the merge logic twice
				while (Start < Num)
				{
					int32 Last = Start;
					int32 DrawCnt = NumDrawsIfKnown[Start];

					if (bMerge && DrawCnt >= 0)
					{
						while (Last < Num - 1 && NumDrawsIfKnown[Last + 1] >= 0 && DrawCnt + NumDrawsIfKnown[Last + 1] <= MinDrawsPerTranslate)
						{
							Last++;
							DrawCnt += NumDrawsIfKnown[Last];
						}
					}
					check(Last >= Start);
					Start = Last + 1;
					EffectiveThreads++;
				} 
				Start = 0;
				int32 ThreadIndex = 0;

				while (Start < Num)       
				{    
					int32 Last = Start;
					int32 DrawCnt = NumDrawsIfKnown[Start];
					int32 TotalMem = bSpewMerge ? CmdLists[Start]->GetUsedMemory() : 0;   // the memory is only accurate if we are spewing because otherwise it isn't done yet!

					if (bMerge && DrawCnt >= 0)
					{
						while (Last < Num - 1 && NumDrawsIfKnown[Last + 1] >= 0 && DrawCnt + NumDrawsIfKnown[Last + 1] <= MinDrawsPerTranslate)
						{
							Last++;
							DrawCnt += NumDrawsIfKnown[Last];
							TotalMem += bSpewMerge ? CmdLists[Start]->GetUsedMemory() : 0;   // the memory is only accurate if we are spewing because otherwise it isn't done yet!
						}
					}

					check(Last >= Start);

					if (!ContextContainer) 
					{
						ContextContainer = RHIGetCommandContextContainer();
					}
					check(ContextContainer);

					FGraphEventArray Prereq;
					FRHICommandListBase** RHICmdLists = (FRHICommandListBase**)Alloc(sizeof(FRHICommandListBase*) * (1 + Last - Start), ALIGNOF(FRHICommandListBase*));
					for (int32 Index = Start; Index <= Last; Index++)
					{
						FGraphEventRef& AnyThreadCompletionEvent = AnyThreadCompletionEvents[Index];
						FRHICommandList* CmdList = CmdLists[Index];
						RHICmdLists[Index - Start] = CmdList;
						if (AnyThreadCompletionEvent.GetReference())
						{
							Prereq.Add(AnyThreadCompletionEvent);
							AllOutstandingTasks.Add(AnyThreadCompletionEvent);
							WaitOutstandingTasks.Add(AnyThreadCompletionEvent);
						}
					}
					UE_CLOG(bSpewMerge, LogTemp, Display, TEXT("Parallel translate %d->%d    %dKB mem   %d draws (-1 = unknown)"), Start, Last, FMath::DivideAndRoundUp(TotalMem, 1024), DrawCnt);

					// this is used to ensure that any old buffer locks are completed before we start any parallel translates
					if (RHIThreadBufferLockFence.GetReference())
					{
						Prereq.Add(RHIThreadBufferLockFence);
					}

					FGraphEventRef TranslateCompletionEvent = TGraphTask<FParallelTranslateCommandList>::CreateTask(&Prereq, ENamedThreads::RenderThread).ConstructAndDispatchWhenReady(&RHICmdLists[0], 1 + Last - Start, ContextContainer);

					AllOutstandingTasks.Add(TranslateCompletionEvent);
					new (AllocCommand<FRHICommandWaitForAndSubmitSubListParallel>()) FRHICommandWaitForAndSubmitSubListParallel(TranslateCompletionEvent, ContextContainer, EffectiveThreads, ThreadIndex++);
					if (GRHIThread)
					{
						FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we don't want stuff after the async cmd list to be bundled with it
					}

					ContextContainer = nullptr;
					Start = Last + 1;
				}
				check(EffectiveThreads == ThreadIndex);
			}
		}

#if !UE_BUILD_SHIPPING
		if (CVarRHICmdFlushOnQueueParallelSubmit.GetValueOnRenderThread())
		{
			FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		}
#endif
		return;
	}
	for (int32 Index = 0; Index < Num; Index++)
	{
		FGraphEventRef& AnyThreadCompletionEvent = AnyThreadCompletionEvents[Index];
		FRHICommandList* CmdList = CmdLists[Index];
		if (AnyThreadCompletionEvent.GetReference())
		{
			if (GRHIThread)
			{
				AllOutstandingTasks.Add(AnyThreadCompletionEvent);
			}
			WaitOutstandingTasks.Add(AnyThreadCompletionEvent);
		}
		new (AllocCommand<FRHICommandWaitForAndSubmitSubList>()) FRHICommandWaitForAndSubmitSubList(AnyThreadCompletionEvent, CmdList);
	}
	if (GRHIThread)
	{
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we don't want stuff after the async cmd list to be bundled with it
	}
}

void FRHICommandListBase::QueueAsyncCommandListSubmit(FGraphEventRef& AnyThreadCompletionEvent, class FRHICommandList* CmdList)
{
	check(IsInRenderingThread() && IsImmediate());
	if (GRHIThread)
	{
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we should start on the stuff before this async list
	}
	if (AnyThreadCompletionEvent.GetReference())
	{
		if (GRHIThread)
		{
			AllOutstandingTasks.Add(AnyThreadCompletionEvent);
		}
		WaitOutstandingTasks.Add(AnyThreadCompletionEvent);
	}
	new (AllocCommand<FRHICommandWaitForAndSubmitSubList>()) FRHICommandWaitForAndSubmitSubList(AnyThreadCompletionEvent, CmdList);
	if (GRHIThread)
	{
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); // we don't want stuff after the async cmd list to be bundled with it
	}
}

DECLARE_DWORD_COUNTER_STAT(TEXT("Num RT Chains Links"), STAT_RTChainLinkCount, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Wait for RT CmdList"), STAT_RTChainWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("RT Chain Execute"), STAT_RTChainExecute, STATGROUP_RHICMDLIST);

struct FRHICommandWaitForAndSubmitRTSubList : public FRHICommand<FRHICommandWaitForAndSubmitRTSubList>
{
	FGraphEventRef EventToWaitFor;
	FRHICommandList* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandWaitForAndSubmitRTSubList(FGraphEventRef& InEventToWaitFor, FRHICommandList* InRHICmdList)
		: EventToWaitFor(InEventToWaitFor)
		, RHICmdList(InRHICmdList)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		INC_DWORD_STAT_BY(STAT_RTChainLinkCount, 1);
		{
			if (EventToWaitFor.GetReference() && !EventToWaitFor->IsComplete())
			{
			SCOPE_CYCLE_COUNTER(STAT_RTChainWait);
				check(!GRHIThread || !IsInRHIThread()); // things should not be dispatched if they can't complete without further waits
				if (IsInRenderingThread())
				{
					if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
					{
						// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
						UE_LOG(LogRHI, Fatal, TEXT("Deadlock in command list processing."));
					}
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventToWaitFor, ENamedThreads::RenderThread_Local);
				}
				else
				{
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventToWaitFor);
				}
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_RTChainExecute);
			RHICmdList->CopyContext(CmdList);
			delete RHICmdList;
		}
	}
};

void FRHICommandListBase::QueueRenderThreadCommandListSubmit(FGraphEventRef& RenderThreadCompletionEvent, class FRHICommandList* CmdList)
{
	check(!IsInRHIThread());
	if (RenderThreadCompletionEvent.GetReference())
	{
		check(!IsInRenderingThread() && !IsImmediate());
		RTTasks.Add(RenderThreadCompletionEvent);
	}
	new (AllocCommand<FRHICommandWaitForAndSubmitRTSubList>()) FRHICommandWaitForAndSubmitRTSubList(RenderThreadCompletionEvent, CmdList);
}

struct FRHICommandSubmitSubList : public FRHICommand<FRHICommandSubmitSubList>
{
	FRHICommandList* RHICmdList;
	FORCEINLINE_DEBUGGABLE FRHICommandSubmitSubList(FRHICommandList* InRHICmdList)
		: RHICmdList(InRHICmdList)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		INC_DWORD_STAT_BY(STAT_ChainLinkCount, 1);
		SCOPE_CYCLE_COUNTER(STAT_ChainExecute);
		RHICmdList->CopyContext(CmdList);
		delete RHICmdList;
	}
};

void FRHICommandListBase::QueueCommandListSubmit(class FRHICommandList* CmdList)
{
	new (AllocCommand<FRHICommandSubmitSubList>()) FRHICommandSubmitSubList(CmdList);
}


void FRHICommandList::BeginScene()
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(BeginScene)();
		return;
	}
	new (AllocCommand<FRHICommandBeginScene>()) FRHICommandBeginScene();
	if (!GRHIThread)
	{
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		QUICK_SCOPE_CYCLE_COUNTER(BeginScene_Flush);
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}
void FRHICommandList::EndScene()
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(EndScene)();
		return;
	}
	new (AllocCommand<FRHICommandEndScene>()) FRHICommandEndScene();
	if (!GRHIThread)
	{
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		QUICK_SCOPE_CYCLE_COUNTER(EndScene_Flush);
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}


void FRHICommandList::BeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI)
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(BeginDrawingViewport)(Viewport, RenderTargetRHI);
		return;
	}
	new (AllocCommand<FRHICommandBeginDrawingViewport>()) FRHICommandBeginDrawingViewport(Viewport, RenderTargetRHI);
	if (!GRHIThread)
	{
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		QUICK_SCOPE_CYCLE_COUNTER(BeginDrawingViewport_Flush);
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}

void FRHICommandList::EndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync)
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(EndDrawingViewport)(Viewport, bPresent, bLockToVsync);
	}
	else
	{
		new (AllocCommand<FRHICommandEndDrawingViewport>()) FRHICommandEndDrawingViewport(Viewport, bPresent, bLockToVsync);
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_EndDrawingViewport_Dispatch);
			FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
		}
	}
	RHIAdvanceFrameForGetViewportBackBuffer();
}

void FRHICommandList::BeginFrame()
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(BeginFrame)();
		return;
	}
	new (AllocCommand<FRHICommandBeginFrame>()) FRHICommandBeginFrame();
	if (!GRHIThread)
	{
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		QUICK_SCOPE_CYCLE_COUNTER(BeginFrame_Flush);
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}

void FRHICommandList::EndFrame()
{
	check(IsImmediate() && IsInRenderingThread());
	if (Bypass())
	{
		CMD_CONTEXT(EndFrame)();
		return;
	}
	new (AllocCommand<FRHICommandEndFrame>()) FRHICommandEndFrame();
	if (!GRHIThread)
	{
		// if we aren't running an RHIThread, there is no good reason to buffer this frame advance stuff and that complicates state management, so flush everything out now
		QUICK_SCOPE_CYCLE_COUNTER(EndFrame_Flush);
		FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	}
}

DECLARE_CYCLE_STAT(TEXT("Explicit wait for tasks"), STAT_ExplicitWait, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Prewait dispatch"), STAT_PrewaitDispatch, STATGROUP_RHICMDLIST);
void FRHICommandListBase::WaitForTasks(bool bKnownToBeComplete)
{
	check(IsImmediate() && IsInRenderingThread());
	if (WaitOutstandingTasks.Num())
	{
		bool bAny = false;
		for (int32 Index = 0; Index < WaitOutstandingTasks.Num(); Index++)
		{
			if (!WaitOutstandingTasks[Index]->IsComplete())
			{
				ensure(!bKnownToBeComplete);
				bAny = true;
				break;
			}
		}
		if (bAny)
		{
			SCOPE_CYCLE_COUNTER(STAT_ExplicitWait);
			check(!FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local));
			FTaskGraphInterface::Get().WaitUntilTasksComplete(WaitOutstandingTasks, ENamedThreads::RenderThread_Local);
		}
		WaitOutstandingTasks.Reset();
	}
}

FScopedCommandListWaitForTasks::~FScopedCommandListWaitForTasks()
{
	check(IsInRenderingThread());
	if (bWaitForTasks)
	{
		if (GRHIThread)
		{
#if 0
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_Dispatch);
				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}
#endif
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_WaitAsync);
				RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForOutstandingTasksOnly);
			}
		}
		else
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_Flush);
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("Explicit wait for dispatch"), STAT_ExplicitWaitDispatch, STATGROUP_RHICMDLIST);
void FRHICommandListBase::WaitForDispatch()
{
	check(IsImmediate() && IsInRenderingThread());
	check(!AllOutstandingTasks.Num()); // dispatch before you get here
	if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
	{
		RenderThreadSublistDispatchTask = nullptr;
	}
	while (RenderThreadSublistDispatchTask.GetReference())
	{
		SCOPE_CYCLE_COUNTER(STAT_ExplicitWaitDispatch);
		if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
		{
			// this is a deadlock. RT tasks must be done by now or they won't be done. We could add a third queue...
			UE_LOG(LogRHI, Fatal, TEXT("Deadlock in FRHICommandListBase::WaitForDispatch."));
		}
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(RenderThreadSublistDispatchTask, ENamedThreads::RenderThread_Local);
		if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
		{
			RenderThreadSublistDispatchTask = nullptr;
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("Explicit wait for RHI thread"), STAT_ExplicitWaitRHIThread, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Explicit wait for RHI thread async dispatch"), STAT_ExplicitWaitRHIThread_Dispatch, STATGROUP_RHICMDLIST);
DECLARE_CYCLE_STAT(TEXT("Deep spin for stray resource init."), STAT_SpinWaitRHIThread, STATGROUP_RHICMDLIST);
void FRHICommandListBase::WaitForRHIThreadTasks()
{
	check(IsImmediate() && IsInRenderingThread());
	bool bAsyncSubmit = CVarRHICmdAsyncRHIThreadDispatch.GetValueOnRenderThread() > 0;
	if (bAsyncSubmit)
	{
		if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
		{
			RenderThreadSublistDispatchTask = nullptr;
		}
		while (RenderThreadSublistDispatchTask.GetReference())
		{
			SCOPE_CYCLE_COUNTER(STAT_ExplicitWaitRHIThread_Dispatch);
			if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
			{
				// we have to spin here because all task threads might be stalled, meaning the fire event anythread task might not be hit.
				// todo, add a third queue
				SCOPE_CYCLE_COUNTER(STAT_SpinWaitRHIThread);
				while (!RenderThreadSublistDispatchTask->IsComplete())
				{
					FPlatformProcess::Sleep(0);
				}
			}
			else
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(RenderThreadSublistDispatchTask, ENamedThreads::RenderThread_Local);
			}
			if (RenderThreadSublistDispatchTask.GetReference() && RenderThreadSublistDispatchTask->IsComplete())
			{
				RenderThreadSublistDispatchTask = nullptr;
			}
		}
		// now we can safely look at RHIThreadTask
	}
	if (RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
	{
		RHIThreadTask = nullptr;
	}
	while (RHIThreadTask.GetReference())
	{
		SCOPE_CYCLE_COUNTER(STAT_ExplicitWaitRHIThread);
		if (FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::RenderThread_Local))
		{
			// we have to spin here because all task threads might be stalled, meaning the fire event anythread task might not be hit.
			// todo, add a third queue
			SCOPE_CYCLE_COUNTER(STAT_SpinWaitRHIThread);
			while (!RHIThreadTask->IsComplete())
			{
				FPlatformProcess::Sleep(0);
			}
		}
		else
		{
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(RHIThreadTask, ENamedThreads::RenderThread_Local);
		}
		if (RHIThreadTask.GetReference() && RHIThreadTask->IsComplete())
		{
			RHIThreadTask = nullptr;
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("RTTask completion join"), STAT_HandleRTThreadTaskCompletion_Join, STATGROUP_RHICMDLIST);
void FRHICommandListBase::HandleRTThreadTaskCompletion(const FGraphEventRef& MyCompletionGraphEvent)
{
	check(!IsImmediate() && !IsInRenderingThread() && !IsInRHIThread())
	for (int32 Index = 0; Index < RTTasks.Num(); Index++)
	{
		if (!RTTasks[Index]->IsComplete())
		{
			MyCompletionGraphEvent->DontCompleteUntil(RTTasks[Index]);
		}
	}
	RTTasks.Empty();
}

static TLockFreeFixedSizeAllocator<sizeof(FRHICommandList), FThreadSafeCounter> RHICommandListAllocator;

void* FRHICommandList::operator new(size_t Size)
{
	// doesn't support derived classes with a different size
	check(Size == sizeof(FRHICommandList));
	return RHICommandListAllocator.Allocate();
	//return FMemory::Malloc(Size);
}

void FRHICommandList::operator delete(void *RawMemory)
{
	check(RawMemory != (void*) &GRHICommandList.GetImmediateCommandList());
	RHICommandListAllocator.Free(RawMemory);
	//FMemory::Free(RawMemory);
}	

void* FRHICommandListBase::operator new(size_t Size)
{
	check(0); // you shouldn't be creating these
	return FMemory::Malloc(Size);
}

void FRHICommandListBase::operator delete(void *RawMemory)
{
	check(RawMemory != (void*) &GRHICommandList.GetImmediateCommandList());
	RHICommandListAllocator.Free(RawMemory);
	//FMemory::Free(RawMemory);
}	

///////// Pass through functions that allow RHIs to optimize certain calls.

FVertexBufferRHIRef FDynamicRHI::CreateAndLockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	FVertexBufferRHIRef VertexBuffer = CreateVertexBuffer_RenderThread(RHICmdList, Size, InUsage, CreateInfo);
	OutDataBuffer = LockVertexBuffer_RenderThread(RHICmdList, VertexBuffer, 0, Size, RLM_WriteOnly);

	return VertexBuffer;
}

FIndexBufferRHIRef FDynamicRHI::CreateAndLockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	FIndexBufferRHIRef IndexBuffer = CreateIndexBuffer_RenderThread(RHICmdList, Stride, Size, InUsage, CreateInfo);
	OutDataBuffer = LockIndexBuffer_RenderThread(RHICmdList, IndexBuffer, 0, Size, RLM_WriteOnly);
	return IndexBuffer;
}


FVertexBufferRHIRef FDynamicRHI::CreateVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	check(IsInRenderingThread());
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CreateVertexBuffer_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHICreateVertexBuffer(Size, InUsage, CreateInfo);
}
FIndexBufferRHIRef FDynamicRHI::CreateIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	check(IsInRenderingThread());
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CreateIndexBuffer_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHICreateIndexBuffer(Stride, Size, InUsage, CreateInfo);
}

FShaderResourceViewRHIRef FDynamicRHI::CreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format)
{
	check(IsInRenderingThread());
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CreateShaderResourceView_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHICreateShaderResourceView(VertexBuffer, Stride, Format);
}

struct FRHICommandUpdateVertexBuffer : public FRHICommand<FRHICommandUpdateVertexBuffer>
{
	FVertexBufferRHIParamRef VertexBuffer;
	void* Buffer;
	uint32 BufferSize;
	uint32 Offset;

	FORCEINLINE_DEBUGGABLE FRHICommandUpdateVertexBuffer(FVertexBufferRHIParamRef InVertexBuffer, void* InBuffer, uint32 InOffset, uint32 InBufferSize)
		: VertexBuffer(InVertexBuffer)
		, Buffer(InBuffer)
		, BufferSize(InBufferSize)
		, Offset(InOffset)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandUpdateVertexBuffer_Execute);
		void* Data = GDynamicRHI->RHILockVertexBuffer(VertexBuffer, Offset, BufferSize, RLM_WriteOnly);
		FMemory::Memcpy(Data, Buffer, BufferSize);
		FMemory::Free(Buffer);
		GDynamicRHI->RHIUnlockVertexBuffer(VertexBuffer);
	}
};

struct FRHICommandUpdateIndexBuffer : public FRHICommand<FRHICommandUpdateIndexBuffer>
{
	FIndexBufferRHIParamRef IndexBuffer;
	void* Buffer;
	uint32 BufferSize;
	uint32 Offset;

	FORCEINLINE_DEBUGGABLE FRHICommandUpdateIndexBuffer(FIndexBufferRHIParamRef InIndexBuffer, void* InBuffer, uint32 InOffset, uint32 InBufferSize)
		: IndexBuffer(InIndexBuffer)
		, Buffer(InBuffer)
		, BufferSize(InBufferSize)
		, Offset(InOffset)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandUpdateIndexBuffer_Execute);
		void* Data = GDynamicRHI->RHILockIndexBuffer(IndexBuffer, Offset, BufferSize, RLM_WriteOnly);
		FMemory::Memcpy(Data, Buffer, BufferSize);
		FMemory::Free(Buffer);
		GDynamicRHI->RHIUnlockIndexBuffer(IndexBuffer);
	}
};

static struct FLockTracker
{
	struct FLockParams
	{
		void* RHIBuffer;
		void* Buffer;
		uint32 BufferSize;
		uint32 Offset;
		EResourceLockMode LockMode;

		FORCEINLINE_DEBUGGABLE FLockParams(void* InRHIBuffer, void* InBuffer, uint32 InOffset, uint32 InBufferSize, EResourceLockMode InLockMode)
			: RHIBuffer(InRHIBuffer)
			, Buffer(InBuffer)
			, BufferSize(InBufferSize)
			, Offset(InOffset)
			, LockMode(InLockMode)
		{
		}
	};
	TArray<FLockParams, TInlineAllocator<16> > OutstandingLocks;
	uint32 TotalMemoryOutstanding;

	FLockTracker()
	{
		TotalMemoryOutstanding = 0;
	}

	FORCEINLINE_DEBUGGABLE void Lock(void* RHIBuffer, void* Buffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
	{
#if DO_CHECK
		for (auto& Parms : OutstandingLocks)
		{
			check(Parms.RHIBuffer != RHIBuffer);
		}
#endif
		OutstandingLocks.Add(FLockParams(RHIBuffer, Buffer, Offset, SizeRHI, LockMode));
		TotalMemoryOutstanding += SizeRHI;
	}
	FORCEINLINE_DEBUGGABLE FLockParams Unlock(void* RHIBuffer)
	{
		for (int32 Index = 0; Index < OutstandingLocks.Num(); Index++)
		{
			if (OutstandingLocks[Index].RHIBuffer == RHIBuffer)
			{
				FLockParams Result = OutstandingLocks[Index];
				OutstandingLocks.RemoveAtSwap(Index, 1, false);
				return Result;
			}
		}
		check(!"Mismatched RHI buffer locks.");
		return FLockParams(nullptr, nullptr, 0, 0, RLM_WriteOnly);
	}
} GLockTracker;


void* FDynamicRHI::LockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FDynamicRHI_LockVertexBuffer_RenderThread);
	check(IsInRenderingThread());
	bool bBuffer = CVarRHICmdBufferWriteLocks.GetValueOnRenderThread() > 0;
	void* Result;
	if (!bBuffer || LockMode != RLM_WriteOnly || RHICmdList.Bypass() || !GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockVertexBuffer_Flush);
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		Result = GDynamicRHI->RHILockVertexBuffer(VertexBuffer, Offset, SizeRHI, LockMode);
	}
	else
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockVertexBuffer_Malloc);
		Result = FMemory::Malloc(SizeRHI, 16);
	}
	check(Result);
	GLockTracker.Lock(VertexBuffer, Result, Offset, SizeRHI, LockMode);
	return Result;
}

void FDynamicRHI::UnlockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FDynamicRHI_UnlockVertexBuffer_RenderThread);
	check(IsInRenderingThread());
	bool bBuffer = CVarRHICmdBufferWriteLocks.GetValueOnRenderThread() > 0;
	FLockTracker::FLockParams Params = GLockTracker.Unlock(VertexBuffer);
	if (!bBuffer || Params.LockMode != RLM_WriteOnly || RHICmdList.Bypass() || !GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockVertexBuffer_Flush);
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		GDynamicRHI->RHIUnlockVertexBuffer(VertexBuffer);
		GLockTracker.TotalMemoryOutstanding = 0;
	}	
	else
	{
		new (RHICmdList.AllocCommand<FRHICommandUpdateVertexBuffer>()) FRHICommandUpdateVertexBuffer(VertexBuffer, Params.Buffer, Params.Offset, Params.BufferSize);
		RHICmdList.RHIThreadFence(true);
		if (GLockTracker.TotalMemoryOutstanding > 256 * 1024)
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockVertexBuffer_FlushForMem);
			// we could be loading a level or something, lets get this stuff going
			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); 
			GLockTracker.TotalMemoryOutstanding = 0;
		}
	}
}

void* FDynamicRHI::LockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FDynamicRHI_LockIndexBuffer_RenderThread);
	check(IsInRenderingThread());
	bool bBuffer = CVarRHICmdBufferWriteLocks.GetValueOnRenderThread() > 0;
	void* Result;
	if (!bBuffer || LockMode != RLM_WriteOnly || RHICmdList.Bypass() || !GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockIndexBuffer_Flush);
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		Result = GDynamicRHI->RHILockIndexBuffer(IndexBuffer, Offset, SizeRHI, LockMode);
	}
	else
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockIndexBuffer_Malloc);
		Result = FMemory::Malloc(SizeRHI, 16);
	}
	check(Result);
	GLockTracker.Lock(IndexBuffer, Result, Offset, SizeRHI, LockMode);
	return Result;
}

void FDynamicRHI::UnlockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FDynamicRHI_UnlockIndexBuffer_RenderThread);
	check(IsInRenderingThread());
	bool bBuffer = CVarRHICmdBufferWriteLocks.GetValueOnRenderThread() > 0;
	FLockTracker::FLockParams Params = GLockTracker.Unlock(IndexBuffer);
	if (!bBuffer || Params.LockMode != RLM_WriteOnly || RHICmdList.Bypass() || !GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockIndexBuffer_Flush);
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		GDynamicRHI->RHIUnlockIndexBuffer(IndexBuffer);
		GLockTracker.TotalMemoryOutstanding = 0;
	}	
	else
	{
		new (RHICmdList.AllocCommand<FRHICommandUpdateIndexBuffer>()) FRHICommandUpdateIndexBuffer(IndexBuffer, Params.Buffer, Params.Offset, Params.BufferSize);
		RHICmdList.RHIThreadFence(true);
		if (GLockTracker.TotalMemoryOutstanding > 256 * 1024)
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockIndexBuffer_FlushForMem);
			// we could be loading a level or something, lets get this stuff going
			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); 
			GLockTracker.TotalMemoryOutstanding = 0;
		}
	}
}

FTexture2DRHIRef FDynamicRHI::AsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_AsyncReallocateTexture2D_Flush);
	RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
	return GDynamicRHI->RHIAsyncReallocateTexture2D(Texture2D, NewMipCount, NewSizeX, NewSizeY, RequestStatus);
}

ETextureReallocationStatus FDynamicRHI::FinalizeAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_FinalizeAsyncReallocateTexture2D_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHIFinalizeAsyncReallocateTexture2D(Texture2D, bBlockUntilCompleted);
}

ETextureReallocationStatus FDynamicRHI::CancelAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CancelAsyncReallocateTexture2D_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHICancelAsyncReallocateTexture2D(Texture2D, bBlockUntilCompleted);
}

FVertexDeclarationRHIRef FDynamicRHI::CreateVertexDeclaration_RenderThread(class FRHICommandListImmediate& RHICmdList, const FVertexDeclarationElementList& Elements)
{
	if(GRHIThread)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CreateVertexDeclaration_WaitRHI);
		RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForRHIThread);
	}
	return GDynamicRHI->RHICreateVertexDeclaration(Elements);
}


void FRHICommandListImmediate::UpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture)
{
	if (Bypass() || !GRHIThread || CVarRHICmdFlushUpdateTextureReference.GetValueOnRenderThread() > 0)
	{
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UpdateTextureReference_FlushRHI);
			ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		}
		CMD_CONTEXT(UpdateTextureReference)(TextureRef, NewTexture);
		return;
	}
	new (AllocCommand<FRHICommandUpdateTextureReference>()) FRHICommandUpdateTextureReference(TextureRef, NewTexture);
	RHIThreadFence(true);
	if (GetUsedMemory() > 256 * 1024)
	{
		// we could be loading a level or something, lets get this stuff going
		ImmediateFlush(EImmediateFlushType::DispatchToRHIThread); 
	}
}


