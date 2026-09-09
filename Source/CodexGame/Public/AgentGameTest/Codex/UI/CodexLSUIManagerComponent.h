// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AgentGameTest/Codex/Game/CodexLSGameState.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "CodexLSUIManagerComponent.generated.h"

class ACodexLSPlayerController;
class UCodexLSGameOverWidget;
class UCodexLSHUDWidget;
class UCodexLSMainMenuWidget;
class UCodexLSPauseMenuWidget;
class UCodexLSPrimaryGameLayout;
class UCodexLSVictoryWidget;
class UWidget;

/**
 * PlayerController-owned UI policy for the mini-game.
 * It creates one root layout, routes phase events, and owns input/pause transitions.
 */
UCLASS(ClassGroup = (LastStand), meta = (BlueprintSpawnableComponent))
class CODEXGAME_API UCodexLSUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCodexLSUIManagerComponent();

	void InitializeForController(ACodexLSPlayerController* InController);
	void StartGameplay();
	void TogglePause();
	void ShowPauseMenu();
	void ResumeGameplay();
	void RestartGameplay();
	void ReturnToMainMenu();
	void RequestExit();
	void DebugSnapshot() const;
	void DebugAction(const FString& Action);

	bool IsInitialized() const { return bInitializationComplete; }
	bool IsPauseOpen() const { return bPauseOpen; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryInitializeUI();
	bool LoadWidgetClasses();
	void ShowMainMenu();
	void ShowGameplayHUD();
	void ShowResultScreen(ECodexLSGamePhase ResultPhase);
	void ApplyGameplayInputMode();
	void ApplyMenuInputMode(UWidget* FocusRoot);
	void SetPlayerGameplayInput(bool bEnabled) const;
	void PopActiveMenu();
	ACodexLSGameState* GetCodexGameState() const;

	UFUNCTION()
	void HandleGamePhaseChanged(ECodexLSGamePhase PreviousPhase, ECodexLSGamePhase NewPhase);

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSPlayerController> Controller;

	UPROPERTY(Transient)
	TObjectPtr<UCodexLSPrimaryGameLayout> PrimaryLayout;

	UPROPERTY(Transient)
	TObjectPtr<UCodexLSHUDWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSGameState> BoundGameState;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSPrimaryGameLayout> PrimaryLayoutClass;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSMainMenuWidget> MainMenuClass;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSPauseMenuWidget> PauseMenuClass;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSGameOverWidget> GameOverClass;

	UPROPERTY(Transient)
	TSubclassOf<UCodexLSVictoryWidget> VictoryClass;

	FTimerHandle InitializationTimerHandle;
	int32 InitializationAttempts = 0;
	bool bInitializationComplete = false;
	bool bPauseOpen = false;
	bool bResultOpen = false;
};
