// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneByteSection.h"
#include "MovieSceneByteTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneByteTrackInstance.h"


UMovieSceneByteTrack::UMovieSceneByteTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

UMovieSceneSection* UMovieSceneByteTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneByteSection::StaticClass(), NAME_None, RF_Transactional);
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneByteTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneByteTrackInstance( *this ) );
}

bool UMovieSceneByteTrack::AddKeyToSection( float Time, uint8 Value )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneByteSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		Modify();

		UMovieSceneByteSection* NewSection = CastChecked<UMovieSceneByteSection>(FindOrAddSection(  Time ));
	
		NewSection->AddKey( Time, Value );

		return true;
	}
	return false;
}

bool UMovieSceneByteTrack::Eval( float Position, float LastPostion, uint8& OutByte ) const
{	
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		OutByte = CastChecked<UMovieSceneByteSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}

void UMovieSceneByteTrack::SetEnum(UEnum* InEnum)
{
	Enum = InEnum;
}

class UEnum* UMovieSceneByteTrack::GetEnum() const
{
	return Enum;
}
