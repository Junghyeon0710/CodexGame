// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "CodexLSGameMode.generated.h"

class ACodexLSEnemySpawner;
class ACodexLSGameState;
class ACodexLSPlayerCharacter;
class UAbilitySystemComponent;
enum class ECodexLSGamePhase : uint8;

USTRUCT(BlueprintType)
struct FCodexLSWaveData
{
	GENERATED_BODY()

	FCodexLSWaveData() = default;
	FCodexLSWaveData(int32 InGruntCount, int32 InRunnerCount, float InSpawnInterval)
		: GruntCount(InGruntCount)
		, RunnerCount(InRunnerCount)
		, SpawnInterval(InSpawnInterval)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Last Stand|Wave", meta = (ClampMin = "0"))
	int32 GruntCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Last Stand|Wave", meta = (ClampMin = "0"))
	int32 RunnerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Last Stand|Wave", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float SpawnInterval = 0.45f;

	int32 GetTotalCount() const { return GruntCount + RunnerCount; }
};

UCLASS()
class CODEXGAME_API ACodexLSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACodexLSGameMode();

	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	ACodexLSGameState* GetCodexGameState() const { return CachedGameState; }

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	void RestartCurrentLevel();

	void DebugGameLoopSnapshot() const;
	void DebugDefeatActiveEnemies(const FString& Filter, int32 MaxCount);
	void DebugForcePlayerDeath();
	void DebugTerminalRace(bool bPlayerDiesFirst);
	void DebugForceNextSpawnFailures(int32 Count);
	void DebugRestartWithHealth(float Health);
	void DebugRestartForSpawnGameOver(int32 SpawnCount);
	void DebugRestartForSpawnFailures(int32 Count);
	void DebugWaveClearThenKillPlayer();

private:
	void TryInitializeGameLoop();
	void BeginInitialPreparation();
	void StartNextWave();
	void BuildSpawnQueue(const FCodexLSWaveData& WaveData);
	void SpawnNextEnemy();
	void QueueWaveCompletionCheck();
	void ResolveWaveCompletion();
	void EnterVictory();
	void EnterGameOver(const FString& Reason);
	void SetGamePhase(ECodexLSGamePhase NewPhase, const FString& Reason);
	void ClearGameLoopTimers();
	void HaltActiveEnemies();
	void ApplyDebugPlayerHealth(float Health);
	bool ApplyGASDamage(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		float Damage,
		const TCHAR* DebugReason) const;
	int32 GetRemainingSpawnCount() const;
	FString GetPhaseName(ECodexLSGamePhase Phase) const;

	UFUNCTION()
	void HandleEnemyDeath(ACodexLSEnemyCharacter* Enemy);

	UFUNCTION()
	void HandlePlayerDeath(ACodexLSPlayerCharacter* Player);

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Wave")
	TArray<FCodexLSWaveData> WaveDefinitions;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Wave", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float InitialPreparationTime = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Wave", meta = (ClampMin = "0.1", ClampMax = "15.0"))
	float BetweenWaveDelay = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Last Stand|Spawn", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxSpawnRequestRetries = 3;

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSGameState> CachedGameState;

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSEnemySpawner> EnemySpawner;

	UPROPERTY(Transient)
	TObjectPtr<ACodexLSPlayerCharacter> PlayerCharacter;

	UPROPERTY(Transient)
	TSet<TObjectPtr<ACodexLSEnemyCharacter>> ActiveEnemies;

	TArray<ECodexLSEnemyArchetype> SpawnQueue;
	int32 NextSpawnQueueIndex = 0;
	int32 CurrentSpawnRequestRetries = 0;
	int32 InitializationAttempts = 0;
	bool bWaveCompletionCheckQueued = false;
	int32 DebugKillPlayerAfterSpawnCount = 0;
	FString RuntimeSessionId;

	FTimerHandle InitializationTimerHandle;
	FTimerHandle PhaseDelayTimerHandle;
	FTimerHandle SpawnTimerHandle;
	FTimerHandle WaveCompletionTimerHandle;
	FTimerHandle DebugScenarioTimerHandle;
};
