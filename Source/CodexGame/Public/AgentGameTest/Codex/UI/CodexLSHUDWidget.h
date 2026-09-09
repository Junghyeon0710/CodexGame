// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AgentGameTest/Codex/Game/CodexLSGameState.h"
#include "CommonActivatableWidget.h"
#include "GameplayEffectTypes.h"
#include "Input/UIActionBindingHandle.h"
#include "TimerManager.h"
#include "CodexLSHUDWidget.generated.h"

class ACodexLSGameState;
class UAbilitySystemComponent;
class UBorder;
class UCommonTextBlock;
class UProgressBar;

/** Event-driven Gameplay HUD. Only the active GAS dash cooldown uses a short timer. */
UCLASS(Abstract, Blueprintable)
class CODEXGAME_API UCodexLSHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	FString GetDebugSummary() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;

private:
	void TryBindGameplayData();
	void UnbindGameplayData();
	void RefreshHealth();
	void RefreshDashCooldown();
	void SetDashReady();
	void ShowAnnouncement(const FText& Message);
	void HideAnnouncement();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleDashCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void HandleWaveChanged(int32 CurrentWave, int32 MaxWave);

	UFUNCTION()
	void HandleAliveEnemyCountChanged(int32 AliveEnemyCount);

	UFUNCTION()
	void HandleScoreChanged(int32 Score);

	UFUNCTION()
	void HandleGamePhaseChanged(ECodexLSGamePhase PreviousPhase, ECodexLSGamePhase NewPhase);

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HealthText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> WaveText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> EnemyCountText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ScoreText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UProgressBar> DashProgressBar;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DashText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PhaseText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> AnnouncementText;

	UPROPERTY(Transient, meta = (BindWidget))
	TObjectPtr<UBorder> AnnouncementPlate;

	TWeakObjectPtr<ACodexLSGameState> BoundGameState;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle HealthDelegateHandle;
	FDelegateHandle MaxHealthDelegateHandle;
	FDelegateHandle DashTagDelegateHandle;
	FTimerHandle BindingRetryTimerHandle;
	FTimerHandle DashUpdateTimerHandle;
	FTimerHandle AnnouncementTimerHandle;
	int32 BindingAttempts = 0;
	int32 CachedWave = 0;
	int32 CachedMaxWave = 0;
	int32 CachedEnemies = 0;
	int32 CachedScore = 0;
	float CachedHealth = 0.0f;
	float CachedMaxHealth = 0.0f;
	float CachedDashRemaining = 0.0f;
};
