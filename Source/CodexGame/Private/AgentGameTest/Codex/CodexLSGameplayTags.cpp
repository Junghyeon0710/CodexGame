// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentGameTest/Codex/CodexLSGameplayTags.h"
#include "AgentGameTest/Codex/CodexLSLog.h"

DEFINE_LOG_CATEGORY(LogCodexLastStand);

namespace CodexLSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability_PrimaryAttack, "InputTag.Ability.PrimaryAttack", "Enhanced Input tag for the primary attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability_Dash, "InputTag.Ability.Dash", "Enhanced Input tag for the dash ability.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_PrimaryAttack, "Ability.Player.PrimaryAttack", "Player primary attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Dash, "Ability.Player.Dash", "Player dash ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_MeleeAttack, "Ability.Enemy.MeleeAttack", "Shared enemy melee attack ability.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Player_Dashing, "State.Player.Dashing", "Applied while the player dash ability is active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Player_Dead, "State.Player.Dead", "Applied when player health reaches zero.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Enemy_Attacking, "State.Enemy.Attacking", "Applied during an enemy melee attack wind-up.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Enemy_Dead, "State.Enemy.Dead", "Applied after an enemy reaches zero health.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Player_PrimaryAttack, "Cooldown.Player.PrimaryAttack", "Primary attack fire-rate cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Player_Dash, "Cooldown.Player.Dash", "Player dash cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Enemy_MeleeAttack, "Cooldown.Enemy.MeleeAttack", "Shared enemy melee attack cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Grunt, "Enemy.Type.Grunt", "Identifies the Grunt enemy archetype.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Runner, "Enemy.Type.Runner", "Identifies the Runner enemy archetype.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller damage magnitude.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Health, "Data.Health", "SetByCaller initial health magnitude.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_MaxHealth, "Data.MaxHealth", "SetByCaller maximum health magnitude.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "SetByCaller cooldown duration.");
}
