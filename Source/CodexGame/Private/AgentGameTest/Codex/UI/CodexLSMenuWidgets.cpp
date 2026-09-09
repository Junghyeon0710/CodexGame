// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/UI/CodexLSMenuWidgets.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Game/CodexLSGameState.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"
#include "AgentGameTest/Codex/UI/CodexLSCommonButton.h"
#include "AgentGameTest/Codex/UI/CodexLSUIManagerComponent.h"
#include "CommonInputModeTypes.h"
#include "CommonTextBlock.h"
#include "Engine/World.h"

namespace
{
	UCodexLSUIManagerComponent* GetUIManager(const UUserWidget* Widget)
	{
		const ACodexLSPlayerController* Controller = Widget
			? Cast<ACodexLSPlayerController>(Widget->GetOwningPlayer())
			: nullptr;
		return Controller ? Controller->GetUIManagerComponent() : nullptr;
	}
}

UCodexLSActivatableMenuBase::UCodexLSActivatableMenuBase()
{
	bIsBackHandler = true;
}

TOptional<FUIInputConfig> UCodexLSActivatableMenuBase::GetDesiredInputConfig() const
{
	FUIInputConfig Config(
		ECommonInputMode::Menu,
		EMouseCaptureMode::NoCapture,
		EMouseLockMode::DoNotLock,
		false);
	Config.bIgnoreMoveInput = true;
	Config.bIgnoreLookInput = true;
	return Config;
}

UCodexLSCommonButton* UCodexLSActivatableMenuBase::FindMenuButton(
	const FName& WidgetName) const
{
	return Cast<UCodexLSCommonButton>(GetWidgetFromName(WidgetName));
}

void UCodexLSMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PlayButton = FindMenuButton(TEXT("PlayButton"));
	ExitButton = FindMenuButton(TEXT("ExitButton"));

	if (PlayButton)
	{
		PlayButton->SetButtonText(FText::FromString(TEXT("PLAY")));
		PlayButton->OnClicked().AddUObject(this, &ThisClass::HandlePlayClicked);
	}
	if (ExitButton)
	{
		ExitButton->SetButtonText(FText::FromString(TEXT("EXIT")));
		ExitButton->OnClicked().AddUObject(this, &ThisClass::HandleExitClicked);
	}

	if (PlayButton && ExitButton)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_MAIN_MENU_BOUND Play=%s Exit=%s"),
			*GetNameSafe(PlayButton), *GetNameSafe(ExitButton));
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_MAIN_MENU_BOUND Play=%s Exit=%s"),
			*GetNameSafe(PlayButton), *GetNameSafe(ExitButton));
	}
}

UWidget* UCodexLSMainMenuWidget::NativeGetDesiredFocusTarget() const
{
	return PlayButton;
}

bool UCodexLSMainMenuWidget::NativeOnHandleBackAction()
{
	// Main-menu Back is intentionally consumed so PIE is never closed by accident.
	return true;
}

void UCodexLSMainMenuWidget::HandlePlayClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->StartGameplay();
	}
}

void UCodexLSMainMenuWidget::HandleExitClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->RequestExit();
	}
}

UCodexLSPauseMenuWidget::UCodexLSPauseMenuWidget()
{
	bIsBackHandler = true;
}

void UCodexLSPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResumeButton = FindMenuButton(TEXT("ResumeButton"));
	RestartButton = FindMenuButton(TEXT("RestartButton"));
	MainMenuButton = FindMenuButton(TEXT("MainMenuButton"));

	if (ResumeButton)
	{
		ResumeButton->SetButtonText(FText::FromString(TEXT("RESUME")));
		ResumeButton->OnClicked().AddUObject(this, &ThisClass::HandleResumeClicked);
	}
	if (RestartButton)
	{
		RestartButton->SetButtonText(FText::FromString(TEXT("RESTART")));
		RestartButton->OnClicked().AddUObject(this, &ThisClass::HandleRestartClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->SetButtonText(FText::FromString(TEXT("MAIN MENU")));
		MainMenuButton->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
	}
}

UWidget* UCodexLSPauseMenuWidget::NativeGetDesiredFocusTarget() const
{
	return ResumeButton;
}

bool UCodexLSPauseMenuWidget::NativeOnHandleBackAction()
{
	HandleResumeClicked();
	return true;
}

void UCodexLSPauseMenuWidget::HandleResumeClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->ResumeGameplay();
	}
}

void UCodexLSPauseMenuWidget::HandleRestartClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->RestartGameplay();
	}
}

void UCodexLSPauseMenuWidget::HandleMainMenuClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->ReturnToMainMenu();
	}
}

void UCodexLSResultWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResultTitleText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("ResultTitleText")));
	FinalScoreText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("FinalScoreText")));
	RestartButton = FindMenuButton(TEXT("RestartButton"));
	MainMenuButton = FindMenuButton(TEXT("MainMenuButton"));

	if (ResultTitleText)
	{
		ResultTitleText->SetText(GetResultTitle());
	}
	if (RestartButton)
	{
		RestartButton->SetButtonText(FText::FromString(TEXT("RESTART")));
		RestartButton->OnClicked().AddUObject(this, &ThisClass::HandleRestartClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->SetButtonText(FText::FromString(TEXT("MAIN MENU")));
		MainMenuButton->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
	}
	const ACodexLSGameState* GameState = GetWorld()
		? GetWorld()->GetGameState<ACodexLSGameState>()
		: nullptr;
	SetFinalScore(GameState ? GameState->GetScore() : FinalScore);
}

UWidget* UCodexLSResultWidgetBase::NativeGetDesiredFocusTarget() const
{
	return RestartButton;
}

void UCodexLSResultWidgetBase::SetFinalScore(int32 InScore)
{
	FinalScore = FMath::Max(0, InScore);
	if (FinalScoreText)
	{
		FinalScoreText->SetText(FText::AsNumber(FinalScore));
	}
}

void UCodexLSResultWidgetBase::HandleRestartClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->RestartGameplay();
	}
}

void UCodexLSResultWidgetBase::HandleMainMenuClicked()
{
	if (UCodexLSUIManagerComponent* Manager = GetUIManager(this))
	{
		Manager->ReturnToMainMenu();
	}
}

FText UCodexLSGameOverWidget::GetResultTitle() const
{
	return FText::FromString(TEXT("GAME OVER"));
}

FText UCodexLSVictoryWidget::GetResultTitle() const
{
	return FText::FromString(TEXT("VICTORY"));
}
