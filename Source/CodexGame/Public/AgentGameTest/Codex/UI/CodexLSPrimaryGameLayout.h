// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "CodexLSPrimaryGameLayout.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;

/**
 * Small CommonUI root used by PROJECT: LAST STAND.
 * It owns one persistent gameplay layer and independent menu/modal stacks.
 */
UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidget* PushWidgetToLayer(
		const FGameplayTag& LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass);

	void ClearLayer(const FGameplayTag& LayerTag);
	UCommonActivatableWidget* GetActiveWidget(const FGameplayTag& LayerTag) const;
	bool HasValidLayers() const;

protected:
	virtual void NativeOnInitialized() override;

private:
	UCommonActivatableWidgetStack* FindLayer(const FGameplayTag& LayerTag) const;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;
};
