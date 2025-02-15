#include "stdafx.h"
#include "common.h"
#include "ai.h"
#include "d20.h"
#include "combat.h"
#include "obj.h"
#include "action_sequence.h"
#include "spell.h"
#include "temple_functions.h"
#include "pathfinding.h"
#include "objlist.h"

#pragma region AI System Implementation
#include "location.h"
#include "critter.h"
#include "weapon.h"
#include "python/python_integration_obj.h"
#include "python/python_object.h"
#include "util/fixes.h"
#include "party.h"
struct AiSystem aiSys;


const auto TestSizeOfAiTactic = sizeof(AiTactic); // should be 2832 (0xB10 )
const auto TestSizeOfAiStrategy = sizeof(AiStrategy); // should be 808 (0x324)

struct AiSystemAddresses : temple::AddressTable
{
	int (__cdecl*UpdateAiFlags)(objHndl obj, int aiFightStatus, objHndl target, int * soundMap);
	void(__cdecl*AiTurnSthg_1005EEC0)(objHndl obj);
	AiSystemAddresses()
	{
		rebase(UpdateAiFlags, 0x1005DA00);
		rebase(AiTurnSthg_1005EEC0, 0x1005EEC0);
	}

	
}addresses;

AiSystem::AiSystem()
{
	combat = &combatSys;
	d20 = &d20Sys;
	actSeq = &actSeqSys;
	spell = &spellSys;
	pathfinding = &pathfindingSys;
	rebase(aiStrategies,0x11868F3C); 
	rebase(aiStrategiesNum,0x11868F40); 
	rebase(aiTacticDefs,0x102E4398); 
	rebase(_AiRemoveFromList, 0x1005A070);
	rebase(_FleeAdd, 0x1005DE60);
	rebase(_ShitlistAdd, 0x1005CC10);
	rebase(_StopAttacking, 0x1005E6A0);
	rebase(_AiSetCombatStatus, 0x1005DA00);
	RegisterNewAiTactics();
}

void AiSystem::aiTacticGetConfig(int tacIdx, AiTactic* aiTacOut, AiStrategy* aiStrat)
{
	SpellPacketBody * spellPktBody = &aiTacOut->spellPktBody;
	spell->spellPacketBodyReset(&aiTacOut->spellPktBody);
	aiTacOut->aiTac = aiStrat->aiTacDefs[tacIdx];
	aiTacOut->field4 = aiStrat->field54[tacIdx];
	aiTacOut->tacIdx = tacIdx;
	int32_t spellEnum = aiStrat->spellsKnown[tacIdx].spellEnum;
	spellPktBody->spellEnum = spellEnum;
	spellPktBody->spellEnumOriginal = spellEnum;
	if (spellEnum != -1)
	{
		aiTacOut->spellPktBody.objHndCaster = aiTacOut->performer;
		aiTacOut->spellPktBody.casterClassCode = aiStrat->spellsKnown[tacIdx].classCode;
		aiTacOut->spellPktBody.spellKnownSlotLevel = aiStrat->spellsKnown[tacIdx].spellLevel;
		spell->spellPacketSetCasterLevel(spellPktBody);
		d20->D20ActnSetSpellData(&aiTacOut->d20SpellData, spellEnum, spellPktBody->casterClassCode, spellPktBody->spellKnownSlotLevel, 0xFF, spellPktBody->metaMagicData);
	}
}

uint32_t AiSystem::AiStrategyParse(objHndl objHnd, objHndl target)
{
	AiTactic aiTac;
	combat->enterCombat(objHnd);
	AiStrategy* aiStrat = &(*aiStrategies)[objects.getInt32(objHnd, obj_f_critter_strategy)];
	if (!actSeq->TurnBasedStatusInit(objHnd)) return 0;
	
	actSeq->curSeqReset(objHnd);
	d20->GlobD20ActnInit();
	spell->spellPacketBodyReset(&aiTac.spellPktBody);
	aiTac.performer = objHnd;
	aiTac.target = target;


	AiCombatRole role = GetRole(objHnd);
	if (role != AiCombatRole::caster)
	{

		// check if disarmed, if so, try to pick up weapon
		if (d20Sys.d20Query(aiTac.performer, DK_QUE_Disarmed))
		{
			logger->info("AiStrategy: \t {} attempting to pickup weapon...", description.getDisplayName(objHnd));
			if (PickUpWeapon(&aiTac))
			{
				actSeq->sequencePerform();
				return 1;
			}
		}

		// wake up friends put to sleep; will do this if friend is within reach or otherwise at a 40% chance
		if (WakeFriend(&aiTac))
		{
			actSeq->sequencePerform();
			return 1;
		}

	}

	// loop through tactics defined in strategy.tab
	for (uint32_t i = 0; i < aiStrat->numTactics; i++)
	{
		aiTacticGetConfig(i, &aiTac, aiStrat);
		logger->info("AiStrategy: \t {} attempting {}...", description._getDisplayName(objHnd, objHnd), aiTac.aiTac->name);
		auto aiFunc = aiTac.aiTac->aiFunc;
		if (!aiFunc) continue;
		if (aiFunc(&aiTac)) {
			logger->info("AiStrategy: \t AI tactic succeeded; performing.");
			actSeq->sequencePerform();
			return 1;
		}
	}

	// if no tactics defined (e.g. frogs), do target closest first to avoid all kinds of sillyness
	if (aiStrat->numTactics == 0)
	{
		TargetClosest(&aiTac);
	}

	// if none of those work, use default
	aiTac.aiTac = &aiTacticDefs[0];
	aiTac.field4 = 0;
	aiTac.tacIdx = -1;
	logger->info("AiStrategy: \t {} attempting default...", description._getDisplayName(objHnd, objHnd));
	if (aiTac.target)
		logger->info("Target: {}", description.getDisplayName(aiTac.target));
	assert(aiTac.aiTac != nullptr);
	if (Default(&aiTac))
	{
		actSeq->sequencePerform();
		return 1;
	}
	logger->info("AiStrategy: \t Default FAILED. Attempting to find pathable party member as target...");
	objHndl pathablePartyMember = pathfindingSys.CanPathToParty(objHnd);
	if (pathablePartyMember)
	{
		aiTac.target = pathablePartyMember;
		if (aiTac.aiTac->aiFunc(&aiTac))
		{
			logger->info("AiStrategy: \t Default tactic succeeded; performing.");
			actSeq->sequencePerform();
			return 1;
		}
	}

	// if that doesn't work either, try to Break Free (NPC might be held back by Web / Entangle)
	if (d20Sys.d20Query(aiTac.performer, DK_QUE_Is_BreakFree_Possible))
	{
		logger->info("AiStrategy: \t {} attempting to break free...", description.getDisplayName(objHnd));
		if (BreakFree(&aiTac))
		{
			actSeq->sequencePerform();
			return 1;
		}
	}

	return 0;
}

bool AiSystem::HasAiFlag(objHndl npc, AiFlag flag) {
	auto aiFlags = templeFuncs.Obj_Get_Field_64bit(npc, obj_f_npc_ai_flags64);
	auto flagBit = (uint64_t)flag;
	return (aiFlags & flagBit) != 0;
}

void AiSystem::SetAiFlag(objHndl npc, AiFlag flag) {
	auto aiFlags = templeFuncs.Obj_Get_Field_64bit(npc, obj_f_npc_ai_flags64);
	aiFlags |= (uint64_t) flag;
	templeFuncs.Obj_Set_Field_64bit(npc, obj_f_npc_ai_flags64, aiFlags);
}

void AiSystem::ClearAiFlag(objHndl npc, AiFlag flag) {
	auto aiFlags = templeFuncs.Obj_Get_Field_64bit(npc, obj_f_npc_ai_flags64);
	aiFlags &= ~ (uint64_t)flag;
	templeFuncs.Obj_Set_Field_64bit(npc, obj_f_npc_ai_flags64, aiFlags);
}

void AiSystem::ShitlistAdd(objHndl npc, objHndl target) {
	_ShitlistAdd(npc, target);
}

void AiSystem::ShitlistRemove(objHndl npc, objHndl target) {
	_AiRemoveFromList(npc, target, 0); // 0 is the shitlist

	auto focus = GetCombatFocus(npc);
	if (focus == target) {
		SetCombatFocus(npc, 0);
	}
	auto hitMeLast = GetWhoHitMeLast(npc);
	if (hitMeLast == target) {
		SetWhoHitMeLast(npc, 0);
	}

	_AiSetCombatStatus(npc, 0, 0, 0); // I think this means stop attacking?
}

void AiSystem::FleeAdd(objHndl npc, objHndl target) {
	_FleeAdd(npc, target);
}

void AiSystem::StopAttacking(objHndl npc) {
	_StopAttacking(npc);
}

objHndl AiSystem::GetCombatFocus(objHndl npc) {
	return templeFuncs.Obj_Get_Field_ObjHnd__fastout(npc, obj_f_npc_combat_focus);
}

objHndl AiSystem::GetWhoHitMeLast(objHndl npc) {
	return templeFuncs.Obj_Get_Field_ObjHnd__fastout(npc, obj_f_npc_who_hit_me_last);
}

void AiSystem::SetCombatFocus(objHndl npc, objHndl target) {
	templeFuncs.Obj_Set_Field_ObjHnd(npc, obj_f_npc_combat_focus, target);
}

void AiSystem::SetWhoHitMeLast(objHndl npc, objHndl target) {
	templeFuncs.Obj_Set_Field_ObjHnd(npc, obj_f_npc_who_hit_me_last, target);
}

int AiSystem::TargetClosest(AiTactic* aiTac)
{
	objHndl performer = aiTac->performer;
	int performerIsIntelligent = (objects.StatLevelGet(performer, stat_intelligence) >= 3);
	//objHndl target; 
	LocAndOffsets performerLoc; 
	float dist = 1000000000.0, reach = critterSys.GetReach(performer, D20A_UNSPECIFIED_ATTACK);
	bool hasGoodTarget = false;
	
	
	locSys.getLocAndOff(aiTac->performer, &performerLoc);
	/*
	if (combatSys.GetClosestEnemy(aiTac->performer, &performerLoc, &target, &dist, 0x21))
	{
		aiTac->target = target;
		
	}
	*/

	logger->debug("{} targeting closest...", objects.description.getDisplayName(performer));

	// ObjList objlist;
	// objlist.ListVicinity(performerLoc.location, OLC_CRITTERS);

	combatSys.groupInitiativeList->GroupSize;

	auto args = PyTuple_New(2);
	PyTuple_SET_ITEM(args, 0, PyObjHndl_Create(performer));

	for ( uint32_t i = 0; i < combatSys.groupInitiativeList->GroupSize; i++)
	{
		// objHndl dude = objlist.get(i);
		objHndl combatant = combatSys.groupInitiativeList->GroupMembers[i];
		PyTuple_SET_ITEM(args, 1, PyObjHndl_Create(combatant));

		auto result = pythonObjIntegration.ExecuteScript("combat", "ShouldIgnoreTarget", args);
		//auto result2 = pythonObjIntegration.ExecuteScript("combat", "TargetClosest", args);
		int ignoreTarget = PyInt_AsLong(result);
		Py_DECREF(result);


		if (!critterSys.IsFriendly(combatant, performer)
			&& !objects.IsUnconscious(combatant)
			&& !ignoreTarget)
		{
			auto distToCombatant = locSys.DistanceToObj(performer, combatant);
			//logger->debug("Checking line of attack for target: {}", description.getDisplayName(combatant));
			bool hasLineOfAttack = combatSys.HasLineOfAttack(performer, combatant);
			if (d20Sys.d20Query(combatant, DK_QUE_Critter_Is_Invisible)
				&& !d20Sys.d20Query(performer, DK_QUE_Critter_Can_See_Invisible))
			{
				distToCombatant = static_cast<float>((distToCombatant + 5.0) * 2.5); // makes invisibile chars less likely to be attacked; also takes into accout stuff like Hide From Animals (albeit in a shitty manner)
			}
			bool isGoodTarget = distToCombatant <= reach && hasLineOfAttack;
			
			if (isGoodTarget)
			{
				if (distToCombatant < dist ) // best
				{
					aiTac->target = combatant;
					dist = distToCombatant;
					hasGoodTarget = true;
				}
				else if (!hasGoodTarget) // is a good target within reach, not necessarily the closest so far, but other good targets haven't been found yet
				{
					aiTac->target = combatant;
					dist = distToCombatant;
					hasGoodTarget = true;
				}
			}
			else if (distToCombatant < dist && !hasGoodTarget)
			{
				aiTac->target = combatant;
				dist = distToCombatant;
			} 
		}

	}
	logger->info("{} targeted.", objects.description.getDisplayName(aiTac->target, aiTac->performer));

	return 0;
}

int AiSystem::TargetThreatened(AiTactic* aiTac)
{
	objHndl performer = aiTac->performer;
	int performerIsIntelligent = (objects.StatLevelGet(performer, stat_intelligence) >= 3);
//	objHndl target;
	LocAndOffsets performerLoc;
	float dist = 1000000000.0;


	locSys.getLocAndOff(aiTac->performer, &performerLoc);

	logger->info("{} targeting threatened...", objects.description.getDisplayName(aiTac->performer));

	ObjList objlist;
	objlist.ListVicinity(performerLoc.location, OLC_CRITTERS);

	auto args = PyTuple_New(2);
	PyTuple_SET_ITEM(args, 0, PyObjHndl_Create(performer));
	
	for (int i = 0; i < objlist.size(); i++)
	{
		objHndl dude = objlist.get(i);
		PyTuple_SET_ITEM(args, 1, PyObjHndl_Create(dude));

		auto result = pythonObjIntegration.ExecuteScript("combat", "ShouldIgnoreTarget", args);
		int ignoreTarget = PyInt_AsLong(result);
		Py_DECREF(result);
		

		if (!critterSys.IsFriendly(dude, performer)
			&& !objects.IsDeadNullDestroyed(dude)
			&& locSys.DistanceToObj(performer, dude)  < dist
			&& combatSys.IsWithinReach(performer, dude) 
			&& !ignoreTarget)
		{
			aiTac->target = dude;
			dist = locSys.DistanceToObj(performer, dude);
		}
	}
	Py_DECREF(args);
	if (dist > 900000000.0)
	{
		aiTac->target = 0;
		//hooked_print_debug_message(" no target found. Attempting Target Closest instead.");
		logger->info("no target found. ");
		//TargetClosest(aiTac);
	} 
	else
	{
		logger->info("{} targeted.", objects.description.getDisplayName(aiTac->target, aiTac->performer));
	}
	

	return 0;
};

int AiSystem::Approach(AiTactic* aiTac)
{
	int d20aNum; 

	d20aNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	if (!aiTac->target)
		return 0;
	if (combatSys.IsWithinReach(aiTac->performer, aiTac->target))
		return 0;
	actSeqSys.curSeqReset(aiTac->performer);
	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_UNSPECIFIED_MOVE, 0);
	d20Sys.GlobD20ActnSetTarget(aiTac->target, 0);
	actSeqSys.ActionAddToSeq();
	if (actSeqSys.ActionSequenceChecksWithPerformerLocation())
	{
		actSeqSys.ActionSequenceRevertPath(d20aNum);
		return 0;
	}
	return 1;
}

int AiSystem::CastParty(AiTactic* aiTac)
{
	auto initialActNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	if (!aiTac->target)
		return 0;
	objHndl enemiesCanMelee[40];
	int castDefensively = 0;
	if (combatSys.GetEnemiesCanMelee(aiTac->performer, enemiesCanMelee) > 0)
		castDefensively = 1;
	d20Sys.d20SendSignal(aiTac->performer, DK_SIG_SetCastDefensively, castDefensively, 0);
	LocAndOffsets targetLoc =	objects.GetLocationFull(aiTac->target);
	auto partyLen = party.GroupListGetLen();
	for (int i = 0; i < partyLen; i++)
	{
		aiTac->spellPktBody.targetListHandles[i] = party.GroupListGetMemberN(i);
	}
	aiTac->spellPktBody.targetListNumItems = partyLen;

	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_CAST_SPELL, 0);
	actSeqSys.ActSeqCurSetSpellPacket(&aiTac->spellPktBody, 1); // ignore LOS changed to 1, was originally 0
	d20Sys.GlobD20ActnSetSpellData(&aiTac->d20SpellData);
	d20Sys.GlobD20ActnSetTarget(aiTac->target, &targetLoc); // originally fetched a concious party member, seems like a bug so I changed it to the target
	actSeqSys.ActionAddToSeq();
	if (actSeqSys.ActionSequenceChecksWithPerformerLocation() != AEC_OK)
	{
		actSeqSys.ActionSequenceRevertPath(initialActNum);
		return 0;
	}
	return 1;
}

int AiSystem::PickUpWeapon(AiTactic* aiTac)
{
	int d20aNum;

	d20aNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;

	if (inventory.ItemWornAt(aiTac->performer, 203) || inventory.ItemWornAt(aiTac->performer, 204))
	{
		return 0;
	}
	if (!d20Sys.d20Query(aiTac->performer, DK_QUE_Disarmed))
		return 0;
	objHndl weapon = d20Sys.d20QueryReturnData(aiTac->performer, DK_QUE_Disarmed, 0, 0);

	if (weapon && !inventory.GetParent(weapon))
	{
		aiTac->target = weapon;
	} else
	{
		LocAndOffsets loc;
		locSys.getLocAndOff(aiTac->performer, &loc);

		ObjList objList;
		objList.ListTile(loc.location, OLC_WEAPON);

		if (objList.size() > 0)
		{
			weapon = objList.get(0);
			aiTac->target = weapon;
		} else
		{
			aiTac->target = 0i64;
		}
	}
	
	
	
	if (!aiTac->target || objects.IsCritter(aiTac->target))
	{
		return 0;
	}


	if (!combatSys.IsWithinReach(aiTac->performer, aiTac->target))
		return 0;
	actSeqSys.curSeqReset(aiTac->performer);
	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_DISARMED_WEAPON_RETRIEVE, 0);
	d20Sys.GlobD20ActnSetTarget(aiTac->target, 0);
	actSeqSys.ActionAddToSeq();
	if (actSeqSys.ActionSequenceChecksWithPerformerLocation())
	{
		actSeqSys.ActionSequenceRevertPath(d20aNum);
		return 0;
	}
	return 1;
}

int AiSystem::BreakFree(AiTactic* aiTac)
{
	int d20aNum;
	objHndl performer = aiTac->performer;
	objHndl target;
	LocAndOffsets performerLoc;
	float dist = 0.0;

	d20aNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	if (!d20Sys.d20Query(performer, DK_QUE_Is_BreakFree_Possible))
		return 0;
	int spellId = (int) d20Sys.d20QueryReturnData(performer, DK_QUE_Is_BreakFree_Possible, 0, 0);

	locSys.getLocAndOff(aiTac->performer, &performerLoc);
	if (combatSys.GetClosestEnemy(aiTac->performer, &performerLoc, &target, &dist, 0x21))
	{
		if (combatSys.IsWithinReach(aiTac->performer, target))
			return 0;
	}
	actSeqSys.curSeqReset(aiTac->performer);
	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_BREAK_FREE, spellId);
	//d20Sys.GlobD20ActnSetTarget(aiTac->performer, 0);
	actSeqSys.ActionAddToSeq();
	if (actSeqSys.ActionSequenceChecksWithPerformerLocation())
	{
		actSeqSys.ActionSequenceRevertPath(d20aNum);
		return 0;
	}

	return 1;
}

int AiSystem::GoMelee(AiTactic* aiTac)
{
	logger->info("Attempting Go Melee...");
	auto performer = aiTac->performer;
	auto tbStat = actSeqSys.curSeqGetTurnBasedStatus();
	auto weapon = critterSys.GetWornItem(performer, EquipSlot::WeaponPrimary);
	if (!weapon)
		return 0;
	if (!inventory.IsRangedWeapon(weapon))
		return 0;
	objHndl * inventoryArray;
	auto invenCount = inventory.GetInventory(performer, &inventoryArray);
	
	for (int i = 0; i < invenCount; i++)
	{
		if (objects.GetType(inventoryArray[i]) == obj_t_weapon)
		{
			auto weapFlags = objects.getInt32(inventoryArray[i], obj_f_weapon_flags);
			if ( (weapFlags & OWF_RANGED_WEAPON) == 0 )
			{
				inventory.ItemUnwieldByIdx(performer, 203);
				inventory.ItemUnwieldByIdx(performer, 204);
				inventory.ItemPlaceInIdx(inventoryArray[i], 203);
				tbStat->hourglassState = actSeqSys.GetHourglassTransition(tbStat->hourglassState, 1);
				free(inventoryArray);
				logger->info("Go Melee succeeded.");
				return 1;
			}

		}
	}
	
	free(inventoryArray);
	return 0;
	
}

int AiSystem::Sniper(AiTactic* aiTac)
{
	auto weapon = critterSys.GetWornItem(aiTac->performer, EquipSlot::WeaponPrimary);
	if (!weapon)
		return 0;
	if (!inventory.IsRangedWeapon(weapon))
		return 0;
	if (!combatSys.AmmoMatchesItemAtSlot(aiTac->performer, EquipSlot::WeaponPrimary))
	{
		GoMelee(aiTac);
		return 0;
	}
	if (!AiFiveFootStepAttempt(aiTac))
		GoMelee(aiTac);
	return Default(aiTac);
}

int AiSystem::CoupDeGrace(AiTactic* aiTac)
{
	int actNum; 
	objHndl origTarget = aiTac->target;

	actNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	aiTac->target = 0i64;
	combatSys.GetClosestEnemy(aiTac, 1);
	if (aiTac->target)
	{
		if (Approach(aiTac)
			|| (d20Sys.GlobD20ActnInit(),
			d20Sys.GlobD20ActnSetTypeAndData1(D20A_COUP_DE_GRACE, 0),
			d20Sys.GlobD20ActnSetTarget(aiTac->target, 0),
			actSeqSys.ActionAddToSeq(),
			!actSeqSys.ActionSequenceChecksWithPerformerLocation()))
		{
			return 1;
		}
		actSeqSys.ActionSequenceRevertPath(actNum);
		return 0;
		
	}

	aiTac->target = origTarget;
	return 0;


}

int AiSystem::ChargeAttack(AiTactic* aiTac)
{
	int actNum0 = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	if (!aiTac->target)
		return 0;
	actSeqSys.curSeqReset(aiTac->performer);
	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_CHARGE, 0);
	objHndl weapon = inventory.ItemWornAt(aiTac->performer, 3);
	if (!weapon)
	{
		//TurnBasedStatus tbStatCopy;
		D20Actn d20aCopy;
		//memcpy(&tbStatCopy, &(*actSeqSys.actSeqCur)->tbStatus, sizeof(TurnBasedStatus));
		memcpy(&d20aCopy, d20Sys.globD20Action, sizeof(D20Actn));
		int natAtk = dispatch.DispatchD20ActionCheck(&d20aCopy, 0, dispTypeGetCritterNaturalAttacksNum);
		if (natAtk > 0)
			d20Sys.GlobD20ActnSetTypeAndData1(D20A_CHARGE, ATTACK_CODE_NATURAL_ATTACK + 1);
		else
			d20Sys.GlobD20ActnSetTypeAndData1(D20A_CHARGE, 0);
	}
	else
		d20Sys.GlobD20ActnSetTypeAndData1(D20A_CHARGE, 0);

	d20Sys.GlobD20ActnSetTarget(aiTac->target, 0);
	actSeqSys.ActionAddToSeq();
	if (actSeqSys.ActionSequenceChecksWithPerformerLocation())
	{
		actSeqSys.ActionSequenceRevertPath(actNum0);
		return 0;
	}
	return 1;
}

void AiSystem::UpdateAiFightStatus(objHndl objIn, int* aiStatus, objHndl* target)
{
	int critterFlags;
	AiFlag aiFlags;

	if (objects.getInt32(objIn, obj_f_critter_flags) & OCF_FLEEING)
	{
		if (target)
			*target = objects.getObjHnd(objIn, obj_f_critter_fleeing_from);
		*aiStatus = 2;
	}
	else
	{
		critterFlags = objects.getInt32(objIn, obj_f_critter_flags);
		if (critterFlags & OCF_SURRENDERED)                     // OCF_SURRENDERED (maybe mixed up with OCF_SPELL_FLEE?)
		{
			if (target)
				*target = objects.getObjHnd(objIn, obj_f_critter_fleeing_from);
			*aiStatus = 3;
		}
		else
		{
			aiFlags = (AiFlag)objects.getInt64(objIn, obj_f_npc_ai_flags64);
			if ((uint64_t)aiFlags & (uint64_t)AiFlag::Fighting)
			{
				if (target)
					*target = objects.getObjHnd(objIn, obj_f_npc_combat_focus);
				*aiStatus = 1;
			}
			else if ((uint64_t)aiFlags & (uint64_t)AiFlag::FindingHelp)
			{
				if (target)
					*target = objects.getObjHnd(objIn, obj_f_npc_combat_focus);
				*aiStatus = 4;
			}
			else
			{
				if (target)
					*target = 0i64;
				*aiStatus = 0;
			}
		}
	}
}

int AiSystem::UpdateAiFlags(objHndl obj, int aiFightStatus, objHndl target, int* soundMap)
{
	return addresses.UpdateAiFlags(obj, aiFightStatus, target, soundMap);
}

void AiSystem::StrategyTabLineParseTactic(AiStrategy* aiStrat, char* tacName, char* middleString, char* spellString)
{ // this functions matches the tactic strings (3 strings) to a tactic def
	int tacIdx = 0;
	if (*tacName)
	{
		for (int i = 0; i < 44 && _stricmp(tacName, aiTacticDefs[i].name); i++){
			++tacIdx;
		}
		if (tacIdx < 44)
		{
			aiStrat->aiTacDefs[aiStrat->numTactics] = &aiTacticDefs[tacIdx];
			aiStrat->field54[aiStrat->numTactics] = 0;
			aiStrat->spellsKnown[aiStrat->numTactics].spellEnum = -1;
			if (*spellString)
				spell->ParseSpellSpecString(&aiStrat->spellsKnown[aiStrat->numTactics], (char *)spellString);
			++aiStrat->numTactics;
			return;
		}
		tacIdx = 0;
		for (int i = 0; i < 100 && aiTacticDefsNew[i].name && _stricmp(tacName, aiTacticDefsNew[i].name); i++)
		{
			tacIdx++;
		}
		if (aiTacticDefsNew[tacIdx].name && tacIdx < 100)
		{
			aiStrat->aiTacDefs[aiStrat->numTactics] = &aiTacticDefsNew[tacIdx];
			aiStrat->field54[aiStrat->numTactics] = 0;
			aiStrat->spellsKnown[aiStrat->numTactics].spellEnum = -1;
			if (*spellString)
				spell->ParseSpellSpecString(&aiStrat->spellsKnown[aiStrat->numTactics], (char *)spellString);
			++aiStrat->numTactics;
			return;
		}
		logger->warn("Error: No Such Tactic {} for Strategy {}", tacName, aiStrat->name);
		return;
		
		
	}
	return;
}

int AiSystem::StrategyTabLineParser(TabFileStatus* tabFile, int n, char** strings)
{
	AiStrategy *aiStrat; 
	const char *v4; 
	signed int i; 
	char v6; 
	char *stratName; 
	unsigned int v8; 
	char **v9; 

	aiStrat = *aiStrategies;
	aiStrat = &aiStrat[n];
	
	v4 = *strings;
	i = 0;
	do
		v6 = *v4++;
	while (v6);
	stratName = (char *)malloc(v4 - ( (*strings )+ 1) + 1);
	aiStrat->name = stratName;
	strcpy((char *)stratName, *strings);
	aiStrat->numTactics = 0;
	v8 = 3;
	v9 = strings + 2;
	do
	{
		if (tabFile->numTabsMax < v8)
			break;
		StrategyTabLineParseTactic(aiStrat, *(v9 - 1), *v9, v9[1]);
		v8 += 3;
		v9 += 3;
		++i;
	} while (i < 20);
	++(*aiStrategiesNum);
	return 0;
}

int AiSystem::AiOnInitiativeAdd(objHndl obj)
{
	int critterStratIdx = objects.getInt32(obj, obj_f_critter_strategy);
	
	assert(critterStratIdx >= 0 && critterStratIdx < *aiStrategiesNum);
	AiStrategy *aiStrats = *aiStrategies;
	AiStrategy * aiStrat = &aiStrats[critterStratIdx];
	AiTactic aiTac;
	aiTac.performer = obj;

	for (int i = 0; i < aiStrat->numTactics; i++)
	{
		aiTacticGetConfig(i, &aiTac, aiStrat);
		auto func = aiTac.aiTac->onInitiativeAdd;
		if (func)
		{
			if (func(&aiTac))
				return 1;
		}
	}
	return 0;
}

AiCombatRole AiSystem::GetRole(objHndl obj)
{
	if (critterSys.IsCaster(obj))
		return AiCombatRole::caster;
	return AiCombatRole::general;
}

BOOL AiSystem::AiFiveFootStepAttempt(AiTactic* aiTac)
{
	objHndl threateners[40];
	auto actNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	if (!combatSys.GetEnemiesCanMelee(aiTac->performer, threateners))
		return 1;
	float overallOffX, overallOffY;
	LocAndOffsets loc;
	locSys.getLocAndOff(aiTac->performer, &loc);
	locSys.GetOverallOffset(loc, &overallOffX, &overallOffY);
	for (float angleDeg = 0.0; angleDeg <= 360.0;  angleDeg += 45.0 )
	{

		float angleRad = static_cast<float>(angleDeg * 0.017453292); // to radians
		auto cosTheta = cosf(angleRad);
		auto sinTheta = sinf(angleRad);
		double fiveFootStepX = overallOffX - cosTheta * 60.0; // five feet radius
		double fiveFootStepY = overallOffY + sinTheta * 60.0;
		loc.FromAbsolute(fiveFootStepX, fiveFootStepY);
		if (!combatSys.GetThreateningCrittersAtLoc(aiTac->performer, &loc, threateners))
		{
			d20Sys.GlobD20ActnSetTypeAndData1(D20A_5FOOTSTEP, 0);
			d20Sys.GlobD20ActnSetTarget(0, &loc);
			if (!actSeqSys.ActionAddToSeq()
				&& actSeqSys.GetPathTargetLocFromCurD20Action(&loc)
				&& !combatSys.GetThreateningCrittersAtLoc(aiTac->performer, &loc, threateners)
				&& !actSeqSys.ActionSequenceChecksWithPerformerLocation())
				return 1;
			actSeqSys.ActionSequenceRevertPath(actNum);
		}
	}
	return 0;
}

void AiSystem::RegisterNewAiTactics()
{
	memset(aiTacticDefsNew, 0, sizeof(AiTacticDef) * AI_TACTICS_NEW_SIZE);

	int n = 0;

	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "asplode");


	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiWakeFriend;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "wake friend");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "use magic item");
	
	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "improve position five foot step");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "disarm");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "sorcerer cast once single");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "coordinated strategy");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "cast best offensive");

	n++;
	aiTacticDefsNew[n].name = new char[100];
	aiTacticDefsNew[n].aiFunc = _AiAsplode;
	memset(aiTacticDefsNew[n].name, 0, 100);
	sprintf(aiTacticDefsNew[n].name, "cast best crowd control");
}

unsigned int AiSystem::Asplode(AiTactic* aiTac)
{
	auto performer = aiTac->performer;
	critterSys.KillByEffect(performer);
	return 0;
}

unsigned AiSystem::WakeFriend(AiTactic* aiTac)
{
	objHndl performer = aiTac->performer;
	objHndl sleeper = 0;
	int performerIsIntelligent = (objects.StatLevelGet(performer, stat_intelligence) >= 3);
	if (!performerIsIntelligent)
		return 0;

	LocAndOffsets performerLoc;
	float sleeperDist = 1000000000.0, enemyDist = 100000.0;


	locSys.getLocAndOff(aiTac->performer, &performerLoc);

	ObjList objlist;
	objlist.ListVicinity(performerLoc.location, OLC_CRITTERS);

	auto args = PyTuple_New(1);

	for (int i = 0; i < objlist.size(); i++)
	{
		objHndl dude = objlist.get(i);
		PyTuple_SET_ITEM(args, 0, PyObjHndl_Create(dude));

		auto result = pythonObjIntegration.ExecuteScript("combat", "IsSleeping", args);

		int isSleeping = PyInt_AsLong(result);
		Py_DECREF(result);

		if (critterSys.IsFriendly(dude, performer))
		{
			if (isSleeping && locSys.DistanceToObj(performer, dude)  < sleeperDist)
			{
				sleeper = dude;
				sleeperDist = locSys.DistanceToObj(performer, dude);
			}
		} 
		
	}
	if (!sleeper)
		return 0;

	// if a sleeper is within reach, do it
	bool shouldWake = combatSys.IsWithinReach(performer, sleeper);
	
	// if not:
	// first of all check if anyone threatens the ai actor
	// if no threats, then 40% chance that you'll try to wake up
	if (!shouldWake )
	{
		int enemyCount;
		auto enemies = combatSys.GetHostileCombatantList(performer, &enemyCount);
		bool isThreatened = false;
		for (int i = 0; i < enemyCount; i++)
		{
			//logger->debug("Enemy under test: {}", description.getDisplayName(enemies[i]));
			if (combatSys.CanMeleeTarget(enemies[i], performer))
			{
				isThreatened = true;
				break;
			}
		}
		delete [] enemies;
		if (!isThreatened)
			shouldWake = (templeFuncs.RNG(1, 100) <= 40);
	}

	if (!shouldWake)
		return 0;

	// do wake action
	int actNum;
	objHndl origTarget = aiTac->target;

	actNum = (*actSeqSys.actSeqCur)->d20ActArrayNum;
	aiTac->target = sleeper;

	if (Approach(aiTac)
		|| (d20Sys.GlobD20ActnInit(),
			d20Sys.GlobD20ActnSetTypeAndData1(D20A_AID_ANOTHER_WAKE_UP, 0),
			d20Sys.GlobD20ActnSetTarget(aiTac->target, 0),
			actSeqSys.ActionAddToSeq(),
			!actSeqSys.ActionSequenceChecksWithPerformerLocation()))
	{
		return 1;
	}
	actSeqSys.ActionSequenceRevertPath(actNum);

	aiTac->target = origTarget;
	return 0;

}

int AiSystem::Default(AiTactic* aiTac)
{
	if (actSeqSys.curSeqGetTurnBasedStatus()->hourglassState < 2)
		return 0;
	if (!aiTac->target)
		return 0;
	d20Sys.GlobD20ActnInit();
	d20Sys.GlobD20ActnSetTypeAndData1(D20A_UNSPECIFIED_ATTACK, 0);
	d20Sys.GlobD20ActnSetTarget(aiTac->target, 0);
	ActionErrorCode addToSeqRes = (ActionErrorCode)actSeqSys.ActionAddToSeq();
	if (addToSeqRes)
	{
		logger->info("AI Default failed, error code: {}", (int)addToSeqRes);
	}
	int performError = actSeqSys.ActionSequenceChecksWithPerformerLocation();
	if (!performError)
	{
		return 1;
	} else
	{
		logger->info("AI Default SequenceCheck failed, error code: {}", static_cast<int>(performError));
	}
	if (!critterSys.IsWieldingRangedWeapon(aiTac->performer))
	{
		logger->info("AI Action Perform: Resetting sequence; Do Unspecified Move Action");
		actSeqSys.curSeqReset(aiTac->performer);
		d20Sys.GlobD20ActnInit();
		d20Sys.GlobD20ActnSetTypeAndData1(D20A_UNSPECIFIED_MOVE, 0);
		d20Sys.GlobD20ActnSetTarget(aiTac->target, 0);
		actSeqSys.ActionAddToSeq();
		performError = actSeqSys.ActionSequenceChecksWithPerformerLocation();
	}
	return performError == 0;
}

int AiSystem::Flank(AiTactic* aiTac)
{
	auto initialActNum = (*actSeqSys.actSeqCur)->d20ActArrayNum; // used for resetting in case of failure
	auto target = aiTac->target;
	if (!target)
		return 0;
	auto performer = aiTac->performer;

	// if wielding ranged, go melee
	GoMelee(aiTac); // checks internally for weapon etc.

	// preliminary checks
	{
		if (!d20Sys.d20QueryWithData(target, DK_QUE_CanBeFlanked, aiTac->performer))
		{
			logger->info("Target inherently unflankable; next.");
			return 0;
		}
			
		if (combatSys.IsFlankedBy(target, performer))
		{
			logger->info("Already flanking; next.");
			return 0;
		}
		if (actSeqSys.isSimultPerformer(performer) && actSeqSys.simulsAbort(performer))
		{
			actSeqSys.curSeqReset(performer);
			return 1;
		}
	
		// added check for surroundment for performance reasons
		PathQuery pq;
		Path pqr;
		pq.flags = static_cast<PathQueryFlags>(PQF_HAS_CRITTER | PQF_TARGET_OBJ | PQF_ADJUST_RADIUS);
		pq.critter = performer;
		pq.targetObj = target;
		pathfindingSys.PathInit(&pqr, &pq);
		pq.tolRadius += critterSys.GetReach(performer, D20A_UNSPECIFIED_ATTACK);
		if (pathfindingSys.TargetSurrounded(&pqr, &pq))
		{
			logger->info("Target surrounded; next.");
			return 0;
		}
	}

	// cycle through surrounding allies and attempt to move to their opposite location
	objHndl allies[40];
	auto numAllies = combatSys.GetEnemiesCanMelee(target, allies);
	if (numAllies <= 0)
	{
		logger->info("No allies to flank with; next.");
		return 0;
	}
		
	logger->info("Flank preliminary checks passed; {} allies found in melee range", numAllies);

	LocAndOffsets tgtLoc = objects.GetLocationFull(target);
	float tgtAbsX, tgtAbsY, 
		tgtRadius = objects.GetRadius(target), 
		flankDist = objects.GetRadius(performer)+ tgtRadius + 8.0;

	locSys.GetOverallOffset(tgtLoc, &tgtAbsX, &tgtAbsY);
	for (int i = 0; i < numAllies; i++)
	{
		if (allies[i] == performer)
			continue;

		LocAndOffsets allyLoc = objects.GetLocationFull(allies[i]),
			flankLoc;
		float allyAbsX, allyAbsY, flankAbsX, flankAbsY, deltaX, deltaY, normalization;
		// get the diametrically opposed location
		{
			locSys.GetOverallOffset(allyLoc, &allyAbsX, &allyAbsY);
			deltaX = allyAbsX - tgtAbsX;
			deltaY = allyAbsY - tgtAbsY;
			normalization = 1.0 / sqrt(deltaY*deltaY + deltaX*deltaX);
			float xHat = deltaX * normalization, // components of unit vector from target to ally
					yHat = deltaY * normalization;

			flankAbsX = tgtAbsX - xHat * flankDist;
			flankAbsY = tgtAbsY - yHat * flankDist;
			flankLoc.FromAbsolute(flankAbsX, flankAbsY);
			if (!pathfindingSys.PathDestIsClear(performer, &flankLoc))
			{
				bool foundFlankLoc = false;
				// try to tweak the angle; the flank check looks for the range of 120 - 240�, so we'll try 135,165,195,225
				float tweakAngles[4] = { -15.0, 15.0, -45.0, 45.0 };
				for (int i = 0; i < 4; i++)
				{
					float tweakAngle = tweakAngles[i] * M_PI / 180;
					float xHat2 = xHat * cosf(tweakAngle) + yHat * sinf(tweakAngle),
						yHat2 =  -xHat * sinf(tweakAngle) + yHat * cosf(tweakAngle) ;
					flankAbsX = tgtAbsX - xHat2 * flankDist;
					flankAbsY = tgtAbsY - yHat2 * flankDist;
					flankLoc.FromAbsolute(flankAbsX, flankAbsY);
					if (pathfindingSys.PathDestIsClear(performer, &flankLoc))
					{
						foundFlankLoc = true;
						break;
					}		
				}
				if (!foundFlankLoc)
				{
					if (i == numAllies-1)
						logger->info("No clear flanking position found; next ai tactic.");
					return 0;
				}	
			}
		}
		logger->info("Found flanking position: {} ; attempting move to position.", flankLoc);
		// create a d20 action using that location
		d20Sys.GlobD20ActnSetTypeAndData1(D20A_UNSPECIFIED_MOVE, 0);
		d20Sys.GlobD20ActnSetTarget(0i64, &flankLoc);
		LocAndOffsets pathTgt;
		auto actionCheckResult = actSeqSys.ActionAddToSeq();
		if (actionCheckResult != AEC_OK)
		{
			logger->info("Failed move to position when adding to sequence.");
		}
		else if (actSeqSys.GetPathTargetLocFromCurD20Action(&pathTgt)
			&& combatSys.CanMeleeTargetFromLoc(performer, target, &pathTgt))
		{
			actionCheckResult = actSeqSys.ActionSequenceChecksWithPerformerLocation();
			if (actionCheckResult == AEC_OK)
			{
				return 1;
			}
		} 
		if (actionCheckResult)
			logger->info("Failed move to position due to sequence checks. Error Code: {}", actionCheckResult);
		else
			logger->info("Failed move to position due to CanMeleeTargetAtLoc check! Location was {}", pathTgt);
		actSeqSys.ActionSequenceRevertPath(initialActNum);
	}
	return 0;
	
}

int AiSystem::AttackThreatened(AiTactic* aiTac)
{
	if (!aiTac->target || !combatSys.CanMeleeTarget(aiTac->performer, aiTac->target))
		return 0;
	return Default(aiTac);
}

void AiSystem::AiTurnSthg_1005EEC0(objHndl obj)
{
	return addresses.AiTurnSthg_1005EEC0(obj);
}
#pragma endregion

#pragma region AI replacement functions

uint32_t _aiStrategyParse(objHndl objHnd, objHndl target)
{
	return aiSys.AiStrategyParse(objHnd, target);
}

int _AiCoupDeGrace(AiTactic* aiTac)
{
	return aiSys.CoupDeGrace(aiTac);
}

int _AiApproach(AiTactic* aiTac)
{
	return aiSys.Approach(aiTac);
}

int _AiCharge(AiTactic* aiTac)
{
	return aiSys.ChargeAttack(aiTac);
}

int _AiTargetClosest(AiTactic * aiTac)
{
	return aiSys.TargetClosest(aiTac);
}


void _StrategyTabLineParser(TabFileStatus* tabFile, int n, char** strings)
{
	aiSys.StrategyTabLineParser(tabFile, n, strings);
}

int _AiOnInitiativeAdd(objHndl obj)
{
	return aiSys.AiOnInitiativeAdd(obj);
}

unsigned int _AiAsplode(AiTactic * aiTac)
{
	return aiSys.Asplode(aiTac);
}

unsigned int _AiWakeFriend(AiTactic * aiTac)
{
	return aiSys.WakeFriend(aiTac);
}

class AiReplacements : public TempleFix
{
public: 
	const char* name() override { return "AI Function Replacements";} 

	static int AiDefault(AiTactic* aiTac);
	static int AiAttack(AiTactic* aiTac);
	static int AiGoMelee(AiTactic* aiTac);
	static int AiSniper(AiTactic* aiTac);
	static int AiAttackThreatened(AiTactic* aiTac);
	static int AiTargetThreatened(AiTactic* aiTac);
	static int AiCastParty(AiTactic* aiTac);
	static int AiFlank(AiTactic* aiTac);

	void apply() override 
	{
		logger->info("Replacing AI functions...");
		
		replaceFunction(0x100E3270, AiDefault);
		replaceFunction(0x100E3A00, _AiTargetClosest);
		replaceFunction(0x100E43F0, AiCastParty);
		replaceFunction(0x100E46C0, AiAttack);
		replaceFunction(0x100E46D0, AiTargetThreatened);
		replaceFunction(0x100E48D0, _AiApproach);
		replaceFunction(0x100E4BD0, _AiCharge);
		replaceFunction(0x100E50C0, _aiStrategyParse);
		replaceFunction(0x100E5460, _AiOnInitiativeAdd);
		replaceFunction(0x100E5500, _StrategyTabLineParser);
		replaceFunction(0x100E55A0, AiGoMelee);
		replaceFunction(0x100E58D0, AiSniper);
		replaceFunction(0x100E5950, AiFlank);
		replaceFunction(0x100E5DB0, _AiCoupDeGrace);
		
	}
} aiReplacements;

int AiReplacements::AiDefault(AiTactic* aiTac)
{
	return aiSys.Default(aiTac);
}

int AiReplacements::AiAttack(AiTactic* aiTac)
{
	return aiSys.Default(aiTac);
}

int AiReplacements::AiTargetThreatened(AiTactic* aiTac)
{
	return aiSys.TargetThreatened(aiTac);
}

int AiReplacements::AiGoMelee(AiTactic* aiTac)
{
	return aiSys.GoMelee(aiTac);
}

int AiReplacements::AiSniper(AiTactic* aiTac)
{
	return aiSys.Sniper(aiTac);
}

int AiReplacements::AiAttackThreatened(AiTactic* aiTac)
{
	return aiSys.AttackThreatened(aiTac);
}

int AiReplacements::AiCastParty(AiTactic* aiTac)
{
	return aiSys.CastParty(aiTac);
}

int AiReplacements::AiFlank(AiTactic* aiTac)
{
	return aiSys.Flank(aiTac);
}
#pragma endregion 
AiPacket::AiPacket(objHndl objIn)
{
	target = 0i64;
	obj = objIn;
	aiFightStatus = 0;
	aiState2 = 0;
	field18 = 10000;
	skillEnum = (SkillEnum)-1;
	scratchObj = 0i64;
	leader = critterSys.GetLeader(objIn);
	aiSys.UpdateAiFightStatus(objIn, &this->aiFightStatus, &this->target);
	field30 = -1;
}