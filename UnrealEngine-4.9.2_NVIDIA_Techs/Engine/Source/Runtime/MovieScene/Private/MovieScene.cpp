// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieScene.h"


/* UMovieScene interface
 *****************************************************************************/

#if WITH_EDITOR

// @todo sequencer: Some of these methods should only be used by tools, and should probably move out of MovieScene!
FGuid UMovieScene::AddSpawnable( const FString& Name, UBlueprint* Blueprint, UObject* CounterpartGamePreviewObject )
{
	check( (Blueprint != nullptr) && (Blueprint->GeneratedClass) );

	Modify();

	FMovieSceneSpawnable NewSpawnable( Name, Blueprint->GeneratedClass, CounterpartGamePreviewObject );
	Spawnables.Add( NewSpawnable );

	// Add a new binding so that tracks can be added to it
	new (ObjectBindings) FMovieSceneBinding( NewSpawnable.GetGuid(), NewSpawnable.GetName() );

	return NewSpawnable.GetGuid();
}


bool UMovieScene::RemoveSpawnable( const FGuid& Guid )
{
	bool bAnythingRemoved = false;
	if( ensure( Guid.IsValid() ) )
	{
		for( auto SpawnableIter( Spawnables.CreateIterator() ); SpawnableIter; ++SpawnableIter )
		{
			auto& CurSpawnable = *SpawnableIter;
			if( CurSpawnable.GetGuid() == Guid )
			{
				Modify();

				{
					UClass* GeneratedClass = CurSpawnable.GetClass();
					UBlueprint* Blueprint = GeneratedClass ? Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy) : nullptr;
					check(nullptr != Blueprint);
					// @todo sequencer: Also remove created Blueprint inner object.  Is this sufficient?  Needs to work with Undo too!
					Blueprint->ClearFlags( RF_Standalone );	// @todo sequencer: Probably not needed for Blueprint
					Blueprint->MarkPendingKill();
				}

				RemoveBinding( Guid );

				// Found it!
				Spawnables.RemoveAt( SpawnableIter.GetIndex() );


				bAnythingRemoved = true;
				break;
			}
		}
	}

	return bAnythingRemoved;
}
#endif //WITH_EDITOR


int32 UMovieScene::GetSpawnableCount() const
{
	return Spawnables.Num();
}


FMovieSceneSpawnable& UMovieScene::GetSpawnable( const int32 Index )
{
	return Spawnables[ Index ];
}


FMovieSceneSpawnable* UMovieScene::FindSpawnable( const FGuid& Guid )
{
	for( auto CurSpawnableIt( Spawnables.CreateIterator() ); CurSpawnableIt; ++CurSpawnableIt )
	{
		auto& CurSpawnable = *CurSpawnableIt;
		if( CurSpawnable.GetGuid() == Guid )
		{
			return &CurSpawnable;
		}
	}

	return nullptr;
}


const FMovieSceneSpawnable* UMovieScene::FindSpawnableForCounterpart( UObject* GamePreviewObject ) const
{
	check( GamePreviewObject != nullptr );

	// Must only be called for objects in game preview worlds
	const bool bIsGamePreviewObject = !!( GamePreviewObject->GetOutermost()->PackageFlags & PKG_PlayInEditor );
	check( bIsGamePreviewObject );

	for( auto CurSpawnableIt( Spawnables.CreateConstIterator() ); CurSpawnableIt; ++CurSpawnableIt )
	{
		auto& CurSpawnable = *CurSpawnableIt;
		if( CurSpawnable.GetCounterpartGamePreviewObject() == GamePreviewObject )
		{
			return &CurSpawnable;
		}
	}

	return nullptr;
}


FGuid UMovieScene::AddPossessable( const FString& Name, UClass* Class )
{
	Modify();

	FMovieScenePossessable NewPossessable( Name, Class );
	Possessables.Add( NewPossessable );

	// Add a new binding so that tracks can be added to it
	new (ObjectBindings) FMovieSceneBinding( NewPossessable.GetGuid(), NewPossessable.GetName() );

	return NewPossessable.GetGuid();
}


bool UMovieScene::RemovePossessable( const FGuid& PossessableGuid )
{
	bool bAnythingRemoved = false;

	for( auto PossesableIter( Possessables.CreateIterator() ); PossesableIter; ++PossesableIter )
	{
		auto& CurPossesable = *PossesableIter;

		if( CurPossesable.GetGuid() == PossessableGuid )
		{	
			Modify();

			// Found it!
			Possessables.RemoveAt( PossesableIter.GetIndex() );

			RemoveBinding( PossessableGuid );

			bAnythingRemoved = true;
			break;
		}
	}

	return bAnythingRemoved;
}


FMovieScenePossessable* UMovieScene::FindPossessable( const FGuid& Guid )
{
	for( auto CurPossessableIt( Possessables.CreateIterator() ); CurPossessableIt; ++CurPossessableIt )
	{
		auto& CurPossessable = *CurPossessableIt;
		if( CurPossessable.GetGuid() == Guid )
		{
			return &CurPossessable;
		}
	}

	return nullptr;
}


int32 UMovieScene::GetPossessableCount() const
{
	return Possessables.Num();
}


FMovieScenePossessable& UMovieScene::GetPossessable( const int32 Index )
{
	return Possessables[Index];
}


TRange<float> UMovieScene::GetTimeRange() const
{
	// Get the range of all sections combined
	TArray< TRange<float> > Bounds;

	for (int32 TypeIndex = 0; TypeIndex < MasterTracks.Num(); ++TypeIndex)
	{
		Bounds.Add(MasterTracks[TypeIndex]->GetSectionBoundaries());
	}

	for (int32 BindingIndex = 0; BindingIndex < ObjectBindings.Num(); ++BindingIndex)
	{
		Bounds.Add(ObjectBindings[BindingIndex].GetTimeRange());
	}

	return TRange<float>::Hull(Bounds);
}


TArray<UMovieSceneSection*> UMovieScene::GetAllSections() const
{
	TArray<UMovieSceneSection*> OutSections;

	// Add all master type sections 
	for( int32 TrackIndex = 0; TrackIndex < MasterTracks.Num(); ++TrackIndex )
	{
		OutSections.Append( MasterTracks[TrackIndex]->GetAllSections() );
	}

	// Add all object binding sections
	for( int32 ObjectBindingIndex = 0; ObjectBindingIndex < ObjectBindings.Num(); ++ObjectBindingIndex )
	{
		const FMovieSceneBinding& Binding = ObjectBindings[ObjectBindingIndex];
		const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();

		for( int32 TrackIndex = 0; TrackIndex < Tracks.Num(); ++TrackIndex )
		{
			OutSections.Append( Tracks[TrackIndex]->GetAllSections() );
		}
	}

	return OutSections;
}


UMovieSceneTrack* UMovieScene::FindTrack( TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid, FName UniqueTrackName ) const
{
	UMovieSceneTrack* FoundTrack = nullptr;
	
	check( UniqueTrackName != NAME_None );
	check( ObjectGuid.IsValid() );
	
	for( int32 ObjectBindingIndex = 0; ObjectBindingIndex < ObjectBindings.Num(); ++ObjectBindingIndex )
	{
		const FMovieSceneBinding& Binding = ObjectBindings[ObjectBindingIndex];

		if( Binding.GetObjectGuid() == ObjectGuid ) 
		{
			const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();
			for( int32 TrackIndex = 0; TrackIndex < Tracks.Num(); ++TrackIndex )
			{
				UMovieSceneTrack* Track = Tracks[TrackIndex];
				if( Track->GetClass() == TrackClass && Track->GetTrackName() == UniqueTrackName )
				{
					FoundTrack = Track;
					break;
				}
			}
		}
	}
	
	return FoundTrack;
}


class UMovieSceneTrack* UMovieScene::AddTrack( TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid )
{
	UMovieSceneTrack* CreatedType = nullptr;

	check( ObjectGuid.IsValid() )

	for( int32 ObjectBindingIndex = 0; ObjectBindingIndex < ObjectBindings.Num(); ++ObjectBindingIndex )
	{
		FMovieSceneBinding& Binding = ObjectBindings[ObjectBindingIndex];

		if( Binding.GetObjectGuid() == ObjectGuid ) 
		{
			Modify();

			CreatedType = NewObject<UMovieSceneTrack>(this, TrackClass, NAME_None, RF_Transactional);
			Binding.AddTrack( *CreatedType );
		}
	}

	return CreatedType;
}


bool UMovieScene::RemoveTrack( UMovieSceneTrack* Track )
{
	Modify();

	bool bAnythingRemoved = false;
	for( int32 ObjectBindingIndex = 0; ObjectBindingIndex < ObjectBindings.Num(); ++ObjectBindingIndex )
	{
		FMovieSceneBinding& Binding = ObjectBindings[ObjectBindingIndex];

		if( Binding.RemoveTrack( *Track ) )
		{
			bAnythingRemoved = true;
			// The track was removed from the current binding, stop searching now as it cannot exist in any other binding
			break;
		}
	}

	return bAnythingRemoved;
}


UMovieSceneTrack* UMovieScene::FindMasterTrack( TSubclassOf<UMovieSceneTrack> TrackClass ) const
{
	UMovieSceneTrack* FoundTrack = nullptr;
	for( int32 TrackIndex = 0; TrackIndex < MasterTracks.Num(); ++TrackIndex )
	{
		UMovieSceneTrack* Track = MasterTracks[TrackIndex];
		if( Track->GetClass() == TrackClass )
		{
			FoundTrack = Track;
			break;
		}
	}

	return FoundTrack;
}


class UMovieSceneTrack* UMovieScene::AddMasterTrack( TSubclassOf<UMovieSceneTrack> TrackClass )
{
	Modify();

	UMovieSceneTrack* CreatedType = NewObject<UMovieSceneTrack>(this, TrackClass, NAME_None, RF_Transactional);

	MasterTracks.Add( CreatedType );
	
	return CreatedType;
}


UMovieSceneTrack* UMovieScene::AddShotTrack( TSubclassOf<UMovieSceneTrack> TrackClass )
{
	if( !ShotTrack )
	{
		Modify();

		ShotTrack = NewObject<UMovieSceneTrack>(this, TrackClass, FName("Shots"), RF_Transactional);
	}

	return ShotTrack;
}


UMovieSceneTrack* UMovieScene::GetShotTrack()
{
	return ShotTrack;
}


void UMovieScene::RemoveShotTrack()
{
	if( ShotTrack )
	{
		Modify();

		ShotTrack = nullptr;
	}
}


bool UMovieScene::RemoveMasterTrack( UMovieSceneTrack* Track ) 
{
	Modify();

	return MasterTracks.RemoveSingle(Track) != 0;
}


bool UMovieScene::IsAMasterTrack(const UMovieSceneTrack* Track) const
{
	for (int32 TrackIndex = 0; TrackIndex < MasterTracks.Num(); ++TrackIndex)
	{
		if (Track == MasterTracks[TrackIndex])
		{
			return true;
		}
	}

	return false;
}


/* UMovieScene implementation
 *****************************************************************************/

void UMovieScene::RemoveBinding(const FGuid& Guid)
{
	// update each type
	for (int32 BindingIndex = 0; BindingIndex < ObjectBindings.Num(); ++BindingIndex)
	{
		if (ObjectBindings[BindingIndex].GetObjectGuid() == Guid)
		{
			ObjectBindings.RemoveAt(BindingIndex);
			break;
		}
	}
}
