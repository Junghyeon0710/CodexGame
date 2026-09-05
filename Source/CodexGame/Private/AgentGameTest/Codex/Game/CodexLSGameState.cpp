// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Game/CodexLSGameState.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "Engine/Engine.h"

namespace
{
	constexpr uint64 CodexStep3DebugMessageKey = 0xC0DE5003ULL;
}

ACodexLSGameState::ACodexLSGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACodexLSGameState::BeginPlay()
{
	Super::BeginPlay();
	UpdateDebugDisplay();
}

void ACodexLSGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(CodexStep3DebugMessageKey);
	}

	Super::EndPlay(EndPlayReason);
}

void ACodexLSGameState::InitializeRuntimeState(int32 InMaxWave)
{
	const int32 SanitizedMaxWave = FMath::Max(0, InMaxWave);
	const bool bWaveChanged = CurrentWave != 0 || MaxWave != SanitizedMaxWave;

	CurrentWave = 0;
	MaxWave = SanitizedMaxWave;
	if (bWaveChanged)
	{
		OnWaveChanged.Broadcast(CurrentWave, MaxWave);
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_WAVE_CHANGED | Current=%d Max=%d"), CurrentWave, MaxWave);
	}

	SetAliveEnemyCount(0);
	SetTotalSpawnedEnemyCount(0);
	SetRemainingSpawnCount(0);
	SetScore(0);
	SetGamePhase(ECodexLSGamePhase::None);
	UpdateDebugDisplay();

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_RUNTIME_INITIALIZED | CurrentWave=%d MaxWave=%d Alive=%d TotalSpawned=%d Remaining=%d Score=%d Phase=%s"),
		CurrentWave, MaxWave, AliveEnemyCount, TotalSpawnedEnemyCount,
		RemainingSpawnCount, Score, *GetPhaseName(GamePhase));
}

bool ACodexLSGameState::SetGamePhase(ECodexLSGamePhase NewPhase)
{
	if (GamePhase == NewPhase)
	{
		return false;
	}

	const ECodexLSGamePhase PreviousPhase = GamePhase;
	GamePhase = NewPhase;
	OnGamePhaseChanged.Broadcast(PreviousPhase, GamePhase);
	UpdateDebugDisplay();

	UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP3_EVENT_PHASE_CHANGED | From=%s To=%s"),
		*GetPhaseName(PreviousPhase), *GetPhaseName(GamePhase));
	return true;
}

bool ACodexLSGameState::SetCurrentWave(int32 NewCurrentWave)
{
	const int32 SanitizedCurrentWave = FMath::Clamp(NewCurrentWave, 0, MaxWave);
	if (CurrentWave == SanitizedCurrentWave)
	{
		return false;
	}

	if (SanitizedCurrentWave != NewCurrentWave)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_WAVE_CLAMPED | Requested=%d Clamped=%d Max=%d"),
			NewCurrentWave, SanitizedCurrentWave, MaxWave);
	}

	CurrentWave = SanitizedCurrentWave;
	OnWaveChanged.Broadcast(CurrentWave, MaxWave);
	UpdateDebugDisplay();

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_WAVE_CHANGED | Current=%d Max=%d"), CurrentWave, MaxWave);
	return true;
}

bool ACodexLSGameState::SetAliveEnemyCount(int32 NewAliveEnemyCount)
{
	const int32 SanitizedCount = FMath::Max(0, NewAliveEnemyCount);
	if (AliveEnemyCount == SanitizedCount)
	{
		return false;
	}

	if (SanitizedCount != NewAliveEnemyCount)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_ALIVE_CLAMPED | Requested=%d Clamped=0"), NewAliveEnemyCount);
	}

	AliveEnemyCount = SanitizedCount;
	OnAliveEnemyCountChanged.Broadcast(AliveEnemyCount);
	UpdateDebugDisplay();

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_ALIVE_CHANGED | Alive=%d"), AliveEnemyCount);
	return true;
}

bool ACodexLSGameState::SetTotalSpawnedEnemyCount(int32 NewTotalSpawnedEnemyCount)
{
	const int32 SanitizedCount = FMath::Max(0, NewTotalSpawnedEnemyCount);
	if (TotalSpawnedEnemyCount == SanitizedCount)
	{
		return false;
	}

	TotalSpawnedEnemyCount = SanitizedCount;
	UpdateDebugDisplay();
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_TOTAL_SPAWNED_CHANGED | TotalSpawned=%d"), TotalSpawnedEnemyCount);
	return true;
}

bool ACodexLSGameState::SetRemainingSpawnCount(int32 NewRemainingSpawnCount)
{
	const int32 SanitizedCount = FMath::Max(0, NewRemainingSpawnCount);
	if (RemainingSpawnCount == SanitizedCount)
	{
		return false;
	}

	RemainingSpawnCount = SanitizedCount;
	UpdateDebugDisplay();
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_REMAINING_CHANGED | Remaining=%d"), RemainingSpawnCount);
	return true;
}

bool ACodexLSGameState::SetScore(int32 NewScore)
{
	const int32 SanitizedScore = FMath::Max(0, NewScore);
	if (Score == SanitizedScore)
	{
		return false;
	}

	Score = SanitizedScore;
	OnScoreChanged.Broadcast(Score);
	UpdateDebugDisplay();

	UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP3_SCORE_CHANGED | Score=%d"), Score);
	return true;
}

void ACodexLSGameState::UpdateDebugDisplay() const
{
	if (!GEngine || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	const FString DebugText = FString::Printf(
		TEXT("PROJECT: LAST STAND [STEP 3]\nWave: %d / %d\nEnemies: %d\nScore: %d\nState: %s"),
		CurrentWave, MaxWave, AliveEnemyCount, Score, *GetPhaseName(GamePhase));

	GEngine->AddOnScreenDebugMessage(
		CodexStep3DebugMessageKey,
		TNumericLimits<float>::Max(),
		FColor::Cyan,
		DebugText);
}

FString ACodexLSGameState::GetPhaseName(ECodexLSGamePhase Phase)
{
	const UEnum* PhaseEnum = StaticEnum<ECodexLSGamePhase>();
	return PhaseEnum
		? PhaseEnum->GetNameStringByValue(static_cast<int64>(Phase))
		: TEXT("Unknown");
}
