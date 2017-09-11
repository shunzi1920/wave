// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Sequencer/Public/MovieSceneTrackEditor.h"

class IPropertyHandle;

class FMarginTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	FMarginTrackEditor( TSharedRef<ISequencer> InSequencer );
	~FMarginTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) override;

private:
	/**
	 * Called by the details panel when an animatable property changes
	 *
	 * @param InObjectsThatChanged	List of objects that changed
	 * @param PropertyValue			Handle to the property value which changed
	 */
	void OnMarginChanged( const class FPropertyChangedParams& PropertyChangedParams );

	/** Called After OnMarginChanged if we actually can key the margin */
	void OnKeyMargin( float KeyTime, const class FPropertyChangedParams* PropertyChangedParams );
};


