// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"

UMovieScene3DTransformSection::UMovieScene3DTransformSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{

}

void UMovieScene3DTransformSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaTime, KeyHandles );

	// Move all the curves in this section
	for( int32 Axis = 0; Axis < 3; ++Axis )
	{
		Translation[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Rotation[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Scale[Axis].ShiftCurve( DeltaTime, KeyHandles );
	}
}

void UMovieScene3DTransformSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	for( int32 Axis = 0; Axis < 3; ++Axis )
	{
		Translation[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Rotation[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Scale[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
	}
}

void UMovieScene3DTransformSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (auto It(Translation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Translation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
		for (auto It(Rotation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Rotation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
		for (auto It(Scale[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Scale[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
	}
}	

void UMovieScene3DTransformSection::EvalTranslation( float Time, FVector& OutTranslation, TArray<bool>& OutHasTranslationKeys ) const
{
	OutHasTranslationKeys.Add(Translation[0].GetNumKeys() > 0);
	OutHasTranslationKeys.Add(Translation[1].GetNumKeys() > 0);
	OutHasTranslationKeys.Add(Translation[2].GetNumKeys() > 0);

	// Evaluate each component of translation if there are any keys

	if (OutHasTranslationKeys[0])
	{
		OutTranslation.X = Translation[0].Eval( Time );
	}
	if (OutHasTranslationKeys[1])
	{
		OutTranslation.Y = Translation[1].Eval( Time );
	}
	if (OutHasTranslationKeys[2])
	{
		OutTranslation.Z = Translation[2].Eval( Time );
	}
}

void UMovieScene3DTransformSection::EvalRotation( float Time, FRotator& OutRotation, TArray<bool>& OutHasRotationKeys ) const
{
	OutHasRotationKeys.Add(Rotation[0].GetNumKeys() > 0);
	OutHasRotationKeys.Add(Rotation[1].GetNumKeys() > 0);
	OutHasRotationKeys.Add(Rotation[2].GetNumKeys() > 0);

	// Evaluate each component of rotation if there are any keys

	if (OutHasRotationKeys[0])
	{
		OutRotation.Roll = Rotation[0].Eval( Time );
	}
	if (OutHasRotationKeys[1])
	{
		OutRotation.Pitch = Rotation[1].Eval( Time );
	}
	if (OutHasRotationKeys[2])
	{
		OutRotation.Yaw = Rotation[2].Eval( Time );
	}
}

void UMovieScene3DTransformSection::EvalScale( float Time, FVector& OutScale, TArray<bool>& OutHasScaleKeys ) const
{
	OutHasScaleKeys.Add(Scale[0].GetNumKeys() > 0);
	OutHasScaleKeys.Add(Scale[1].GetNumKeys() > 0);
	OutHasScaleKeys.Add(Scale[2].GetNumKeys() > 0);

	// Evaluate each component of scale if there are any keys

	if (OutHasScaleKeys[0])
	{
		OutScale.X = Scale[0].Eval( Time );
	}
	if (OutHasScaleKeys[1])
	{
		OutScale.Y = Scale[1].Eval( Time );
	}
	if (OutHasScaleKeys[2])
	{
		OutScale.Z = Scale[2].Eval( Time );
	}
}

/**
 * Chooses an appropriate curve from an axis and a set of curves
 */
static FRichCurve* ChooseCurve( EAxis::Type Axis, FRichCurve* Curves )
{
	switch( Axis )
	{
	case EAxis::X:
		return &Curves[0];
		break;
	case EAxis::Y:
		return &Curves[1];
		break;
	case EAxis::Z:
		return &Curves[2];
		break;
	default:
		check( false );
		return NULL;
		break;
	}
}

FRichCurve& UMovieScene3DTransformSection::GetTranslationCurve( EAxis::Type Axis ) 
{
	return *ChooseCurve( Axis, Translation );
}

FRichCurve& UMovieScene3DTransformSection::GetRotationCurve( EAxis::Type Axis )
{
	return *ChooseCurve( Axis, Rotation );
}

FRichCurve& UMovieScene3DTransformSection::GetScaleCurve( EAxis::Type Axis )
{
	return *ChooseCurve( Axis, Scale );
}



void UMovieScene3DTransformSection::AddTranslationKeys( const FTransformKey& TransformKey )
{
	const float Time = TransformKey.GetKeyTime();

	const bool bUnwindRotation = false; // Not needed for translation

	// Key each component.  Note: we always key each component if no key exists
	if( Translation[0].GetNumKeys() == 0 || TransformKey.ShouldKeyTranslation( EAxis::X ) )
	{
		AddKeyToCurve( Translation[0], Time, TransformKey.GetTranslationValue().X );
	}

	if( Translation[1].GetNumKeys() == 0 || TransformKey.ShouldKeyTranslation( EAxis::Y ) )
	{ 
		AddKeyToCurve( Translation[1], Time, TransformKey.GetTranslationValue().Y );
	}

	if( Translation[2].GetNumKeys() == 0 || TransformKey.ShouldKeyTranslation( EAxis::Z ) )
	{ 
		AddKeyToCurve( Translation[2], Time, TransformKey.GetTranslationValue().Z );
	}
}

void UMovieScene3DTransformSection::AddRotationKeys( const FTransformKey& TransformKey, const bool bUnwindRotation )
{
	const float Time = TransformKey.GetKeyTime();

	// Key each component.  Note: we always key each component if no key exists
	if( Rotation[0].GetNumKeys() == 0 || TransformKey.ShouldKeyRotation( EAxis::X ) )
	{
		AddKeyToCurve( Rotation[0], Time, TransformKey.GetRotationValue().Roll );
	}

	if( Rotation[1].GetNumKeys() == 0 || TransformKey.ShouldKeyRotation( EAxis::Y ) )
	{ 
		AddKeyToCurve( Rotation[1], Time, TransformKey.GetRotationValue().Pitch );
	}

	if( Rotation[2].GetNumKeys() == 0 || TransformKey.ShouldKeyRotation( EAxis::Z ) )
	{ 
		AddKeyToCurve( Rotation[2], Time, TransformKey.GetRotationValue().Yaw );
	}
}

void UMovieScene3DTransformSection::AddScaleKeys( const FTransformKey& TransformKey )
{
	const float Time = TransformKey.GetKeyTime();

	const bool bUnwindRotation = false; // Not needed for scale

	// Key each component.  Note: we always key each component if no key exists
	if( Scale[0].GetNumKeys() == 0 || TransformKey.ShouldKeyScale( EAxis::X ) )
	{
		AddKeyToCurve( Scale[0], Time, TransformKey.GetScaleValue().X );
	}

	if( Scale[1].GetNumKeys() == 0 || TransformKey.ShouldKeyScale( EAxis::Y ) )
	{ 
		AddKeyToCurve( Scale[1], Time, TransformKey.GetScaleValue().Y );
	}

	if( Scale[2].GetNumKeys() == 0 || TransformKey.ShouldKeyScale( EAxis::Z ) )
	{ 
		AddKeyToCurve( Scale[2], Time, TransformKey.GetScaleValue().Z );
	}
}

bool UMovieScene3DTransformSection::NewKeyIsNewData(const FTransformKey& TransformKey) const
{
	bool bHasEmptyKeys = false;
	for (int32 i = 0; i < 3; ++i)
	{
		bHasEmptyKeys = bHasEmptyKeys ||
			Translation[i].GetNumKeys() == 0 ||
			Rotation[i].GetNumKeys() == 0 ||
			Scale[i].GetNumKeys() == 0;
	}
	return bHasEmptyKeys || TransformKey.ShouldKeyAny();
}
