// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IPropertyHandle;
class UClass;
class UProperty;

/**
 * Parameters for determining if a property can be keyed.
 */
struct SEQUENCER_API FCanKeyPropertyParams
{
	FCanKeyPropertyParams();

	/**
	 * Creates new can key property parameters.
	 * @param InObjectClass the class of the object which has the property to be keyed.
	 * @param InPropertyPath an array of UProperty objects which represents a path of properties to get from
	 *        the root object to the property to be keyed.
	 */
	FCanKeyPropertyParams(UClass* InObjectClass, TArray<UProperty*> InPropertyPath);

	/**
	* Creates new can key property parameters.
	* @param InObjectClass the class of the object which has the property to be keyed.
	* @param InPropertyHandle a handle to the property to be keyed.
	*/
	FCanKeyPropertyParams(UClass* InObjectClass, const IPropertyHandle& InPropertyHandle);

	/** The class of the object which has the property to be keyed. */
	const UClass* ObjectClass;
	/** An array of UProperty objects which represents a path of properties to get from the root object to the
	property to be keyed. */
	TArray<UProperty*> PropertyPath;
};

/**
 * Parameters for keying a property.
 */
struct SEQUENCER_API FKeyPropertyParams
{
	FKeyPropertyParams();

	/**
	* Creates new key property parameters.
	* @param InObjectsToKey an array of the objects who's property will be keyed.
	* @param InPropertyPath an array of UProperty objects which represents a path of properties to get from
	*        the root object to the property to be keyed.
	*/
	FKeyPropertyParams(TArray<UObject*> InObjectsToKey, TArray<UProperty*> InPropertyPath);

	/**
	* Creates new key property parameters.
	* @param InObjectsToKey an array of the objects who's property will be keyed.
	* @param InPropertyHandle a handle to the property to be keyed.
	*/
	FKeyPropertyParams(TArray<UObject*> InObjectsToKey, const IPropertyHandle& InPropertyHandle);

	/** An array of the objects who's property will be keyed. */
	TArray<UObject*> ObjectsToKey;
	/** An array of UProperty objects which represents a path of properties to get from the root object to the
	property to be keyed. */
	TArray<UProperty*> PropertyPath;
};

/**
 * Parameters for the property changed callback.
 */
class SEQUENCER_API FPropertyChangedParams
{
public:
	/**
	 * Gets the value of the property that changed.
	 */
	template<typename ValueType>
	ValueType* GetPropertyValue() const
	{
		void* CurrentObject = ObjectsThatChanged[0];
		for (int32 i = 0; i < PropertyPath.Num(); i++)
		{
			CurrentObject = PropertyPath[i]->ContainerPtrToValuePtr<ValueType>(CurrentObject, 0);
		}
		return (ValueType*)CurrentObject;
	}

	/** Gets the property path as a period seperated string of property names. */
	FString GetPropertyPathString() const;

	/** An array of the objects that changed. */
	TArray<UObject*> ObjectsThatChanged;
	/** An array of UProperty objects which represents a path of properties to get from the root object to the 
	property that changed. */
	TArray<UProperty*> PropertyPath;
	/** Represents the FName of an inner property which should be keyed for a struct property.  If all inner 
	properties should be keyed, this will be FName::None. */
	FName StructPropertyNameToKey;
	/** If autokey is required. */
	bool bRequireAutoKey;
};