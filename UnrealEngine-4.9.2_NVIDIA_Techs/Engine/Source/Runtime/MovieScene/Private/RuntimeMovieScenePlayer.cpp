// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneBindings.h"
#include "MovieSceneInstance.h"
#include "RuntimeMovieScenePlayer.h"


/* URuntimeMovieScenePlayer static functions
 *****************************************************************************/

URuntimeMovieScenePlayer* URuntimeMovieScenePlayer::CreateRuntimeMovieScenePlayer(ULevel* Level, UMovieSceneBindings* MovieSceneBindings)
{	
	URuntimeMovieScenePlayer* NewRuntimeMovieScenePlayer = NewObject<URuntimeMovieScenePlayer>((UObject*)GetTransientPackage(), NAME_None, RF_Transient);
	check(NewRuntimeMovieScenePlayer != nullptr);

	// Associate the player with its world
	NewRuntimeMovieScenePlayer->World = Level->OwningWorld;
	check( NewRuntimeMovieScenePlayer->World.IsValid() );

	// Attach bindings to the newly-created RuntimeMovieScenePlayer
	NewRuntimeMovieScenePlayer->SetMovieSceneBindings( MovieSceneBindings );

	// Spawn actors for each spawnable!

	// Add this to the level's list of active RuntimeMovieScenePlayers.  The level will own this player from now on.
	check( Level != nullptr );
	Level->AddActiveRuntimeMovieScenePlayer( NewRuntimeMovieScenePlayer );

	return NewRuntimeMovieScenePlayer;
}


/* URuntimeMovieScenePlayer structors
 *****************************************************************************/

URuntimeMovieScenePlayer::URuntimeMovieScenePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsPlaying(false)
	, MovieSceneBindings(nullptr)
	, TimeCursorPosition(0.0f)
{ }


/* URuntimeMovieScenePlayer interface
 *****************************************************************************/

bool URuntimeMovieScenePlayer::IsPlaying() const
{
	return bIsPlaying;
}


void URuntimeMovieScenePlayer::Pause()
{
	bIsPlaying = false;
}


void URuntimeMovieScenePlayer::Play()
{
	// Init the root movie scene instance
	if (MovieSceneBindings != nullptr)
	{
		UMovieScene* MovieScene = MovieSceneBindings->GetRootMovieScene();
	
		// @todo Sequencer playback: Should we recreate the instance every time?
		RootMovieSceneInstance = MakeShareable(new FMovieSceneInstance(*MovieScene));

		// @odo Sequencer Should we spawn actors here?
		SpawnActorsForMovie(RootMovieSceneInstance.ToSharedRef());
		RootMovieSceneInstance->RefreshInstance(*this);
	}
	
	bIsPlaying = true;
}


/* IMovieScenePlayer interface
 *****************************************************************************/

void URuntimeMovieScenePlayer::SpawnActorsForMovie(TSharedRef<FMovieSceneInstance> MovieSceneInstance)
{
	UWorld* WorldPtr = World.Get();
	if( WorldPtr != nullptr && MovieSceneBindings != nullptr )
	{
		UMovieScene* MovieScene = MovieSceneInstance->GetMovieScene();
		if( MovieScene != nullptr )
		{
			TArray<FSpawnedActorInfo>* FoundSpawnedActors = InstanceToSpawnedActorMap.Find( MovieSceneInstance );
			if( FoundSpawnedActors )
			{
				// Remove existing spawned actors for this movie
				DestroyActorsForMovie( MovieSceneInstance );
			}

			TArray<FSpawnedActorInfo>& SpawnedActorList = InstanceToSpawnedActorMap.Add( MovieSceneInstance, TArray<FSpawnedActorInfo>() );

			for( auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex )
			{
				auto& Spawnable = MovieScene->GetSpawnable( SpawnableIndex );

				UClass* GeneratedClass = Spawnable.GetClass();
				if ( GeneratedClass != nullptr )
				{
					const bool bIsActorBlueprint = GeneratedClass->IsChildOf( AActor::StaticClass() );
					if ( bIsActorBlueprint )
					{
						AActor* ActorCDO = CastChecked< AActor >( GeneratedClass->ClassDefaultObject );

						const FVector SpawnLocation = ActorCDO->GetRootComponent()->RelativeLocation;
						const FRotator SpawnRotation = ActorCDO->GetRootComponent()->RelativeRotation;

						FActorSpawnParameters SpawnInfo;
						SpawnInfo.ObjectFlags = RF_NoFlags;
						AActor* NewActor = WorldPtr->SpawnActor( GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo );
						if( NewActor )
						{	
							// Actor was spawned OK!
							FSpawnedActorInfo NewInfo;
							NewInfo.RuntimeGuid = Spawnable.GetGuid();
							NewInfo.SpawnedActor = NewActor;
						
							SpawnedActorList.Add( NewInfo );
						}
					}
				}
			}
		}
	}
}


void URuntimeMovieScenePlayer::DestroyActorsForMovie(TSharedRef<FMovieSceneInstance> MovieSceneInstance)
{
	UWorld* WorldPtr = World.Get();
	if( WorldPtr != nullptr && MovieSceneBindings != nullptr )
	{
		TArray<FSpawnedActorInfo>* SpawnedActors = InstanceToSpawnedActorMap.Find( MovieSceneInstance );
		if( SpawnedActors )
		{
			TArray<FSpawnedActorInfo>& SpawnedActorsRef = *SpawnedActors;
			for( int32 ActorIndex = 0; ActorIndex < SpawnedActors->Num(); ++ActorIndex )
			{
				AActor* Actor = SpawnedActorsRef[ActorIndex].SpawnedActor.Get();
				if( Actor )
				{
					// @todo Sequencer figure this out.  Defaults to false.
					const bool bNetForce = false;

					// At runtime, level modification is not needed
					const bool bShouldModifyLevel = false;
					Actor->GetWorld()->DestroyActor( Actor, bNetForce, bShouldModifyLevel );
				}
			}
			InstanceToSpawnedActorMap.Remove( MovieSceneInstance );
		}
	}
}


void URuntimeMovieScenePlayer::SetMovieSceneBindings( UMovieSceneBindings* NewMovieSceneBindings )
{
	MovieSceneBindings = NewMovieSceneBindings;
}

	
void URuntimeMovieScenePlayer::GetRuntimeObjects(TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray<UObject*>& OutObjects) const
{
	// @todo sequencer runtime: Add support for individually spawning actors on demand when first requested?
	//    This may be important to reduce the up-front hitch when spawning actors for the entire movie, but
	//    may introduce smaller hitches during playback.  Needs experimentation.

	// First, check to see if we have a spawned actor for this handle
	const TArray<FSpawnedActorInfo>* SpawnedActors = InstanceToSpawnedActorMap.Find( MovieSceneInstance );
	if( SpawnedActors && SpawnedActors->Num() > 0 )
	{
		const TArray<FSpawnedActorInfo>& SpawnedActorInfoRef = *SpawnedActors;

		for( int32 SpawnedActorIndex = 0; SpawnedActorIndex < SpawnedActorInfoRef.Num(); ++SpawnedActorIndex )
		{
			const FSpawnedActorInfo ActorInfo = SpawnedActorInfoRef[SpawnedActorIndex];

			if( ActorInfo.RuntimeGuid == ObjectHandle && ActorInfo.SpawnedActor.IsValid() )
			{
				OutObjects.Add( ActorInfo.SpawnedActor.Get() );
			}
		}
	
	}
	// Otherwise, check to see if we have one or more possessed actors that are mapped to this handle
	else if( MovieSceneBindings != nullptr )
	{
		for (FMovieSceneBoundObjectInfo& BoundObject : MovieSceneBindings->FindBoundObjects(ObjectHandle))
		{
			if (BoundObject.Tag.IsEmpty() == false)
			{
				AActor* Actor = Cast<AActor>(BoundObject.Object);
				if (Actor != nullptr)
				{
					for (UActorComponent* ActorComponent : Actor->GetComponents())
					{
						if (ActorComponent->GetName() == BoundObject.Tag)
						{
							OutObjects.Add(ActorComponent);
						}
					}
				}
			}
			else
			{
				OutObjects.Add(BoundObject.Object);
			}
		}
	}
}


void URuntimeMovieScenePlayer::UpdateCameraCut(UObject* ObjectToViewThrough, bool bNewCameraCut) const
{
}


EMovieScenePlayerStatus::Type URuntimeMovieScenePlayer::GetPlaybackStatus() const
{
	return bIsPlaying ? EMovieScenePlayerStatus::Playing : EMovieScenePlayerStatus::Stopped;
}


void URuntimeMovieScenePlayer::AddOrUpdateMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd)
{
	SpawnActorsForMovie( InstanceToAdd );
}


void URuntimeMovieScenePlayer::RemoveMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove)
{
	const bool bDestroyAll = true;
	DestroyActorsForMovie( InstanceToRemove );
}


TSharedRef<FMovieSceneInstance> URuntimeMovieScenePlayer::GetRootMovieSceneInstance() const
{
	return RootMovieSceneInstance.ToSharedRef();
}


/* IRuntimeMovieScenePlayerInterface interface
 *****************************************************************************/

void URuntimeMovieScenePlayer::Tick(const float DeltaSeconds)
{
	float LastTimePosition = TimeCursorPosition;

	if (bIsPlaying)
	{
		TimeCursorPosition += DeltaSeconds;
	}

	if(RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition, LastTimePosition, *this);
	}
}
