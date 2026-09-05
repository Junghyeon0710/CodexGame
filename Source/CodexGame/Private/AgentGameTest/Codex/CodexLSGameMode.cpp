// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/CodexLSGameMode.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Game/CodexLSGameState.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerController.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerState.h"
#include "AgentGameTest/Codex/Spawn/CodexLSEnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 MaxInitializationAttempts = 50;

	enum class ECodexLSPendingQAScenario : uint8
	{
		None,
		HealthBoost,
		SpawnGameOver,
		SpawnFailures
	};

	struct FCodexLSPendingQAScenario
	{
		ECodexLSPendingQAScenario Scenario = ECodexLSPendingQAScenario::None;
		int32 Value = 0;
		float Health = 0.0f;
	};

	FCodexLSPendingQAScenario PendingQAScenario;
}

ACodexLSGameMode::ACodexLSGameMode()
{
	DefaultPawnClass = ACodexLSPlayerCharacter::StaticClass();
	PlayerControllerClass = ACodexLSPlayerController::StaticClass();
	PlayerStateClass = ACodexLSPlayerState::StaticClass();
	GameStateClass = ACodexLSGameState::StaticClass();

	WaveDefinitions.Emplace(5, 0, 0.45f);
	WaveDefinitions.Emplace(7, 3, 0.45f);
	WaveDefinitions.Emplace(10, 6, 0.45f);
}

void ACodexLSGameMode::StartPlay()
{
	Super::StartPlay();

	RuntimeSessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_SESSION_BEGIN Session=%s Waves=%d Preparing=%.1f Transition=%.1f"),
		*RuntimeSessionId, WaveDefinitions.Num(), InitialPreparationTime, BetweenWaveDelay);

	GetWorldTimerManager().SetTimer(
		InitializationTimerHandle,
		this,
		&ThisClass::TryInitializeGameLoop,
		0.1f,
		true,
		0.0f);
}

void ACodexLSGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGameLoopTimers();

	if (PlayerCharacter)
	{
		PlayerCharacter->OnPlayerDeath.RemoveDynamic(this, &ThisClass::HandlePlayerDeath);
	}

	for (ACodexLSEnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy)
		{
			Enemy->OnEnemyDeath.RemoveDynamic(this, &ThisClass::HandleEnemyDeath);
		}
	}
	ActiveEnemies.Reset();

	Super::EndPlay(EndPlayReason);
}

void ACodexLSGameMode::TryInitializeGameLoop()
{
	++InitializationAttempts;

	if (!CachedGameState)
	{
		CachedGameState = GetGameState<ACodexLSGameState>();
	}

	if (!EnemySpawner)
	{
		for (TActorIterator<ACodexLSEnemySpawner> It(GetWorld()); It; ++It)
		{
			EnemySpawner = *It;
			break;
		}
	}

	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ACodexLSPlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (!CachedGameState || !EnemySpawner || !PlayerCharacter)
	{
		if (InitializationAttempts >= MaxInitializationAttempts)
		{
			GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
			UE_LOG(LogCodexLastStand, Error,
				TEXT("CODEX_STEP3_INIT_FAILED Session=%s Attempts=%d GameState=%s Spawner=%s Player=%s"),
				*RuntimeSessionId, InitializationAttempts,
				*GetNameSafe(CachedGameState), *GetNameSafe(EnemySpawner),
				*GetNameSafe(PlayerCharacter));
		}
		return;
	}

	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	PlayerCharacter->OnPlayerDeath.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeath);
	CachedGameState->InitializeRuntimeState(WaveDefinitions.Num());

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_INIT_SUCCESS Session=%s Attempts=%d GameState=%s Spawner=%s Player=%s"),
		*RuntimeSessionId, InitializationAttempts, *CachedGameState->GetName(),
		*EnemySpawner->GetName(), *PlayerCharacter->GetName());

#if !UE_BUILD_SHIPPING
	const FCodexLSPendingQAScenario QAScenario = PendingQAScenario;
	PendingQAScenario = FCodexLSPendingQAScenario();
	if (QAScenario.Health > 0.0f)
	{
		ApplyDebugPlayerHealth(QAScenario.Health);
	}
	if (QAScenario.Scenario == ECodexLSPendingQAScenario::SpawnGameOver)
	{
		DebugKillPlayerAfterSpawnCount = FMath::Max(1, QAScenario.Value);
	}
	else if (QAScenario.Scenario == ECodexLSPendingQAScenario::SpawnFailures)
	{
		EnemySpawner->DebugForceNextSpawnFailures(FMath::Max(0, QAScenario.Value));
	}
	if (QAScenario.Scenario != ECodexLSPendingQAScenario::None)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_QA_RESTART_ARMED Session=%s Scenario=%d Value=%d Health=%.0f"),
			*RuntimeSessionId, static_cast<int32>(QAScenario.Scenario),
			QAScenario.Value, QAScenario.Health);
	}
#endif

	BeginInitialPreparation();
}

void ACodexLSGameMode::BeginInitialPreparation()
{
	if (!CachedGameState || CachedGameState->GetGamePhase() != ECodexLSGamePhase::None)
	{
		return;
	}

	SetGamePhase(ECodexLSGamePhase::Preparing, TEXT("GameStart"));
	GetWorldTimerManager().SetTimer(
		PhaseDelayTimerHandle,
		this,
		&ThisClass::StartNextWave,
		InitialPreparationTime,
		false);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_PREPARING Session=%s Delay=%.1f"),
		*RuntimeSessionId, InitialPreparationTime);
}

void ACodexLSGameMode::StartNextWave()
{
	if (!CachedGameState || !EnemySpawner || !PlayerCharacter)
	{
		return;
	}

	const ECodexLSGamePhase CurrentPhase = CachedGameState->GetGamePhase();
	if (CurrentPhase != ECodexLSGamePhase::Preparing &&
		CurrentPhase != ECodexLSGamePhase::WaveClear)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_WAVE_START_BLOCKED Session=%s Phase=%s"),
			*RuntimeSessionId, *GetPhaseName(CurrentPhase));
		return;
	}

	const int32 NextWave = CachedGameState->GetCurrentWave() + 1;
	if (!WaveDefinitions.IsValidIndex(NextWave - 1))
	{
		EnterVictory();
		return;
	}

	if (!ActiveEnemies.IsEmpty())
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP3_WAVE_START_BLOCKED Session=%s Reason=ActiveEnemies Count=%d"),
			*RuntimeSessionId, ActiveEnemies.Num());
		return;
	}

	const FCodexLSWaveData& Data = WaveDefinitions[NextWave - 1];
	BuildSpawnQueue(Data);
	CachedGameState->SetCurrentWave(NextWave);
	CachedGameState->SetAliveEnemyCount(0);
	CachedGameState->SetRemainingSpawnCount(SpawnQueue.Num());
	SetGamePhase(ECodexLSGamePhase::WaveInProgress, TEXT("WaveStart"));

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_WAVE_START Session=%s Wave=%d MaxWave=%d Grunt=%d Runner=%d Planned=%d Interval=%.2f"),
		*RuntimeSessionId, NextWave, WaveDefinitions.Num(), Data.GruntCount,
		Data.RunnerCount, Data.GetTotalCount(), Data.SpawnInterval);

	GetWorldTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&ThisClass::SpawnNextEnemy,
		Data.SpawnInterval,
		true,
		0.1f);
}

void ACodexLSGameMode::BuildSpawnQueue(const FCodexLSWaveData& WaveData)
{
	SpawnQueue.Reset(WaveData.GetTotalCount());
	NextSpawnQueueIndex = 0;
	CurrentSpawnRequestRetries = 0;

	for (int32 Index = 0; Index < WaveData.GruntCount; ++Index)
	{
		SpawnQueue.Add(ECodexLSEnemyArchetype::Grunt);
	}
	for (int32 Index = 0; Index < WaveData.RunnerCount; ++Index)
	{
		SpawnQueue.Add(ECodexLSEnemyArchetype::Runner);
	}

	FRandomStream ShuffleStream(0xC0D300 + CachedGameState->GetCurrentWave() + 1);
	for (int32 Index = SpawnQueue.Num() - 1; Index > 0; --Index)
	{
		SpawnQueue.Swap(Index, ShuffleStream.RandRange(0, Index));
	}
}

void ACodexLSGameMode::SpawnNextEnemy()
{
	if (!CachedGameState ||
		CachedGameState->GetGamePhase() != ECodexLSGamePhase::WaveInProgress)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	if (!SpawnQueue.IsValidIndex(NextSpawnQueueIndex))
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		CachedGameState->SetRemainingSpawnCount(0);
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_SPAWN_COMPLETE Session=%s Wave=%d Alive=%d TotalSpawned=%d"),
			*RuntimeSessionId, CachedGameState->GetCurrentWave(),
			CachedGameState->GetAliveEnemyCount(),
			CachedGameState->GetTotalSpawnedEnemyCount());
		QueueWaveCompletionCheck();
		return;
	}

	ACodexLSEnemyCharacter* SpawnedEnemy = nullptr;
	FString SpawnPointName;
	FVector SpawnLocation = FVector::ZeroVector;
	float PlayerDistance = 0.0f;
	const ECodexLSEnemyArchetype Archetype = SpawnQueue[NextSpawnQueueIndex];
	const bool bSpawned = EnemySpawner->SpawnEnemy(
		Archetype,
		PlayerCharacter,
		SpawnedEnemy,
		SpawnPointName,
		SpawnLocation,
		PlayerDistance);

	if (!bSpawned || !SpawnedEnemy)
	{
		++CurrentSpawnRequestRetries;
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_SPAWN_RETRY Session=%s Wave=%d QueueIndex=%d Retry=%d/%d Type=%s"),
			*RuntimeSessionId, CachedGameState->GetCurrentWave(), NextSpawnQueueIndex,
			CurrentSpawnRequestRetries, MaxSpawnRequestRetries,
			Archetype == ECodexLSEnemyArchetype::Grunt ? TEXT("Grunt") : TEXT("Runner"));

		if (CurrentSpawnRequestRetries >= MaxSpawnRequestRetries)
		{
			UE_LOG(LogCodexLastStand, Error,
				TEXT("CODEX_STEP3_SPAWN_DROPPED Session=%s Wave=%d QueueIndex=%d Type=%s"),
				*RuntimeSessionId, CachedGameState->GetCurrentWave(), NextSpawnQueueIndex,
				Archetype == ECodexLSEnemyArchetype::Grunt ? TEXT("Grunt") : TEXT("Runner"));
			++NextSpawnQueueIndex;
			CurrentSpawnRequestRetries = 0;
			CachedGameState->SetRemainingSpawnCount(GetRemainingSpawnCount());
		}
		return;
	}

	CurrentSpawnRequestRetries = 0;
	++NextSpawnQueueIndex;
	SpawnedEnemy->OnEnemyDeath.AddUniqueDynamic(this, &ThisClass::HandleEnemyDeath);
	ActiveEnemies.Add(SpawnedEnemy);
	CachedGameState->SetTotalSpawnedEnemyCount(
		CachedGameState->GetTotalSpawnedEnemyCount() + 1);
	CachedGameState->SetAliveEnemyCount(ActiveEnemies.Num());
	CachedGameState->SetRemainingSpawnCount(GetRemainingSpawnCount());

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_SPAWN Session=%s Success=true Wave=%d Type=%s Enemy=%s Point=%s Distance=%.0f Location=(%.0f,%.0f,%.0f) Alive=%d Remaining=%d TotalSpawned=%d"),
		*RuntimeSessionId, CachedGameState->GetCurrentWave(),
		Archetype == ECodexLSEnemyArchetype::Grunt ? TEXT("Grunt") : TEXT("Runner"),
		*SpawnedEnemy->GetName(), *SpawnPointName, PlayerDistance,
		SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z,
		CachedGameState->GetAliveEnemyCount(), CachedGameState->GetRemainingSpawnCount(),
		CachedGameState->GetTotalSpawnedEnemyCount());

#if !UE_BUILD_SHIPPING
	if (DebugKillPlayerAfterSpawnCount > 0 &&
		CachedGameState->GetTotalSpawnedEnemyCount() >= DebugKillPlayerAfterSpawnCount &&
		CachedGameState->GetRemainingSpawnCount() > 0)
	{
		const int32 TriggerCount = DebugKillPlayerAfterSpawnCount;
		DebugKillPlayerAfterSpawnCount = 0;
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_QA_SPAWN_GAME_OVER_TRIGGER Session=%s Spawned=%d Remaining=%d Trigger=%d"),
			*RuntimeSessionId, CachedGameState->GetTotalSpawnedEnemyCount(),
			CachedGameState->GetRemainingSpawnCount(), TriggerCount);
		DebugForcePlayerDeath();
		return;
	}
#endif

	if (CachedGameState->GetRemainingSpawnCount() == 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP3_SPAWN_COMPLETE Session=%s Wave=%d Alive=%d TotalSpawned=%d"),
			*RuntimeSessionId, CachedGameState->GetCurrentWave(),
			CachedGameState->GetAliveEnemyCount(),
			CachedGameState->GetTotalSpawnedEnemyCount());
		QueueWaveCompletionCheck();
	}
}

void ACodexLSGameMode::HandleEnemyDeath(ACodexLSEnemyCharacter* Enemy)
{
	if (!Enemy || !CachedGameState)
	{
		return;
	}

	Enemy->OnEnemyDeath.RemoveDynamic(this, &ThisClass::HandleEnemyDeath);
	const int32 AliveBefore = ActiveEnemies.Num();
	const int32 RemovedCount = ActiveEnemies.Remove(Enemy);
	CachedGameState->SetAliveEnemyCount(ActiveEnemies.Num());

	if (RemovedCount == 0)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_ENEMY_DEATH Session=%s First=false Enemy=%s Alive=%d Score=%d"),
			*RuntimeSessionId, *Enemy->GetName(), ActiveEnemies.Num(), CachedGameState->GetScore());
		return;
	}

	const ECodexLSGamePhase Phase = CachedGameState->GetGamePhase();
	const int32 ScoreBefore = CachedGameState->GetScore();
	int32 Reward = 0;
	if (Phase != ECodexLSGamePhase::GameOver && Phase != ECodexLSGamePhase::Victory)
	{
		Reward = Enemy->GetScoreValue();
		CachedGameState->SetScore(ScoreBefore + Reward);
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_ENEMY_DEATH Session=%s First=true Enemy=%s Type=%s Reward=%d AliveBefore=%d AliveAfter=%d Remaining=%d ScoreBefore=%d ScoreAfter=%d Phase=%s"),
		*RuntimeSessionId, *Enemy->GetName(), *Enemy->GetEnemyArchetypeName(), Reward,
		AliveBefore, ActiveEnemies.Num(), CachedGameState->GetRemainingSpawnCount(),
		ScoreBefore, CachedGameState->GetScore(), *GetPhaseName(Phase));

	if (Phase == ECodexLSGamePhase::WaveInProgress)
	{
		QueueWaveCompletionCheck();
	}
}

void ACodexLSGameMode::HandlePlayerDeath(ACodexLSPlayerCharacter* Player)
{
	if (Player != PlayerCharacter)
	{
		return;
	}

	EnterGameOver(TEXT("PlayerDeathEvent"));
}

void ACodexLSGameMode::QueueWaveCompletionCheck()
{
	if (bWaveCompletionCheckQueued || !CachedGameState ||
		CachedGameState->GetGamePhase() != ECodexLSGamePhase::WaveInProgress ||
		GetRemainingSpawnCount() != 0 || !ActiveEnemies.IsEmpty())
	{
		return;
	}

	bWaveCompletionCheckQueued = true;
	GetWorldTimerManager().SetTimer(
		WaveCompletionTimerHandle,
		this,
		&ThisClass::ResolveWaveCompletion,
		KINDA_SMALL_NUMBER,
		false);
}

void ACodexLSGameMode::ResolveWaveCompletion()
{
	bWaveCompletionCheckQueued = false;
	if (!CachedGameState ||
		CachedGameState->GetGamePhase() != ECodexLSGamePhase::WaveInProgress ||
		GetRemainingSpawnCount() != 0 || !ActiveEnemies.IsEmpty())
	{
		return;
	}

	if (!PlayerCharacter || PlayerCharacter->IsDead())
	{
		EnterGameOver(TEXT("PlayerDeadBeforeWaveResolution"));
		return;
	}

	const int32 ClearedWave = CachedGameState->GetCurrentWave();
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_WAVE_CLEAR Session=%s Wave=%d Alive=0 Remaining=0 Score=%d"),
		*RuntimeSessionId, ClearedWave, CachedGameState->GetScore());

	if (ClearedWave >= CachedGameState->GetMaxWave())
	{
		EnterVictory();
		return;
	}

	SetGamePhase(ECodexLSGamePhase::WaveClear, TEXT("AllEnemiesDefeated"));
	GetWorldTimerManager().SetTimer(
		PhaseDelayTimerHandle,
		this,
		&ThisClass::StartNextWave,
		BetweenWaveDelay,
		false);
}

void ACodexLSGameMode::EnterVictory()
{
	if (!CachedGameState || CachedGameState->GetGamePhase() == ECodexLSGamePhase::GameOver ||
		CachedGameState->GetGamePhase() == ECodexLSGamePhase::Victory)
	{
		return;
	}

	if (CachedGameState->GetCurrentWave() != CachedGameState->GetMaxWave() ||
		GetRemainingSpawnCount() != 0 || !ActiveEnemies.IsEmpty() ||
		!PlayerCharacter || PlayerCharacter->IsDead())
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP3_VICTORY_BLOCKED Session=%s Wave=%d/%d Alive=%d Remaining=%d PlayerDead=%s"),
			*RuntimeSessionId, CachedGameState->GetCurrentWave(), CachedGameState->GetMaxWave(),
			ActiveEnemies.Num(), GetRemainingSpawnCount(),
			PlayerCharacter && PlayerCharacter->IsDead() ? TEXT("true") : TEXT("false"));
		return;
	}

	ClearGameLoopTimers();
	SpawnQueue.Reset();
	NextSpawnQueueIndex = 0;
	CachedGameState->SetRemainingSpawnCount(0);
	PlayerCharacter->SetGameplayInputEnabled(false);
	SetGamePhase(ECodexLSGamePhase::Victory, TEXT("FinalWaveCleared"));

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_VICTORY Session=%s Wave=%d Alive=0 Remaining=0 Score=%d TotalSpawned=%d Wave4Started=false"),
		*RuntimeSessionId, CachedGameState->GetCurrentWave(), CachedGameState->GetScore(),
		CachedGameState->GetTotalSpawnedEnemyCount());
}

void ACodexLSGameMode::EnterGameOver(const FString& Reason)
{
	if (!CachedGameState || CachedGameState->GetGamePhase() == ECodexLSGamePhase::GameOver)
	{
		return;
	}

	const ECodexLSGamePhase PreviousPhase = CachedGameState->GetGamePhase();
	if (PreviousPhase == ECodexLSGamePhase::Victory)
	{
		return;
	}

	const int32 PendingBefore = GetRemainingSpawnCount();
	ClearGameLoopTimers();
	SpawnQueue.Reset();
	NextSpawnQueueIndex = 0;
	CurrentSpawnRequestRetries = 0;
	CachedGameState->SetRemainingSpawnCount(0);
	SetGamePhase(ECodexLSGamePhase::GameOver, Reason);

	if (PlayerCharacter)
	{
		PlayerCharacter->SetGameplayInputEnabled(false);
	}
	HaltActiveEnemies();

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_GAME_OVER Session=%s From=%s Reason=%s Wave=%d Alive=%d PendingCancelled=%d Score=%d SpawnTimer=false TransitionTimer=false GameOverPriority=true"),
		*RuntimeSessionId, *GetPhaseName(PreviousPhase), *Reason,
		CachedGameState->GetCurrentWave(), CachedGameState->GetAliveEnemyCount(), PendingBefore,
		CachedGameState->GetScore());
}

void ACodexLSGameMode::SetGamePhase(ECodexLSGamePhase NewPhase, const FString& Reason)
{
	if (!CachedGameState)
	{
		return;
	}

	const ECodexLSGamePhase OldPhase = CachedGameState->GetGamePhase();
	if (OldPhase == NewPhase)
	{
		return;
	}

	CachedGameState->SetGamePhase(NewPhase);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_PHASE Session=%s From=%s To=%s Reason=%s Wave=%d Alive=%d Remaining=%d Score=%d"),
		*RuntimeSessionId, *GetPhaseName(OldPhase), *GetPhaseName(NewPhase), *Reason,
		CachedGameState->GetCurrentWave(), CachedGameState->GetAliveEnemyCount(),
		CachedGameState->GetRemainingSpawnCount(), CachedGameState->GetScore());
}

void ACodexLSGameMode::ClearGameLoopTimers()
{
	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	GetWorldTimerManager().ClearTimer(PhaseDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveCompletionTimerHandle);
	GetWorldTimerManager().ClearTimer(DebugScenarioTimerHandle);
	bWaveCompletionCheckQueued = false;
}

void ACodexLSGameMode::HaltActiveEnemies()
{
	for (ACodexLSEnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy && !Enemy->IsDead())
		{
			Enemy->StopCombatForGameEnd();
		}
	}
}

bool ACodexLSGameMode::ApplyGASDamage(
	UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC,
	float Damage,
	const TCHAR* DebugReason) const
{
	if (!SourceASC || !TargetASC || Damage <= 0.0f)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(SourceASC->GetAvatarActor());
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
		UCodexLSGE_Damage::StaticClass(), 1.0f, Context);
	if (!Spec.IsValid())
	{
		return false;
	}

	Spec.Data->SetSetByCallerMagnitude(CodexLSGameplayTags::Data_Damage, Damage);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_GAS_DAMAGE Session=%s Reason=%s Source=%s Target=%s Damage=%.0f"),
		*RuntimeSessionId, DebugReason, *GetNameSafe(SourceASC->GetAvatarActor()),
		*GetNameSafe(TargetASC->GetAvatarActor()), Damage);
	return true;
}

void ACodexLSGameMode::DebugDefeatActiveEnemies(const FString& Filter, int32 MaxCount)
{
	UAbilitySystemComponent* PlayerASC = PlayerCharacter
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter)
		: nullptr;
	if (!PlayerASC)
	{
		return;
	}

	FString NormalizedFilter = Filter;
	NormalizedFilter.ToUpperInline();
	TArray<ACodexLSEnemyCharacter*> Snapshot;
	Snapshot.Reserve(ActiveEnemies.Num());
	for (ACodexLSEnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy && !Enemy->IsDead())
		{
			Snapshot.Add(Enemy);
		}
	}

	int32 DefeatedCount = 0;
	for (ACodexLSEnemyCharacter* Enemy : Snapshot)
	{
		if (MaxCount >= 0 && DefeatedCount >= MaxCount)
		{
			break;
		}

		const FString TypeName = Enemy->GetEnemyArchetypeName().ToUpper();
		if (!NormalizedFilter.IsEmpty() && NormalizedFilter != TEXT("ALL") &&
			!TypeName.Contains(NormalizedFilter))
		{
			continue;
		}

		if (ApplyGASDamage(PlayerASC, Enemy->GetAbilitySystemComponent(),
			Enemy->GetHealth() + 1000.0f, TEXT("DefeatActiveEnemy")))
		{
			++DefeatedCount;
		}
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_DEFEAT Session=%s Filter=%s Requested=%d Applied=%d"),
		*RuntimeSessionId, *NormalizedFilter, MaxCount, DefeatedCount);
}

void ACodexLSGameMode::DebugForcePlayerDeath()
{
	if (!PlayerCharacter || PlayerCharacter->IsDead())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter);
	UAbilitySystemComponent* SourceASC = nullptr;
	for (ACodexLSEnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy && !Enemy->IsDead())
		{
			SourceASC = Enemy->GetAbilitySystemComponent();
			break;
		}
	}
	if (!SourceASC)
	{
		SourceASC = TargetASC;
	}

	const float PlayerHealth = TargetASC
		? TargetASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute())
		: 0.0f;
	ApplyGASDamage(SourceASC, TargetASC, PlayerHealth + 1000.0f, TEXT("ForcePlayerDeath"));
}

void ACodexLSGameMode::DebugTerminalRace(bool bPlayerDiesFirst)
{
	if (!CachedGameState ||
		CachedGameState->GetGamePhase() != ECodexLSGamePhase::WaveInProgress ||
		CachedGameState->GetRemainingSpawnCount() != 0 ||
		ActiveEnemies.Num() != 1 || !PlayerCharacter || PlayerCharacter->IsDead())
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_QA_TERMINAL_RACE_SKIPPED Session=%s Phase=%s Alive=%d Remaining=%d PlayerDead=%s"),
			*RuntimeSessionId,
			CachedGameState ? *GetPhaseName(CachedGameState->GetGamePhase()) : TEXT("Missing"),
			ActiveEnemies.Num(), CachedGameState ? CachedGameState->GetRemainingSpawnCount() : -1,
			PlayerCharacter && PlayerCharacter->IsDead() ? TEXT("true") : TEXT("false"));
		return;
	}

	ACodexLSEnemyCharacter* FinalEnemy = nullptr;
	for (ACodexLSEnemyCharacter* Enemy : ActiveEnemies)
	{
		if (Enemy && !Enemy->IsDead())
		{
			FinalEnemy = Enemy;
			break;
		}
	}

	if (!FinalEnemy || !PlayerCharacter)
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_QA_TERMINAL_RACE_SKIPPED Session=%s Enemy=%s Player=%s"),
			*RuntimeSessionId, *GetNameSafe(FinalEnemy), *GetNameSafe(PlayerCharacter));
		return;
	}

	UAbilitySystemComponent* PlayerASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter);
	if (bPlayerDiesFirst)
	{
		DebugForcePlayerDeath();
		ApplyGASDamage(PlayerASC, FinalEnemy->GetAbilitySystemComponent(),
			FinalEnemy->GetHealth() + 1000.0f, TEXT("TerminalRaceEnemyAfterPlayer"));
	}
	else
	{
		ApplyGASDamage(PlayerASC, FinalEnemy->GetAbilitySystemComponent(),
			FinalEnemy->GetHealth() + 1000.0f, TEXT("TerminalRaceEnemyFirst"));
		DebugForcePlayerDeath();
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_TERMINAL_RACE Session=%s Order=%s FinalPhase=%s GameOverPriority=true"),
		*RuntimeSessionId, bPlayerDiesFirst ? TEXT("PlayerFirst") : TEXT("EnemyFirst"),
		CachedGameState ? *GetPhaseName(CachedGameState->GetGamePhase()) : TEXT("Missing"));
}

void ACodexLSGameMode::DebugForceNextSpawnFailures(int32 Count)
{
	if (EnemySpawner)
	{
		EnemySpawner->DebugForceNextSpawnFailures(FMath::Max(0, Count));
	}
}

void ACodexLSGameMode::DebugRestartWithHealth(float Health)
{
#if !UE_BUILD_SHIPPING
	PendingQAScenario.Scenario = ECodexLSPendingQAScenario::HealthBoost;
	PendingQAScenario.Value = 0;
	PendingQAScenario.Health = FMath::Max(1.0f, Health);
	RestartCurrentLevel();
#endif
}

void ACodexLSGameMode::DebugRestartForSpawnGameOver(int32 SpawnCount)
{
#if !UE_BUILD_SHIPPING
	PendingQAScenario.Scenario = ECodexLSPendingQAScenario::SpawnGameOver;
	PendingQAScenario.Value = FMath::Max(1, SpawnCount);
	PendingQAScenario.Health = 100000.0f;
	RestartCurrentLevel();
#endif
}

void ACodexLSGameMode::DebugRestartForSpawnFailures(int32 Count)
{
#if !UE_BUILD_SHIPPING
	PendingQAScenario.Scenario = ECodexLSPendingQAScenario::SpawnFailures;
	PendingQAScenario.Value = FMath::Max(0, Count);
	PendingQAScenario.Health = 100000.0f;
	RestartCurrentLevel();
#endif
}

void ACodexLSGameMode::DebugWaveClearThenKillPlayer()
{
#if !UE_BUILD_SHIPPING
	if (!CachedGameState || CachedGameState->GetGamePhase() != ECodexLSGamePhase::WaveInProgress ||
		CachedGameState->GetRemainingSpawnCount() != 0 || ActiveEnemies.IsEmpty())
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_QA_TRANSITION_GAME_OVER_SKIPPED Session=%s Phase=%s Alive=%d Remaining=%d"),
			*RuntimeSessionId,
			CachedGameState ? *GetPhaseName(CachedGameState->GetGamePhase()) : TEXT("Missing"),
			ActiveEnemies.Num(), CachedGameState ? CachedGameState->GetRemainingSpawnCount() : -1);
		return;
	}

	DebugDefeatActiveEnemies(TEXT("ALL"), -1);
	GetWorldTimerManager().SetTimer(
		DebugScenarioTimerHandle,
		this,
		&ThisClass::DebugForcePlayerDeath,
		0.2f,
		false);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_TRANSITION_GAME_OVER_QUEUED Session=%s Delay=0.2"),
		*RuntimeSessionId);
#endif
}

void ACodexLSGameMode::ApplyDebugPlayerHealth(float Health)
{
#if !UE_BUILD_SHIPPING
	UAbilitySystemComponent* PlayerASC = PlayerCharacter
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter)
		: nullptr;
	if (!PlayerASC || PlayerCharacter->IsDead())
	{
		UE_LOG(LogCodexLastStand, Warning,
			TEXT("CODEX_STEP3_QA_HEALTH_BOOST_SKIPPED Session=%s Health=%.0f Reason=MissingASCOrDead"),
			*RuntimeSessionId, Health);
		return;
	}

	const float SanitizedHealth = FMath::Max(1.0f, Health);
	PlayerASC->SetNumericAttributeBase(UCodexLSAttributeSet::GetMaxHealthAttribute(), SanitizedHealth);
	PlayerASC->SetNumericAttributeBase(UCodexLSAttributeSet::GetHealthAttribute(), SanitizedHealth);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_QA_HEALTH_BOOST Session=%s Health=%.0f"),
		*RuntimeSessionId, SanitizedHealth);
#endif
}

void ACodexLSGameMode::DebugGameLoopSnapshot() const
{
	if (!CachedGameState)
	{
		return;
	}

	const UAbilitySystemComponent* PlayerASC = PlayerCharacter
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter)
		: nullptr;
	const float Health = PlayerASC
		? PlayerASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute())
		: -1.0f;
	const float MaxHealth = PlayerASC
		? PlayerASC->GetNumericAttribute(UCodexLSAttributeSet::GetMaxHealthAttribute())
		: -1.0f;
	const int32 AbilityCount = PlayerASC ? PlayerASC->GetActivatableAbilities().Num() : -1;
	const bool bDeadTag = PlayerASC &&
		PlayerASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dead);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_SNAPSHOT Session=%s Phase=%s Wave=%d/%d Alive=%d Tracked=%d Remaining=%d TotalSpawned=%d Score=%d PlayerHealth=%.0f/%.0f PlayerDead=%s DeadTag=%s Input=%s Abilities=%d PrepareOrTransitionTimer=%s SpawnTimer=%s CompletionTimer=%s"),
		*RuntimeSessionId, *GetPhaseName(CachedGameState->GetGamePhase()),
		CachedGameState->GetCurrentWave(), CachedGameState->GetMaxWave(),
		CachedGameState->GetAliveEnemyCount(), ActiveEnemies.Num(),
		CachedGameState->GetRemainingSpawnCount(), CachedGameState->GetTotalSpawnedEnemyCount(),
		CachedGameState->GetScore(), Health, MaxHealth,
		PlayerCharacter && PlayerCharacter->IsDead() ? TEXT("true") : TEXT("false"),
		bDeadTag ? TEXT("true") : TEXT("false"),
		PlayerCharacter && PlayerCharacter->IsGameplayInputEnabled() ? TEXT("true") : TEXT("false"),
		AbilityCount,
		GetWorldTimerManager().IsTimerActive(PhaseDelayTimerHandle) ? TEXT("true") : TEXT("false"),
		GetWorldTimerManager().IsTimerActive(SpawnTimerHandle) ? TEXT("true") : TEXT("false"),
		GetWorldTimerManager().IsTimerActive(WaveCompletionTimerHandle) ? TEXT("true") : TEXT("false"));
}

void ACodexLSGameMode::RestartCurrentLevel()
{
	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP3_RESTART_REQUEST Session=%s Level=%s Phase=%s"),
		*RuntimeSessionId, *CurrentLevel,
		CachedGameState ? *GetPhaseName(CachedGameState->GetGamePhase()) : TEXT("Missing"));

	ClearGameLoopTimers();
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevel), true);
}

int32 ACodexLSGameMode::GetRemainingSpawnCount() const
{
	return FMath::Max(0, SpawnQueue.Num() - NextSpawnQueueIndex);
}

FString ACodexLSGameMode::GetPhaseName(ECodexLSGamePhase Phase) const
{
	const UEnum* PhaseEnum = StaticEnum<ECodexLSGamePhase>();
	return PhaseEnum ? PhaseEnum->GetNameStringByValue(static_cast<int64>(Phase)) : TEXT("Unknown");
}
