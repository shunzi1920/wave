// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"

USequencerSettings* USequencerSettingsContainer::GetOrCreate(const TCHAR* InName)
{
	static const TCHAR* SettingsContainerName = TEXT("SequencerSettingsContainer");

	auto* Outer = FindObject<USequencerSettingsContainer>(GetTransientPackage(), SettingsContainerName);
	if (!Outer)
	{
		Outer = NewObject<USequencerSettingsContainer>(GetTransientPackage(), USequencerSettingsContainer::StaticClass(), SettingsContainerName);
		Outer->AddToRoot();
	}
	
	USequencerSettings* Inst = FindObject<USequencerSettings>( Outer, InName );
	if (!Inst)
	{
		Inst = NewObject<USequencerSettings>( Outer, USequencerSettings::StaticClass(), InName );
		Inst->LoadConfig();
	}

	return Inst;
}

USequencerSettings::USequencerSettings( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	bAutoKeyEnabled = false;
	bIsSnapEnabled = true;
	TimeSnapInterval = .05f;
	bSnapKeyTimesToInterval = true;
	bSnapKeyTimesToKeys = true;
	bSnapSectionTimesToInterval = true;
	bSnapSectionTimesToSections = true;
	bSnapPlayTimeToInterval = true;
	bSnapPlayTimeToDraggedKey = false;
	CurveValueSnapInterval = 10.0f;
	bSnapCurveValueToInterval = true;
	bIsUsingCleanView = false;
	bAutoScrollEnabled = true;
	bShowCurveEditor = false;
	bShowCurveEditorCurveToolTips = true;
}

bool USequencerSettings::GetAutoKeyEnabled() const
{
	return bAutoKeyEnabled;
}

void USequencerSettings::SetAutoKeyEnabled(bool InbAutoKeyEnabled)
{
	if ( bAutoKeyEnabled != InbAutoKeyEnabled )
	{
		bAutoKeyEnabled = InbAutoKeyEnabled;
		SaveConfig();
	}
}

bool USequencerSettings::GetIsSnapEnabled() const
{
	return bIsSnapEnabled;
}

void USequencerSettings::SetIsSnapEnabled(bool InbIsSnapEnabled)
{
	if ( bIsSnapEnabled != InbIsSnapEnabled )
	{
		bIsSnapEnabled = InbIsSnapEnabled;
		SaveConfig();
	}
}

float USequencerSettings::GetTimeSnapInterval() const
{
	return TimeSnapInterval;
}

void USequencerSettings::SetTimeSnapInterval(float InTimeSnapInterval)
{
	if ( TimeSnapInterval != InTimeSnapInterval )
	{
		TimeSnapInterval = InTimeSnapInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapKeyTimesToInterval() const
{
	return bSnapKeyTimesToInterval;
}

void USequencerSettings::SetSnapKeyTimesToInterval(bool InbSnapKeyTimesToInterval)
{
	if ( bSnapKeyTimesToInterval != InbSnapKeyTimesToInterval )
	{
		bSnapKeyTimesToInterval = InbSnapKeyTimesToInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapKeyTimesToKeys() const
{
	return bSnapKeyTimesToKeys;
}

void USequencerSettings::SetSnapKeyTimesToKeys(bool InbSnapKeyTimesToKeys)
{
	if ( bSnapKeyTimesToKeys != InbSnapKeyTimesToKeys )
	{
		bSnapKeyTimesToKeys = InbSnapKeyTimesToKeys;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapSectionTimesToInterval() const
{
	return bSnapSectionTimesToInterval;
}

void USequencerSettings::SetSnapSectionTimesToInterval(bool InbSnapSectionTimesToInterval)
{
	if ( bSnapSectionTimesToInterval != InbSnapSectionTimesToInterval )
	{
		bSnapSectionTimesToInterval = InbSnapSectionTimesToInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapSectionTimesToSections() const
{
	return bSnapSectionTimesToSections;
}

void USequencerSettings::SetSnapSectionTimesToSections( bool InbSnapSectionTimesToSections )
{
	if ( bSnapSectionTimesToSections != InbSnapSectionTimesToSections )
	{
		bSnapSectionTimesToSections = InbSnapSectionTimesToSections;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapPlayTimeToInterval() const
{
	return bSnapPlayTimeToInterval;
}

void USequencerSettings::SetSnapPlayTimeToInterval(bool InbSnapPlayTimeToInterval)
{
	if ( bSnapPlayTimeToInterval != InbSnapPlayTimeToInterval )
	{
		bSnapPlayTimeToInterval = InbSnapPlayTimeToInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapPlayTimeToDraggedKey() const
{
	return bSnapPlayTimeToDraggedKey;
}

void USequencerSettings::SetSnapPlayTimeToDraggedKey(bool InbSnapPlayTimeToDraggedKey)
{
	if ( bSnapPlayTimeToDraggedKey != InbSnapPlayTimeToDraggedKey )
	{
		bSnapPlayTimeToDraggedKey = InbSnapPlayTimeToDraggedKey;
		SaveConfig();
	}
}

float USequencerSettings::GetCurveValueSnapInterval() const
{
	return CurveValueSnapInterval;
}

void USequencerSettings::SetCurveValueSnapInterval( float InCurveValueSnapInterval )
{
	if ( CurveValueSnapInterval != InCurveValueSnapInterval )
	{
		CurveValueSnapInterval = InCurveValueSnapInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetSnapCurveValueToInterval() const
{
	return bSnapCurveValueToInterval;
}

void USequencerSettings::SetSnapCurveValueToInterval( bool InbSnapCurveValueToInterval )
{
	if ( bSnapCurveValueToInterval != InbSnapCurveValueToInterval )
	{
		bSnapCurveValueToInterval = InbSnapCurveValueToInterval;
		SaveConfig();
	}
}

bool USequencerSettings::GetIsUsingCleanView() const
{
	return bIsUsingCleanView;
}

void USequencerSettings::SetIsUsingCleanView(bool InbIsUsingCleanView )
{
	if (bIsUsingCleanView != InbIsUsingCleanView)
	{
		bIsUsingCleanView = InbIsUsingCleanView;
		SaveConfig();
	}
}

bool USequencerSettings::GetAutoScrollEnabled() const
{
	return bAutoScrollEnabled;
}

void USequencerSettings::SetAutoScrollEnabled(bool bInAutoScrollEnabled)
{
	if (bAutoScrollEnabled != bInAutoScrollEnabled)
	{
		bAutoScrollEnabled = bInAutoScrollEnabled;
		SaveConfig();
	}
}


bool USequencerSettings::GetShowCurveEditor() const
{
	return bShowCurveEditor;
}

void USequencerSettings::SetShowCurveEditor(bool InbShowCurveEditor)
{
	if (bShowCurveEditor != InbShowCurveEditor)
	{
		bShowCurveEditor = InbShowCurveEditor;
		OnShowCurveEditorChanged.Broadcast();
		SaveConfig();
	}
}

bool USequencerSettings::GetShowCurveEditorCurveToolTips() const
{
	return bShowCurveEditorCurveToolTips;
}

void USequencerSettings::SetShowCurveEditorCurveToolTips(bool InbShowCurveEditorCurveToolTips)
{
	if (bShowCurveEditorCurveToolTips != InbShowCurveEditorCurveToolTips)
	{
		bShowCurveEditorCurveToolTips = InbShowCurveEditorCurveToolTips;
		SaveConfig();
	}
}

float USequencerSettings::SnapTimeToInterval( float InTimeValue ) const
{
	return TimeSnapInterval > 0
		? FMath::RoundToInt( InTimeValue / TimeSnapInterval ) * TimeSnapInterval
		: InTimeValue;
}

USequencerSettings::FOnShowCurveEditorChanged& USequencerSettings::GetOnShowCurveEditorChanged()
{
	return OnShowCurveEditorChanged;
}

