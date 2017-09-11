// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Wraps and builds a toolbar which works with the SSequencerCurveEditor. */
class SSequencerCurveEditorToolBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSequencerCurveEditorToolBar )
	{}

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedPtr<FUICommandList> CurveEditorCommandList );

private:
	/** Makes the curve editor view options menu for the toolbar. */
	TSharedRef<SWidget> MakeCurveEditorViewOptionsMenu(TSharedPtr<FUICommandList> CurveEditorCommandList);
	/** Makes the curve editor curve options menu for the toolbar. */
	TSharedRef<SWidget> MakeCurveEditorCurveOptionsMenu(TSharedPtr<FUICommandList> CurveEditorCommandList);
};