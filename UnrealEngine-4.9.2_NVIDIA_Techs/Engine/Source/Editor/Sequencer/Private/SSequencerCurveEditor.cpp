// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SCurveEditor.h"
#include "Sequencer.h"
#include "TimeSliderController.h"

#define LOCTEXT_NAMESPACE "SequencerCurveEditor"

void SSequencerCurveEditor::Construct( const FArguments& InArgs, TSharedRef<FSequencer> InSequencer, TSharedRef<ITimeSliderController> InTimeSliderController )
{
	Sequencer = InSequencer;
	SequencerSettings = InSequencer->GetSettings();

	TimeSliderController = InTimeSliderController;
	NodeTreeSelectionChangedHandle = Sequencer.Pin()->GetSelection().GetOnOutlinerNodeSelectionChanged().AddSP(this, &SSequencerCurveEditor::NodeTreeSelectionChanged);

	// @todo sequencer: switch to lambda capture expressions when support is introduced?
	auto ViewRange = InArgs._ViewRange;
	auto OnViewRangeChanged = InArgs._OnViewRangeChanged;

	SCurveEditor::Construct(
		SCurveEditor::FArguments()
		.ViewMinInput_Lambda( [=]{ return ViewRange.Get().GetLowerBoundValue(); } )
		.ViewMaxInput_Lambda( [=]{ return ViewRange.Get().GetUpperBoundValue(); } )
		.OnSetInputViewRange_Lambda( [=](float InLowerBound, float InUpperBound){ OnViewRangeChanged.ExecuteIfBound(TRange<float>(InLowerBound, InUpperBound), EViewRangeInterpolation::Immediate); } )
		.HideUI( false )
		.ZoomToFitHorizontal( true )
		.ShowCurveSelector( false )
		.ShowZoomButtons( false )
		.ShowInputGridNumbers( false )
		.SnappingEnabled( this, &SSequencerCurveEditor::GetCurveSnapEnabled )
		.InputSnap( this, &SSequencerCurveEditor::GetCurveTimeSnapInterval )
		.OutputSnap( this, &SSequencerCurveEditor::GetCurveValueSnapInterval )
		.TimelineLength( 0.0f )
		.GridColor( FLinearColor( 0.3f, 0.3f, 0.3f, 0.3f ) )
	);

	GetSettings()->GetOnCurveEditorCurveVisibilityChanged().AddSP(this, &SSequencerCurveEditor::OnCurveEditorCurveVisibilityChanged);
}

FReply SSequencerCurveEditor::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Curve Editor takes precedence
	FReply Reply = SCurveEditor::OnMouseButtonDown( MyGeometry, MouseEvent );
	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}

	return TimeSliderController->OnMouseButtonDown( AsShared(), MyGeometry, MouseEvent );
}


FReply SSequencerCurveEditor::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Curve Editor takes precedence
	FReply Reply = SCurveEditor::OnMouseButtonUp( MyGeometry, MouseEvent );
	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}

	return TimeSliderController->OnMouseButtonUp( AsShared(), MyGeometry, MouseEvent );
}


FReply SSequencerCurveEditor::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = SCurveEditor::OnMouseMove( MyGeometry, MouseEvent );
	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}

	return TimeSliderController->OnMouseMove( AsShared(), MyGeometry, MouseEvent );
}

FReply SSequencerCurveEditor::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = TimeSliderController->OnMouseWheel( AsShared(), MyGeometry, MouseEvent );
	if ( Reply.IsEventHandled() )
	{
		return Reply;
	}

	return SCurveEditor::OnMouseWheel( MyGeometry, MouseEvent );
}

template<class ParentNodeType>
TSharedPtr<ParentNodeType> GetParentOfType( TSharedRef<FSequencerDisplayNode> InNode, ESequencerNode::Type InNodeType )
{
	TSharedPtr<FSequencerDisplayNode> CurrentNode = InNode->GetParent();
	while ( CurrentNode.IsValid() )
	{
		if ( CurrentNode->GetType() == InNodeType )
		{
			break;
		}
		CurrentNode = CurrentNode->GetParent();
	}
	return StaticCastSharedPtr<ParentNodeType>( CurrentNode );
}

void SSequencerCurveEditor::SetSequencerNodeTree( TSharedPtr<FSequencerNodeTree> InSequencerNodeTree )
{
	SequencerNodeTree = InSequencerNodeTree;
	UpdateCurveOwner();
}

void SSequencerCurveEditor::UpdateCurveOwner()
{
	FSequencerCurveOwner* NewCurveOwner = new FSequencerCurveOwner( SequencerNodeTree, GetSettings()->GetCurveVisibility() );

	bool bAllFound = false;
	if (NewCurveOwner != NULL && CurveOwner.IsValid())
	{
		TSet<FName> NewCurveOwnerCurveNames;
		if (NewCurveOwner != NULL)
		{
			for (auto Curve : NewCurveOwner->GetCurves())
			{
				NewCurveOwnerCurveNames.Add(Curve.CurveName);
			}
		}

		TSet<FName> CurveOwnerCurveNames;
		if (CurveOwner.Get() != NULL)
		{
			if (CurveOwner.Get()->GetCurves().Num() == NewCurveOwner->GetCurves().Num())
			{
				bAllFound = true;
				for (auto Curve : CurveOwner.Get()->GetCurves())
				{
					if (!NewCurveOwnerCurveNames.Contains(Curve.CurveName))
					{
						bAllFound = false;
					}
				}
			}
		}
	}

	if (bAllFound)
	{
		UpdateCurveViewModelSelection();
		return;
	}

	CurveOwner = MakeShareable( NewCurveOwner );
	SetCurveOwner( CurveOwner.Get() );

	UpdateCurveViewModelSelection();
}

bool SSequencerCurveEditor::GetCurveSnapEnabled() const
{
	return SequencerSettings->GetIsSnapEnabled();
}

float SSequencerCurveEditor::GetCurveTimeSnapInterval() const
{
	return SequencerSettings->GetSnapKeyTimesToInterval()
		? SequencerSettings->GetTimeSnapInterval()
		: 0;
}

float SSequencerCurveEditor::GetCurveValueSnapInterval() const
{
	return SequencerSettings->GetSnapCurveValueToInterval()
		? SequencerSettings->GetCurveValueSnapInterval()
		: 0;
}

void SSequencerCurveEditor::NodeTreeSelectionChanged()
{
	if (SequencerNodeTree.IsValid())
	{
		if (GetSettings()->GetCurveVisibility() == ECurveEditorCurveVisibility::SelectedCurves)
		{
			UpdateCurveOwner();
		}

		UpdateCurveViewModelSelection();
	}
}

void SSequencerCurveEditor::UpdateCurveViewModelSelection()
{
	ClearSelectedCurveViewModels();
	for (auto SelectedCurve : CurveOwner->GetSelectedCurves())
	{
		SetSelectedCurveViewModel(SelectedCurve);
	}
}

void SSequencerCurveEditor::OnCurveEditorCurveVisibilityChanged()
{
	UpdateCurveOwner();
}

SSequencerCurveEditor::~SSequencerCurveEditor()
{
	if ( Sequencer.IsValid() )
	{
		Sequencer.Pin()->GetSelection().GetOnOutlinerNodeSelectionChanged().Remove( NodeTreeSelectionChangedHandle );
	}

	GetSettings()->GetOnCurveEditorCurveVisibilityChanged().RemoveAll(this);
}

#undef LOCTEXT_NAMESPACE