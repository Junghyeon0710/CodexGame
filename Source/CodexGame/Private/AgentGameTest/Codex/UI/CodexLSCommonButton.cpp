// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/UI/CodexLSCommonButton.h"

#include "CommonTextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UCodexLSCommonButton::UCodexLSCommonButton()
{
	SetIsFocusable(true);
	SetMinDimensions(360, 64);
	ButtonText = FText::FromString(TEXT("ACTION"));

	static ConstructorHelpers::FObjectFinder<USoundBase> ConfirmSoundFinder(
		TEXT("/Game/AgentGameTest/Codex/Audio/UI/S_UI_Confirm_Codex.S_UI_Confirm_Codex"));
	ConfirmSound = ConfirmSoundFinder.Object;
}

void UCodexLSCommonButton::NativeOnClicked()
{
	// Play before broadcasting OnClicked so level travel/restart cannot cut the cue off.
	if (!GetLocked() && IsInteractionEnabled() && ConfirmSound)
	{
		UGameplayStatics::PlaySound2D(this, ConfirmSound, 0.55f, 1.0f);
	}
	Super::NativeOnClicked();
}

void UCodexLSCommonButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ButtonLabel = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("ButtonLabel")));
	SetButtonText(ButtonText);
}

void UCodexLSCommonButton::SetButtonText(const FText& InText)
{
	ButtonText = InText;
	if (ButtonLabel)
	{
		ButtonLabel->SetText(ButtonText);
	}
}

void UCodexLSCommonButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	ApplyLabelColor(FLinearColor(1.0f, 0.78f, 0.24f, 1.0f));
}

void UCodexLSCommonButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	ApplyLabelColor(FLinearColor(0.92f, 0.94f, 0.95f, 1.0f));
}

void UCodexLSCommonButton::NativeOnPressed()
{
	Super::NativeOnPressed();
	ApplyLabelColor(FLinearColor(1.0f, 0.90f, 0.56f, 1.0f));
}

void UCodexLSCommonButton::NativeOnReleased()
{
	Super::NativeOnReleased();
	ApplyLabelColor(IsHovered()
		? FLinearColor(1.0f, 0.78f, 0.24f, 1.0f)
		: FLinearColor(0.92f, 0.94f, 0.95f, 1.0f));
}

FReply UCodexLSCommonButton::NativeOnFocusReceived(
	const FGeometry& InGeometry,
	const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	ApplyLabelColor(FLinearColor(1.0f, 0.78f, 0.24f, 1.0f));
	return Reply;
}

void UCodexLSCommonButton::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	ApplyLabelColor(IsHovered()
		? FLinearColor(1.0f, 0.78f, 0.24f, 1.0f)
		: FLinearColor(0.92f, 0.94f, 0.95f, 1.0f));
}

void UCodexLSCommonButton::ApplyLabelColor(const FLinearColor& Color)
{
	if (ButtonLabel)
	{
		ButtonLabel->SetColorAndOpacity(FSlateColor(Color));
	}
}
