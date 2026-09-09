// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/UI/CodexLSPrimaryGameLayout.h"

#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UCodexLSPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	GameLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("GameLayer")));
	MenuLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("MenuLayer")));
	ModalLayer = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("ModalLayer")));

	if (GameLayer)
	{
		GameLayer->SetTransitionDuration(0.12f);
	}
	if (MenuLayer)
	{
		MenuLayer->SetTransitionDuration(0.16f);
	}
	if (ModalLayer)
	{
		ModalLayer->SetTransitionDuration(0.12f);
	}

	if (HasValidLayers())
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_LAYOUT_READY Game=%s Menu=%s Modal=%s"),
			*GetNameSafe(GameLayer), *GetNameSafe(MenuLayer), *GetNameSafe(ModalLayer));
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_LAYOUT_READY Game=%s Menu=%s Modal=%s"),
			*GetNameSafe(GameLayer), *GetNameSafe(MenuLayer), *GetNameSafe(ModalLayer));
	}
}

UCommonActivatableWidget* UCodexLSPrimaryGameLayout::PushWidgetToLayer(
	const FGameplayTag& LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UCommonActivatableWidgetStack* Layer = FindLayer(LayerTag);
	if (!Layer || !WidgetClass)
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_LAYER_PUSH_FAILED Layer=%s Class=%s"),
			*LayerTag.ToString(), *GetNameSafe(WidgetClass));
		return nullptr;
	}

	UCommonActivatableWidget* Widget = Layer->AddWidget(WidgetClass);
	if (Widget)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_LAYER_PUSH Layer=%s Widget=%s Success=true"),
			*LayerTag.ToString(), *GetNameSafe(Widget));
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_LAYER_PUSH Layer=%s Widget=%s Success=false"),
			*LayerTag.ToString(), *GetNameSafe(Widget));
	}
	return Widget;
}

void UCodexLSPrimaryGameLayout::ClearLayer(const FGameplayTag& LayerTag)
{
	if (UCommonActivatableWidgetStack* Layer = FindLayer(LayerTag))
	{
		Layer->ClearWidgets();
	}
}

UCommonActivatableWidget* UCodexLSPrimaryGameLayout::GetActiveWidget(
	const FGameplayTag& LayerTag) const
{
	const UCommonActivatableWidgetStack* Layer = FindLayer(LayerTag);
	return Layer ? Layer->GetActiveWidget() : nullptr;
}

bool UCodexLSPrimaryGameLayout::HasValidLayers() const
{
	return GameLayer && MenuLayer && ModalLayer;
}

UCommonActivatableWidgetStack* UCodexLSPrimaryGameLayout::FindLayer(
	const FGameplayTag& LayerTag) const
{
	if (LayerTag == CodexLSGameplayTags::UI_Layer_Game)
	{
		return GameLayer;
	}
	if (LayerTag == CodexLSGameplayTags::UI_Layer_Menu)
	{
		return MenuLayer;
	}
	if (LayerTag == CodexLSGameplayTags::UI_Layer_Modal)
	{
		return ModalLayer;
	}
	return nullptr;
}
