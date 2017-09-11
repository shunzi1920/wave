// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SClanItem.h"
#include "ClanInfoViewModel.h"
#include "SFriendsAndChatCombo.h"

#define LOCTEXT_NAMESPACE "SClassItem"

class SClanItemImpl : public SClanItem
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FClanInfoViewModel>& InClanInfoViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		ClanInfoViewModel = InClanInfoViewModel;

		FFriendsAndChatComboButtonStyle ActionButtonStyle;
		ActionButtonStyle.ComboButtonStyle = &FriendStyle.FriendsComboStyle.ActionComboButtonStyle;
		ActionButtonStyle.ButtonTextStyle = &FriendStyle.FriendsComboStyle.ActionComboButtonTextStyle;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SBorder)
			.Padding(FMargin(0))
			.BorderBackgroundColor(FLinearColor::Transparent)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(10, 10)
					.AutoWidth()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Left)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							// Clan Image
							SNew(SImage)
							.Image(&FriendStyle.UTImageBrush)
						]
						+SOverlay::Slot()
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Right)
						[
							// Primary Clan Indicator
							SNew(SImage)
							.Image(&FriendStyle.AwayBrush )
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(15, 0)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Right)
					[
						SAssignNew(ActionMenuButton, SFriendsAndChatCombo)
						.ComboStyle(&FriendStyle.FriendsComboStyle)
						.ButtonStyleOverride(&ActionButtonStyle)
						.ButtonText(FText::GetEmpty())
						.bShowIcon(false)
						.DropdownItems(this, &SClanItemImpl::GetActionItems)
						.bSetButtonTextToSelectedItem(false)
						.bAutoCloseWhenClicked(true)
						.ButtonSize(FriendStyle.FriendsComboStyle.ActionComboButtonSize)
						.Placement(MenuPlacement_ComboBoxRight)
						.OnDropdownItemClicked(this, &SClanItemImpl::HandleItemClicked)
						.OnDropdownOpened(this, &SClanItemImpl::HandleActionMenuOpened)
						.Visibility(this, &SClanItemImpl::ActionMenuButtonVisibility)
						.Cursor(EMouseCursor::Hand)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.Padding(0, 10)
					[
						SNew(SButton)
						.OnClicked(this, &SClanItemImpl::OnOpenClanDetails)
						.ButtonStyle(&FriendStyle.GlobalChatButtonStyle)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(&FriendStyle.ClanMembersBrush)
							]
							+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Padding(10)
							[
								SNew(STextBlock)
								.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
								.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
								.Text(this, &SClanItemImpl::GetClanMemberCountText)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(&FriendStyle.ClanDetailsBrush)
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
					.Text(ClanInfoViewModel->GetClanTitle())
				]
			]
		]);
	}

private:

	FReply OnOpenClanDetails()
	{
		ClanInfoViewModel->OpenClanDetails().Broadcast(ClanInfoViewModel.ToSharedRef());
		return FReply::Handled();
	}

	SFriendsAndChatCombo::FItemsArray GetActionItems() const
	{
		SFriendsAndChatCombo::FItemsArray ActionItems;
		// Need to Add some actions

		TArray<EClanActionType::Type> Actions;
		ClanInfoViewModel->EnumerateActions(Actions);
 
 		for (const auto& ClanAction : Actions)
 		{
			ActionItems.AddItem(EClanActionType::ToText(ClanAction), nullptr, FName(*EClanActionType::ToText(ClanAction).ToString()), true);
 		}

		return ActionItems;
	}

	void HandleItemClicked(FName ItemTag)
	{
	}

	void HandleActionMenuOpened() const
	{
	}

	EVisibility ActionMenuButtonVisibility() const
	{
		return EVisibility::Visible;
	}

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	FText GetClanMemberCountText() const
	{
		return FText::AsNumber(ClanInfoViewModel->GetMemberCount());
	}


	TSharedPtr<FClanInfoViewModel> ClanInfoViewModel;

	/** Holds the style to use when making the widget. */
	FFriendsListStyle FriendStyle;

	TSharedPtr<SWidget> MenuContent;

	EPopupMethod MenuMethod;

	TSharedPtr<SFriendsAndChatCombo> ActionMenuButton;

	/** Cached actual dropdown button */
};

TSharedRef<SClanItem> SClanItem::New()
{
	return MakeShareable(new SClanItemImpl());
}

#undef LOCTEXT_NAMESPACE
