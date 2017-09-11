// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SChatChromeTabQuickSettings.h"
#include "IChatTabViewModel.h"
#include "ChatViewModel.h"

#define LOCTEXT_NAMESPACE "SChatChromeTabQuickSettingsImpl"

class SChatChromeTabQuickSettingsImpl : public SChatChromeTabQuickSettings
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<class IChatTabViewModel>& InChromeTabViewModel) override
	{
		FriendStyle = *InArgs._FriendStyle;
		ChatType = InArgs._ChatType;
		ChromeTabViewModel = InChromeTabViewModel;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
 			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Black)
 			.Padding(8.0f)
 			.BorderImage(&FriendStyle.FriendsChatStyle.ChatBackgroundBrush)
 			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
					.Text(LOCTEXT("Include", "Include:"))
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(OptionsContainer, SVerticalBox)
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 10.0f, 0.f, 0.f)
				.AutoHeight()
				[
					SNew(SButton)
					.OnClicked(this, &SChatChromeTabQuickSettingsImpl::Reset)
					.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
						.Text(LOCTEXT("Reset", "Reset"))
					]
				]
			]
		]);
		GenerateOptions();
	}

private:

	void GenerateOptions()
	{
		OptionsContainer->ClearChildren();

		EChatMessageType::Type MessageType = (EChatMessageType::Type)1;
		for (; MessageType <= EChatMessageType::Global; MessageType = (EChatMessageType::Type)(MessageType << 1))
		{
			if (MessageType != EChatMessageType::Custom && MessageType != ChatType)
			{
				OptionsContainer->AddSlot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.IsChecked(this, &SChatChromeTabQuickSettingsImpl::OnEditableCheckboxState, MessageType)
						.OnCheckStateChanged(this, &SChatChromeTabQuickSettingsImpl::OnEditableChanged, MessageType)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
						.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
						.Text(EChatMessageType::ToText(MessageType))
					]
				];
			}
		}
	}

	ECheckBoxState OnEditableCheckboxState(EChatMessageType::Type MessageType) const
	{
		if (ChromeTabViewModel.IsValid() && ChromeTabViewModel->GetChatViewModel().IsValid())
		{
			return ChromeTabViewModel->GetChatViewModel()->IsChannelSet(MessageType) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
		return ECheckBoxState::Unchecked;
	}

	void OnEditableChanged(ECheckBoxState InNewValue, EChatMessageType::Type MessageType)
	{
		if (ChromeTabViewModel.IsValid() && ChromeTabViewModel->GetChatViewModel().IsValid())
		{
			ChromeTabViewModel->GetChatViewModel()->ToggleChannel(MessageType);
		}
	}

	FReply Reset()
	{
		if (ChromeTabViewModel.IsValid() && ChromeTabViewModel->GetChatViewModel().IsValid())
		{
			if (ChatType == EChatMessageType::Custom)
			{
				ChromeTabViewModel->GetChatViewModel()->SetChannelFlags(EChatMessageType::Global | EChatMessageType::Game | EChatMessageType::Whisper);
			}
			else
			{
				ChromeTabViewModel->GetChatViewModel()->SetChannelFlags(ChatType);
			}
		}
		return FReply::Handled();
	}

	TSharedPtr<IChatTabViewModel> ChromeTabViewModel;
	TSharedPtr<SVerticalBox> OptionsContainer;
	FFriendsAndChatStyle FriendStyle;
	EChatMessageType::Type ChatType;
};

TSharedRef<SChatChromeTabQuickSettings> SChatChromeTabQuickSettings::New()
{
	return MakeShareable(new SChatChromeTabQuickSettingsImpl());
}

#undef LOCTEXT_NAMESPACE
