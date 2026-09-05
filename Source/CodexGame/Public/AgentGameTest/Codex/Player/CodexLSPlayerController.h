// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "CodexLSPlayerController.generated.h"

UCLASS()
class CODEXGAME_API ACodexLSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACodexLSPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION(Exec)
	void CodexDebugInputChord(FString Chord, bool bDash = false, float HoldSeconds = 0.35f);

	UFUNCTION(Exec)
	void CodexDebugSetMouse(int32 ScreenX, int32 ScreenY);

	UFUNCTION(Exec)
	void CodexDebugEnemyScenario(FString Scenario);

	UFUNCTION(Exec)
	void CodexDebugAttackEnemy(FString NameContains);

	UFUNCTION(Exec)
	void CodexDebugCombatSnapshot();

	UFUNCTION(Exec)
	void CodexDebugGameLoopSnapshot();

	UFUNCTION(Exec)
	void CodexDebugDefeatEnemies(FString Filter, int32 Count);

	UFUNCTION(Exec)
	void CodexDebugKillPlayer();

	UFUNCTION(Exec)
	void CodexDebugTerminalRace(FString Order);

	UFUNCTION(Exec)
	void CodexDebugForceSpawnFailures(int32 Count);

	UFUNCTION(Exec)
	void CodexDebugRestartGameLoop();

	UFUNCTION(Exec)
	void CodexDebugSetPlayerHealth(float Health);

	UFUNCTION(Exec)
	void CodexDebugRestartWithHealth(float Health);

	UFUNCTION(Exec)
	void CodexDebugRestartForSpawnGameOver(int32 SpawnCount);

	UFUNCTION(Exec)
	void CodexDebugRestartForSpawnFailures(int32 Count);

	UFUNCTION(Exec)
	void CodexDebugWaveClearThenKillPlayer();

	void PressDebugKey(const FKey& Key);
	void ReleaseDebugKeys();
	void PressDebugDash();
	void ReleaseDebugDash();
	void PressDebugPrimaryAttack();
	void ReleaseDebugPrimaryAttack();
	void DebugSoloGrunt();
	void DebugSoloRunner();
	void DebugMultiEnemy();
	void DebugAttackNearestEnemy();
	void DebugSnapshot();
	void DebugBoostPlayerHealth();
	void DebugSetLitView();
	void DebugDefeatAllWaveEnemies();
	void DebugForcePlayerDeath();
	void DebugRestartLevel();

	TArray<FKey> DebugHeldKeys;
	FTimerHandle DebugReleaseTimer;
	FTimerHandle DebugDashPressTimer;
	FTimerHandle DebugDashReleaseTimer;
	FTimerHandle DebugAttackPressTimer;
	FTimerHandle DebugAttackReleaseTimer;
};
