// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "Toolkits/IToolkitHost.h"
#include "SequencerAssetEditor.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/ILevelViewport.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Sequencer.h"
#include "MovieScene.h"
#include "EditorWidgetsModule.h"
#include "SequencerActorBindingManager.h"
#include "SequencerObjectChangeListener.h"
#include "SDockTab.h"


#define LOCTEXT_NAMESPACE "Sequencer"

const FName FSequencerAssetEditor::SequencerMainTabId( TEXT( "Sequencer_SequencerMain" ) );


namespace SequencerDefs
{
	static const FName SequencerAppIdentifier( TEXT( "SequencerApp" ) );
}


void FSequencerAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if( FSequencer::IsSequencerEnabled() && !IsWorldCentricAssetEditor() )
	{
		WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SequencerAssetEditor", "Sequencer"));

		TabManager->RegisterTabSpawner( SequencerMainTabId, FOnSpawnTab::CreateSP(this, &FSequencerAssetEditor::SpawnTab_SequencerMain) )
			.SetDisplayName( LOCTEXT("SequencerMainTab", "Sequencer") )
			.SetGroup( WorkspaceMenuCategory.ToSharedRef() );
	}

}


void FSequencerAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	if( FSequencer::IsSequencerEnabled() && !IsWorldCentricAssetEditor() )
	{
		TabManager->UnregisterTabSpawner( SequencerMainTabId );
	}
	
	// @todo remove when world-centric mode is added
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.AttachSequencer( SNullWidget::NullWidget, nullptr );
}


void FSequencerAssetEditor::InitSequencerAssetEditor( const EToolkitMode::Type Mode, const FSequencerViewParams& InViewParams, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UMovieScene* InRootMovieScene, const TArray<FOnCreateTrackEditor>& TrackEditorDelegates, bool bEditWithinLevelEditor )
{
	{
		const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Sequencer_Layout")
			->AddArea
			(
				FTabManager::NewPrimaryArea()
					->Split
					(
						FTabManager::NewStack()
						->AddTab(SequencerMainTabId, ETabState::OpenedTab)
					)
			);

		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = false;
		FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, SequencerDefs::SequencerAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InRootMovieScene);
	}
	
	// Init Sequencer
	Sequencer = MakeShareable(new FSequencer);
	
	FSequencerInitParams SequencerInitParams;
	{
		SequencerInitParams.ViewParams = InViewParams;
		SequencerInitParams.ObjectChangeListener = MakeShareable(new FSequencerObjectChangeListener(Sequencer.ToSharedRef(), bEditWithinLevelEditor));
		SequencerInitParams.ObjectBindingManager = MakeShareable(new FSequencerActorBindingManager(SequencerInitParams.ObjectChangeListener.ToSharedRef(), Sequencer.ToSharedRef()));
		SequencerInitParams.RootMovieScene = InRootMovieScene;
		SequencerInitParams.bEditWithinLevelEditor = bEditWithinLevelEditor;
		SequencerInitParams.ToolkitHost = InitToolkitHost;
	}

	Sequencer->InitSequencer( SequencerInitParams, TrackEditorDelegates );

	if (bEditWithinLevelEditor)
	{
		// @todo remove when world-centric mode is added
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		LevelEditorModule.AttachSequencer( Sequencer->GetSequencerWidget(), SharedThis(this));
			
		// We need to find out when the user loads a new map, because we might need to re-create puppet actors
		// when previewing a MovieScene
		LevelEditorModule.OnMapChanged().AddSP(Sequencer.ToSharedRef(), &FSequencer::OnMapChanged);
	}
}


TSharedRef<ISequencer> FSequencerAssetEditor::GetSequencerInterface() const
{ 
	return Sequencer.ToSharedRef(); 
}


FSequencerAssetEditor::FSequencerAssetEditor()
{
	// Register sequencer menu extenders.
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );
	int32 NewIndex = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(
		FAssetEditorExtender::CreateRaw( this, &FSequencerAssetEditor::GetContextSensitiveSequencerExtender ) );
	SequencerExtenderHandle = SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates()[NewIndex].GetHandle();
}


FSequencerAssetEditor::~FSequencerAssetEditor()
{
	Sequencer->OnClose();

	// Unregister delegates
	if( FModuleManager::Get().IsModuleLoaded( TEXT( "LevelEditor" ) ) )
	{
		auto& LevelEditorModule = FModuleManager::LoadModuleChecked< FLevelEditorModule >( TEXT( "LevelEditor" ) );
		LevelEditorModule.OnMapChanged().RemoveAll( this );
	}

	// Un-Register sequencer menu extenders.
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>( "Sequencer" );
	SequencerModule.GetMenuExtensibilityManager()->GetExtenderDelegates().RemoveAll( [this]( const FAssetEditorExtender& Extender )
	{
		return SequencerExtenderHandle == Extender.GetHandle();
	} );
}


TSharedRef<SDockTab> FSequencerAssetEditor::SpawnTab_SequencerMain(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SequencerMainTabId);

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("Sequencer.Tabs.SequencerMain") )
		.Label( LOCTEXT("SequencerMainTitle", "Sequencer") )
		.TabColorScale( GetTabColorScale() )
		[
			Sequencer->GetSequencerWidget()
		];
}


FName FSequencerAssetEditor::GetToolkitFName() const
{
	static FName SequencerName("Sequencer");
	return SequencerName;
}


FText FSequencerAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Sequencer");
}


FLinearColor FSequencerAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.7, 0.0f, 0.0f, 0.5f );
}


FString FSequencerAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Sequencer ").ToString();
}

TSharedRef<FExtender> FSequencerAssetEditor::GetContextSensitiveSequencerExtender( const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> ContextSensitiveObjects )
{
	TSharedRef<FExtender> AddTrackMenuExtender( new FExtender() );
	AddTrackMenuExtender->AddMenuExtension(
		SequencerMenuExtensionPoints::AddTrackMenu_PropertiesSection,
		EExtensionHook::Before,
		CommandList,
		FMenuExtensionDelegate::CreateRaw( this, &FSequencerAssetEditor::ExtendSequencerAddTrackMenu, ContextSensitiveObjects ) );
	return AddTrackMenuExtender;
}

void FSequencerAssetEditor::ExtendSequencerAddTrackMenu( FMenuBuilder& AddTrackMenuBuilder, TArray<UObject*> ContextObjects )
{
	if ( ContextObjects.Num() == 1 )
	{
		AActor* Actor = Cast<AActor>( ContextObjects[0] );
		if ( Actor != nullptr )
		{
			AddTrackMenuBuilder.BeginSection( "Components", LOCTEXT( "ComponentsSection", "Components" ) );
			{
				for ( UActorComponent* Component : Actor->GetComponents() )
				{
					FUIAction AddComponentAction( FExecuteAction::CreateSP( this, &FSequencerAssetEditor::AddComponentTrack, Component ) );
					FText AddComponentLabel = FText::FromString(Component->GetName());
					FText AddComponentToolTip = FText::Format( LOCTEXT( "ComponentToolTipFormat", "Add {0} component" ), FText::FromString( Component->GetName() ) );
					AddTrackMenuBuilder.AddMenuEntry( AddComponentLabel, AddComponentToolTip, FSlateIcon(), AddComponentAction );
				}
			}
			AddTrackMenuBuilder.EndSection();
		}
	}
}

void FSequencerAssetEditor::AddComponentTrack( UActorComponent* Component )
{
	Sequencer->GetHandleToObject( Component );
}


#undef LOCTEXT_NAMESPACE
