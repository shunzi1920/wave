// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerObjectChangeListener.h"
#include "PropertyEditing.h"
#include "IPropertyChangeListener.h"

DEFINE_LOG_CATEGORY_STATIC(LogSequencerTools, Log, All);

FSequencerObjectChangeListener::FSequencerObjectChangeListener( TSharedRef<ISequencer> InSequencer, bool bInListenForActorsOnly )
	: Sequencer( InSequencer )
	, bListenForActorsOnly( bInListenForActorsOnly )
{
	FCoreUObjectDelegates::OnPreObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPreEditChange);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FSequencerObjectChangeListener::OnObjectPostEditChange);
	//GEditor->OnPreActorMoved.AddRaw(this, &FSequencerObjectChangeListener::OnActorPreEditMove);
	GEditor->OnActorMoved().AddRaw( this, &FSequencerObjectChangeListener::OnActorPostEditMove );
}

FSequencerObjectChangeListener::~FSequencerObjectChangeListener()
{
	FCoreUObjectDelegates::OnPreObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	GEditor->OnActorMoved().RemoveAll( this );
}

void FSequencerObjectChangeListener::OnPropertyChanged(const TArray<UObject*>& ChangedObjects, const IPropertyHandle& PropertyHandle, bool bRequireAutoKey) const
{
	BroadcastPropertyChanged(FKeyPropertyParams(ChangedObjects, PropertyHandle), bRequireAutoKey);
}

void FSequencerObjectChangeListener::BroadcastPropertyChanged( FKeyPropertyParams KeyPropertyParams, bool bRequireAutoKey ) const
{
	const UStructProperty* StructProperty = Cast<const UStructProperty>(KeyPropertyParams.PropertyPath.Last());
	const UStructProperty* ParentStructProperty = nullptr;
	if (KeyPropertyParams.PropertyPath.Num() > 1)
	{
		ParentStructProperty = Cast<const UStructProperty>(KeyPropertyParams.PropertyPath[KeyPropertyParams.PropertyPath.Num() - 2]);
	}

	FPropertyChangedParams Params;
	Params.bRequireAutoKey = bRequireAutoKey;
	Params.ObjectsThatChanged = KeyPropertyParams.ObjectsToKey;
	Params.StructPropertyNameToKey = NAME_None;

	bool bFoundAndBroadcastedDelegate = false;
	if (ParentStructProperty)
	{
		Params.PropertyPath.Append(KeyPropertyParams.PropertyPath.GetData(), KeyPropertyParams.PropertyPath.Num() - 1);

		// If the property parent is a struct, see if this property parent can be keyed. (e.g R,G,B,A for a color)
		FOnAnimatablePropertyChanged Delegate = ClassToPropertyChangedMap.FindRef(ParentStructProperty->Struct->GetFName());
		if (Delegate.IsBound())
		{
			if (!bRequireAutoKey)
			{
				Params.StructPropertyNameToKey = KeyPropertyParams.PropertyPath.Last()->GetFName();
			}
			bFoundAndBroadcastedDelegate = true;
			Delegate.Broadcast(Params);
		}
	}

	if (!bFoundAndBroadcastedDelegate)
	{
		Params.PropertyPath = KeyPropertyParams.PropertyPath;
		if (StructProperty)
		{
			ClassToPropertyChangedMap.FindRef(StructProperty->Struct->GetFName()).Broadcast(Params);
		}
		else
		{
			// the property in question is not a struct or an inner of the struct. See if it is directly keyable
			ClassToPropertyChangedMap.FindRef(KeyPropertyParams.PropertyPath.Last()->GetClass()->GetFName()).Broadcast(Params);
		}
	}
}

bool FSequencerObjectChangeListener::IsObjectValidForListening( UObject* Object ) const
{
	// @todo Sequencer - Pre/PostEditChange is sometimes called for inner objects of other objects (like actors with components)
	// We only want the outer object so assume it's an actor for now
	return bListenForActorsOnly ? Object->IsA<AActor>() || Object->IsA<UActorComponent>() : true;
}

FOnAnimatablePropertyChanged& FSequencerObjectChangeListener::GetOnAnimatablePropertyChanged( FName PropertyTypeName )
{
	return ClassToPropertyChangedMap.FindOrAdd( PropertyTypeName );
}

FOnPropagateObjectChanges& FSequencerObjectChangeListener::GetOnPropagateObjectChanges()
{
	return OnPropagateObjectChanges;
}

bool FSequencerObjectChangeListener::FindPropertySetter( const UClass& ObjectClass, const FName PropertyTypeName, const FString& PropertyVarName ) const
{
	bool bFound = ClassToPropertyChangedMap.Contains( PropertyTypeName );

	if( bFound )
	{
		static const FString Set(TEXT("Set"));

		const FString FunctionString = Set + PropertyVarName;

		FName FunctionName = FName(*FunctionString);

		bFound = ObjectClass.FindFunctionByName(FunctionName) != nullptr;
	}

	return bFound;
}

bool FSequencerObjectChangeListener::CanKeyProperty(FCanKeyPropertyParams CanKeyPropertyParams) const
{
	const UStructProperty* StructProperty = Cast<const UStructProperty>(CanKeyPropertyParams.PropertyPath.Last());
	const UStructProperty* ParentStructProperty = nullptr;
	if (CanKeyPropertyParams.PropertyPath.Num() > 1)
	{
		ParentStructProperty = Cast<const UStructProperty>(CanKeyPropertyParams.PropertyPath[CanKeyPropertyParams.PropertyPath.Num() - 2]);
	}

	FString PropertyVarName;

	bool bFound = false;
	if ( StructProperty )
	{
		bFound = FindPropertySetter(*CanKeyPropertyParams.ObjectClass, StructProperty->Struct->GetFName(), StructProperty->GetName() );
	}
	
	if( !bFound && ParentStructProperty )
	{
		// If the property parent is a struct, see if this property parent can be keyed.
		bFound = FindPropertySetter(*CanKeyPropertyParams.ObjectClass, ParentStructProperty->Struct->GetFName(), ParentStructProperty->GetName());
	}

	if( !bFound )
	{
		// the property in question is not a struct or an inner of the struct. See if it is directly keyable
		bFound = FindPropertySetter(*CanKeyPropertyParams.ObjectClass, CanKeyPropertyParams.PropertyPath.Last()->GetClass()->GetFName(), CanKeyPropertyParams.PropertyPath.Last()->GetName());
	}

	return bFound;
}

void FSequencerObjectChangeListener::KeyProperty(FKeyPropertyParams KeyPropertyParams) const
{
	const bool bRequireAutoKey = false;
	BroadcastPropertyChanged(KeyPropertyParams, bRequireAutoKey);
}

void FSequencerObjectChangeListener::OnObjectPreEditChange( UObject* Object, const FEditPropertyChain& PropertyChain )
{
	// We only care if we are in auto-key mode and not attempting to change properties of a CDO (which cannot be animated)
	if( Sequencer.IsValid() && Sequencer.Pin()->GetAutoKeyEnabled() && !Object->HasAnyFlags(RF_ClassDefaultObject) && PropertyChain.GetActiveMemberNode() )
	{
		// Register with the property editor module that we'd like to know about animated float properties that change
		FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

		// Sometimes due to edit inline new the object is not actually the object that contains the property
		if ( IsObjectValidForListening(Object) && Object->GetClass()->HasProperty(PropertyChain.GetActiveMemberNode()->GetValue()) )
		{
			TSharedPtr<IPropertyChangeListener> PropertyChangeListener = ActivePropertyChangeListeners.FindRef( Object );

			if( !PropertyChangeListener.IsValid() )
			{
				PropertyChangeListener = PropertyEditor.CreatePropertyChangeListener();
				
				ActivePropertyChangeListeners.Add( Object, PropertyChangeListener );

				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged, true );

				FPropertyListenerSettings Settings;
				// Ignore array and object properties
				Settings.bIgnoreArrayProperties = true;
				Settings.bIgnoreObjectProperties = true;
				// Property flags which must be on the property
				Settings.RequiredPropertyFlags = 0;
				// Property flags which cannot be on the property
				Settings.DisallowedPropertyFlags = CPF_EditConst;

				PropertyChangeListener->SetObject( *Object, Settings );
			}
		}
	}
}

void FSequencerObjectChangeListener::OnObjectPostEditChange( UObject* Object, FPropertyChangedEvent& PropertyChangedEvent )
{
	if( Object && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		bool bIsObjectValid = IsObjectValidForListening( Object );

		bool bShouldPropagateChanges = bIsObjectValid;

		// We only care if we are in auto-key mode and not attempting to change properties of a CDO (which cannot be animated)
		if( Sequencer.IsValid() && Sequencer.Pin()->GetAutoKeyEnabled() && bIsObjectValid && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			TSharedPtr< IPropertyChangeListener > Listener;
			ActivePropertyChangeListeners.RemoveAndCopyValue( Object, Listener );

			if( Listener.IsValid() )
			{
				check( Listener.IsUnique() );
					
				// Don't recache new values, the listener will be destroyed after this call
				const bool bRecacheNewValues = false;

				const bool bFoundChanges = Listener->ScanForChanges( bRecacheNewValues );

				// If the listener did not find any changes we care about, propagate changes to puppets
				// @todo Sequencer - We might need to check per changed property
				bShouldPropagateChanges = !bFoundChanges;
			}
		}

		if( bShouldPropagateChanges )
		{
			OnPropagateObjectChanges.Broadcast( Object );
		}
	}
}


void FSequencerObjectChangeListener::OnActorPostEditMove( AActor* Actor )
{
	// @todo sequencer actors: Currently this only fires on a "final" move.  For our purposes we probably
	// want to get an update every single movement, even while dragging an object.
	FPropertyChangedEvent PropertyChangedEvent(nullptr);
	OnObjectPostEditChange( Actor, PropertyChangedEvent );
}

void FSequencerObjectChangeListener::TriggerAllPropertiesChanged(UObject* Object)
{
	if( Object )
	{
		// @todo Sequencer - Pre/PostEditChange is sometimes called for inner objects of other objects (like actors with components)
		// We only want the outer object so assume it's an actor for now
		bool bObjectIsActor = Object->IsA( AActor::StaticClass() );

		// Default to propagating changes to objects only if they are actors
		// if this change is handled by auto-key we will not propagate changes
		bool bShouldPropagateChanges = bObjectIsActor;

		// We only care if we are not attempting to change properties of a CDO (which cannot be animated)
		if( Sequencer.IsValid() && bObjectIsActor && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		{
			TSharedPtr<IPropertyChangeListener> PropertyChangeListener = ActivePropertyChangeListeners.FindRef( Object );
			
			if( !PropertyChangeListener.IsValid() )
			{
				// Register with the property editor module that we'd like to know about animated float properties that change
				FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

				PropertyChangeListener = PropertyEditor.CreatePropertyChangeListener();
				
				PropertyChangeListener->GetOnPropertyChangedDelegate().AddRaw( this, &FSequencerObjectChangeListener::OnPropertyChanged, false );

				FPropertyListenerSettings Settings;
				// Ignore array and object properties
				Settings.bIgnoreArrayProperties = true;
				Settings.bIgnoreObjectProperties = true;
				// Property flags which must be on the property
				Settings.RequiredPropertyFlags = 0;
				// Property flags which cannot be on the property
				Settings.DisallowedPropertyFlags = CPF_EditConst;

				PropertyChangeListener->SetObject( *Object, Settings );
			}

			PropertyChangeListener->TriggerAllPropertiesChangedDelegate();
		}
	}
}
