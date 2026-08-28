// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CodexLSAttributeSet.generated.h"

#define CODEXLS_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class CODEXGAME_API UCodexLSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCodexLSAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Last Stand|Attributes")
	FGameplayAttributeData Health;
	CODEXLS_ATTRIBUTE_ACCESSORS(UCodexLSAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Last Stand|Attributes")
	FGameplayAttributeData MaxHealth;
	CODEXLS_ATTRIBUTE_ACCESSORS(UCodexLSAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Last Stand|Attributes")
	FGameplayAttributeData IncomingDamage;
	CODEXLS_ATTRIBUTE_ACCESSORS(UCodexLSAttributeSet, IncomingDamage);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};

#undef CODEXLS_ATTRIBUTE_ACCESSORS
