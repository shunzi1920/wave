// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class FTrackInstancePropertyBindings;
class UMovieSceneByteTrack;


/**
 * Instance of a UMovieSceneByteTrack
 */
class FMovieSceneByteTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneByteTrackInstance( UMovieSceneByteTrack& InByteTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}

private:

	/** Byte track that is being instanced */
	UMovieSceneByteTrack* ByteTrack;

	/** Runtime property bindings */
	TSharedPtr<FTrackInstancePropertyBindings> PropertyBindings;

	/** Map from object to initial state */
	TMap< TWeakObjectPtr<UObject>, uint8 > InitByteMap;
};
