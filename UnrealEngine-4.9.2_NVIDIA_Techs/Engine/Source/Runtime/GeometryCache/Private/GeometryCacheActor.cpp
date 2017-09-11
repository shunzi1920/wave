// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GeometryCacheModulePrivatePCH.h"
#include "GeometryCacheActor.h"
#include "UnrealEd.h"
#include "GeometryCacheComponent.h"

AGeometryCacheActor::AGeometryCacheActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GeometryCacheComponent = CreateDefaultSubobject<UGeometryCacheComponent>(TEXT("GeometryCacheComponent"));
	RootComponent = GeometryCacheComponent;
}

class UGeometryCacheComponent* AGeometryCacheActor::GetGeometryCacheComponent() const
{
	return GeometryCacheComponent;
}

bool AGeometryCacheActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	Super::GetReferencedContentObjects(Objects);

	if (GeometryCacheComponent && GeometryCacheComponent->GeometryCache)
	{
		Objects.Add(GeometryCacheComponent->GeometryCache);
	}

	return true;
}
