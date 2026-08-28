// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/Test/CodexLSGASTestTarget.h"

#include "AgentGameTest/Codex/CodexLSLog.h"
#include "AgentGameTest/Codex/GAS/CodexLSAbilitySystemComponent.h"
#include "AgentGameTest/Codex/GAS/CodexLSAttributeSet.h"
#include "AgentGameTest/Codex/GAS/CodexLSGameplayEffects.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACodexLSGASTestTarget::ACodexLSGASTestTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisibleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleMesh"));
	VisibleMesh->SetupAttachment(SceneRoot);
	VisibleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	VisibleMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TargetMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (TargetMesh.Succeeded())
	{
		VisibleMesh->SetStaticMesh(TargetMesh.Object);
	}

	AbilitySystemComponent =
		CreateDefaultSubobject<UCodexLSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UCodexLSAttributeSet>(TEXT("AttributeSet"));
	DefaultAttributesEffect = UCodexLSGE_DefaultAttributes::StaticClass();

	Tags.Add(TEXT("Codex.GAS.TestTarget"));
}

UAbilitySystemComponent* ACodexLSGASTestTarget::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACodexLSGASTestTarget::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	HealthChangedDelegateHandle =
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(UCodexLSAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::HandleHealthChanged);

	if (HasAuthority() && DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec =
			AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.0f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	UE_LOG(LogCodexLastStand, Log, TEXT("GAS Test Target Ready: %s | Health=%.0f MaxHealth=%.0f"),
		*GetName(), AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
}

void ACodexLSGASTestTarget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	const float Damage = FMath::Max(0.0f, ChangeData.OldValue - ChangeData.NewValue);
	if (Damage > 0.0f)
	{
		UE_LOG(LogCodexLastStand, Log, TEXT("TestTarget Damage Received: %.0f"), Damage);
	}

	UE_LOG(LogCodexLastStand, Log, TEXT("TestTarget Health: %.0f -> %.0f | Target=%s"),
		ChangeData.OldValue, ChangeData.NewValue, *GetName());
}
