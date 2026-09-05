// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/CodexLSGameMode.h"
#include "AgentGameTest/Codex/AI/CodexLSEnemyAIController.h"
#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"

ACodexLSPlayerController::ACodexLSPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ACodexLSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACodexLSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ThisClass::DebugSoloGrunt);
	InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ThisClass::DebugSoloRunner);
	InputComponent->BindKey(EKeys::F11, IE_Pressed, this, &ThisClass::DebugMultiEnemy);
	InputComponent->BindKey(EKeys::F12, IE_Pressed, this, &ThisClass::DebugAttackNearestEnemy);
	InputComponent->BindKey(EKeys::Insert, IE_Pressed, this, &ThisClass::DebugSnapshot);
	InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ThisClass::DebugBoostPlayerHealth);
	InputComponent->BindKey(EKeys::End, IE_Pressed, this, &ThisClass::DebugSetLitView);
	InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &ThisClass::DebugDefeatAllWaveEnemies);
	InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &ThisClass::DebugForcePlayerDeath);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ThisClass::DebugRestartLevel);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("STEP3 QA Keys Bound | F6=Restart F7=GameOver F8=DefeatWave F9=SoloGrunt F10=SoloRunner F11=Multi F12=Attack Insert=Snapshot Home=BoostHealth End=LitView"));
}

void ACodexLSPlayerController::CodexDebugInputChord(FString Chord, bool bDash, float HoldSeconds)
{
	if (!IsLocalController() || !GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(DebugReleaseTimer);
	GetWorld()->GetTimerManager().ClearTimer(DebugDashPressTimer);
	GetWorld()->GetTimerManager().ClearTimer(DebugDashReleaseTimer);
	ReleaseDebugDash();
	ReleaseDebugKeys();

	Chord.ToUpperInline();
	if (Chord == TEXT("LMB"))
	{
		PressDebugKey(EKeys::LeftMouseButton);
	}
	else
	{
		for (const TCHAR KeyCharacter : Chord)
		{
			const FKey* Key = nullptr;
			switch (KeyCharacter)
			{
			case TEXT('W'): Key = &EKeys::W; break;
			case TEXT('A'): Key = &EKeys::A; break;
			case TEXT('S'): Key = &EKeys::S; break;
			case TEXT('D'): Key = &EKeys::D; break;
			default: break;
			}

			if (Key && !DebugHeldKeys.Contains(*Key))
			{
				PressDebugKey(*Key);
			}
		}
	}

	if (bDash)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DebugDashPressTimer, this, &ThisClass::PressDebugDash, 0.05f, false);
		GetWorld()->GetTimerManager().SetTimer(
			DebugDashReleaseTimer, this, &ThisClass::ReleaseDebugDash, 0.11f, false);
	}

	GetWorld()->GetTimerManager().SetTimer(
		DebugReleaseTimer,
		this,
		&ThisClass::ReleaseDebugKeys,
		FMath::Max(0.12f, HoldSeconds),
		false);

	UE_LOG(LogCodexLastStand, Log, TEXT("Debug Input Chord: %s | Dash=%s | Hold=%.2f"),
		*Chord, bDash ? TEXT("true") : TEXT("false"), HoldSeconds);
}

void ACodexLSPlayerController::CodexDebugSetMouse(int32 ScreenX, int32 ScreenY)
{
	SetMouseLocation(ScreenX, ScreenY);
	UE_LOG(LogCodexLastStand, Log, TEXT("Debug Mouse Position: X=%d Y=%d"), ScreenX, ScreenY);
}

void ACodexLSPlayerController::CodexDebugEnemyScenario(FString Scenario)
{
	if (!GetWorld())
	{
		return;
	}

	Scenario.ToUpperInline();
	if (Scenario == TEXT("MULTI"))
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("STEP2 QA Scenario: MULTI (level actors unchanged)"));
		return;
	}

	const bool bKeepGrunt = Scenario == TEXT("SOLOGRUNT");
	const bool bKeepRunner = Scenario == TEXT("SOLORUNNER");
	if (!bKeepGrunt && !bKeepRunner)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("Unknown STEP2 QA Scenario: %s (expected SoloGrunt, SoloRunner, or Multi)"),
			*Scenario);
		return;
	}

	ACodexLSEnemyCharacter* KeptEnemy = nullptr;
	int32 RemovedCount = 0;
	for (TActorIterator<ACodexLSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		ACodexLSEnemyCharacter* Enemy = *It;
		const bool bMatchingType = bKeepGrunt
			? Enemy->GetEnemyArchetype() == ECodexLSEnemyArchetype::Grunt
			: Enemy->GetEnemyArchetype() == ECodexLSEnemyArchetype::Runner;

		if (!KeptEnemy && bMatchingType && !Enemy->IsDead())
		{
			KeptEnemy = Enemy;
			continue;
		}

		Enemy->Destroy();
		++RemovedCount;
	}

	if (KeptEnemy)
	{
		KeptEnemy->SetActorLocation(FVector(0.0f, 950.0f, 90.0f), false, nullptr,
			ETeleportType::TeleportPhysics);
		if (AController* EnemyController = KeptEnemy->GetController())
		{
			EnemyController->StopMovement();
		}
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("STEP2 QA Scenario: %s | Kept=%s Removed=%d"),
		*Scenario, *GetNameSafe(KeptEnemy), RemovedCount);
}

void ACodexLSPlayerController::CodexDebugAttackEnemy(FString NameContains)
{
	if (!GetWorld())
	{
		return;
	}

	NameContains.ToUpperInline();
	ACodexLSEnemyCharacter* BestEnemy = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<ACodexLSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		ACodexLSEnemyCharacter* Enemy = *It;
		const FString SearchText =
			(Enemy->GetName() + TEXT(" ") + Enemy->GetEnemyArchetypeName()).ToUpper();
		if (Enemy->IsDead() || (!NameContains.IsEmpty() && !SearchText.Contains(NameContains)))
		{
			continue;
		}

		const float DistanceSquared = GetPawn()
			? FVector::DistSquared2D(GetPawn()->GetActorLocation(), Enemy->GetActorLocation())
			: 0.0f;
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestEnemy = Enemy;
		}
	}

	if (!BestEnemy)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("STEP2 QA Attack: no living enemy matched '%s'"), *NameContains);
		return;
	}

	FVector2D ScreenPosition;
	if (!ProjectWorldLocationToScreen(
		BestEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f),
		ScreenPosition,
		true))
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("STEP2 QA Attack: failed to project %s to screen"), *BestEnemy->GetName());
		return;
	}

	SetMouseLocation(FMath::RoundToInt(ScreenPosition.X), FMath::RoundToInt(ScreenPosition.Y));
	GetWorld()->GetTimerManager().ClearTimer(DebugAttackPressTimer);
	GetWorld()->GetTimerManager().ClearTimer(DebugAttackReleaseTimer);
	GetWorld()->GetTimerManager().SetTimer(
		DebugAttackPressTimer, this, &ThisClass::PressDebugPrimaryAttack, 0.06f, false);
	GetWorld()->GetTimerManager().SetTimer(
		DebugAttackReleaseTimer, this, &ThisClass::ReleaseDebugPrimaryAttack, 0.12f, false);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("STEP2 QA Attack Queued | Enemy=%s Type=%s Screen=(%.0f, %.0f) Health=%.0f"),
		*BestEnemy->GetName(), *BestEnemy->GetEnemyArchetypeName(),
		ScreenPosition.X, ScreenPosition.Y, BestEnemy->GetHealth());
}

void ACodexLSPlayerController::CodexDebugCombatSnapshot()
{
	float PlayerHealth = -1.0f;
	float PlayerMaxHealth = -1.0f;
	if (APawn* ControlledPawn = GetPawn())
	{
		if (const UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn))
		{
			PlayerHealth = ASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute());
			PlayerMaxHealth = ASC->GetNumericAttribute(UCodexLSAttributeSet::GetMaxHealthAttribute());
		}
	}

	int32 LivingEnemies = 0;
	int32 TotalEnemies = 0;
	for (TActorIterator<ACodexLSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		ACodexLSEnemyCharacter* Enemy = *It;
		++TotalEnemies;
		LivingEnemies += Enemy->IsDead() ? 0 : 1;
		const ACodexLSEnemyAIController* EnemyAI =
			Cast<ACodexLSEnemyAIController>(Enemy->GetController());

		UE_LOG(LogCodexLastStand, Log,
			TEXT("STEP2 ENEMY SNAPSHOT | Name=%s Type=%s Health=%.0f/%.0f Speed=%.0f Damage=%.0f Cooldown=%.1f Range=%.0f Dead=%s AI=%s Location=(%.0f,%.0f,%.0f)"),
			*Enemy->GetName(), *Enemy->GetEnemyArchetypeName(), Enemy->GetHealth(),
			Enemy->GetMaxHealth(), Enemy->GetCharacterMovement()->MaxWalkSpeed,
			Enemy->GetAttackDamage(), Enemy->GetAttackCooldown(), Enemy->GetAttackRange(),
			Enemy->IsDead() ? TEXT("true") : TEXT("false"),
			EnemyAI ? *EnemyAI->GetEnemyAIStateName() : TEXT("NoController"),
			Enemy->GetActorLocation().X, Enemy->GetActorLocation().Y, Enemy->GetActorLocation().Z);
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP2_SNAPSHOT | PlayerHealth=%.0f/%.0f Enemies=%d Living=%d"),
		PlayerHealth, PlayerMaxHealth, TotalEnemies, LivingEnemies);

	CodexDebugGameLoopSnapshot();
}

void ACodexLSPlayerController::CodexDebugGameLoopSnapshot()
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugGameLoopSnapshot();
	}
}

void ACodexLSPlayerController::CodexDebugDefeatEnemies(FString Filter, int32 Count)
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugDefeatActiveEnemies(Filter, Count);
	}
}

void ACodexLSPlayerController::CodexDebugKillPlayer()
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugForcePlayerDeath();
	}
}

void ACodexLSPlayerController::CodexDebugTerminalRace(FString Order)
{
	Order.ToUpperInline();
	const bool bPlayerFirst = Order == TEXT("PLAYERFIRST") || Order == TEXT("PLAYER");
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugTerminalRace(bPlayerFirst);
	}
}

void ACodexLSPlayerController::CodexDebugForceSpawnFailures(int32 Count)
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugForceNextSpawnFailures(Count);
	}
}

void ACodexLSPlayerController::CodexDebugRestartGameLoop()
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->RestartCurrentLevel();
	}
}

void ACodexLSPlayerController::CodexDebugSetPlayerHealth(float Health)
{
	APawn* ControlledPawn = GetPawn();
	UAbilitySystemComponent* ASC = ControlledPawn
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn)
		: nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dead))
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_QA_HEALTH_BOOST_SKIPPED Health=%.0f Reason=MissingASCOrDead"),
			Health);
		return;
	}

	const float SanitizedHealth = FMath::Max(1.0f, Health);
	ASC->SetNumericAttributeBase(UCodexLSAttributeSet::GetMaxHealthAttribute(), SanitizedHealth);
	ASC->SetNumericAttributeBase(UCodexLSAttributeSet::GetHealthAttribute(), SanitizedHealth);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_HEALTH_BOOST Health=%.0f"), SanitizedHealth);
}

void ACodexLSPlayerController::CodexDebugRestartWithHealth(float Health)
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugRestartWithHealth(Health);
	}
}

void ACodexLSPlayerController::CodexDebugRestartForSpawnGameOver(int32 SpawnCount)
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugRestartForSpawnGameOver(SpawnCount);
	}
}

void ACodexLSPlayerController::CodexDebugRestartForSpawnFailures(int32 Count)
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugRestartForSpawnFailures(Count);
	}
}

void ACodexLSPlayerController::CodexDebugWaveClearThenKillPlayer()
{
	if (ACodexLSGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACodexLSGameMode>()
		: nullptr)
	{
		GameMode->DebugWaveClearThenKillPlayer();
	}
}

void ACodexLSPlayerController::PressDebugKey(const FKey& Key)
{
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f));
	DebugHeldKeys.AddUnique(Key);
}

void ACodexLSPlayerController::ReleaseDebugKeys()
{
	for (const FKey& Key : DebugHeldKeys)
	{
		InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0f));
	}
	DebugHeldKeys.Reset();
}

void ACodexLSPlayerController::PressDebugDash()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1.0f));
}

void ACodexLSPlayerController::ReleaseDebugDash()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Released, 0.0f));
}

void ACodexLSPlayerController::PressDebugPrimaryAttack()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Pressed, 1.0f));
}

void ACodexLSPlayerController::ReleaseDebugPrimaryAttack()
{
	InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Released, 0.0f));
}

void ACodexLSPlayerController::DebugSoloGrunt()
{
	CodexDebugEnemyScenario(TEXT("SoloGrunt"));
}

void ACodexLSPlayerController::DebugSoloRunner()
{
	CodexDebugEnemyScenario(TEXT("SoloRunner"));
}

void ACodexLSPlayerController::DebugMultiEnemy()
{
	CodexDebugEnemyScenario(TEXT("Multi"));
}

void ACodexLSPlayerController::DebugAttackNearestEnemy()
{
	CodexDebugAttackEnemy(TEXT(""));
}

void ACodexLSPlayerController::DebugSnapshot()
{
	CodexDebugCombatSnapshot();
}

void ACodexLSPlayerController::DebugBoostPlayerHealth()
{
	constexpr float DebugHealth = 1000.0f;
	CodexDebugSetPlayerHealth(DebugHealth);
}

void ACodexLSPlayerController::DebugSetLitView()
{
	ConsoleCommand(TEXT("viewmode lit"), true);
	UE_LOG(LogCodexLastStand, Log, TEXT("STEP2 QA ViewMode | Lit"));
}

void ACodexLSPlayerController::DebugDefeatAllWaveEnemies()
{
	CodexDebugDefeatEnemies(TEXT("All"), -1);
}

void ACodexLSPlayerController::DebugForcePlayerDeath()
{
	CodexDebugKillPlayer();
}

void ACodexLSPlayerController::DebugRestartLevel()
{
	CodexDebugRestartGameLoop();
}
