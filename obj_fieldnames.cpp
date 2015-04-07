
#include "stdafx.h"
#include "obj_fieldnames.h"

static const char *objectFieldNames[] = {
	"ObjBegin",
	"Location",
	"OffsetX",
	"OffsetY",
	"ShadowArt2d",
	"BlitFlags",
	"BlitColor",
	"Transparency",
	"ModelScale",
	"LightFlags",
	"LightMaterial",
	"LightColor",
	"LightRadius",
	"LightAngleStart",
	"LightAngleEnd",
	"LightType",
	"LightFacingX",
	"LightFacingY",
	"LightFacingZ",
	"LightOffsetX",
	"LightOffsetY",
	"LightOffsetZ",
	"Flags",
	"SpellFlags",
	"Name",
	"Description",
	"Size",
	"HpPts",
	"HpAdj",
	"HpDamage",
	"Material",
	"ScriptsIdx",
	"SoundEffect",
	"Category",
	"Rotation",
	"SpeedWalk",
	"SpeedRun",
	"BaseMesh",
	"BaseAnim",
	"Radius",
	"RenderHeight3d",
	"Conditions",
	"ConditionArg0",
	"PermanentMods",
	"Initiative",
	"Dispatcher",
	"Subinitiative",
	"SecretdoorFlags",
	"SecretdoorEffectname",
	"SecretdoorDc",
	"PadI7",
	"PadI8",
	"PadI9",
	"PadI0",
	"OffsetZ",
	"RotationPitch",
	"PadF3",
	"PadF4",
	"PadF5",
	"PadF6",
	"PadF7",
	"PadF8",
	"PadF9",
	"PadF0",
	"PadI640",
	"PadI641",
	"PadI642",
	"PadI643",
	"PadI644",
	"LastHitBy",
	"PadObj1",
	"PadObj2",
	"PadObj3",
	"PadObj4",
	"PermanentModData",
	"AttackTypesIdx",
	"AttackBonusIdx",
	"StrategyState",
	"PadIas4",
	"PadI64as0",
	"PadI64as1",
	"PadI64as2",
	"PadI64as3",
	"PadI64as4",
	"PadObjas0",
	"PadObjas1",
	"PadObjas2",
	"ObjEnd",
	"PortalBegin",
	"PortalFlags",
	"PortalLockDc",
	"PortalKeyId",
	"PortalNotifyNpc",
	"PortalPadI1",
	"PortalPadI2",
	"PortalPadI3",
	"PortalPadI4",
	"PortalPadI5",
	"PortalPadObj1",
	"PortalPadIas1",
	"PortalPadI64as1",
	"PortalEnd",
	"ContainerBegin",
	"ContainerFlags",
	"ContainerLockDc",
	"ContainerKeyId",
	"ContainerInventoryNum",
	"ContainerInventoryListIdx",
	"ContainerInventorySource",
	"ContainerNotifyNpc",
	"ContainerPadI1",
	"ContainerPadI2",
	"ContainerPadI3",
	"ContainerPadI4",
	"ContainerPadI5",
	"ContainerPadObj1",
	"ContainerPadObj2",
	"ContainerPadIas1",
	"ContainerPadI64as1",
	"ContainerPadObjas1",
	"ContainerEnd",
	"SceneryBegin",
	"SceneryFlags",
	"SceneryPadObj0",
	"SceneryRespawnDelay",
	"SceneryPadI0",
	"SceneryPadI1",
	"SceneryTeleportTo",
	"SceneryPadI4",
	"SceneryPadI5",
	"SceneryPadObj1",
	"SceneryPadIas1",
	"SceneryPadI64as1",
	"SceneryEnd",
	"ProjectileBegin",
	"ProjectileFlagsCombat",
	"ProjectileFlagsCombatDamage",
	"ProjectileParentWeapon",
	"ProjectileParentAmmo",
	"ProjectilePartSysId",
	"ProjectileAccelerationX",
	"ProjectileAccelerationY",
	"ProjectileAccelerationZ",
	"ProjectilePadI4",
	"ProjectilePadObj1",
	"ProjectilePadObj2",
	"ProjectilePadObj3",
	"ProjectilePadIas1",
	"ProjectilePadI64as1",
	"ProjectilePadObjas1",
	"ProjectileEnd",
	"ItemBegin",
	"ItemFlags",
	"ItemParent",
	"ItemWeight",
	"ItemWorth",
	"ItemInvAid",
	"ItemInvLocation",
	"ItemGroundMesh",
	"ItemGroundAnim",
	"ItemDescriptionUnknown",
	"ItemDescriptionEffects",
	"ItemSpellIdx",
	"ItemSpellIdxFlags",
	"ItemSpellChargesIdx",
	"ItemAiAction",
	"ItemWearFlags",
	"ItemMaterialSlot",
	"ItemQuantity",
	"ItemPadI1",
	"ItemPadI2",
	"ItemPadI3",
	"ItemPadI4",
	"ItemPadI5",
	"ItemPadI6",
	"ItemPadObj1",
	"ItemPadObj2",
	"ItemPadObj3",
	"ItemPadObj4",
	"ItemPadObj5",
	"ItemPadWielderConditionArray",
	"ItemPadWielderArgumentArray",
	"ItemPadI64as1",
	"ItemPadI64as2",
	"ItemPadObjas1",
	"ItemPadObjas2",
	"ItemEnd",
	"WeaponBegin",
	"WeaponFlags",
	"WeaponRange",
	"WeaponAmmoType",
	"WeaponAmmoConsumption",
	"WeaponMissileAid",
	"WeaponCritHitChart",
	"WeaponAttacktype",
	"WeaponDamageDice",
	"WeaponAnimtype",
	"WeaponType",
	"WeaponCritRange",
	"WeaponPadI1",
	"WeaponPadI2",
	"WeaponPadObj1",
	"WeaponPadObj2",
	"WeaponPadObj3",
	"WeaponPadObj4",
	"WeaponPadObj5",
	"WeaponPadIas1",
	"WeaponPadI64as1",
	"WeaponEnd",
	"AmmoBegin",
	"AmmoFlags",
	"AmmoQuantity",
	"AmmoType",
	"AmmoPadI1",
	"AmmoPadI2",
	"AmmoPadObj1",
	"AmmoPadIas1",
	"AmmoPadI64as1",
	"AmmoEnd",
	"ArmorBegin",
	"ArmorFlags",
	"ArmorAcAdj",
	"ArmorMaxDexBonus",
	"ArmorArcaneSpellFailure",
	"ArmorArmorCheckPenalty",
	"ArmorPadI1",
	"ArmorPadIas1",
	"ArmorPadI64as1",
	"ArmorEnd",
	"MoneyBegin",
	"MoneyFlags",
	"MoneyQuantity",
	"MoneyType",
	"MoneyPadI1",
	"MoneyPadI2",
	"MoneyPadI3",
	"MoneyPadI4",
	"MoneyPadI5",
	"MoneyPadIas1",
	"MoneyPadI64as1",
	"MoneyEnd",
	"FoodBegin",
	"FoodFlags",
	"FoodPadI1",
	"FoodPadI2",
	"FoodPadIas1",
	"FoodPadI64as1",
	"FoodEnd",
	"ScrollBegin",
	"ScrollFlags",
	"ScrollPadI1",
	"ScrollPadI2",
	"ScrollPadIas1",
	"ScrollPadI64as1",
	"ScrollEnd",
	"KeyBegin",
	"KeyKeyId",
	"KeyPadI1",
	"KeyPadI2",
	"KeyPadIas1",
	"KeyPadI64as1",
	"KeyEnd",
	"WrittenBegin",
	"WrittenFlags",
	"WrittenSubtype",
	"WrittenTextStartLine",
	"WrittenTextEndLine",
	"WrittenPadI1",
	"WrittenPadI2",
	"WrittenPadIas1",
	"WrittenPadI64as1",
	"WrittenEnd",
	"BagBegin",
	"BagFlags",
	"BagSize",
	"BagEnd",
	"GenericBegin",
	"GenericFlags",
	"GenericUsageBonus",
	"GenericUsageCountRemaining",
	"GenericPadIas1",
	"GenericPadI64as1",
	"GenericEnd",
	"CritterBegin",
	"CritterFlags",
	"CritterFlags2",
	"CritterAbilitiesIdx",
	"CritterLevelIdx",
	"CritterRace",
	"CritterGender",
	"CritterAge",
	"CritterHeight",
	"CritterWeight",
	"CritterExperience",
	"CritterPadI1",
	"CritterAlignment",
	"CritterDeity",
	"CritterDomain1",
	"CritterDomain2",
	"CritterAlignmentChoice",
	"CritterSchoolSpecialization",
	"CritterSpellsKnownIdx",
	"CritterSpellsMemorizedIdx",
	"CritterSpellsCastIdx",
	"CritterFeatIdx",
	"CritterFeatCountIdx",
	"CritterFleeingFrom",
	"CritterPortrait",
	"CritterMoneyIdx",
	"CritterInventoryNum",
	"CritterInventoryListIdx",
	"CritterInventorySource",
	"CritterDescriptionUnknown",
	"CritterFollowerIdx",
	"CritterTeleportDest",
	"CritterTeleportMap",
	"CritterDeathTime",
	"CritterSkillIdx",
	"CritterReach",
	"CritterSubdualDamage",
	"CritterPadI4",
	"CritterPadI5",
	"CritterSequence",
	"CritterHairStyle",
	"CritterStrategy",
	"CritterPadI3",
	"CritterMonsterCategory",
	"CritterPadI642",
	"CritterPadI643",
	"CritterPadI644",
	"CritterPadI645",
	"CritterDamageIdx",
	"CritterAttacksIdx",
	"CritterSeenMaplist",
	"CritterPadI64as2",
	"CritterPadI64as3",
	"CritterPadI64as4",
	"CritterPadI64as5",
	"CritterEnd",
	"PcBegin",
	"PcFlags",
	"PcPadIas0",
	"PcPadI64as0",
	"PcPlayerName",
	"PcGlobalFlags",
	"PcGlobalVariables",
	"PcVoiceIdx",
	"PcRollCount",
	"PcPadI2",
	"PcWeaponslotsIdx",
	"PcPadIas2",
	"PcPadI64as1",
	"PcEnd",
	"NpcBegin",
	"NpcFlags",
	"NpcLeader",
	"NpcAiData",
	"NpcCombatFocus",
	"NpcWhoHitMeLast",
	"NpcWaypointsIdx",
	"NpcWaypointCurrent",
	"NpcStandpointDayInternalDoNotUse",
	"NpcStandpointNightInternalDoNotUse",
	"NpcFaction",
	"NpcRetailPriceMultiplier",
	"NpcSubstituteInventory",
	"NpcReactionBase",
	"NpcChallengeRating",
	"NpcReactionPcIdx",
	"NpcReactionLevelIdx",
	"NpcReactionTimeIdx",
	"NpcGeneratorData",
	"NpcAiListIdx",
	"NpcSaveReflexesBonus",
	"NpcSaveFortitudeBonus",
	"NpcSaveWillpowerBonus",
	"NpcAcBonus",
	"NpcAddMesh",
	"NpcWaypointAnim",
	"NpcPadI3",
	"NpcPadI4",
	"NpcPadI5",
	"NpcAiFlags64",
	"NpcPadI642",
	"NpcPadI643",
	"NpcPadI644",
	"NpcPadI645",
	"NpcHitdiceIdx",
	"NpcAiListTypeIdx",
	"NpcPadIas3",
	"NpcPadIas4",
	"NpcPadIas5",
	"NpcStandpoints",
	"NpcPadI64as2",
	"NpcPadI64as3",
	"NpcPadI64as4",
	"NpcPadI64as5",
	"NpcEnd",
	"TrapBegin",
	"TrapFlags",
	"TrapDifficulty",
	"TrapPadI2",
	"TrapPadIas1",
	"TrapPadI64as1",
	"TrapEnd",
	"TotalNormal",
	"TransientBegin",
	"RenderColor",
	"RenderColors",
	"RenderPalette",
	"RenderScale",
	"RenderAlpha",
	"RenderX",
	"RenderY",
	"RenderWidth",
	"RenderHeight",
	"Palette",
	"Color",
	"Colors",
	"RenderFlags",
	"TempId",
	"LightHandle",
	"OverlayLightHandles",
	"InternalFlags",
	"FindNode",
	"AnimationHandle",
	"GrappleState",
	"TransientEnd",
	"Type",
	"PrototypeHandle"
};

static const uint32_t objectFieldCount = sizeof(objectFieldNames) / sizeof(const char*);

string GetFieldName(int objFieldIdx) {

	if (objFieldIdx < 0 || objFieldIdx >= objectFieldCount) {
		return format("UnknownField{}", objFieldIdx);
	}

	return objectFieldNames[objFieldIdx];

}