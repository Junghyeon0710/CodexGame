// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CodexLSGameState.generated.h"

UENUM(BlueprintType)
enum class ECodexLSGamePhase : uint8
{
	None,
	Preparing,
	WaveInProgress,
	WaveClear,
	Victory,
	GameOver
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FCodexLSGamePhaseChangedSignature,
	ECodexLSGamePhase, PreviousPhase,
	ECodexLSGamePhase, NewPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FCodexLSWaveChangedSignature,
	int32, CurrentWave,
	int32, MaxWave);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCodexLSAliveEnemyCountChangedSignature,
	int32, AliveEnemyCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCodexLSScoreChangedSignature,
	int32, Score);

/**
 * Canonical, UI-readable runtime state for the Last Stand game loop.
 * Mutations are change-only and update delegates plus the temporary STEP 3 debug display.
 */
UCLASS()
class CODEXGAME_API ACodexLSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACodexLSGameState();

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	void InitializeRuntimeState(int32 InMaxWave);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetGamePhase(ECodexLSGamePhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetCurrentWave(int32 NewCurrentWave);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetAliveEnemyCount(int32 NewAliveEnemyCount);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetTotalSpawnedEnemyCount(int32 NewTotalSpawnedEnemyCount);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetRemainingSpawnCount(int32 NewRemainingSpawnCount);

	UFUNCTION(BlueprintCallable, Category = "Last Stand|Game Loop")
	bool SetScore(int32 NewScore);

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	ECodexLSGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetMaxWave() const { return MaxWave; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetAliveEnemyCount() const { return AliveEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetTotalSpawnedEnemyCount() const { return TotalSpawnedEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetRemainingSpawnCount() const { return RemainingSpawnCount; }

	UFUNCTION(BlueprintPure, Category = "Last Stand|Game Loop")
	int32 GetScore() const { return Score; }

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Game Loop|Events")
	FCodexLSGamePhaseChangedSignature OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Game Loop|Events")
	FCodexLSWaveChangedSignature OnWaveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Game Loop|Events")
	FCodexLSAliveEnemyCountChangedSignature OnAliveEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Last Stand|Game Loop|Events")
	FCodexLSScoreChangedSignature OnScoreChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdateDebugDisplay() const;
	static FString GetPhaseName(ECodexLSGamePhase Phase);

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	ECodexLSGamePhase GamePhase = ECodexLSGamePhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWave = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 MaxWave = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 AliveEnemyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 TotalSpawnedEnemyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 RemainingSpawnCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Game Loop", meta = (AllowPrivateAccess = "true"))
	int32 Score = 0;
};
