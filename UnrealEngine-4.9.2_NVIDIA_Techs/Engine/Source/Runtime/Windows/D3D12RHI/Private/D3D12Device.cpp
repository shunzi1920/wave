// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
D3D12Device.cpp: D3D device RHI implementation.
=============================================================================*/

#include "D3D12RHIPrivate.h"
#include "AllowWindowsPlatformTypes.h"
	#include <delayimp.h>
#include "HideWindowsPlatformTypes.h"

namespace D3D12RHI
{
	extern void EmptyD3DSamplerStateCache();
}
using namespace D3D12RHI;

bool D3D12RHI_ShouldCreateWithD3DDebug()
{
	// Use a debug device if specified on the command line.
	return
		FParse::Param(FCommandLine::Get(),TEXT("d3ddebug")) ||
		FParse::Param(FCommandLine::Get(),TEXT("d3debug")) ||
		FParse::Param(FCommandLine::Get(),TEXT("dxdebug"));
}

bool D3D12RHI_ShouldAllowAsyncResourceCreation()
{
	static bool bAllowAsyncResourceCreation = !FParse::Param(FCommandLine::Get(),TEXT("nod3dasync"));
	return bAllowAsyncResourceCreation;
}

IMPLEMENT_MODULE(FD3D12DynamicRHIModule, D3D12RHI);

FD3D12DynamicRHI* FD3D12DynamicRHI::SingleD3DRHI = nullptr;

IRHICommandContext* FD3D12DynamicRHI::RHIGetDefaultContext()
{
	return static_cast<IRHICommandContext*>(&GetRHIDevice()->GetDefaultCommandContext());
}

#if D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE

//todo recycle these to avoid alloc


class FD3D12CommandContextContainer : public IRHICommandContextContainer
{
	FD3D12Device* OwningDevice;
	FD3D12CommandContext* CmdContext;
	TArray<FD3D12CommandListHandle> CommandLists;

public:

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void* RawMemory);

	FD3D12CommandContextContainer(FD3D12Device* InOwningDevice)
		: OwningDevice(InOwningDevice), CmdContext(nullptr)
	{
		CommandLists.Reserve (16);
	}

	virtual ~FD3D12CommandContextContainer() override
	{
	}

	virtual IRHICommandContext* GetContext() override
	{
		check(CmdContext == nullptr);
		CmdContext = OwningDevice->ObtainCommandContext();
		check(CmdContext != nullptr);

		if (CmdContext->CommandListHandle == nullptr)
		{
			CmdContext->OpenCommandList();
		}

		CmdContext->ClearState();

		return CmdContext;
	}

	virtual void FinishContext() override
	{
		// We never "Finish" the default context. It gets submitted when FlushCommands() is called.
		check(!CmdContext->IsDefaultContext());

		CmdContext->Finish(CommandLists);

		OwningDevice->ReleaseCommandContext(CmdContext);
		CmdContext = nullptr;
	}

	virtual void SubmitAndFreeContextContainer(int32 Index, int32 Num) override
	{
		if (Index == 0)
		{
			check((IsInRenderingThread() || IsInRHIThread()));

			OwningDevice->GetDefaultCommandContext().FlushCommands();
		}

		// Add the current lists for execution (now or possibly later)
		for (int32 i = 0; i < CommandLists.Num(); ++i)
		{
			OwningDevice->PendingCommandLists.Add(CommandLists[i]);
			OwningDevice->PendingCommandListsTotalWorkCommands +=
				CommandLists[i].GetCurrentOwningContext()->numClears +
				CommandLists[i].GetCurrentOwningContext()->numCopies +
				CommandLists[i].GetCurrentOwningContext()->numDraws;
		}

		CommandLists.Reset();

		// Submission occurs when a batch is finished
		const bool FinalCommandListInBatch = Index == (Num - 1);
		if (FinalCommandListInBatch && OwningDevice->PendingCommandLists.Num() > 0)
		{
#if SUPPORTS_MEMORY_RESIDENCY
			OwningDevice->GetOwningRHI()->GetResourceResidencyManager().MakeResident();
#endif
			OwningDevice->GetCommandListManager().ExecuteCommandLists(OwningDevice->PendingCommandLists);
			OwningDevice->PendingCommandLists.Reset();
			OwningDevice->PendingCommandListsTotalWorkCommands = 0;
		}

		delete this;
	}
};

static TLockFreeFixedSizeAllocator<sizeof(FD3D12CommandContextContainer), FThreadSafeCounter> FD3D12CommandContextContainerAllocator;

void* FD3D12CommandContextContainer::operator new(size_t Size)
{
	// doesn't support derived classes with a different size
	check(Size == sizeof(FD3D12CommandContextContainer));
	return FD3D12CommandContextContainerAllocator.Allocate();
}

void FD3D12CommandContextContainer::operator delete(void* RawMemory)
{
	FD3D12CommandContextContainerAllocator.Free(RawMemory);
}	

IRHICommandContextContainer* FD3D12DynamicRHI::RHIGetCommandContextContainer()
{
	return new FD3D12CommandContextContainer(GetRHIDevice());
}

#endif // D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE

FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InParent) :
	OwningRHI(*InParent->GetOwningRHI()),
	UploadHeapAllocator(InParent, D3D12_HEAP_TYPE_UPLOAD),
	bUsingTessellation(false),
	PendingNumVertices(0),
	PendingVertexDataStride(0),
	PendingPrimitiveType(0),
	PendingNumPrimitives(0),
	PendingMinVertexIndex(0),
	PendingNumIndices(0),
	PendingIndexDataStride(0),
	CurrentDepthTexture(nullptr),
	NumSimultaneousRenderTargets(0),
	NumUAVs(0),
	CurrentDSVAccessType(FExclusiveDepthStencil::DepthWrite_StencilWrite),
	bDiscardSharedConstants(false),
	CommandListHandle(),
    FD3D12DeviceChild(InParent)
{
	FMemory::Memzero(DirtyUniformBuffers, sizeof(DirtyUniformBuffers));

	UploadHeapAllocator.SetCurrentCommandContext(this);

	// Initialize the constant buffers.
	InitConstantBuffers();

	// Create the dynamic vertex and index buffers used for Draw[Indexed]PrimitiveUP.
	DynamicVB = new FD3D12DynamicBuffer(GetParentDevice(), UploadHeapAllocator);
	DynamicIB = new FD3D12DynamicBuffer(GetParentDevice(), UploadHeapAllocator);

	StateCache.Init(GetParentDevice(), this, nullptr);
}

FD3D12CommandContext::~FD3D12CommandContext()
{
	ClearState();
	StateCache.Clear();	// Clears its descriptor cache too

	DynamicVB = nullptr;
	DynamicIB = nullptr;

	// Release references to bound uniform buffers.
	for (int32 Frequency = 0; Frequency < SF_NumFrequencies; ++Frequency)
	{
		for (int32 BindIndex = 0; BindIndex < MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE; ++BindIndex)
		{
			BoundUniformBuffers[Frequency][BindIndex].SafeRelease();
		}
	}

	// Release allocators last because when other objects are deleted, they may be returned to an allocator
	UploadHeapAllocator.ReleaseAllResources();
}

FD3D12Device::FD3D12Device(FD3D12DynamicRHI* InOwningRHI, IDXGIFactory4* InDXGIFactory, FD3D12Adapter& InAdapter) :
    OwningRHI(InOwningRHI),
    DXGIFactory(InDXGIFactory),
    DeviceAdapter(InAdapter),
    Direct3DDevice(nullptr),
    DxgiAdapter3(nullptr),
    bDeviceRemoved(false),
    RTVAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256),
    DSVAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256),
    SRVAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024),
    UAVAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024),
    SamplerAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128),
    CBVAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024),
    SamplerID(0),
    OcclusionQueryHeap(this, D3D12_QUERY_HEAP_TYPE_OCCLUSION, 32768),
    PipelineStateCache(this),
    ResourceHelper(this),
    DeferredDeletionQueue(this),
    DefaultBufferAllocator(this),
    PendingCommandListsTotalWorkCommands(0)
{
}

FD3D12DynamicHeapAllocator* FD3D12DynamicRHI::HelperThreadDynamicHeapAllocator = nullptr;

TAutoConsoleVariable<int32> CVarD3D12ZeroBufferSizeInMB(
	TEXT("d3d12.ZeroBufferSizeInMB"),
	4,
	TEXT("The D3D12 RHI needs a static allocation of zeroes to use when streaming textures asynchronously. It should be large enough to support the largest mipmap you need to stream. The default is 4MB."),
	ECVF_ReadOnly
	);

FD3D12DynamicRHI::FD3D12DynamicRHI(IDXGIFactory4* InDXGIFactory, FD3D12Adapter& InAdapter) :
	DXGIFactory(InDXGIFactory),
	CommitResourceTableCycles(0),
	CacheResourceTableCalls(0),
	CacheResourceTableCycles(0),
	SetShaderTextureCycles(0),
	SetShaderTextureCalls(0),
	SetTextureInTableCalls(0),
	SceneFrameCounter(0),
	ResourceTableFrameCounter(INDEX_NONE),
	bForceSingleQueueGPU(false),
	NumThreadDynamicHeapAllocators(0),
	ViewportFrameCounter(0),
	MainAdapter(InAdapter),
	MainDevice(nullptr)
	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	, VxgiInterface(NULL)
	, VxgiRendererD3D12(NULL)
#endif
	// NVCHANGE_END: Add VXGI
{
	FMemory::Memzero(ThreadDynamicHeapAllocatorArray, sizeof(ThreadDynamicHeapAllocatorArray));

	// The FD3D12DynamicRHI must be a singleton
	check(SingleD3DRHI == nullptr);

	// This should be called once at the start 
	check( IsInGameThread() );
	check( !GIsThreadedRendering );

	SingleD3DRHI = this;

	check(MainAdapter.IsValid());
	MainDevice = new FD3D12Device(this, DXGIFactory, MainAdapter);

    FeatureLevel = MainAdapter.MaxSupportedFeatureLevel;

    GPUProfilingData.Init(MainDevice);

#if SUPPORTS_MEMORY_RESIDENCY
	ResourceResidencyManager.Init(MainDevice);
#endif

	// Allocate a buffer of zeroes. This is used when we need to pass D3D memory
	// that we don't care about and will overwrite with valid data in the future.
	ZeroBufferSize = FMath::Max(CVarD3D12ZeroBufferSizeInMB.GetValueOnAnyThread(), 0) * (1 << 20);
	ZeroBuffer = FMemory::Malloc(ZeroBufferSize);
	FMemory::Memzero(ZeroBuffer,ZeroBufferSize);

	GPoolSizeVRAMPercentage = 0;
	GTexturePoolSize = 0;
	GConfig->GetInt(TEXT("TextureStreaming"), TEXT("PoolSizeVRAMPercentage"), GPoolSizeVRAMPercentage, GEngineIni);

	// Initialize the RHI capabilities.
	check(FeatureLevel == D3D_FEATURE_LEVEL_11_0 || FeatureLevel == D3D_FEATURE_LEVEL_10_0 );

	if (FeatureLevel == D3D_FEATURE_LEVEL_10_0)
	{
		GSupportsDepthFetchDuringDepthTest = false;
	}

	// ES2 feature level emulation in D3D11
	if (FParse::Param(FCommandLine::Get(), TEXT("FeatureLevelES2")) && !GIsEditor)
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::ES2;
		GMaxRHIShaderPlatform = SP_PCD3D_ES2;
	}
	else if ((FParse::Param(FCommandLine::Get(), TEXT("FeatureLevelES31")) || FParse::Param(FCommandLine::Get(), TEXT("FeatureLevelES3_1"))) && !GIsEditor)
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::ES3_1;
		GMaxRHIShaderPlatform = SP_PCD3D_ES3_1;
	}
	else if (FeatureLevel == D3D_FEATURE_LEVEL_11_0)
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::SM5;
		GMaxRHIShaderPlatform = SP_PCD3D_SM5;
	}
	else if (FeatureLevel == D3D_FEATURE_LEVEL_10_0)
	{
		GMaxRHIFeatureLevel = ERHIFeatureLevel::SM4;
		GMaxRHIShaderPlatform = SP_PCD3D_SM4;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("ForceSingleQueue")))
	{
		bForceSingleQueueGPU = true;
	}

	// Initialize the platform pixel format map.
	GPixelFormats[ PF_Unknown		].PlatformFormat	= DXGI_FORMAT_UNKNOWN;
	GPixelFormats[ PF_A32B32G32R32F	].PlatformFormat	= DXGI_FORMAT_R32G32B32A32_FLOAT;
	GPixelFormats[ PF_B8G8R8A8		].PlatformFormat	= DXGI_FORMAT_B8G8R8A8_TYPELESS;
	GPixelFormats[ PF_G8			].PlatformFormat	= DXGI_FORMAT_R8_UNORM;
	GPixelFormats[ PF_G16			].PlatformFormat	= DXGI_FORMAT_R16_UNORM;
	GPixelFormats[ PF_DXT1			].PlatformFormat	= DXGI_FORMAT_BC1_TYPELESS;
	GPixelFormats[ PF_DXT3			].PlatformFormat	= DXGI_FORMAT_BC2_TYPELESS;
	GPixelFormats[ PF_DXT5			].PlatformFormat	= DXGI_FORMAT_BC3_TYPELESS;
	GPixelFormats[ PF_BC4			].PlatformFormat	= DXGI_FORMAT_BC4_UNORM;
	GPixelFormats[ PF_UYVY			].PlatformFormat	= DXGI_FORMAT_UNKNOWN;		// TODO: Not supported in D3D11
#if DEPTH_32_BIT_CONVERSION
	GPixelFormats[ PF_DepthStencil	].PlatformFormat	= DXGI_FORMAT_R32G8X24_TYPELESS; 
	GPixelFormats[ PF_DepthStencil	].BlockBytes		= 5;
	GPixelFormats[ PF_X24_G8 ].PlatformFormat			= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
	GPixelFormats[ PF_X24_G8].BlockBytes				= 5;
#else
	GPixelFormats[ PF_DepthStencil	].PlatformFormat	= DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[ PF_DepthStencil	].BlockBytes		= 4;
	GPixelFormats[ PF_X24_G8 ].PlatformFormat			= DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	GPixelFormats[ PF_X24_G8].BlockBytes				= 4;
#endif
	GPixelFormats[ PF_ShadowDepth	].PlatformFormat	= DXGI_FORMAT_R16_TYPELESS;
	GPixelFormats[ PF_ShadowDepth	].BlockBytes		= 2;
	GPixelFormats[ PF_R32_FLOAT		].PlatformFormat	= DXGI_FORMAT_R32_FLOAT;
	GPixelFormats[ PF_G16R16		].PlatformFormat	= DXGI_FORMAT_R16G16_UNORM;
	GPixelFormats[ PF_G16R16F		].PlatformFormat	= DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[ PF_G16R16F_FILTER].PlatformFormat	= DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[ PF_G32R32F		].PlatformFormat	= DXGI_FORMAT_R32G32_FLOAT;
	GPixelFormats[ PF_A2B10G10R10   ].PlatformFormat    = DXGI_FORMAT_R10G10B10A2_UNORM;
	GPixelFormats[ PF_A16B16G16R16  ].PlatformFormat    = DXGI_FORMAT_R16G16B16A16_UNORM;
	GPixelFormats[ PF_D24 ].PlatformFormat				= DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[ PF_R16F			].PlatformFormat	= DXGI_FORMAT_R16_FLOAT;
	GPixelFormats[ PF_R16F_FILTER	].PlatformFormat	= DXGI_FORMAT_R16_FLOAT;

	GPixelFormats[ PF_FloatRGB	].PlatformFormat		= DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[ PF_FloatRGB	].BlockBytes			= 4;
	GPixelFormats[ PF_FloatRGBA	].PlatformFormat		= DXGI_FORMAT_R16G16B16A16_FLOAT;
	GPixelFormats[ PF_FloatRGBA	].BlockBytes			= 8;

	GPixelFormats[ PF_FloatR11G11B10].PlatformFormat	= DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[ PF_FloatR11G11B10].BlockBytes		= 4;

	GPixelFormats[ PF_V8U8			].PlatformFormat	= DXGI_FORMAT_R8G8_SNORM;
	GPixelFormats[ PF_BC5			].PlatformFormat	= DXGI_FORMAT_BC5_UNORM;
	GPixelFormats[ PF_A1			].PlatformFormat	= DXGI_FORMAT_R1_UNORM; // Not supported for rendering.
	GPixelFormats[ PF_A8			].PlatformFormat	= DXGI_FORMAT_A8_UNORM;
	GPixelFormats[ PF_R32_UINT		].PlatformFormat	= DXGI_FORMAT_R32_UINT;
	GPixelFormats[ PF_R32_SINT		].PlatformFormat	= DXGI_FORMAT_R32_SINT;

	GPixelFormats[ PF_R16_UINT         ].PlatformFormat = DXGI_FORMAT_R16_UINT;
	GPixelFormats[ PF_R16_SINT         ].PlatformFormat = DXGI_FORMAT_R16_SINT;
	GPixelFormats[ PF_R16G16B16A16_UINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UINT;
	GPixelFormats[ PF_R16G16B16A16_SINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_SINT;

	GPixelFormats[ PF_R5G6B5_UNORM	].PlatformFormat	= DXGI_FORMAT_B5G6R5_UNORM;
	GPixelFormats[ PF_R8G8B8A8		].PlatformFormat	= DXGI_FORMAT_R8G8B8A8_TYPELESS;
	GPixelFormats[ PF_R8G8			].PlatformFormat	= DXGI_FORMAT_R8G8_UNORM;
	GPixelFormats[ PF_R32G32B32A32_UINT].PlatformFormat = DXGI_FORMAT_R32G32B32A32_UINT;
	GPixelFormats[ PF_R16G16_UINT].PlatformFormat = DXGI_FORMAT_R16G16_UINT;

	GPixelFormats[ PF_BC6H			].PlatformFormat	= DXGI_FORMAT_BC6H_UF16;
	GPixelFormats[ PF_BC7			].PlatformFormat	= DXGI_FORMAT_BC7_TYPELESS;

    // MS - Not doing any feature level checks. D3D12 currently supports these limits.
    // However this may need to be revisited if new feature levels are introduced with different HW requirement
	GSupportsSeparateRenderTargetBlendState = true;
	GMaxTextureDimensions = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	GMaxCubeTextureDimensions = D3D12_REQ_TEXTURECUBE_DIMENSION;
	GMaxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;

	GMaxShadowDepthBufferSizeX = 4096;
	GMaxShadowDepthBufferSizeY = 4096;

	// Enable multithreading if not in the editor (editor crashes with multithreading enabled).
	if (!GIsEditor)
	{
		GRHISupportsRHIThread = true;
	}
	GRHISupportsParallelRHIExecute = D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE;
}

void FD3D12Device::CreateCommandContexts()
{
    check(CommandContextArray.Num() == 0);

	const uint32 NumContexts = FTaskGraphInterface::Get().GetNumWorkerThreads() + 1;

	// We never make the default context free for allocation by the context containers
	CommandContextArray.Reserve(NumContexts);
	FreeCommandContexts.Reserve(NumContexts - 1);

	for (uint32 i = 0; i < NumContexts; ++i)
	{
		FD3D12CommandContext* NewCmdContext = new FD3D12CommandContext(this);

		// without that the first RHIClear would get a scissor rect of (0,0)-(0,0) which means we get a draw call clear 
		NewCmdContext->RHISetScissorRect(false, 0, 0, 0, 0);

		CommandContextArray.Add(NewCmdContext);

		// Make available all but the first command context for parallel threads
		if (i > 0)
		{
			FreeCommandContexts.Add(CommandContextArray[i]);
		}
	}

	CommandContextArray[0]->OpenCommandList();
}

FD3D12DynamicRHI::~FD3D12DynamicRHI()
{
	UE_LOG(LogD3D12RHI, Log, TEXT("~FD3D12DynamicRHI"));

	check(MainDevice == nullptr);
}

void FD3D12DynamicRHI::Shutdown()
{
	check(IsInGameThread() && IsInRenderingThread());  // require that the render thread has been shut down

	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	ReleaseVxgiInterface();
	UnloadVxgiModule();
#endif
	// NVCHANGE_END: Add VXGI

	// Cleanup the D3D devices.
	MainDevice->CleanupD3DDevice();
	delete(MainDevice);
	MainDevice = nullptr;

	// Release buffered timestamp queries
	GPUProfilingData.FrameTiming.ReleaseResource();

	// Release the buffer of zeroes.
	FMemory::Free(ZeroBuffer);
	ZeroBuffer = NULL;
	ZeroBufferSize = 0;
}

void FD3D12CommandContext::RHIPushEvent(const TCHAR* Name)
{
    OwningRHI.PushGPUEvent(Name);
}

void FD3D12CommandContext::RHIPopEvent()
{
    OwningRHI.PopGPUEvent();
}

/**
* Returns a supported screen resolution that most closely matches the input.
* @param Width - Input: Desired resolution width in pixels. Output: A width that the platform supports.
* @param Height - Input: Desired resolution height in pixels. Output: A height that the platform supports.
*/
void FD3D12DynamicRHI::RHIGetSupportedResolution( uint32 &Width, uint32 &Height )
{
	uint32 InitializedMode = false;
	DXGI_MODE_DESC BestMode;
	BestMode.Width = 0;
	BestMode.Height = 0;

	{
		HRESULT HResult = S_OK;
		TRefCountPtr<IDXGIAdapter> Adapter;
		HResult = DXGIFactory->EnumAdapters(GetRHIDevice()->GetAdapterIndex(), Adapter.GetInitReference());
		if (DXGI_ERROR_NOT_FOUND == HResult)
		{
			return;
		}
		if (FAILED(HResult))
		{
			return;
		}

		// get the description of the adapter
		DXGI_ADAPTER_DESC AdapterDesc;
		VERIFYD3D11RESULT(Adapter->GetDesc(&AdapterDesc));

#ifndef PLATFORM_XBOXONE // No need for display mode enumeration on console
		// Enumerate outputs for this adapter
		// TODO: Cap at 1 for default output
		for(uint32 o = 0;o < 1; o++)
		{
			TRefCountPtr<IDXGIOutput> Output;
			HResult = Adapter->EnumOutputs(o,Output.GetInitReference());
			if (DXGI_ERROR_NOT_FOUND == HResult)
			{
				break;
			}
			if (FAILED(HResult))
			{
				return;
			}

			// TODO: GetDisplayModeList is a terribly SLOW call.  It can take up to a second per invocation.
			//  We might want to work around some DXGI badness here.
			DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			uint32 NumModes = 0;
			HResult = Output->GetDisplayModeList(Format,0,&NumModes,NULL);
			if (HResult == DXGI_ERROR_NOT_FOUND)
			{
				return;
			}
			else if (HResult == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				UE_LOG(LogD3D12RHI, Fatal,
					TEXT("This application cannot be run over a remote desktop configuration")
					);
				return;
			}
			DXGI_MODE_DESC* ModeList = new DXGI_MODE_DESC[ NumModes ];
			VERIFYD3D11RESULT(Output->GetDisplayModeList(Format,0,&NumModes,ModeList));

			for(uint32 m = 0;m < NumModes;m++)
			{
				// Search for the best mode

				// Suppress static analysis warnings about a potentially out-of-bounds read access to ModeList. This is a false positive - Index is always within range.
				CA_SUPPRESS( 6385 );
				bool IsEqualOrBetterWidth = FMath::Abs((int32)ModeList[m].Width - (int32)Width) <= FMath::Abs((int32)BestMode.Width - (int32)Width);
				bool IsEqualOrBetterHeight = FMath::Abs((int32)ModeList[m].Height - (int32)Height) <= FMath::Abs((int32)BestMode.Height - (int32)Height);
				if (!InitializedMode || (IsEqualOrBetterWidth && IsEqualOrBetterHeight))
				{
					BestMode = ModeList[m];
					InitializedMode = true;
				}
			}

			delete [] ModeList;
		}
#endif // PLATFORM_XBOXONE
	}

	check(InitializedMode);
	Width = BestMode.Width;
	Height = BestMode.Height;
}

// Suppress static analysis warnings in FD3D12DynamicRHI::RHIGetAvailableResolutions() about a potentially out-of-bounds read access to ModeList. This is a false positive - Index is always within range.
#if USING_CODE_ANALYSIS
MSVC_PRAGMA(warning(push))
	MSVC_PRAGMA(warning(disable:6385))
#endif	// USING_CODE_ANALYSIS

	// Re-enable static code analysis warning C6385.
#if USING_CODE_ANALYSIS
	MSVC_PRAGMA(warning(pop))
#endif	// USING_CODE_ANALYSIS

	void FD3D12DynamicRHI::GetBestSupportedMSAASetting( DXGI_FORMAT PlatformFormat, uint32 MSAACount, uint32& OutBestMSAACount, uint32& OutMSAAQualityLevels )
{
	//  We disable MSAA for Feature level 10
	if (GMaxRHIFeatureLevel == ERHIFeatureLevel::SM4)
	{
		OutBestMSAACount = 1;
		OutMSAAQualityLevels = 0;
		return;
	}

	// start counting down from current setting (indicated the current "best" count) and move down looking for support
	for (uint32 SampleCount = MSAACount; SampleCount > 0; SampleCount--)
	{
		// The multisampleQualityLevels struct serves as both the input and output to CheckFeatureSupport.
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQualityLevels = {};
		multisampleQualityLevels.SampleCount = SampleCount;

        if (SUCCEEDED(GetRHIDevice()->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisampleQualityLevels, sizeof(multisampleQualityLevels))))
		{
			OutBestMSAACount = SampleCount;
			OutMSAAQualityLevels = multisampleQualityLevels.NumQualityLevels;
			break;
		}
	}

	return;
}

uint32 FD3D12DynamicRHI::GetMaxMSAAQuality(uint32 SampleCount)
{
	if (SampleCount <= DX_MAX_MSAA_COUNT)
	{
		// 0 has better quality (a more even distribution)
		// higher quality levels might be useful for non box filtered AA or when using weighted samples 
		return 0;
		//		return AvailableMSAAQualities[SampleCount];
	}
	// not supported
	return 0xffffffff;
}

void FD3D12Device::CreateSignatures()
{
	// Graphics
	struct
	{
		D3D12_SHADER_VISIBILITY Vis;
		D3D12_DESCRIPTOR_RANGE_TYPE Type;
		uint32 Count;
		uint32 BaseShaderReg;
	} RangeDesc[Graphics_DescriptorTableCount] =
	{
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0 },
	};

	CD3DX12_ROOT_PARAMETER TableSlots[Graphics_DescriptorTableCount];
	CD3DX12_DESCRIPTOR_RANGE DescriptorRanges[Graphics_DescriptorTableCount];

	for (uint32 i = 0; i < Graphics_DescriptorTableCount; i++)
	{
		DescriptorRanges[i].Init(
			RangeDesc[i].Type,
			RangeDesc[i].Count,
			RangeDesc[i].BaseShaderReg
			);

		TableSlots[i].InitAsDescriptorTable(1, &DescriptorRanges[i], RangeDesc[i].Vis);
	}

	{
		CD3DX12_ROOT_SIGNATURE_DESC RootDesc;
		RootDesc.Init(Graphics_DescriptorTableCount, TableSlots, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		TRefCountPtr<ID3DBlob> SerializedRS;
		VERIFYD3D11RESULT(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1, SerializedRS.GetInitReference(), nullptr));
		VERIFYD3D11RESULT(GetDevice()->CreateRootSignature(0, SerializedRS->GetBufferPointer(), SerializedRS->GetBufferSize(), IID_PPV_ARGS(GraphicsRS.GetInitReference())));
	}

	// Compute
	uint32 RangeIndex = 0;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;

	{
		CD3DX12_ROOT_SIGNATURE_DESC RootDesc;
		RootDesc.Init(RangeIndex, TableSlots, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

		TRefCountPtr<ID3DBlob> SerializedRS;
		VERIFYD3D11RESULT(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1, SerializedRS.GetInitReference(), nullptr));
		VERIFYD3D11RESULT(GetDevice()->CreateRootSignature(0, SerializedRS->GetBufferPointer(), SerializedRS->GetBufferSize(), IID_PPV_ARGS(ComputeRS.GetInitReference())));
	}

	// ExecuteIndirect command signatures
	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	commandSignatureDesc.NumArgumentDescs = 1;
	commandSignatureDesc.ByteStride = 20;

	D3D12_INDIRECT_ARGUMENT_DESC indirectParameterDesc[1] = {};
    indirectParameterDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	commandSignatureDesc.pArgumentDescs = indirectParameterDesc;

	VERIFYD3D11RESULT(GetDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(DrawIndirectCommandSignature.GetInitReference())));

	indirectParameterDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	VERIFYD3D11RESULT(GetDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(DrawIndexedIndirectCommandSignature.GetInitReference())));

    indirectParameterDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	VERIFYD3D11RESULT(GetDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(DispatchIndirectCommandSignature.GetInitReference())));

	CD3DX12_HEAP_PROPERTIES CounterHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC CounterBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(COUNTER_HEAP_SIZE);

	VERIFYD3D11RESULT(
		GetDevice()->CreateCommittedResource(
		&CounterHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CounterBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(CounterUploadHeap.GetInitReference())
		)
		);

	VERIFYD3D11RESULT(
		CounterUploadHeap->Map (
		0,
		NULL,
		&CounterUploadHeapData)
		);

	CounterUploadHeapIndex = 0;
}

void FD3D12Device::SetupAfterDeviceCreation()
{
	CreateSignatures();

    PipelineStateCache = FD3D12PipelineStateCache(this);
    FString GraphicsCacheFile = FPaths::GameSavedDir() / TEXT("D3DGraphics.ushaderprecache");
    FString ComputeCacheFile = FPaths::GameSavedDir() / TEXT("D3DCompute.ushaderprecache");

	PipelineStateCache.Init(GraphicsCacheFile, ComputeCacheFile);
	PipelineStateCache.RebuildFromDiskCache(GraphicsRS, ComputeRS);

	CreateCommandContexts();

	UpdateMSAASettings();

	if (GRHISupportsAsyncTextureCreation)
	{
		UE_LOG(LogD3D12RHI, Log, TEXT("Async texture creation enabled"));
	}
	else
	{
		UE_LOG(LogD3D12RHI, Log, TEXT("Async texture creation disabled: %s"),
			D3D12RHI_ShouldAllowAsyncResourceCreation() ? TEXT("no driver support") : TEXT("disabled by user"));
	}
}

void FD3D12Device::UpdateMSAASettings()
{	
	check(DX_MAX_MSAA_COUNT == 8);

	// quality levels are only needed for CSAA which we cannot use with custom resolves

	// 0xffffffff means not available
	AvailableMSAAQualities[0] = 0xffffffff;
	AvailableMSAAQualities[1] = 0xffffffff;
	AvailableMSAAQualities[2] = 0;
	AvailableMSAAQualities[3] = 0xffffffff;
	AvailableMSAAQualities[4] = 0;
	AvailableMSAAQualities[5] = 0xffffffff;
	AvailableMSAAQualities[6] = 0xffffffff;
	AvailableMSAAQualities[7] = 0xffffffff;
	AvailableMSAAQualities[8] = 0;
}

void FD3D12Device::CleanupD3DDevice()
{
	if (GIsRHIInitialized)
	{
		// Ensure any pending rendering is finished
		GetCommandListManager().SignalFrameComplete(true);

		check(Direct3DDevice);

        PipelineStateCache.Close();

		// Reset the RHI initialized flag.
		GIsRHIInitialized = false;

		check(!GIsCriticalError);

		// Ask all initialized FRenderResources to release their RHI resources.
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			FRenderResource* Resource = *ResourceIt;
			check(Resource->IsInitialized());
			Resource->ReleaseRHI();
		}

		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseDynamicRHI();
		}

		Viewports.Empty();
		DrawingViewport = nullptr;
		SamplerMap.Empty();

		ReleasePooledUniformBuffers();
		ReleasePooledTextures();

		// Wait for the command queues to flush
		CommandListManager.WaitForCommandQueueFlush();
		CopyCommandListManager.WaitForCommandQueueFlush();

		// Delete array index 0 (the default context) last
		for (int32 i = CommandContextArray.Num() - 1; i > 0; i--)
		{
			delete CommandContextArray[i];
			CommandContextArray[i] = nullptr;
		}

		// This is needed so the deferred deletion queue empties out completely because the resources in the
		// queue use the frame fence values. This includes any deleted resources that may have been deleted 
		// after the above signal frame
		GetCommandListManager().SignalFrameComplete(true);

		// Cleanup resources
		DeferredDeletionQueue.Clear();

		// Cleanup the allocator near the end, as some resources may be returned to the allocator
		DefaultBufferAllocator.FreeDefaultBufferPools();
		/*
		// Cleanup thread resources
        for (uint32 index; (index = InterlockedDecrement(&NumThreadDynamicHeapAllocators)) != (uint32)-1;)
		{
            FD3D12DynamicHeapAllocator* pHeapAllocator = ThreadDynamicHeapAllocatorArray[index];
			pHeapAllocator->ReleaseAllResources();
			delete(pHeapAllocator);
		}
		*/

		// enable the code below to view detailed resource leaks report
		//TRefCountPtr<ID3D11Debug> d3dDebug;
		//if (SUCCEEDED(Direct3DDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)d3dDebug.GetInitReference())))
		//{
		//          d3dDebug->ReportLiveDeviceObjects((D3D11_RLDO_FLAGS)0x2);
		//}

		CommandListManager.Destroy();
		CopyCommandListManager.Destroy();

		// Flush all pending deletes before destroying the device.
		FRHIResource::FlushPendingDeletes();

		Direct3DDevice = nullptr;
	}
}

void FD3D12DynamicRHI::RHIFlushResources()
{
	// Nothing to do (yet!)
}

void FD3D12DynamicRHI::RHIAcquireThreadOwnership()
{
}

void FD3D12DynamicRHI::RHIReleaseThreadOwnership()
{
	// Nothing to do
}

void FD3D12CommandContext::RHIAutomaticCacheFlushAfterComputeShader(bool bEnable)
{
	StateCache.AutoFlushComputeShaderCache(bEnable);
}

void FD3D12CommandContext::RHIFlushComputeShaderCache()
{
	StateCache.FlushComputeShaderCache(true);
}

void* FD3D12DynamicRHI::RHIGetNativeDevice()
{
    return (void*)GetRHIDevice()->GetDevice();
}
