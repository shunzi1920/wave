// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SClanHome : public SUserWidget
{
public:

	SLATE_USER_ARGS(SClanHome) { }
	SLATE_ARGUMENT(const FFriendsListStyle*, FriendStyle)
	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs, const TSharedRef<class FClanCollectionViewModel>& ViewModel) = 0;
};
