// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FriendsMarkupStyle.generated.h"

/**
 * Interface for the services manager.
 */
USTRUCT()
struct FRIENDSANDCHAT_API FFriendsMarkupStyle
	: public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	// Default Constructor
	FFriendsMarkupStyle() { }

	// Default Destructor
	virtual ~FFriendsMarkupStyle() { }

	/**
	 * Override widget style function.
	 */
	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const override { }

	// Holds the widget type name
	static const FName TypeName;

	/**
	 * Get the type name.
	 * @return the type name
	 */
	virtual const FName GetTypeName() const override { return TypeName; };

	/**
	 * Get the default style.
	 * @return the default style
	 */
	static const FFriendsMarkupStyle& GetDefault();

	/** Markup Button style */
	UPROPERTY()
	FButtonStyle MarkupButtonStyle;
	FFriendsMarkupStyle& SetMarkupButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FTextBlockStyle MarkupTextStyle;
	FFriendsMarkupStyle& SetMarkupTextStyle(const FTextBlockStyle& InTextStyle);

};

