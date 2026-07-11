#include "BBGameplayTags.h"

namespace BBGameplayTags
{
	// --- Input Tags ---
	UE_DEFINE_GAMEPLAY_TAG(InputTag_LMB, "InputTag.LMB");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_RMB, "InputTag.RMB");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Shift, "InputTag.Shift");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Q, "InputTag.Q");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_R, "InputTag.R");

	// --- Ability Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_LMB, "Ability.Hunter.Ghost.LMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_RMB, "Ability.Hunter.Ghost.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_Shift, "Ability.Hunter.Ghost.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_Q, "Ability.Hunter.Ghost.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_R, "Ability.Hunter.Ghost.R");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Ghost_Passive, "Ability.Hunter.Ghost.Passive");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_LMB, "Ability.Hunter.Eluna.LMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_RMB, "Ability.Hunter.Eluna.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_Shift, "Ability.Hunter.Eluna.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_Q, "Ability.Hunter.Eluna.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_R, "Ability.Hunter.Eluna.R");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_Passive, "Ability.Hunter.Eluna.Passive");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_GroundDash, "Ability.Hunter.Eluna.GroundDash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Eluna_AerialDash, "Ability.Hunter.Eluna.AerialDash");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_LMB, "Ability.Hunter.Kingpin.LMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_RMB, "Ability.Hunter.Kingpin.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_Shift, "Ability.Hunter.Kingpin.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_Q, "Ability.Hunter.Kingpin.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_R, "Ability.Hunter.Kingpin.R");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_Passive, "Ability.Hunter.Kingpin.Passive");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_GroundDash, "Ability.Hunter.Kingpin.GroundDash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Kingpin_AerialDash, "Ability.Hunter.Kingpin.AerialDash");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_LMB, "Ability.Hunter.Hudson.LMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_RMB, "Ability.Hunter.Hudson.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_Shift, "Ability.Hunter.Hudson.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_Q, "Ability.Hunter.Hudson.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_R, "Ability.Hunter.Hudson.R");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_Passive, "Ability.Hunter.Hudson.Passive");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_RMB_Loop, "Ability.Hunter.Hudson.RMB_Loop");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Hunter_Hudson_R_Reel,   "Ability.Hunter.Hudson.R_Reel");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Category_Passive, "Ability.Category.Passive");

	// --- State Tags ---
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(State_Spiked, "State.Spiked");
	UE_DEFINE_GAMEPLAY_TAG(State_Gliding, "State.Gliding");
	UE_DEFINE_GAMEPLAY_TAG(State_Mantling, "State.Mantling");
	UE_DEFINE_GAMEPLAY_TAG(State_Wisp, "State.Wisp");
	UE_DEFINE_GAMEPLAY_TAG(State_Charging, "State.Charging");
	UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_InBrush, "State.InBrush");
	UE_DEFINE_GAMEPLAY_TAG(State_Dazed, "State.Dazed");
	UE_DEFINE_GAMEPLAY_TAG(State_Hooked, "State.Hooked");
	UE_DEFINE_GAMEPLAY_TAG(State_MostWanted, "State.MostWanted");
	UE_DEFINE_GAMEPLAY_TAG(State_TrainCircleImmune, "State.TrainCircleImmune");
	UE_DEFINE_GAMEPLAY_TAG(State_Executing, "State.Executing");
	UE_DEFINE_GAMEPLAY_TAG(State_Vulnerable, "State.Vulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_AntiHeal, "State.AntiHeal");
	UE_DEFINE_GAMEPLAY_TAG(State_Slowed, "State.Slowed");
	UE_DEFINE_GAMEPLAY_TAG(State_Grounded, "State.Grounded");
	UE_DEFINE_GAMEPLAY_TAG(State_Hudson_Firing, "State.Hudson.Firing");
	UE_DEFINE_GAMEPLAY_TAG(State_Hudson_Spinning, "State.Hudson.Spinning");
	UE_DEFINE_GAMEPLAY_TAG(State_Hudson_SpunUp, "State.Hudson.SpunUp");
	UE_DEFINE_GAMEPLAY_TAG(State_Hudson_Hovering, "State.Hudson.Hovering");
	UE_DEFINE_GAMEPLAY_TAG(State_Hudson_Hooked, "State.Hudson.Hooked");

	// --- Input Tags (item powers) ---
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Power1, "InputTag.Power1");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Power2, "InputTag.Power2");

	// --- Effect Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Effect_Damage, "Effect.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Cooldown, "Effect.Cooldown");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Buff_MoveSpeed, "Effect.Buff.MoveSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Effect_StormDamage, "Effect.StormDamage");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Equipment_Weapon, "Effect.Equipment.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Equipment_Helmet, "Effect.Equipment.Helmet");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Equipment_Boots, "Effect.Equipment.Boots");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Equipment_Armor, "Effect.Equipment.Armor");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Power, "Effect.Power");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Consumable_Heal, "Effect.Consumable.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Consumable_Buff, "Effect.Consumable.Buff");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Daze, "Effect.Daze");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Knockup, "Effect.Knockup");
	UE_DEFINE_GAMEPLAY_TAG(Effect_AntiHeal, "Effect.AntiHeal");
	UE_DEFINE_GAMEPLAY_TAG(Effect_StormShift_BulletTrains, "Effect.StormShift.BulletTrains");

	// --- Cooldown Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Ghost_RMB, "Cooldown.Hunter.Ghost.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Ghost_Shift, "Cooldown.Hunter.Ghost.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Ghost_Q, "Cooldown.Hunter.Ghost.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Ghost_R, "Cooldown.Hunter.Ghost.R");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Eluna_Q, "Cooldown.Hunter.Eluna.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Eluna_R, "Cooldown.Hunter.Eluna.R");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Eluna_RMB, "Cooldown.Hunter.Eluna.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Eluna_GroundDash, "Cooldown.Hunter.Eluna.GroundDash");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Eluna_AerialDash, "Cooldown.Hunter.Eluna.AerialDash");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_Q, "Cooldown.Hunter.Kingpin.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_R, "Cooldown.Hunter.Kingpin.R");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_RMB, "Cooldown.Hunter.Kingpin.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_Shift, "Cooldown.Hunter.Kingpin.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_GroundDash, "Cooldown.Hunter.Kingpin.GroundDash");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Kingpin_AerialDash, "Cooldown.Hunter.Kingpin.AerialDash");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Hudson_RMB, "Cooldown.Hunter.Hudson.RMB");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Hudson_Shift, "Cooldown.Hunter.Hudson.Shift");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Hudson_Q, "Cooldown.Hunter.Hudson.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Hunter_Hudson_R, "Cooldown.Hunter.Hudson.R");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Power1, "Cooldown.Power1");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Power2, "Cooldown.Power2");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "Cooldown.Dash");

	// --- GameplayCue Tags ---
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ghost_LMB_Impact, "GameplayCue.Ghost.LMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ghost_Q_Fire, "GameplayCue.Ghost.Q.Fire");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_LMB_Fire, "GameplayCue.Hunter.Hudson.LMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_LMB_Impact, "GameplayCue.Hunter.Hudson.LMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_LMB_Heal, "GameplayCue.Hunter.Hudson.LMB.Heal");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_RMB_Start, "GameplayCue.Hunter.Hudson.RMB.Start");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_RMB_Loop, "GameplayCue.Hunter.Hudson.RMB.Loop");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_RMB_End, "GameplayCue.Hunter.Hudson.RMB.End");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Shift_Start, "GameplayCue.Hunter.Hudson.Shift.Start");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Shift_Loop, "GameplayCue.Hunter.Hudson.Shift.Loop");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Shift_Impact, "GameplayCue.Hunter.Hudson.Shift.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Q_Cast, "GameplayCue.Hunter.Hudson.Q.Cast");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Q_Active, "GameplayCue.Hunter.Hudson.Q.Active");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Q_Tick, "GameplayCue.Hunter.Hudson.Q.Tick");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_R_Fire, "GameplayCue.Hunter.Hudson.R.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_R_Tether, "GameplayCue.Hunter.Hudson.R.Tether");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_R_Impact, "GameplayCue.Hunter.Hudson.R.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_R_Reel, "GameplayCue.Hunter.Hudson.R.Reel");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Hudson_Passive_Pulse, "GameplayCue.Hunter.Hudson.Passive.Pulse");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_LMB_Fire, "GameplayCue.Hunter.Ghost.LMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_RMB_Cast, "GameplayCue.Hunter.Ghost.RMB.Cast");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_RMB_Impact, "GameplayCue.Hunter.Ghost.RMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Shift_Start, "GameplayCue.Hunter.Ghost.Shift.Start");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Shift_Trail, "GameplayCue.Hunter.Ghost.Shift.Trail");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Q_Telegraph, "GameplayCue.Hunter.Ghost.Q.Telegraph");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Q_Fire, "GameplayCue.Hunter.Ghost.Q.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Q_Impact, "GameplayCue.Hunter.Ghost.Q.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_R_Warning, "GameplayCue.Hunter.Ghost.R.Warning");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_R_Active, "GameplayCue.Hunter.Ghost.R.Active");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Ghost_Passive_Pulse, "GameplayCue.Hunter.Ghost.Passive.Pulse");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_LMB_Fire, "GameplayCue.Hunter.Eluna.LMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_LMB_Impact, "GameplayCue.Hunter.Eluna.LMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_RMB_Fire, "GameplayCue.Hunter.Eluna.RMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_RMB_Impact, "GameplayCue.Hunter.Eluna.RMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_Shift_Trail, "GameplayCue.Hunter.Eluna.Shift.Trail");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_Q_Cast, "GameplayCue.Hunter.Eluna.Q.Cast");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_Q_Active, "GameplayCue.Hunter.Eluna.Q.Active");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_R_Beam, "GameplayCue.Hunter.Eluna.R.Beam");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Eluna_Passive_Carry, "GameplayCue.Hunter.Eluna.Passive.Carry");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_LMB_Fire, "GameplayCue.Hunter.Kingpin.LMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_LMB_Impact, "GameplayCue.Hunter.Kingpin.LMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_RMB_Fire, "GameplayCue.Hunter.Kingpin.RMB.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_RMB_Impact, "GameplayCue.Hunter.Kingpin.RMB.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_Shift_Start, "GameplayCue.Hunter.Kingpin.Shift.Start");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_Shift_Trail, "GameplayCue.Hunter.Kingpin.Shift.Trail");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_Q_Telegraph, "GameplayCue.Hunter.Kingpin.Q.Telegraph");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_Q_Impact, "GameplayCue.Hunter.Kingpin.Q.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_R_Start, "GameplayCue.Hunter.Kingpin.R.Start");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_R_Impact, "GameplayCue.Hunter.Kingpin.R.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Hunter_Kingpin_Passive_Pulse, "GameplayCue.Hunter.Kingpin.Passive.Pulse");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_EmergencyPlatform_Preview, "GameplayCue.Power.EmergencyPlatform.Preview");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_EmergencyPlatform_Spawn, "GameplayCue.Power.EmergencyPlatform.Spawn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_GrapplingHook_Fire, "GameplayCue.Power.GrapplingHook.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_GrapplingHook_Tether, "GameplayCue.Power.GrapplingHook.Tether");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_GrapplingHook_Impact, "GameplayCue.Power.GrapplingHook.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_BungeeShot_Fire, "GameplayCue.Power.BungeeShot.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_BungeeShot_Tether, "GameplayCue.Power.BungeeShot.Tether");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_BungeeShot_Release, "GameplayCue.Power.BungeeShot.Release");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_TacticalNuke_Warning, "GameplayCue.Power.TacticalNuke.Warning");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_TacticalNuke_Impact, "GameplayCue.Power.TacticalNuke.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_TacticalNuke_Burn, "GameplayCue.Power.TacticalNuke.Burn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_RegenerativeArmor_Pulse, "GameplayCue.Power.RegenerativeArmor.Pulse");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Power_RegenerativeArmor_Loop, "GameplayCue.Power.RegenerativeArmor.Loop");

	// --- Event Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Event_Kill, "Event.Kill");
	UE_DEFINE_GAMEPLAY_TAG(Event_CCApplied, "Event.CCApplied");
	UE_DEFINE_GAMEPLAY_TAG(Event_DamageTaken, "Event.DamageTaken");

	// --- SetByCaller Tags ---
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_Damage, "SetByCaller.Damage");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_HealAmount, "SetByCaller.HealAmount");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MaxHealth, "SetByCaller.MaxHealth");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MaxShield, "SetByCaller.MaxShield");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_AttackPower, "SetByCaller.AttackPower");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_AbilityPower, "SetByCaller.AbilityPower");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_CritChance, "SetByCaller.CritChance");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_CooldownReduction, "SetByCaller.CooldownReduction");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MoveSpeed, "SetByCaller.MoveSpeed");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_GlideSpeed, "SetByCaller.GlideSpeed");
}
