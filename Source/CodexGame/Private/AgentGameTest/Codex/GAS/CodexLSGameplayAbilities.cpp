// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/GAS/CodexLSGameplayAbilities.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"
#include "AgentGameTest/Codex/Player/CodexLSPlayerCharacter.h"
#include "Engine/World.h"

UCodexLSGameplayAbility::UCodexLSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

UCodexLSGA_PrimaryAttack::UCodexLSGA_PrimaryAttack()
{
	InputTag = CodexLSGameplayTags::InputTag_Ability_PrimaryAttack;
	CooldownGameplayEffectClass = UCodexLSGE_PrimaryAttackCooldown::StaticClass();
	DamageEffectClass = UCodexLSGE_Damage::StaticClass();

	FGameplayTagContainer Tags;
	Tags.AddTag(CodexLSGameplayTags::Ability_Player_PrimaryAttack);
	SetAssetTags(Tags);
	ActivationBlockedTags.AddTag(CodexLSGameplayTags::State_Player_Dead);
}

void UCodexLSGA_PrimaryAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACodexLSPlayerCharacter* Character = Cast<ACodexLSPlayerCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	if (!Character || !SourceASC || !DamageEffectClass)
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("PrimaryAttack failed: invalid Character, ASC, or DamageEffect."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector AimDirection = Character->GetAimDirection();
	UE_LOG(LogCodexLastStand, Log, TEXT("PrimaryAttack Activated | Aim=(%.2f, %.2f, %.2f)"),
		AimDirection.X, AimDirection.Y, AimDirection.Z);

	FHitResult HitResult;
	if (Character->TracePrimaryAttack(AttackRange, HitResult))
	{
		AActor* HitActor = HitResult.GetActor();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

		if (TargetASC)
		{
			FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
			EffectContext.AddSourceObject(Character);

			FGameplayEffectSpecHandle DamageSpec =
				SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);

			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(CodexLSGameplayTags::Data_Damage, Damage);
				TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

				UE_LOG(LogCodexLastStand, Log, TEXT("PrimaryAttack Hit: %s | Damage Applied: %.0f"),
					*GetNameSafe(HitActor), Damage);
			}
		}
		else
		{
			UE_LOG(LogCodexLastStand, Warning, TEXT("PrimaryAttack Hit non-GAS actor: %s"),
				*GetNameSafe(HitActor));
		}
	}
	else
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("PrimaryAttack Miss"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UCodexLSGA_Dash::UCodexLSGA_Dash()
{
	InputTag = CodexLSGameplayTags::InputTag_Ability_Dash;
	CooldownGameplayEffectClass = UCodexLSGE_DashCooldown::StaticClass();
	ActivationOwnedTags.AddTag(CodexLSGameplayTags::State_Player_Dashing);

	FGameplayTagContainer Tags;
	Tags.AddTag(CodexLSGameplayTags::Ability_Player_Dash);
	SetAssetTags(Tags);
	ActivationBlockedTags.AddTag(CodexLSGameplayTags::State_Player_Dead);
}

void UCodexLSGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACodexLSPlayerCharacter* Character = Cast<ACodexLSPlayerCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!Character || !ASC)
	{
		UE_LOG(LogCodexLastStand, Error, TEXT("Dash failed: invalid Character or ASC."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDirection = Character->PerformDash(DashSpeed);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("Dash Activated | Direction=(%.2f, %.2f, %.2f) | StateTag=%s | Cooldown Started=3.0"),
		DashDirection.X, DashDirection.Y, DashDirection.Z,
		ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dashing) ? TEXT("present") : TEXT("missing"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DashEndTimer, this, &ThisClass::FinishDash, DashDuration, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UCodexLSGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashEndTimer);
	}

	if (ActorInfo)
	{
		if (ACodexLSPlayerCharacter* Character =
			Cast<ACodexLSPlayerCharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->StopDashMovement();
		}
	}

	TWeakObjectPtr<UAbilitySystemComponent> ASC =
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	UE_LOG(LogCodexLastStand, Log, TEXT("Dash Ended | Cancelled=%s | StateTag=%s"),
		bWasCancelled ? TEXT("true") : TEXT("false"),
		ASC.IsValid() && ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dashing)
			? TEXT("present") : TEXT("removed"));
}

void UCodexLSGA_Dash::FinishDash()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UCodexLSGA_EnemyMeleeAttack::UCodexLSGA_EnemyMeleeAttack()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CooldownGameplayEffectClass = UCodexLSGE_EnemyMeleeCooldown::StaticClass();
	ActivationOwnedTags.AddTag(CodexLSGameplayTags::State_Enemy_Attacking);
	ActivationBlockedTags.AddTag(CodexLSGameplayTags::State_Enemy_Dead);

	FGameplayTagContainer Tags;
	Tags.AddTag(CodexLSGameplayTags::Ability_Enemy_MeleeAttack);
	SetAssetTags(Tags);
}

void UCodexLSGA_EnemyMeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACodexLSEnemyCharacter* Enemy =
		ActorInfo ? Cast<ACodexLSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Enemy || Enemy->IsDead() || !Enemy->GetCombatTarget() ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy MeleeAttack Activated | Enemy=%s Type=%s Damage=%.0f Cooldown=%.1f Target=%s"),
		*Enemy->GetName(), *Enemy->GetEnemyArchetypeName(), Enemy->GetAttackDamage(),
		Enemy->GetAttackCooldown(), *GetNameSafe(Enemy->GetCombatTarget()));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AttackTimer, this, &ThisClass::ResolveMeleeAttack, AttackWindup, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UCodexLSGA_EnemyMeleeAttack::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const ACodexLSEnemyCharacter* Enemy =
		ActorInfo ? Cast<ACodexLSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Enemy || !CooldownGameplayEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, CooldownGameplayEffectClass,
		GetAbilityLevel(Handle, ActorInfo));
	if (CooldownSpec.IsValid())
	{
		CooldownSpec.Data->SetSetByCallerMagnitude(
			CodexLSGameplayTags::Data_Cooldown, Enemy->GetAttackCooldown());
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
	}
}

void UCodexLSGA_EnemyMeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTimer);
	}

	const TWeakObjectPtr<UAbilitySystemComponent> ASC =
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const TWeakObjectPtr<AActor> Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy MeleeAttack Ended | Enemy=%s Cancelled=%s AttackingTag=%s"),
		*GetNameSafe(Avatar.Get()), bWasCancelled ? TEXT("true") : TEXT("false"),
		ASC.IsValid() && ASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Enemy_Attacking)
			? TEXT("present") : TEXT("removed"));
}

void UCodexLSGA_EnemyMeleeAttack::ResolveMeleeAttack()
{
	ACodexLSEnemyCharacter* Enemy =
		CurrentActorInfo ? Cast<ACodexLSEnemyCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	if (!Enemy || Enemy->IsDead())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Enemy->PerformMeleeAttack();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
