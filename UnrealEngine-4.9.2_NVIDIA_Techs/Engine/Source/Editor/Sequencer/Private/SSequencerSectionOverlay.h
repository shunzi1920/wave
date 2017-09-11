// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * An overlay that displays global information in the section area
 */
class SSequencerSectionOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSequencerSectionOverlay )
		: _DisplayTickLines( true )
		, _DisplayScrubPosition( false )
	{}
		SLATE_ATTRIBUTE( bool, DisplayTickLines )
		SLATE_ATTRIBUTE( bool, DisplayScrubPosition )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<class FSequencerTimeSliderController> InTimeSliderController )
	{
		bDisplayScrubPosition = InArgs._DisplayScrubPosition;
		bDisplayTickLines = InArgs._DisplayTickLines;
		TimeSliderController = InTimeSliderController;
	}

private:
	/** SWidget Interface */
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

private:
	/** Controller for manipulating time */
	TSharedPtr<class FSequencerTimeSliderController> TimeSliderController;
	/** Whether or not to display the scrub position */
	TAttribute<bool> bDisplayScrubPosition;
	/** Whether or not to display tick lines */
	TAttribute<bool> bDisplayTickLines;

};