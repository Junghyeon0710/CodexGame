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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Player_Dashing, "State.Player.Dashing", "Applied while the player dash ability is active.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Player_PrimaryAttack, "Cooldown.Player.PrimaryAttack", "Primary attack fire-rate cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Player_Dash, "Cooldown.Player.Dash", "Player dash cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller damage magnitude.");
}
