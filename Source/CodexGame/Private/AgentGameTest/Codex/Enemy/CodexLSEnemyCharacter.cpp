// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Enemy/CodexLSEnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AgentGameTest/Codex/AI/CodexLSEnemyAIController.h"
#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/GAS/CodexLSAbilitySystemComponent.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayAbilities.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACodexLSEnemyCharacter::ACodexLSEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	Movement->MaxWalkSpeed = 285.0f;
	Movement->BrakingDecelerationWalking = 1600.0f;
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 180.0f;
	Movement->AvoidanceWeight = 0.5f;

	VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyVisual"));
	VisibleMesh->SetupAttachment(RootComponent);
	VisibleMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisibleMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisibleMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	VisibleMesh->SetGenerateOverlapEvents(false);
	VisibleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -35.0f));
	VisibleMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.3f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMesh.Succeeded())
	{
		VisibleMesh->SetStaticMesh(DefaultMesh.Object);
	}

	AbilitySystemComponent =
		CreateDefaultSubobject<UCodexLSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UCodexLSAttributeSet>(TEXT("AttributeSet"));
	DefaultAttributesEffect = UCodexLSGE_EnemyDefaultAttributes::StaticClass();
	DamageEffect = UCodexLSGE_Damage::StaticClass();
	MeleeAbility = UCodexLSGA_EnemyMeleeAttack::StaticClass();

	AIControllerClass = ACodexLSEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	Tags.Add(TEXT("Codex.Enemy"));
}

void ACodexLSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyDebugColor();
	InitializeAbilitySystem();
}

void ACodexLSEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent
				->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
				.Remove(HealthChangedDelegateHandle);
			HealthChangedDelegateHandle.Reset();
		}

		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->ClearActorInfo();
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ACodexLSEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ACodexLSEnemyCharacter::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.0f;
}

float ACodexLSEnemyCharacter::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.0f;
}

FString ACodexLSEnemyCharacter::GetEnemyArchetypeName() const
{
	return EnemyArchetype == ECodexLSEnemyArchetype::Grunt ? TEXT("Grunt") : TEXT("Runner");
}

bool ACodexLSEnemyCharacter::TryActivateMeleeAbility()
{
	if (bDead || !AbilitySystemComponent || !CombatTarget.IsValid())
	{
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(CodexLSGameplayTags::Ability_Enemy_MeleeAttack);
	const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);

	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy Attack Request | Enemy=%s Type=%s Activated=%s Target=%s"),
		*GetName(), *GetEnemyArchetypeName(), bActivated ? TEXT("true") : TEXT("false"),
		*GetNameSafe(CombatTarget.Get()));

	return bActivated;
}

bool ACodexLSEnemyCharacter::PerformMeleeAttack()
{
	AActor* TargetActor = CombatTarget.Get();
	if (bDead || !TargetActor || !AbilitySystemComponent || !DamageEffect || !GetWorld())
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC || TargetASC->HasMatchingGameplayTag(CodexLSGameplayTags::State_Player_Dead))
	{
		return false;
	}

	FVector AttackDirection = TargetActor->GetActorLocation() - GetActorLocation();
	AttackDirection.Z = 0.0f;
	if (!AttackDirection.Normalize())
	{
		return false;
	}

	SetActorRotation(AttackDirection.Rotation());

	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
	const FVector TraceEnd = TraceStart + AttackDirection * AttackRange;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CodexLSEnemyMeleeAttack), false, this);
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	const bool bAnyHit = GetWorld()->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MeleeTraceRadius),
		QueryParams);

	bool bHitTarget = false;
	if (bAnyHit)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.GetActor() == TargetActor)
			{
				bHitTarget = true;
				break;
			}
		}
	}

	DrawDebugLine(GetWorld(), TraceStart, TraceEnd,
		bHitTarget ? FColor::Red : FColor::Orange, false, 0.35f, 0, 4.0f);
	DrawDebugSphere(GetWorld(), TraceEnd, MeleeTraceRadius, 16,
		bHitTarget ? FColor::Red : FColor::Orange, false, 0.35f, 0, 2.0f);

	if (!bHitTarget)
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("Enemy Melee Miss | Enemy=%s Target=%s"),
			*GetName(), *GetNameSafe(TargetActor));
		return false;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	FGameplayEffectSpecHandle DamageSpec =
		AbilitySystemComponent->MakeOutgoingSpec(DamageEffect, 1.0f, Context);

	if (!DamageSpec.IsValid())
	{
		return false;
	}

	DamageSpec.Data->SetSetByCallerMagnitude(CodexLSGameplayTags::Data_Damage, AttackDamage);
	TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy Melee Hit | Enemy=%s Type=%s Target=%s Damage=%.0f"),
		*GetName(), *GetEnemyArchetypeName(), *GetNameSafe(TargetActor), AttackDamage);
	return true;
}

void ACodexLSEnemyCharacter::SetCombatTarget(AActor* NewTarget)
{
	CombatTarget = NewTarget;
}

void ACodexLSEnemyCharacter::StopEnemyAI()
{
	if (ACodexLSEnemyAIController* EnemyController =
		Cast<ACodexLSEnemyAIController>(GetController()))
	{
		EnemyController->EnterDeadState();
	}
	else if (Controller)
	{
		Controller->StopMovement();
	}
}

void ACodexLSEnemyCharacter::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	HealthChangedDelegateHandle =
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::HandleHealthChanged);

	if (HasAuthority())
	{
		ApplyDefaultAttributes();
		GrantDefaultAbility();
		if (EnemyTypeTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(EnemyTypeTag);
		}
	}

	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy ASC Initialized | Enemy=%s Type=%s Owner=%s Avatar=%s Health=%.0f MaxHealth=%.0f Abilities=%d"),
		*GetName(), *GetEnemyArchetypeName(),
		*GetNameSafe(AbilitySystemComponent->GetOwnerActor()),
		*GetNameSafe(AbilitySystemComponent->GetAvatarActor()),
		GetHealth(), GetMaxHealth(), AbilitySystemComponent->GetActivatableAbilities().Num());
}

void ACodexLSEnemyCharacter::ApplyDefaultAttributes()
{
	if (bAttributesApplied || !DefaultAttributesEffect || !AbilitySystemComponent)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	FGameplayEffectSpecHandle Spec =
		AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.0f, Context);

	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(CodexLSGameplayTags::Data_MaxHealth, InitialMaxHealth);
		Spec.Data->SetSetByCallerMagnitude(CodexLSGameplayTags::Data_Health, InitialMaxHealth);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		bAttributesApplied = true;
	}
}

void ACodexLSEnemyCharacter::GrantDefaultAbility()
{
	if (!MeleeAbility || !AbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayAbilitySpec& ExistingSpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (ExistingSpec.Ability && ExistingSpec.Ability->GetClass() == MeleeAbility)
		{
			return;
		}
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(MeleeAbility, 1, INDEX_NONE, this));
}

void ACodexLSEnemyCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	const float Damage = FMath::Max(0.0f, ChangeData.OldValue - ChangeData.NewValue);
	if (Damage > 0.0f)
	{
		UE_LOG(LogCodexLastStand, Log,
			TEXT("Enemy Damage Received | Enemy=%s Type=%s Damage=%.0f Health: %.0f -> %.0f"),
			*GetName(), *GetEnemyArchetypeName(), Damage, ChangeData.OldValue, ChangeData.NewValue);
	}

	if (!bDead && ChangeData.NewValue <= 0.0f && ChangeData.OldValue > 0.0f)
	{
		EnterDeathState();
	}
}

void ACodexLSEnemyCharacter::EnterDeathState()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	CombatTarget.Reset();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->AddLooseGameplayTag(CodexLSGameplayTags::State_Enemy_Dead);
	}

	StopEnemyAI();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnEnemyDeath.Broadcast(this);
	UE_LOG(LogCodexLastStand, Log,
		TEXT("Enemy Dead | Enemy=%s Type=%s DeadTag=%s DeathEvent=BroadcastOnce DestroyDelay=%.1f"),
		*GetName(), *GetEnemyArchetypeName(),
		AbilitySystemComponent &&
			AbilitySystemComponent->HasMatchingGameplayTag(CodexLSGameplayTags::State_Enemy_Dead)
				? TEXT("present") : TEXT("missing"),
		DestroyDelay);

	SetLifeSpan(DestroyDelay);
}

void ACodexLSEnemyCharacter::ApplyDebugColor()
{
	if (UMaterialInstanceDynamic* Material = VisibleMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), EnemyColor);
	}
}

ACodexLSEnemyGrunt::ACodexLSEnemyGrunt()
{
	EnemyArchetype = ECodexLSEnemyArchetype::Grunt;
	EnemyTypeTag = CodexLSGameplayTags::Enemy_Type_Grunt;
	InitialMaxHealth = 100.0f;
	AttackDamage = 18.0f;
	AttackCooldown = 1.5f;
	AttackRange = 165.0f;
	MeleeTraceRadius = 70.0f;
	EnemyColor = FLinearColor(0.35f, 0.03f, 0.03f, 1.0f);
	GetCharacterMovement()->MaxWalkSpeed = 285.0f;
	VisibleMesh->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.35f));
}

ACodexLSEnemyRunner::ACodexLSEnemyRunner()
{
	EnemyArchetype = ECodexLSEnemyArchetype::Runner;
	EnemyTypeTag = CodexLSGameplayTags::Enemy_Type_Runner;
	InitialMaxHealth = 60.0f;
	AttackDamage = 10.0f;
	AttackCooldown = 0.9f;
	AttackRange = 145.0f;
	MeleeTraceRadius = 60.0f;
	EnemyColor = FLinearColor(1.0f, 0.22f, 0.02f, 1.0f);
	GetCharacterMovement()->MaxWalkSpeed = 520.0f;
	VisibleMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.9f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RunnerMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (RunnerMesh.Succeeded())
	{
		VisibleMesh->SetStaticMesh(RunnerMesh.Object);
	}
}
