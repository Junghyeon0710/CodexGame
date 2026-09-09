// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/UI/CodexLSHUDWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/Game/CodexLSGameState.h"
#include "CommonInputModeTypes.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"

namespace
{
	constexpr int32 MaxHUDBindingAttempts = 60;
}

TOptional<FUIInputConfig> UCodexLSHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(
		ECommonInputMode::Game,
		EMouseCaptureMode::NoCapture,
		EMouseLockMode::DoNotLock,
		false);
}

void UCodexLSHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	HealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("HealthBar")));
	HealthText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("HealthText")));
	WaveText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("WaveText")));
	EnemyCountText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("EnemyCountText")));
	ScoreText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("ScoreText")));
	DashProgressBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("DashProgressBar")));
	DashText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("DashText")));
	PhaseText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("PhaseText")));
	AnnouncementText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("AnnouncementText")));
	AnnouncementPlate = Cast<UBorder>(GetWidgetFromName(TEXT("AnnouncementPlate")));

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (AnnouncementText)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (AnnouncementPlate)
	{
		AnnouncementPlate->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetDashReady();

	const bool bHasRequiredWidgets = HealthBar && HealthText && WaveText && EnemyCountText &&
		ScoreText && DashProgressBar && DashText && PhaseText && AnnouncementText &&
		AnnouncementPlate;
	if (bHasRequiredWidgets)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_HUD_WIDGETS_READY Success=true"));
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_HUD_WIDGETS_READY Success=false"));
	}
}

void UCodexLSHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	BindingAttempts = 0;
	TryBindGameplayData();
}

void UCodexLSHUDWidget::NativeOnDeactivated()
{
	UnbindGameplayData();
	Super::NativeOnDeactivated();
}

void UCodexLSHUDWidget::NativeDestruct()
{
	UnbindGameplayData();
	Super::NativeDestruct();
}

void UCodexLSHUDWidget::TryBindGameplayData()
{
	++BindingAttempts;
	APlayerController* Controller = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!World || !Controller)
	{
		return;
	}

	if (!BoundGameState.IsValid())
	{
		if (ACodexLSGameState* GameState = World->GetGameState<ACodexLSGameState>())
		{
			BoundGameState = GameState;
			GameState->OnWaveChanged.AddUniqueDynamic(this, &ThisClass::HandleWaveChanged);
			GameState->OnAliveEnemyCountChanged.AddUniqueDynamic(
				this, &ThisClass::HandleAliveEnemyCountChanged);
			GameState->OnScoreChanged.AddUniqueDynamic(this, &ThisClass::HandleScoreChanged);
			GameState->OnGamePhaseChanged.AddUniqueDynamic(
				this, &ThisClass::HandleGamePhaseChanged);

			HandleWaveChanged(GameState->GetCurrentWave(), GameState->GetMaxWave());
			HandleAliveEnemyCountChanged(GameState->GetAliveEnemyCount());
			HandleScoreChanged(GameState->GetScore());
			HandleGamePhaseChanged(GameState->GetGamePhase(), GameState->GetGamePhase());
		}
	}

	if (!BoundASC.IsValid())
	{
		APawn* Pawn = Controller->GetPawn();
		if (UAbilitySystemComponent* ASC = Pawn
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)
			: nullptr)
		{
			BoundASC = ASC;
			HealthDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
				UCodexLSAttributeSet::GetHealthAttribute()).AddUObject(
					this, &ThisClass::HandleHealthChanged);
			MaxHealthDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
				UCodexLSAttributeSet::GetMaxHealthAttribute()).AddUObject(
					this, &ThisClass::HandleMaxHealthChanged);
			DashTagDelegateHandle = ASC->RegisterAndCallGameplayTagEvent(
				CodexLSGameplayTags::Cooldown_Player_Dash,
				FOnGameplayEffectTagCountChanged::FDelegate::CreateUObject(
					this, &ThisClass::HandleDashCooldownTagChanged),
				EGameplayTagEventType::NewOrRemoved);
			RefreshHealth();
		}
	}

	if (BoundGameState.IsValid() && BoundASC.IsValid())
	{
		World->GetTimerManager().ClearTimer(BindingRetryTimerHandle);
		UE_LOG(LogCodexLastStand, Log,
			TEXT("CODEX_STEP5_HUD_BOUND Attempts=%d Delegates=7 Snapshot=%s"),
			BindingAttempts, *GetDebugSummary());
		return;
	}

	if (BindingAttempts < MaxHUDBindingAttempts)
	{
		World->GetTimerManager().SetTimer(
			BindingRetryTimerHandle,
			this,
			&ThisClass::TryBindGameplayData,
			0.1f,
			false);
	}
	else
	{
		UE_LOG(LogCodexLastStand, Error,
			TEXT("CODEX_STEP5_HUD_BIND_FAILED GameState=%s ASC=%s Attempts=%d"),
			BoundGameState.IsValid() ? TEXT("true") : TEXT("false"),
			BoundASC.IsValid() ? TEXT("true") : TEXT("false"), BindingAttempts);
	}
}

void UCodexLSHUDWidget::UnbindGameplayData()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindingRetryTimerHandle);
		World->GetTimerManager().ClearTimer(DashUpdateTimerHandle);
		World->GetTimerManager().ClearTimer(AnnouncementTimerHandle);
	}

	if (ACodexLSGameState* GameState = BoundGameState.Get())
	{
		GameState->OnWaveChanged.RemoveDynamic(this, &ThisClass::HandleWaveChanged);
		GameState->OnAliveEnemyCountChanged.RemoveDynamic(
			this, &ThisClass::HandleAliveEnemyCountChanged);
		GameState->OnScoreChanged.RemoveDynamic(this, &ThisClass::HandleScoreChanged);
		GameState->OnGamePhaseChanged.RemoveDynamic(this, &ThisClass::HandleGamePhaseChanged);
	}

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		if (HealthDelegateHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
				.Remove(HealthDelegateHandle);
		}
		if (MaxHealthDelegateHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetMaxHealthAttribute())
				.Remove(MaxHealthDelegateHandle);
		}
		if (DashTagDelegateHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(
				DashTagDelegateHandle,
				CodexLSGameplayTags::Cooldown_Player_Dash,
				EGameplayTagEventType::NewOrRemoved);
		}
	}

	HealthDelegateHandle.Reset();
	MaxHealthDelegateHandle.Reset();
	DashTagDelegateHandle.Reset();
	BoundGameState.Reset();
	BoundASC.Reset();
}

void UCodexLSHUDWidget::RefreshHealth()
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}

	CachedHealth = ASC->GetNumericAttribute(UCodexLSAttributeSet::GetHealthAttribute());
	CachedMaxHealth = ASC->GetNumericAttribute(UCodexLSAttributeSet::GetMaxHealthAttribute());
	const float Percent = CachedMaxHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp(CachedHealth / CachedMaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
	}
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(
			TEXT("%.0f / %.0f"),
			FMath::Clamp(CachedHealth, 0.0f, FMath::Max(CachedMaxHealth, 0.0f)),
			FMath::Max(CachedMaxHealth, 0.0f))));
	}
}

void UCodexLSHUDWidget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealth();
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_HUD_HEALTH Old=%.1f New=%.1f Max=%.1f"),
		ChangeData.OldValue, ChangeData.NewValue, CachedMaxHealth);
}

void UCodexLSHUDWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealth();
}

void UCodexLSHUDWidget::HandleWaveChanged(int32 CurrentWave, int32 MaxWave)
{
	CachedWave = FMath::Max(0, CurrentWave);
	CachedMaxWave = FMath::Max(0, MaxWave);
	if (WaveText)
	{
		WaveText->SetText(CachedWave > 0
			? FText::FromString(FString::Printf(TEXT("WAVE %d / %d"), CachedWave, CachedMaxWave))
			: FText::FromString(TEXT("PREPARING")));
	}
	if (CachedWave > 0)
	{
		ShowAnnouncement(FText::FromString(FString::Printf(TEXT("WAVE %d"), CachedWave)));
	}
	UE_LOG(LogCodexLastStand, Log,
		TEXT("CODEX_STEP5_HUD_WAVE Current=%d Max=%d"), CachedWave, CachedMaxWave);
}

void UCodexLSHUDWidget::HandleAliveEnemyCountChanged(int32 AliveEnemyCount)
{
	CachedEnemies = FMath::Max(0, AliveEnemyCount);
	if (EnemyCountText)
	{
		EnemyCountText->SetText(FText::FromString(
			FString::Printf(TEXT("ENEMIES ALIVE  %d"), CachedEnemies)));
	}
}

void UCodexLSHUDWidget::HandleScoreChanged(int32 Score)
{
	CachedScore = FMath::Max(0, Score);
	if (ScoreText)
	{
		ScoreText->SetText(FText::Format(
			FText::FromString(TEXT("SCORE  {0}")), FText::AsNumber(CachedScore)));
	}
	UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP5_HUD_SCORE Value=%d"), CachedScore);
}

void UCodexLSHUDWidget::HandleGamePhaseChanged(
	ECodexLSGamePhase PreviousPhase,
	ECodexLSGamePhase NewPhase)
{
	if (!PhaseText)
	{
		return;
	}

	const UEnum* PhaseEnum = StaticEnum<ECodexLSGamePhase>();
	PhaseText->SetText(FText::FromString(PhaseEnum
		? PhaseEnum->GetNameStringByValue(static_cast<int64>(NewPhase)).ToUpper()
		: TEXT("UNKNOWN")));

	if (NewPhase == ECodexLSGamePhase::Preparing)
	{
		ShowAnnouncement(FText::FromString(TEXT("GET READY")));
	}
	else if (NewPhase == ECodexLSGamePhase::WaveClear)
	{
		ShowAnnouncement(FText::FromString(TEXT("WAVE CLEAR")));
	}
}

void UCodexLSHUDWidget::HandleDashCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DashUpdateTimerHandle);
	if (NewCount > 0)
	{
		RefreshDashCooldown();
		World->GetTimerManager().SetTimer(
			DashUpdateTimerHandle,
			this,
			&ThisClass::RefreshDashCooldown,
			0.05f,
			true);
	}
	else
	{
		SetDashReady();
		UE_LOG(LogCodexLastStand, Log, TEXT("CODEX_STEP5_HUD_DASH READY"));
	}
}

void UCodexLSHUDWidget::RefreshDashCooldown()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		SetDashReady();
		return;
	}

	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(CodexLSGameplayTags::Cooldown_Player_Dash);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	float BestRemaining = 0.0f;
	float BestDuration = 0.0f;
	for (const TPair<float, float>& Time : Times)
	{
		if (Time.Key > BestRemaining)
		{
			BestRemaining = Time.Key;
			BestDuration = Time.Value;
		}
	}

	CachedDashRemaining = FMath::Max(0.0f, BestRemaining);
	if (CachedDashRemaining <= KINDA_SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().ClearTimer(DashUpdateTimerHandle);
		SetDashReady();
		return;
	}

	if (DashText)
	{
		DashText->SetText(FText::FromString(
			FString::Printf(TEXT("DASH  %.1f"), CachedDashRemaining)));
	}
	if (DashProgressBar)
	{
		const float RechargePercent = BestDuration > KINDA_SMALL_NUMBER
			? 1.0f - FMath::Clamp(CachedDashRemaining / BestDuration, 0.0f, 1.0f)
			: 0.0f;
		DashProgressBar->SetPercent(RechargePercent);
	}
}

void UCodexLSHUDWidget::SetDashReady()
{
	CachedDashRemaining = 0.0f;
	if (DashText)
	{
		DashText->SetText(FText::FromString(TEXT("DASH  READY")));
	}
	if (DashProgressBar)
	{
		DashProgressBar->SetPercent(1.0f);
	}
}

void UCodexLSHUDWidget::ShowAnnouncement(const FText& Message)
{
	if (!AnnouncementText || !GetWorld())
	{
		return;
	}
	AnnouncementText->SetText(Message);
	AnnouncementText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (AnnouncementPlate)
	{
		AnnouncementPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	GetWorld()->GetTimerManager().ClearTimer(AnnouncementTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		AnnouncementTimerHandle, this, &ThisClass::HideAnnouncement, 1.25f, false);
}

void UCodexLSHUDWidget::HideAnnouncement()
{
	if (AnnouncementText)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (AnnouncementPlate)
	{
		AnnouncementPlate->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FString UCodexLSHUDWidget::GetDebugSummary() const
{
	return FString::Printf(
		TEXT("HP=%.0f/%.0f Wave=%d/%d Enemies=%d Score=%d Dash=%.1f"),
		CachedHealth, CachedMaxHealth, CachedWave, CachedMaxWave,
		CachedEnemies, CachedScore, CachedDashRemaining);
}
