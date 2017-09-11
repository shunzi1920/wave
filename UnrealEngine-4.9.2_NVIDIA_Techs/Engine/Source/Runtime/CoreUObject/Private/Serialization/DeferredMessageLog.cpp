// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnAsyncLoading.cpp: Unreal async messsage log.
=============================================================================*/

#include "CoreUObjectPrivate.h"
#include "Serialization/DeferredMessageLog.h"


FDeferredMessageLog::FDeferredMessageLog(const FName& InLogCategory)
: LogCategory(InLogCategory)
{
	TArray<TSharedRef<FTokenizedMessage>>** ExistingCategoryMessages = Messages.Find(LogCategory);
	if (!ExistingCategoryMessages)
	{
		TArray<TSharedRef<FTokenizedMessage>>* CategoryMessages = new TArray<TSharedRef<FTokenizedMessage>>();
		Messages.Add(LogCategory, CategoryMessages);
	}
}

void FDeferredMessageLog::AddMessage(TSharedRef<FTokenizedMessage>& Message)
{
	TArray<TSharedRef<FTokenizedMessage>>* CategoryMessages = Messages.FindRef(LogCategory);
	check(CategoryMessages);
	CategoryMessages->Add(Message);
}

TSharedRef<FTokenizedMessage> FDeferredMessageLog::Info(const FText& InMessage)
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Info, InMessage);
	AddMessage(Message);
	return Message;
}

TSharedRef<FTokenizedMessage> FDeferredMessageLog::Warning(const FText& InMessage)
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Warning, InMessage);
	AddMessage(Message);
	return Message;
}

TSharedRef<FTokenizedMessage> FDeferredMessageLog::Error(const FText& InMessage)
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Error, InMessage);
	AddMessage(Message);
	return Message;
}

void FDeferredMessageLog::Flush()
{
	for (auto& CategoryMessages : Messages)
	{
		if (CategoryMessages.Value->Num())
		{
			FMessageLog LoaderLog(CategoryMessages.Key);
			LoaderLog.AddMessages(*CategoryMessages.Value);
			CategoryMessages.Value->Empty(CategoryMessages.Value->Num());
		}
	}
}

TMap<FName, TArray<TSharedRef<FTokenizedMessage>>*> FDeferredMessageLog::Messages;
