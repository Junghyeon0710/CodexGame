// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UCodexLSAttributeSet::UCodexLSAttributeSet()
{
}

void UCodexLSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UCodexLSAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCodexLSAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UCodexLSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UCodexLSAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UCodexLSAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = FMath::Max(0.0f, GetIncomingDamage());
		SetIncomingDamage(0.0f);

		if (Damage > 0.0f)
		{
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - Damage, 0.0f, GetMaxHealth()));

			const AActor* TargetActor =
				Data.Target.AbilityActorInfo.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;

			UE_LOG(LogCodexLastStand, Log,
				TEXT("Damage Applied: %.0f | Target=%s | Health: %.0f -> %.0f"),
				Damage, *GetNameSafe(TargetActor), OldHealth, GetHealth());
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}

void UCodexLSAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCodexLSAttributeSet, Health, OldHealth);
}

void UCodexLSAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCodexLSAttributeSet, MaxHealth, OldMaxHealth);
}
