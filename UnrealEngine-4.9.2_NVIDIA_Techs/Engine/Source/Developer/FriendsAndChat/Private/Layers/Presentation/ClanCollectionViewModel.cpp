// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ClanCollectionViewModel.h"
#include "ClanInfoViewModel.h"
#include "ClanRepository.h"
#include "ClanInfo.h"
#include "IFriendList.h"

class FClanCollectionViewModelImpl
	: public FClanCollectionViewModel
{
public:

	virtual FText GetClanTitle() override
	{
		if(ClanList.Num() > 0)
		{
			return ClanList[0]->GetClanTitle();
		}
		return FText::GetEmpty();
	}

	virtual void EnumerateJoinedClans(TArray<TSharedRef<FClanInfoViewModel>>& OUTClanList) override
	{
		for (const auto& ClanItem : ClanList)
		{
			OUTClanList.Add(ClanItem);
		}
	}

	virtual int32 NumberOfJoinedClans() const override
	{
		return ClanList.Num();
	}

	virtual int32 NumberOfClanInvites() const override
	{
		return ClanList.Num();
	}


	DECLARE_DERIVED_EVENT(FClanCollectionViewModelImpl, FClanCollectionViewModel::FOpenClanDetails, FOpenClanDetails);
	virtual FOpenClanDetails& OpenClanDetails() override
	{
		return OpenClanDetailsEvent;
	}

private:
	void Initialize()
	{
		TArray<TSharedRef<IClanInfo>> ClanInfos;
		ClanRepository->EnumerateClanList(ClanInfos);
		for (const auto& ClanItem : ClanInfos)
		{
			TSharedRef<FClanInfoViewModel> CIModel = FClanInfoViewModelFactory::Create(ClanItem, FriendsListFactory, FriendsAndChatManager.Pin().ToSharedRef());
			CIModel->OpenClanDetails().AddSP(this, &FClanCollectionViewModelImpl::OnOpenClanDetails);
			ClanList.Add(CIModel);
		}
	}

	void OnOpenClanDetails(const TSharedRef<FClanInfoViewModel > &ClanInfoViewModel)
	{
		OpenClanDetailsEvent.Broadcast(ClanInfoViewModel);
	}

private:
	FClanCollectionViewModelImpl(
	const TSharedRef<IClanRepository>& InClanRepository,
	const TSharedRef<IFriendListFactory>& InFriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: ClanRepository(InClanRepository)
		, FriendsListFactory(InFriendsListFactory)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{
	}

	FOpenClanDetails OpenClanDetailsEvent;
	TSharedRef<IClanRepository> ClanRepository;
	TArray<TSharedRef<FClanInfoViewModel>> ClanList;
	TSharedRef<IFriendListFactory> FriendsListFactory;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;

	friend FClanCollectionViewModelFactory;
};

TSharedRef< FClanCollectionViewModel > FClanCollectionViewModelFactory::Create(
	const TSharedRef<IClanRepository>& ClanRepository,
	const TSharedRef<IFriendListFactory>& FriendsListFactory,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FClanCollectionViewModelImpl > ViewModel(new FClanCollectionViewModelImpl(ClanRepository, FriendsListFactory, FriendsAndChatManager));
	ViewModel->Initialize();
	return ViewModel;
}