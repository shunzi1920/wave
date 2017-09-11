// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"
#include "SubMovieSceneSection.h"
#include "MovieSceneShotSection.generated.h"

/**
 * Movie shots are sections on the shots track, that show what the viewer "sees"
 */
UCLASS( MinimalAPI )
class UMovieSceneShotSection : public USubMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	/** Sets the camera guid for this shot section */
	void SetCameraGuid(const FGuid& InGuid);

	/** Gets the camera guid for this shot section */
	virtual FGuid GetCameraGuid() const;

	/** 
	 * Sets the shot display name and shot number
	 * 
 	 * @param InDisplayName	The display name shown next to each shot in Sequencer
	 * @param InShotNumber	The generated shot number.
	 */
	virtual void SetShotNameAndNumber(const FText& InDisplayName, int32 InShotNumber);

	/** @return The display name for the shot */
	FText GetShotDisplayName() const { return DisplayName; }

	/** @return The shot number generated for this shot */
	int32 GetShotNumber() const { return ShotNumber; }
private:
	/** The camera possessable or spawnable that this movie shot uses */
	UPROPERTY()
	FGuid CameraGuid;

	/** The shot's display name */
	UPROPERTY()
	FText DisplayName;
	
	 /** The Shot number.  Shot numbers are generally part of the display name but is stored seperatley so we can use it to generate subsequent shot numbers */
	UPROPERTY()
	int32 ShotNumber;
};
