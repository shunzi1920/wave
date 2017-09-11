// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFriendActions : public SUserWidget
{
public:
	SLATE_USER_ARGS(SFriendActions)
	{ }
	SLATE_ARGUMENT( const FFriendsAndChatStyle*, FriendStyle )
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	virtual void Construct( const FArguments& InArgs, const TSharedRef<class FFriendViewModel>& ViewModel ) = 0;
};
