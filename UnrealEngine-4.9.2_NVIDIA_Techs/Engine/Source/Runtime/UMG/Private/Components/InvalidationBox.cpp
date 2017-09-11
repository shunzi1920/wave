// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "InvalidationBox.h"
#include "SInvalidationPanel.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UInvalidationBox

UInvalidationBox::UInvalidationBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Visibility = ESlateVisibility::SelfHitTestInvisible;
}

void UInvalidationBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyInvalidationPanel.Reset();
}

TSharedRef<SWidget> UInvalidationBox::RebuildWidget()
{
	MyInvalidationPanel = 
		SNew(SInvalidationPanel)
		.CacheRelativeTransforms(CacheRelativeTransforms);

	if ( IsDesignTime() )
	{
		MyInvalidationPanel->SetCanCache(false);
	}

	if ( GetChildrenCount() > 0 )
	{
		MyInvalidationPanel->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}
	
	return BuildDesignTimeWidget(MyInvalidationPanel.ToSharedRef());
}

void UInvalidationBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyInvalidationPanel.IsValid() )
	{
		MyInvalidationPanel->SetContent(Slot->Content ? Slot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UInvalidationBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyInvalidationPanel.IsValid() )
	{
		MyInvalidationPanel->SetContent(SNullWidget::NullWidget);
	}
}

void UInvalidationBox::InvalidateCache()
{
	if ( MyInvalidationPanel.IsValid() )
	{
		return MyInvalidationPanel->InvalidateCache();
	}
}

#if WITH_EDITOR

const FSlateBrush* UInvalidationBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.MenuAnchor");
}

const FText UInvalidationBox::GetPaletteCategory()
{
	return LOCTEXT("Optimization", "Optimization");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
