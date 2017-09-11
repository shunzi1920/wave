// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

class ITimeSlider;

/**
 * The public interface of SequencerModule
 */
class FSequencerWidgetsModule : public IModuleInterface
{

public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	virtual TSharedRef<ITimeSlider> CreateTimeSlider( const TSharedRef<class ITimeSliderController>& InController, bool bMirrorLabels );
};

