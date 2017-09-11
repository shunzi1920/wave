// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MarginTrackEditor.h"
#include "Editor/MovieSceneTools/Public/PropertySection.h"
#include "Editor/MovieSceneTools/Public/MovieSceneToolHelpers.h"
#include "Runtime/UMG/Public/Animation/MovieSceneMarginSection.h"
#include "Runtime/UMG/Public/Animation/MovieSceneMarginTrack.h"
#include "Editor/Sequencer/Public/ISectionLayoutBuilder.h"
#include "Editor/Sequencer/Public/ISequencerObjectChangeListener.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"

class FMarginPropertySection : public FPropertySection
{
public:
	FMarginPropertySection( UMovieSceneSection& InSectionObject, FName SectionName )
		: FPropertySection(InSectionObject, SectionName) {}

	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override
	{
		UMovieSceneMarginSection* MarginSection = Cast<UMovieSceneMarginSection>(&SectionObject);

		LayoutBuilder.AddKeyArea("Left", NSLOCTEXT("FMarginPropertySection", "MarginLeft", "Left"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetLeftCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Top", NSLOCTEXT("FMarginPropertySection", "MarginTop", "Top"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetTopCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Right", NSLOCTEXT("FMarginPropertySection", "MarginRight", "Right"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetRightCurve(), MarginSection)));
		LayoutBuilder.AddKeyArea("Bottom", NSLOCTEXT("FMarginPropertySection", "MarginBottom", "Bottom"), MakeShareable(new FFloatCurveKeyArea(&MarginSection->GetBottomCurve(), MarginSection)));
	}
};

FMarginTrackEditor::FMarginTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
	// Get the object change listener for the sequencer and register a delegates for when properties change that we care about
	ISequencerObjectChangeListener& ObjectChangeListener = InSequencer->GetObjectChangeListener();
	ObjectChangeListener.GetOnAnimatablePropertyChanged( "Margin" ).AddRaw( this, &FMarginTrackEditor::OnMarginChanged );
}

FMarginTrackEditor::~FMarginTrackEditor()
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	if( Sequencer.IsValid() )
	{
		ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
		ObjectChangeListener.GetOnAnimatablePropertyChanged( "Margin" ).RemoveAll( this );
	}
}



TSharedRef<FMovieSceneTrackEditor> FMarginTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FMarginTrackEditor( InSequencer ) );
}

bool FMarginTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneMarginTrack::StaticClass();
}

TSharedRef<ISequencerSection> FMarginTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	UClass* SectionClass = SectionObject.GetOuter()->GetClass();

	TSharedRef<ISequencerSection> NewSection = MakeShareable( new FMarginPropertySection( SectionObject, Track->GetTrackName() ) );

	return NewSection;
}


void FMarginTrackEditor::OnMarginChanged(  const FPropertyChangedParams& PropertyChangedParams )
{
	AnimatablePropertyChanged
	(
		UMovieSceneMarginTrack::StaticClass(), 
		PropertyChangedParams.bRequireAutoKey,
		FOnKeyProperty::CreateRaw(this, &FMarginTrackEditor::OnKeyMargin, &PropertyChangedParams ) 
	);
}


void FMarginTrackEditor::OnKeyMargin( float KeyTime, const FPropertyChangedParams* PropertyChangedParams )
{
	FMargin MarginValue = *PropertyChangedParams->GetPropertyValue<FMargin>();
	FName PropertyName = PropertyChangedParams->PropertyPath.Last()->GetFName();

	for( int32 ObjectIndex = 0; ObjectIndex < PropertyChangedParams->ObjectsThatChanged.Num(); ++ObjectIndex )
	{
		UObject* Object = PropertyChangedParams->ObjectsThatChanged[ObjectIndex];

		FMarginKey Key;
		Key.bAddKeyEvenIfUnchanged = !PropertyChangedParams->bRequireAutoKey;
		Key.CurveName = PropertyChangedParams->StructPropertyNameToKey;
		Key.Value = MarginValue;

		FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
		if (ObjectHandle.IsValid())
		{
			UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, UMovieSceneMarginTrack::StaticClass(), PropertyName );
			if( ensure( Track ) )
			{
				UMovieSceneMarginTrack* MarginTrack = CastChecked<UMovieSceneMarginTrack>(Track);
				MarginTrack->SetPropertyNameAndPath( PropertyName, PropertyChangedParams->GetPropertyPathString() );
				// Find or add a new section at the auto-key time and changing the property same property
				// AddKeyToSection is not actually a virtual, it's redefined in each class with a different type
				bool bSuccessfulAdd = MarginTrack->AddKeyToSection( KeyTime, Key );
				if (bSuccessfulAdd)
				{
					MarginTrack->SetAsShowable();
				}
			}
		}
	}
}