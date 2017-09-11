// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFriendItem : public SUserWidget
{
public:
	SLATE_USER_ARGS(SFriendItem)
	{ }
	SLATE_ARGUMENT( const FFriendsListStyle*, FriendStyle )
	SLATE_ARGUMENT(const FFriendsFontStyle*, FontStyle)
	SLATE_ARGUMENT(EPopupMethod, Method)
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	virtual void Construct( const FArguments& InArgs, const TSharedRef<class FFriendViewModel>& ViewModel ) = 0;
};
