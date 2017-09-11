// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "NiagaraEditorPrivatePCH.h"
#include "NiagaraSequencer.h"


UEmitterMovieSceneTrack::UEmitterMovieSceneTrack(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UNiagaraMovieSceneSection::UNiagaraMovieSceneSection(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}


/*
 *  This is called when the user edits a track in the effect editor timeline
 */
void INiagaraTrackInstance::RefreshInstance(const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player)
{
	// every track should have exactly one section, always
	check(Track->GetAllSections().Num() == 1)

	UNiagaraMovieSceneSection *Section = Cast<UNiagaraMovieSceneSection>( Track->GetAllSections()[0] );
	
	check(Section);

	Track->GetEmitter()->GetProperties()->StartTime = Section->GetStartTime();
	Track->GetEmitter()->GetProperties()->EndTime = Section->GetEndTime();
}
