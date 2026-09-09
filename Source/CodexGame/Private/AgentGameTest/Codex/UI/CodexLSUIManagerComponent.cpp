// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/UI/CodexLSUIManagerComponent.h"

#include "AgentGameTest/Codex/CodexLSGameMode.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"
#include "AgentGameTest/Codex/UI/CodexLSHUDWidget.h"
#include "AgentGameTest/Codex/UI/CodexLSMenuWidgets.h"
#include "AgentGameTest/Codex/UI/CodexLSPrimaryGameLayout.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	constexpr int32 MaxUIInitializationAttempts = 80;
	const TCHAR* PrimaryLayoutPath =
		TEXT("/Game/AgentGameTest/Codex/UI/Layout/W_PrimaryGameLayout_Codex.W_PrimaryGameLayout_Codex_C");
	const TCHAR* HUDPath =
		TEXT("/Game/AgentGameTest/Codex/UI/HUD/W_GameHUD_Codex.W_GameHUD_Codex_C");
	const TCHAR* MainMenuPath =
		TEXT("/Game/AgentGameTest/Codex/UI/Menu/W_MainMenu_Codex.W_MainMenu_Codex_C");
	const TCHAR* PauseMenuPath =
		TEXT("/Game/AgentGameTest/Codex/UI/Menu/W_PauseMenu_Codex.W_PauseMenu_Codex_C");
	const TCHAR* GameOverPath =
		TEXT("/Game/AgentGameTest/Codex/UI/Result/W_GameOver_Codex.W_GameOver_Codex_C");
	const TCHAR* VictoryPath =
		TEXT("/Game/AgentGameTest/Codex/UI/Result/W_Victory_Codex.W_Victory_Codex_C");
}

UCodexLSUIManagerComponent::UCodexLSUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCodexLSUIManagerComponent::InitializeForController(
	ACodexLSPlayerController* InController)
{
	if (bInitializationComplete || !InController || !InController->IsLocalController())
	{
		return;
	}

	Controller = InController;
	InitializationAttempts = 0;
	TryInitializeUI();
}

void UCodexLSUIManagerComponent::TryInitializeUI()
{
	++InitializationAttempts;
	UWorld* World = GetWorld();
	ACodexLSGameMode* GameMode = World ? World->GetAuthGameMode<ACodexLSGameMode>() : nullptr;
	if (!World || !Controller || !GameMode)
	{
		if (World && InitializationAttempts < MaxUIInitializationAttempts)
		{
			World->GetTimerManager().SetTimer(
				InitializationTimerHandle, this, &ThisClass::TryInitializeUI, 0.1f, false);
		}
		return;
	}

	if (!GameMode->UsesFrontendUI())
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_UI_SKIPPED GameMode=%s Reason=FrontendDisabled"),
			*GameMode->GetName());
		return;
	}

	if (!LoadWidgetClasses())
	{
		if (InitializationAttempts < MaxUIInitializationAttempts)
		{
			World->GetTimerManager().SetTimer(
				InitializationTimerHandle, this, &ThisClass::TryInitializeUI, 0.1f, false);
		}
		else
		{
			UE_LOG(LogCodexLastStand, Error,
				TEXT("CODEX_STEP5_UI_INIT_FAILED Reason=MissingWidgetClass Attempts=%d"),
				InitializationAttempts);
		}
		return;
	}

	PrimaryLayout = CreateWidget<UCodexLSPrimaryGameLayout>(Controller, PrimaryLayoutClass);
	if (!PrimaryLayout)
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("CODEX_STEP5_UI_INIT_FAILED Reason=LayoutCreate"));
		return;
	}
	PrimaryLayout->AddToPlayerScreen(100);
	if (!PrimaryLayout->HasValidLayers())
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("CODEX_STEP5_UI_INIT_FAILED Reason=InvalidLayers"));
		return;
	}

	BoundGameState = GetCodexGameState();
	if (BoundGameState)
	{
		BoundGameState->OnGamePhaseChanged.AddUniqueDynamic(
			this, &ThisClass::HandleGamePhaseChanged);
	}

	HUDWidget = Cast<UCodexLSHUDWidget>(PrimaryLayout->PushWidgetToLayer(
		CodexLSGameplayTags::UI_Layer_Game, HUDWidgetClass));
	if (!HUDWidget)
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("CODEX_STEP5_UI_INIT_FAILED Reason=HUDPush"));
		return;
	}

	bInitializationComplete = true;
	World->GetTimerManager().ClearTimer(InitializationTimerHandle);
	if (GameMode->ShouldShowMainMenuOnBoot())
	{
		ShowMainMenu();
	}
	else
	{
		ShowGameplayHUD();
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_UI_READY Attempts=%d Layout=%s HUD=%s MainMenuBoot=%s"),
		InitializationAttempts, *PrimaryLayout->GetName(), *HUDWidget->GetName(),
		GameMode->ShouldShowMainMenuOnBoot() ? TEXT("true") : TEXT("false"));
}

bool UCodexLSUIManagerComponent::LoadWidgetClasses()
{
	if (!PrimaryLayoutClass)
	{
		PrimaryLayoutClass = LoadClass<UCodexLSPrimaryGameLayout>(nullptr, PrimaryLayoutPath);
	}
	if (!HUDWidgetClass)
	{
		HUDWidgetClass = LoadClass<UCodexLSHUDWidget>(nullptr, HUDPath);
	}
	if (!MainMenuClass)
	{
		MainMenuClass = LoadClass<UCodexLSMainMenuWidget>(nullptr, MainMenuPath);
	}
	if (!PauseMenuClass)
	{
		PauseMenuClass = LoadClass<UCodexLSPauseMenuWidget>(nullptr, PauseMenuPath);
	}
	if (!GameOverClass)
	{
		GameOverClass = LoadClass<UCodexLSGameOverWidget>(nullptr, GameOverPath);
	}
	if (!VictoryClass)
	{
		VictoryClass = LoadClass<UCodexLSVictoryWidget>(nullptr, VictoryPath);
	}

	return PrimaryLayoutClass && HUDWidgetClass && MainMenuClass && PauseMenuClass &&
		GameOverClass && VictoryClass;
}

void UCodexLSUIManagerComponent::ShowMainMenu()
{
	if (!PrimaryLayout)
	{
		return;
	}
	PrimaryLayout->ClearLayer(CodexLSGameplayTags::UI_Layer_Menu);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetPlayerGameplayInput(false);
	bPauseOpen = false;
	bResultOpen = false;

	UCommonActivatableWidget* Menu = PrimaryLayout->PushWidgetToLayer(
		CodexLSGameplayTags::UI_Layer_Menu, MainMenuClass);
	ApplyMenuInputMode(Menu);
	if (Menu)
	{
		Menu->RequestRefreshFocus();
	}
	UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP5_MAIN_MENU_SHOWN Focus=PLAY"));
}

void UCodexLSUIManagerComponent::ShowGameplayHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	SetPlayerGameplayInput(true);
	ApplyGameplayInputMode();
	bPauseOpen = false;
	bResultOpen = false;
}

void UCodexLSUIManagerComponent::StartGameplay()
{
	if (!bInitializationComplete || bResultOpen)
	{
		return;
	}
	PopActiveMenu();
	ShowGameplayHUD();
	if (ACodexLSGameMode* GameMode = GetWorld()->GetAuthGameMode<ACodexLSGameMode>())
	{
		GameMode->StartGameplayFromMainMenu();
	}
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_PLAY_ACCEPTED MenuPopped=true HUDVisible=true Input=Gameplay"));
}

void UCodexLSUIManagerComponent::TogglePause()
{
	if (bPauseOpen)
	{
		ResumeGameplay();
	}
	else
	{
		ShowPauseMenu();
	}
}

void UCodexLSUIManagerComponent::ShowPauseMenu()
{
	if (!bInitializationComplete || bPauseOpen || bResultOpen || !PrimaryLayout)
	{
		return;
	}
	const ACodexLSGameState* GameState = GetCodexGameState();
	if (!GameState || GameState->GetGamePhase() == ECodexLSGamePhase::None ||
		GameState->GetGamePhase() == ECodexLSGamePhase::Victory ||
		GameState->GetGamePhase() == ECodexLSGamePhase::GameOver)
	{
		return;
	}

	SetPlayerGameplayInput(false);
	UGameplayStatics::SetGamePaused(this, true);
	bPauseOpen = true;
	UCommonActivatableWidget* Pause = PrimaryLayout->PushWidgetToLayer(
		CodexLSGameplayTags::UI_Layer_Menu, PauseMenuClass);
	ApplyMenuInputMode(Pause);
	if (Pause)
	{
		Pause->RequestRefreshFocus();
	}
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_PAUSE_SHOWN Paused=%s Focus=RESUME"),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"));
}

void UCodexLSUIManagerComponent::ResumeGameplay()
{
	if (!bPauseOpen)
	{
		return;
	}
	UGameplayStatics::SetGamePaused(this, false);
	PopActiveMenu();
	bPauseOpen = false;
	ShowGameplayHUD();
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_RESUMED Paused=false Input=Gameplay HUDPreserved=true"));
}

void UCodexLSUIManagerComponent::RestartGameplay()
{
	UGameplayStatics::SetGamePaused(this, false);
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP5_RESTART_BUTTON AutoPlay=true"));
		GameMode->RestartCurrentLevel();
	}
}

void UCodexLSUIManagerComponent::ReturnToMainMenu()
{
	UGameplayStatics::SetGamePaused(this, false);
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP5_MAIN_MENU_BUTTON AutoPlay=false"));
		GameMode->ReturnToMainMenu();
	}
}

void UCodexLSUIManagerComponent::RequestExit()
{
	UWorld* World = GetWorld();
	if (!World || !Controller)
	{
		return;
	}
#if WITH_EDITOR
	if (World->WorldType == EWorldType::PIE)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_EXIT_SAFE_NOOP Environment=PIE EditorRemainsOpen=true"));
		return;
	}
#endif
	UKismetSystemLibrary::QuitGame(World, Controller, EQuitPreference::Quit, false);
}

void UCodexLSUIManagerComponent::ShowResultScreen(ECodexLSGamePhase ResultPhase)
{
	if (!PrimaryLayout || bResultOpen)
	{
		return;
	}
	UGameplayStatics::SetGamePaused(this, false);
	PrimaryLayout->ClearLayer(CodexLSGameplayTags::UI_Layer_Menu);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetPlayerGameplayInput(false);
	bPauseOpen = false;
	bResultOpen = true;

	TSubclassOf<UCommonActivatableWidget> ResultClass;
	if (ResultPhase == ECodexLSGamePhase::Victory)
	{
		ResultClass = VictoryClass;
	}
	else
	{
		ResultClass = GameOverClass;
	}
	UCodexLSResultWidgetBase* Result = Cast<UCodexLSResultWidgetBase>(
		PrimaryLayout->PushWidgetToLayer(CodexLSGameplayTags::UI_Layer_Menu, ResultClass));
	const int32 FinalScore = BoundGameState ? BoundGameState->GetScore() : 0;
	if (Result)
	{
		Result->SetFinalScore(FinalScore);
		Result->RequestRefreshFocus();
	}
	ApplyMenuInputMode(Result);

	if (Result)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_RESULT_SHOWN Type=%s FinalScore=%d Focus=RESTART Success=true"),
			ResultPhase == ECodexLSGamePhase::Victory ? TEXT("Victory") : TEXT("GameOver"),
			FinalScore);
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_RESULT_SHOWN Type=%s FinalScore=%d Focus=RESTART Success=false"),
			ResultPhase == ECodexLSGamePhase::Victory ? TEXT("Victory") : TEXT("GameOver"),
			FinalScore);
	}
}

void UCodexLSUIManagerComponent::HandleGamePhaseChanged(
	ECodexLSGamePhase PreviousPhase,
	ECodexLSGamePhase NewPhase)
{
	if (NewPhase == ECodexLSGamePhase::GameOver || NewPhase == ECodexLSGamePhase::Victory)
	{
		ShowResultScreen(NewPhase);
	}
}

void UCodexLSUIManagerComponent::ApplyGameplayInputMode()
{
	if (!Controller)
	{
		return;
	}
	Controller->bShowMouseCursor = true;
	Controller->bEnableClickEvents = false;
	Controller->bEnableMouseOverEvents = false;
	Controller->DefaultMouseCursor = EMouseCursor::Crosshairs;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);
}

void UCodexLSUIManagerComponent::ApplyMenuInputMode(UWidget* FocusRoot)
{
	if (!Controller)
	{
		return;
	}
	Controller->bShowMouseCursor = true;
	Controller->bEnableClickEvents = true;
	Controller->bEnableMouseOverEvents = true;
	Controller->DefaultMouseCursor = EMouseCursor::Default;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	UWidget* FocusTarget = FocusRoot;
	if (UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(FocusRoot))
	{
		if (UWidget* DesiredTarget = Activatable->GetDesiredFocusTarget())
		{
			FocusTarget = DesiredTarget;
		}
	}
	if (FocusTarget)
	{
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
	}
	Controller->SetInputMode(InputMode);
	if (FocusTarget)
	{
		FocusTarget->SetFocus();
		FocusTarget->SetUserFocus(Controller);
		FocusTarget->SetKeyboardFocus();

		TWeakObjectPtr<UCodexLSUIManagerComponent> WeakThis(this);
		TWeakObjectPtr<UWidget> WeakFocusTarget(FocusTarget);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateWeakLambda(this, [WeakThis, WeakFocusTarget]()
				{
					UCodexLSUIManagerComponent* Manager = WeakThis.Get();
					UWidget* DeferredTarget = WeakFocusTarget.Get();
					if (!Manager || !Manager->Controller || !DeferredTarget)
					{
						return;
					}

					FInputModeUIOnly DeferredInputMode;
					DeferredInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
					DeferredInputMode.SetWidgetToFocus(DeferredTarget->TakeWidget());
					Manager->Controller->SetInputMode(DeferredInputMode);
					DeferredTarget->SetFocus();
					DeferredTarget->SetUserFocus(Manager->Controller);
					DeferredTarget->SetKeyboardFocus();
					UE_LOG(LogCodexLastStand, Log,
						TEXT("CODEX_STEP5_MENU_FOCUS_APPLIED Target=%s Requested=true"),
						*GetNameSafe(DeferredTarget));
				}));
		}
	}
	if (FocusTarget)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_MENU_FOCUS Target=%s"), *GetNameSafe(FocusTarget));
	}
	else
	{
		UE_LOG(LogCodexLastStand, Warning, TEXT("CODEX_STEP5_MENU_FOCUS Target=None"));
	}
}

void UCodexLSUIManagerComponent::SetPlayerGameplayInput(bool bEnabled) const
{
	if (ACodexLSPlayerCharacter* Player = Controller
		? Cast<ACodexLSPlayerCharacter>(Controller->GetPawn())
		: nullptr)
	{
		Player->SetGameplayInputEnabled(bEnabled);
	}
}

void UCodexLSUIManagerComponent::PopActiveMenu()
{
	if (!PrimaryLayout)
	{
		return;
	}
	if (UCommonActivatableWidget* Active = PrimaryLayout->GetActiveWidget(
		CodexLSGameplayTags::UI_Layer_Menu))
	{
		Active->DeactivateWidget();
	}
}

ACodexLSGameState* UCodexLSUIManagerComponent::GetCodexGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ACodexLSGameState>() : nullptr;
}

void UCodexLSUIManagerComponent::DebugAction(const FString& Action)
{
	FString Upper = Action;
	Upper.ToUpperInline();
	if (Upper == TEXT("PLAY"))
	{
		StartGameplay();
	}
	else if (Upper == TEXT("PAUSE"))
	{
		ShowPauseMenu();
	}
	else if (Upper == TEXT("RESUME") || Upper == TEXT("BACK"))
	{
		ResumeGameplay();
	}
	else if (Upper == TEXT("RESTART"))
	{
		RestartGameplay();
	}
	else if (Upper == TEXT("MAINMENU") || Upper == TEXT("MENU"))
	{
		ReturnToMainMenu();
	}
	else if (Upper == TEXT("EXIT"))
	{
		RequestExit();
	}
	else
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP5_UI_ACTION_UNKNOWN Action=%s"), *Action);
	}
}

void UCodexLSUIManagerComponent::DebugSnapshot() const
{
	const ACodexLSGameState* GameState = GetCodexGameState();
	const UCommonActivatableWidget* ActiveMenu = PrimaryLayout
		? PrimaryLayout->GetActiveWidget(CodexLSGameplayTags::UI_Layer_Menu)
		: nullptr;
	const UEnum* PhaseEnum = StaticEnum<ECodexLSGamePhase>();
	const FString Phase = GameState && PhaseEnum
		? PhaseEnum->GetNameStringByValue(static_cast<int64>(GameState->GetGamePhase()))
		: TEXT("Missing");
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_UI_SNAPSHOT Ready=%s Phase=%s Menu=%s Pause=%s Result=%s WorldPaused=%s Cursor=%s HUD={%s}"),
		bInitializationComplete ? TEXT("true") : TEXT("false"), *Phase,
		*GetNameSafe(ActiveMenu), bPauseOpen ? TEXT("true") : TEXT("false"),
		bResultOpen ? TEXT("true") : TEXT("false"),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"),
		Controller && Controller->bShowMouseCursor ? TEXT("true") : TEXT("false"),
		HUDWidget ? *HUDWidget->GetDebugSummary() : TEXT("Missing"));
}

void UCodexLSUIManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitializationTimerHandle);
	}
	if (BoundGameState)
	{
		BoundGameState->OnGamePhaseChanged.RemoveDynamic(
			this, &ThisClass::HandleGamePhaseChanged);
	}
	UGameplayStatics::SetGamePaused(this, false);
	Super::EndPlay(EndPlayReason);
}
