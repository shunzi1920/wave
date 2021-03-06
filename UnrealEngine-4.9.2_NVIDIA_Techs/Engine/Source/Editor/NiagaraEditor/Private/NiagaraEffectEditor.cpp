// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "NiagaraEffect.h"

#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "SDockTab.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectEditor"


const FName FNiagaraEffectEditor::UpdateTabId(TEXT("NiagaraEditor_Effect"));
const FName FNiagaraEffectEditor::ViewportTabID(TEXT("NiagaraEffectEditor_Viewport"));
const FName FNiagaraEffectEditor::EmitterEditorTabID(TEXT("NiagaraEffectEditor_EmitterEditor"));
const FName FNiagaraEffectEditor::DevEmitterEditorTabID(TEXT("NiagaraEffectEditor_DevEmitterEditor"));
const FName FNiagaraEffectEditor::CurveEditorTabID(TEXT("NiagaraEffectEditor_CurveEditor"));
const FName FNiagaraEffectEditor::SequencerTabID(TEXT("NiagaraEffectEditor_Sequencer"));

void FNiagaraEffectEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_NiagaraEffectEditor", "Niagara Effect"));

	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	TabManager->RegisterTabSpawner(ViewportTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("Preview", "Preview"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	TabManager->RegisterTabSpawner(EmitterEditorTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab_EmitterList))
		.SetDisplayName(LOCTEXT("Emitters", "Emitters"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	TabManager->RegisterTabSpawner(DevEmitterEditorTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab_DevEmitterList))
		.SetDisplayName(LOCTEXT("DevEmitters", "Emitters_Dev"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	TabManager->RegisterTabSpawner(CurveEditorTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab_CurveEd))
		.SetDisplayName(LOCTEXT("Curves", "Curves"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	TabManager->RegisterTabSpawner(SequencerTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab_Sequencer))
		.SetDisplayName(LOCTEXT("Timeline", "Timeline"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());


	TabManager->RegisterTabSpawner(UpdateTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab))
		.SetDisplayName(LOCTEXT("NiagaraEffect", "Niagara Effect"))
		.SetGroup( WorkspaceMenuCategory.ToSharedRef() );


}

void FNiagaraEffectEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(UpdateTabId);
	TabManager->UnregisterTabSpawner(ViewportTabID);
	TabManager->UnregisterTabSpawner(EmitterEditorTabID);
	TabManager->UnregisterTabSpawner(DevEmitterEditorTabID);
	TabManager->UnregisterTabSpawner(CurveEditorTabID);
	TabManager->UnregisterTabSpawner(SequencerTabID);
}




FNiagaraEffectEditor::~FNiagaraEffectEditor()
{

}


void FNiagaraEffectEditor::InitNiagaraEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEffect* InEffect)
{
	Effect = InEffect;
	check(Effect != NULL);
	EffectInstance = new FNiagaraEffectInstance(InEffect);

	if (!Sequencer.IsValid())
	{
		FSequencerViewParams ViewParams( TEXT( "NiagaraSequencerSettings" ) );
		ViewParams.InitalViewRange = TRange<float>(-0.02f, 3.2f);
		ViewParams.InitialScrubPosition = 0;

		SequencerBindingManager = MakeShareable(new FNiagaraSequencerObjectBindingManager());

		MovieScene = NewObject<UMovieScene>(InEffect, FName("Niagara Effect MovieScene"), RF_RootSet);

		ISequencerModule &SeqModule = FModuleManager::LoadModuleChecked< ISequencerModule >("Sequencer");
		FDelegateHandle CreateTrackEditorHandle = SeqModule.RegisterTrackEditor_Handle(FOnCreateTrackEditor::CreateStatic(&FNiagaraEffectEditor::CreateTrackEditor));
		Sequencer = SeqModule.CreateSequencer(MovieScene, ViewParams, SequencerBindingManager.ToSharedRef());

		for (TSharedPtr<FNiagaraSimulation> Emitter : EffectInstance->GetEmitters())
		{
			UEmitterMovieSceneTrack *Track = Cast<UEmitterMovieSceneTrack> (MovieScene->AddMasterTrack(UEmitterMovieSceneTrack::StaticClass()) );
			 Track->SetEmitter(Emitter);
		}

	}




	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Niagara_Effect_Layout_v7")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->AddTab(ViewportTabID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(EmitterEditorTabID, ETabState::OpenedTab)
					->AddTab(DevEmitterEditorTabID, ETabState::OpenedTab)
				)
			)
			->Split
			(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.3f)
			->AddTab(CurveEditorTabID, ETabState::OpenedTab)
			->AddTab(SequencerTabID, ETabState::OpenedTab)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Effect);
	
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddMenuExtender(NiagaraEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

FName FNiagaraEffectEditor::GetToolkitFName() const
{
	return FName("Niagara");
}

FText FNiagaraEffectEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Niagara");
}

FString FNiagaraEffectEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Niagara ").ToString();
}


FLinearColor FNiagaraEffectEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}

void FNiagaraEffectEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	//Rebuild the effect after every edit.
// 	if (EffectInstance)
// 	{
// 		EffectInstance->Init();
// 	}
}

/** Create new tab for the supplied graph - don't call this directly, call SExplorer->FindTabForGraph.*/
TSharedRef<SNiagaraEffectEditorWidget> FNiagaraEffectEditor::CreateEditorWidget(UNiagaraEffect* InEffect)
{
	check(InEffect != NULL);
	EffectInstance = new FNiagaraEffectInstance(InEffect);
	
	if (!EditorCommands.IsValid())
	{
		EditorCommands = MakeShareable(new FUICommandList);
	}

	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "NIAGARA");

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EffectLabel", "Niagara Effect"))
				.TextStyle(FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
			]
		];

		
		
	// Make the effect editor widget
	return SNew(SNiagaraEffectEditorWidget).EffectObj(InEffect).EffectInstance(EffectInstance).EffectEditor(this);
}


TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == UpdateTabId);

	TSharedRef<SNiagaraEffectEditorWidget> Editor = CreateEditorWidget(Effect);

	UpdateEditorPtr = Editor; // Keep pointer to editor

	return SNew(SDockTab)
		.Label(LOCTEXT("NiagaraEffect", "Niagara Effect"))
		.TabColorScale(GetTabColorScale())
		[
			Editor
		];
}




TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == ViewportTabID);

	Viewport = SNew(SNiagaraEffectEditorViewport);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			Viewport.ToSharedRef()
		];

	Viewport->SetPreviewEffect(EffectInstance);
	Viewport->OnAddedToTab(SpawnedTab);

	return SpawnedTab;
}



TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab_EmitterList(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == EmitterEditorTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SAssignNew(EmitterEditorWidget, SNiagaraEffectEditorWidget).EffectInstance(EffectInstance).EffectEditor(this)
		];


	return SpawnedTab;
}

TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab_DevEmitterList(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == DevEmitterEditorTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SAssignNew(DevEmitterEditorWidget, SNiagaraEffectEditorWidget).EffectInstance(EffectInstance).EffectEditor(this).bForDev(true)
		];


	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab_CurveEd(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == CurveEditorTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SAssignNew(TimeLine, SNiagaraTimeline)
		];

	return SpawnedTab;
}



TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab_Sequencer(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == SequencerTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			Sequencer->GetSequencerWidget()
		];

	return SpawnedTab;
}





void FNiagaraEffectEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TSharedRef<SWidget> AddEmitterBox)
		{
			ToolbarBuilder.BeginSection("AddEmitter");
			{
				ToolbarBuilder.AddWidget(AddEmitterBox);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	TSharedRef<SWidget> AddEmitterBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SButton)
			.OnClicked(this, &FNiagaraEffectEditor::OnAddEmitterClicked)
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("LevelEditor.Add"))
					]
				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NiagaraToolbar_AddEmitter", "Add Emitter"))
					]
			]
		];

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, AddEmitterBox)
		);

	AddToolbarExtender(ToolbarExtender);

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddToolbarExtender(NiagaraEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}



FReply FNiagaraEffectEditor::OnAddEmitterClicked()
{
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");

	UNiagaraEmitterProperties *Props = Effect->AddEmitterProperties();
	TSharedPtr<FNiagaraSimulation> NewEmitter = EffectInstance->AddEmitter(Props);
	Effect->CreateEffectRendererProps(NewEmitter);
	Viewport->SetPreviewEffect(EffectInstance);
	EmitterEditorWidget->UpdateList();
	DevEmitterEditorWidget->UpdateList();
	
	Effect->MarkPackageDirty();

	return FReply::Handled();
}

FReply FNiagaraEffectEditor::OnDuplicateEmitterClicked(TSharedPtr<FNiagaraSimulation> Emitter)
{
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");

	if (UNiagaraEmitterProperties* ToDupe = Emitter->GetProperties().Get())
	{
		UNiagaraEmitterProperties *Props = CastChecked<UNiagaraEmitterProperties>(StaticDuplicateObject(ToDupe,Effect,NULL));
		Effect->AddEmitterProperties(Props);
		TSharedPtr<FNiagaraSimulation> NewEmitter = EffectInstance->AddEmitter(Props);
		Effect->CreateEffectRendererProps(NewEmitter);
		Viewport->SetPreviewEffect(EffectInstance);
		EmitterEditorWidget->UpdateList();
		DevEmitterEditorWidget->UpdateList();

		Effect->MarkPackageDirty();
	}
	return FReply::Handled();
}

FReply FNiagaraEffectEditor::OnDeleteEmitterClicked(TSharedPtr<FNiagaraSimulation> Emitter)
{
	if (UNiagaraEmitterProperties* ToDelete = Emitter->GetProperties().Get())
	{
		Effect->DeleteEmitterProperties(ToDelete);
		EffectInstance->DeleteEmitter(Emitter);
		Viewport->SetPreviewEffect(EffectInstance);
		EmitterEditorWidget->UpdateList();
		DevEmitterEditorWidget->UpdateList();

		Effect->MarkPackageDirty();
	}
	return FReply::Handled();
}


FReply FNiagaraEffectEditor::OnEmitterSelected(TSharedPtr<FNiagaraSimulation> SelectedItem, ESelectInfo::Type SelType)
{
 	if (SelectedItem.Get() != nullptr)
	{
		if (UNiagaraEmitterProperties* PinnedProps = SelectedItem->GetProperties().Get())
		{
			if (PinnedProps->UpdateScriptProps.ExternalConstants.GetNumDataObjectConstants() > 0)
			{
				FNiagaraVariableInfo VarInfo;
				UNiagaraDataObject* DataObj;
				PinnedProps->UpdateScriptProps.ExternalConstants.GetDataObjectConstant(0, DataObj, VarInfo);

				if (UNiagaraCurveDataObject* CurvObj = Cast<UNiagaraCurveDataObject>(DataObj))
				{
					TimeLine.Get()->SetCurve(CurvObj->GetCurveObject());
				}
			}
		}
	}
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
