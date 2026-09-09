// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"
#include "CodexLSCommonButton.generated.h"

class UCommonTextBlock;

/** Shared tactical menu button with real CommonUI focus and click handling. */
UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSCommonButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UCodexLSCommonButton();

	UFUNCTION(BlueprintCallable, Category = "Last Stand|UI")
	void SetButtonText(const FText& InText);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;
	virtual FReply NativeOnFocusReceived(
		const FGeometry& InGeometry,
		const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	void ApplyLabelColor(const FLinearColor& Color);

	UPROPERTY(EditAnywhere, Category = "Last Stand|UI")
	FText ButtonText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ButtonLabel;
};
