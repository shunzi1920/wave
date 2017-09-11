// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFriendsListContainer : public SUserWidget
{
public:

	SLATE_USER_ARGS(SFriendsListContainer) { }
	SLATE_ARGUMENT(const FFriendsListStyle*, FriendStyle)
	SLATE_ARGUMENT(const FFriendsFontStyle*, FontStyle)
	SLATE_ARGUMENT(EPopupMethod, Method)
	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs, const TSharedRef<class FFriendListViewModel>& ViewModel) = 0;
};
