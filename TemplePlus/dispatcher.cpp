#include "stdafx.h"
#include "dispatcher.h"
#include "d20.h"
#include "temple_functions.h"
#include "obj.h"
#include "condition.h"
#include "bonus.h"
#include "action_sequence.h"
#include "turn_based.h"
#include "damage.h"
#include "util/fixes.h"

class DispatcherReplacements : public TempleFix {
public:
	const char* name() override {
		return "Dispatcher System Function Replacements";
	}

	void apply() override {
		logger->info("Replacing basic Dispatcher functions");
		
		replaceFunction(0x1004D700, _DispIoCheckIoType1);
		replaceFunction(0x1004D720, _DispIoCheckIoType2);
		replaceFunction(0x1004D760, _DispIoCheckIoType4);
		replaceFunction(0x1004D780, _DispIoCheckIoType5);
		replaceFunction(0x1004D7A0, _DispIoCheckIoType6);
		replaceFunction(0x1004D7C0, _DispIoCheckIoType7);
		replaceFunction(0x1004D7E0, _DispIoCheckIoType8);
		replaceFunction(0x1004D800, _DispIoCheckIoType9);
		replaceFunction(0x1004D820, _DispIoCheckIoType10);
		replaceFunction(0x1004D840, _DispIoCheckIoType11);
		replaceFunction(0x1004D860, _DispIoCheckIoType12);
		replaceFunction(0x1004D8A0, _DispIoCheckIoType14);
		replaceFunction(0x1004F780, _PackDispatcherIntoObjFields);
		
		replaceFunction(0x100E1E30, _DispatcherRemoveSubDispNodes);
		replaceFunction(0x100E2400, _DispatcherClearField);
		replaceFunction(0x100E2720, _DispatcherClearPermanentMods);
		replaceFunction(0x100E2740, _DispatcherClearItemConds);
		replaceFunction(0x100E2760, _DispatcherClearConds);
		replaceFunction(0x100E2120, _DispatcherProcessor);
		replaceFunction(0x100E1F10, _DispatcherInit);
		replaceFunction(0x1004DBA0, DispIOType21Init);
		replaceFunction(0x1004D3A0, _Dispatch62);
		replaceFunction(0x1004D440, _Dispatch63);
		replaceFunction(0x1004DEC0, _DispatchAttackBonus);
		replaceFunction(0x1004E040, _DispatchDamage);
		replaceFunction(0x1004E790, _dispatchTurnBasedStatusInit); 

		replaceFunction(0x1004ED70, _dispatch1ESkillLevel); 
		
 
	}
} dispatcherReplacements;

struct DispatcherSystemAddresses : temple::AddressTable
{
	int(__cdecl *DispatchAttackBonus)(objHndl attacker, objHndl victim, DispIoAttackBonus *dispIoIn, enum_disp_type, int key);
	DispatcherSystemAddresses()
	{
		rebase(DispatchAttackBonus, 0x1004DEC0);
	}
} addresses;

#pragma region Dispatcher System Implementation
DispatcherSystem dispatch;

void DispatcherSystem::DispatcherProcessor(Dispatcher* dispatcher, enum_disp_type dispType, uint32_t dispKey, DispIO* dispIO)
{
	_DispatcherProcessor(dispatcher, dispType, dispKey, dispIO);
}

Dispatcher * DispatcherSystem::DispatcherInit(objHndl objHnd)
{
	return _DispatcherInit(objHnd);
}

bool DispatcherSystem::dispatcherValid(Dispatcher* dispatcher)
{
	return (dispatcher != nullptr && dispatcher != (Dispatcher*)-1);
}

void  DispatcherSystem::DispatcherClearField(Dispatcher * dispatcher, CondNode ** dispCondList)
{
	_DispatcherClearField(dispatcher, dispCondList);
}

void  DispatcherSystem::DispatcherClearPermanentMods(Dispatcher * dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->permanentMods);
}

void  DispatcherSystem::DispatcherClearItemConds(Dispatcher * dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->itemConds);
}

void  DispatcherSystem::DispatcherClearConds(Dispatcher *dispatcher)
{
	_DispatcherClearConds(dispatcher);
}

int32_t DispatcherSystem::dispatch1ESkillLevel(objHndl objHnd, SkillEnum skill, BonusList* bonOut, objHndl objHnd2, int32_t flag)
{
	DispIoBonusAndObj dispIO;
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatcherValid(dispatcher)) return 0;

	dispIO.dispIOType = dispIOTypeSkillLevel;
	dispIO.returnVal = flag;
	dispIO.bonOut = bonOut;
	dispIO.obj = objHnd2;
	if (!bonOut)
	{
		dispIO.bonOut = &dispIO.bonlist;
		bonusSys.initBonusList(&dispIO.bonlist);
	}
	DispatcherProcessor(dispatcher, dispTypeSkillLevel, skill + 20, &dispIO);
	return bonusSys.getOverallBonus(dispIO.bonOut);
	
}

float DispatcherSystem::Dispatch29hGetMoveSpeed(objHndl objHnd, void* iO) // including modifiers like armor restirction
{
	float result = 30.0;
	uint32_t objHndLo = (uint32_t)(objHnd & 0xffffFFFF);
	uint32_t objHndHi = (uint32_t)((objHnd >>32) & 0xffffFFFF);
	macAsmProl;
	__asm{
		mov ecx, this;
		mov esi, [ecx]._Dispatch29hMovementSthg;
		mov eax, iO;
		push eax;
		mov eax, objHndHi;
		push eax;
		mov eax, objHndLo;
		push eax;
		call esi;
		add esp, 12;
		fstp result;
	}
	macAsmEpil
	return result;
}

void DispatcherSystem::dispIOTurnBasedStatusInit(DispIOTurnBasedStatus* dispIOtbStat)
{
	dispIOtbStat->dispIOType = dispIOTypeTurnBasedStatus;
	dispIOtbStat->tbStatus = nullptr;
}


void DispatcherSystem::dispatchTurnBasedStatusInit(objHndl objHnd, DispIOTurnBasedStatus* dispIOtB)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (dispatcherValid(dispatcher))
	{
		DispatcherProcessor(dispatcher, dispTypeTurnBasedStatusInit, 0, dispIOtB);
		if (dispIOtB->tbStatus)
		{
			if (dispIOtB->tbStatus->hourglassState > 0)
			{
				d20Sys.d20SendSignal(objHnd, DK_SIG_BeginTurn, 0, 0);
			}
		}
	}
}


DispIoCondStruct* DispatcherSystem::DispIoCheckIoType1(DispIoCondStruct* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeCondStruct) return nullptr;
	return dispIo;
}

DispIoBonusList* DispatcherSystem::DispIoCheckIoType2(DispIoBonusList* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeBonusList) return nullptr;
	return dispIo;
}

DispIoBonusList* DispatcherSystem::DispIoCheckIoType2(DispIO* dispIoBonusList)
{
	return DispIoCheckIoType2( (DispIoBonusList*) dispIoBonusList);
}

DispIoSavingThrow* DispatcherSystem::DispIoCheckIoType3(DispIoSavingThrow* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeSavingThrow) return nullptr;
	return dispIo;
}

DispIoSavingThrow* DispatcherSystem::DispIoCheckIoType3(DispIO* dispIo)
{
	return DispIoCheckIoType3((DispIoSavingThrow*)dispIo);
}

DispIoDamage* DispatcherSystem::DispIoCheckIoType4(DispIoDamage* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeDamage) return nullptr;
	return dispIo;
}

DispIoDamage* DispatcherSystem::DispIoCheckIoType4(DispIO* dispIo)
{
	return DispIoCheckIoType4((DispIoDamage*)dispIo);
}

DispIoAttackBonus* DispatcherSystem::DispIoCheckIoType5(DispIoAttackBonus* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeAttackBonus) return nullptr;
	return dispIo;
}

DispIoAttackBonus* DispatcherSystem::DispIoCheckIoType5(DispIO* dispIo)
{
	return DispIoCheckIoType5((DispIoAttackBonus*)dispIo);
}

DispIoD20Signal* DispatcherSystem::DispIoCheckIoType6(DispIoD20Signal* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeSendSignal) return nullptr;
	return dispIo;
}

DispIoD20Signal* DispatcherSystem::DispIoCheckIoType6(DispIO* dispIo)
{
	return DispIoCheckIoType6((DispIoD20Signal*)dispIo);
}

DispIoD20Query* DispatcherSystem::DispIoCheckIoType7(DispIoD20Query* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeQuery) return nullptr;
	return dispIo;
}

DispIoD20Query* DispatcherSystem::DispIoCheckIoType7(DispIO* dispIo)
{
	return DispIoCheckIoType7((DispIoD20Query*)dispIo);
}

DispIOTurnBasedStatus* DispatcherSystem::DispIoCheckIoType8(DispIOTurnBasedStatus* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeTurnBasedStatus) return nullptr;
	return dispIo;
}

DispIoTooltip* DispatcherSystem::DispIoCheckIoType9(DispIoTooltip* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeTooltip) return nullptr;
	return dispIo;
}

DispIoTooltip* DispatcherSystem::DispIoCheckIoType9(DispIO* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeTooltip) return nullptr;
	return static_cast<DispIoTooltip*>(dispIo);
}

DispIoBonusAndObj* DispatcherSystem::DispIoCheckIoType10(DispIoBonusAndObj* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeSkillLevel) return nullptr;
	return dispIo;
}

DispIoDispelCheck* DispatcherSystem::DispIOCheckIoType11(DispIoDispelCheck* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeDispelCheck) return nullptr;
	return dispIo;
}

DispIoD20ActionTurnBased* DispatcherSystem::DispIOCheckIoType12(DispIoD20ActionTurnBased* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeD20ActionTurnBased) return nullptr;
	return dispIo;
}

DispIoMoveSpeed * DispatcherSystem::DispIOCheckIoType13(DispIoMoveSpeed* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeMoveSpeed) return nullptr;
	return dispIo;
}

DispIoMoveSpeed* DispatcherSystem::DispIOCheckIoType13(DispIO* dispIo)
{
	return DispIOCheckIoType13((DispIoMoveSpeed*)dispIo);
}

DispIOBonusListAndSpellEntry* DispatcherSystem::DispIOCheckIoType14(DispIOBonusListAndSpellEntry* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeBonusListAndSpellEntry) return nullptr;
	return dispIo;
}

void DispatcherSystem::PackDispatcherIntoObjFields(objHndl objHnd, Dispatcher* dispatcher)
{
	int numArgs;
	int k;
	int hashkey;
	int numConds;
	int condArgs[64] = {0,};
	const char* name = description.getDisplayName(objHnd);

	k = 0;
	d20Sys.d20SendSignal(objHnd, DK_SIG_Pack, 0, 0);
	objects.PropCollectionRemoveField(objHnd, obj_f_conditions);
	objects.PropCollectionRemoveField(objHnd, obj_f_condition_arg0);
	objects.ClearArrayField(objHnd, obj_f_permanent_mods);
	objects.ClearArrayField(objHnd, obj_f_permanent_mod_data);
	numConds = conds.GetActiveCondsNum(dispatcher);
	for (int i = 0; i < numConds; i++)
	{
		numArgs = conds.ConditionsExtractInfo(dispatcher, i, &hashkey, condArgs);
		objects.setArrayFieldByValue(objHnd, obj_f_conditions, i, hashkey);
		for (int j = 0; j < numArgs; ++k)
		{
			objects.setArrayFieldByValue(objHnd, obj_f_condition_arg0, k, condArgs[j++]);
		}
			
	}
	k = 0;
	numConds = conds.GetPermanentModsAndItemCondCount(dispatcher);
	for (int i = 0; i < numConds; i++)
	{
		numArgs = conds.PermanentAndItemModsExtractInfo(dispatcher, i, &hashkey, condArgs);
		if (hashkey)
		{
			objects.setArrayFieldByValue(objHnd, obj_f_permanent_mods, i, hashkey);
			for (int j = 0; j < numArgs; ++k)
			{
				objects.setArrayFieldByValue(objHnd, obj_f_permanent_mod_data, k, condArgs[j++]);
			}
				
		}
	}
}

int DispatcherSystem::DispatchAttackBonus(objHndl objHnd, objHndl victim, DispIoAttackBonus* dispIo, enum_disp_type dispType, int key)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatcherValid(dispatcher))
		return 0;
	DispIoAttackBonus dispIoLocal;
	DispIoAttackBonus * dispIoToUse = dispIo;
	if (!dispIo)
	{
		dispIoLocal.attackPacket.victim = victim;
		dispIoLocal.attackPacket.attacker = objHnd;
		dispIoLocal.attackPacket.d20ActnType = (D20ActionType)dispType; //  that's the original code, jones...
		dispIoLocal.attackPacket.ammoItem = 0i64;
		dispIoLocal.attackPacket.weaponUsed = 0i64;
		dispIoToUse = &dispIoLocal;
	}
	DispatcherProcessor(dispatcher, dispType, key, dispIoToUse);
	return bonusSys.getOverallBonus(&dispIoToUse->bonlist);

	// return addresses.DispatchAttackBonus(objHnd, victim, dispIo, dispType, key);
}

int DispatcherSystem::DispatchToHitBonusBase(objHndl objHndCaller, DispIoAttackBonus* dispIo)
{
	int key = 0;
	if (dispIo)
	{
		key = dispIo->attackPacket.dispKey;
	}
	return DispatchAttackBonus(objHndCaller, 0i64, dispIo, dispTypeToHitBonusBase, key);
}

int DispatcherSystem::DispatchGetSizeCategory(objHndl obj)
{
	Dispatcher * dispatcher = objects.GetDispatcher(obj);
	DispIoD20Query dispIo; 

	if (dispatcherValid(dispatcher))
	{
		dispIo.dispIOType = dispIOTypeQuery;
		dispIo.return_val = objects.getInt32(obj, obj_f_size);
		dispIo.data1 = 0;
		dispIo.data2 = 0;
		DispatcherProcessor(
			(Dispatcher *)dispatcher, dispTypeGetSizeCategory,	0, &dispIo);
		return dispIo.return_val;
	}
	return 0;
}

void DispatcherSystem::DispatchConditionRemove(Dispatcher* dispatcher, CondNode* cond)
{
	for (auto subDispNode = dispatcher->subDispNodes[dispTypeConditionRemove]; subDispNode; subDispNode = subDispNode->next)
	{
		if (subDispNode->subDispDef->dispKey == 0)
		{
			DispatcherCallbackArgs dca;
			dca.dispIO = nullptr;
			dca.dispType = dispTypeConditionRemove;
			dca.dispKey = 0;
			dca.objHndCaller = dispatcher->objHnd;
			dca.subDispNode = subDispNode;
			subDispNode->subDispDef->dispCallback(dca.subDispNode, dca.objHndCaller, dca.dispType, dca.dispKey, dca.dispIO);
		}
	}

	for (auto subDispNode = dispatcher->subDispNodes[dispTypeConditionRemove2]; subDispNode; subDispNode = subDispNode->next)
	{
		if (subDispNode->subDispDef->dispKey == 0)
		{
			DispatcherCallbackArgs dca;
			dca.dispIO = nullptr;
			dca.dispType = dispTypeConditionRemove2;
			dca.dispKey = 0;
			dca.objHndCaller = dispatcher->objHnd;
			dca.subDispNode = subDispNode;
			subDispNode->subDispDef->dispCallback(dca.subDispNode, dca.objHndCaller, dca.dispType, dca.dispKey, dca.dispIO);
		}
	}
	cond->flags |= 1;
}

unsigned DispatcherSystem::Dispatch35BaseCasterLevelModify(objHndl obj, SpellPacketBody* spellPkt)
{
	auto _dispatcher = objects.GetDispatcher(obj);
	if (!dispatch.dispatcherValid(_dispatcher))
		return 0;
	DispIoD20Query dispIo;
	dispIo.return_val = spellPkt->baseCasterLevel;
	DispatcherProcessor(_dispatcher, dispTypeBaseCasterLevelMod, 0, &dispIo);
	spellPkt->baseCasterLevel = dispIo.return_val;
	return dispIo.return_val;
}

void DispatcherSystem::DispIoDamageInit(DispIoDamage* dispIoDamage)
{
	dispIoDamage->dispIOType = dispIOTypeDamage;
	damage.DamagePacketInit(&dispIoDamage->damage);
	dispIoDamage->attackPacket.attacker=0i64;
	dispIoDamage->attackPacket.victim = 0i64;
	dispIoDamage->attackPacket.dispKey = 0;
	*(int*)&dispIoDamage->attackPacket.flags = 0;
	dispIoDamage->attackPacket.weaponUsed = 0i64;
	dispIoDamage->attackPacket.ammoItem = 0i64;
	dispIoDamage->attackPacket.d20ActnType= D20A_NONE;

}

int32_t DispatcherSystem::DispatchDamage(objHndl objHnd, DispIoDamage* dispIoDamage, enum_disp_type dispType, D20DispatcherKey key)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatch.dispatcherValid(dispatcher)) return 0;
	DispIoDamage * dispIo = dispIoDamage;
	DispIoDamage dispIoLocal;
	if (!dispIoDamage)
	{
		dispatch.DispIoDamageInit(&dispIoLocal);
		dispIo = &dispIoLocal;
	}
	logger->info("Dispatching damage event for {} - type {}, key {}, victim {}", 
		description.getDisplayName(objHnd), dispType, key, description.getDisplayName( dispIoDamage->attackPacket.victim) );
	dispatch.DispatcherProcessor(dispatcher, dispType, key, dispIo);
	return 1;
}

int DispatcherSystem::DispatchD20ActionCheck(D20Actn* d20a, TurnBasedStatus* turnBasedStatus, enum_disp_type dispType)
{
	auto dispatcher = objects.GetDispatcher(d20a->d20APerformer);
	if (dispatch.dispatcherValid(dispatcher))
	{
		DispIoD20ActionTurnBased dispIo;
		dispIo.tbStatus = turnBasedStatus;
		dispIo.dispIOType = dispIOTypeD20ActionTurnBased;
		dispIo.returnVal = 0;
		dispIo.d20a = d20a;
		dispatch.DispatcherProcessor(dispatcher, dispType, d20a->d20ActType + 75, &dispIo);
		return dispIo.returnVal;
	}
	return 0;
}

int DispatcherSystem::Dispatch60GetAttackDice(objHndl obj, DispIoAttackDice* dispIo)
{
	BonusList localBonlist;
	if (!dispIo->bonlist)
		dispIo->bonlist = &localBonlist;

	Dispatcher * dispatcher = objects.GetDispatcher(obj);
	if (!dispatcherValid(dispatcher))
		return 0;
	if (dispIo->weapon)
	{
		int weaponDice = objects.getInt32(dispIo->weapon, obj_f_weapon_damage_dice);
		dispIo->dicePacked = weaponDice;
		dispIo->attackDamageType = (DamageType)objects.getInt32(dispIo->weapon, obj_f_weapon_attacktype);
	}
	DispatcherProcessor(dispatcher, dispTypeGetAttackDice, 0, dispIo);
	int overallBonus = bonusSys.getOverallBonus(dispIo->bonlist);
	Dice diceNew ;
	diceNew = diceNew.FromPacked(dispIo->dicePacked);
	int bonus = diceNew.GetModifier() + overallBonus;
	int diceType = diceNew.GetSides();
	int diceNum = diceNew.GetCount();
	Dice diceNew2(diceNum, diceType, bonus);
	return diceNew.ToPacked();


}
#pragma endregion





#pragma region Dispatcher Functions

void __cdecl _DispatcherRemoveSubDispNodes(Dispatcher * dispatcher, CondNode * cond)
{
	for (uint32_t i = 0; i < dispTypeCount; i++)
	{
		SubDispNode ** ppSubDispNode = &dispatcher->subDispNodes[i];
		while (*ppSubDispNode != nullptr)
		{
			if ((*ppSubDispNode)->condNode == cond)
			{
				SubDispNode * savedNext = (*ppSubDispNode)->next;
				free(*ppSubDispNode);
				*ppSubDispNode = savedNext;

			}
			else
			{
				ppSubDispNode = &((*ppSubDispNode)->next);
			}

		}
	}

	};


void __cdecl _DispatcherClearField(Dispatcher *dispatcher, CondNode ** dispCondList)
{
	CondNode * cond = *dispCondList;
	objHndl obj = dispatcher->objHnd;
	while (cond != nullptr)
	{
		SubDispNode * subDispNode_TypeRemoveCond = dispatcher->subDispNodes[2];
		CondNode * nextCond = cond->nextCondNode;

		while (subDispNode_TypeRemoveCond != nullptr)
		{

			SubDispDef * sdd = subDispNode_TypeRemoveCond->subDispDef;
			if (sdd->dispKey == 0 && (subDispNode_TypeRemoveCond->condNode->flags & 1) == 0
				&& subDispNode_TypeRemoveCond->condNode == cond)
			{
				sdd->dispCallback(subDispNode_TypeRemoveCond, obj, dispTypeConditionRemove, 0, nullptr);
			}
			subDispNode_TypeRemoveCond = subDispNode_TypeRemoveCond->next;
		}
		_DispatcherRemoveSubDispNodes(dispatcher, cond);
		free(cond);
		cond = nextCond;

	}
	*dispCondList = nullptr;
};

void __cdecl _DispatcherClearPermanentMods(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->permanentMods);
};

void __cdecl _DispatcherClearItemConds(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->itemConds);
};

void __cdecl _DispatcherClearConds(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->conditions);
};

DispIoCondStruct * _DispIoCheckIoType1(DispIoCondStruct * dispIo)
{
	return dispatch.DispIoCheckIoType1(dispIo);
}

DispIoBonusList* _DispIoCheckIoType2(DispIoBonusList* dispIo)
{
	return dispatch.DispIoCheckIoType2(dispIo);
}

DispIoSavingThrow* _DispIOCheckIoType3(DispIoSavingThrow* dispIo)
{
	return dispatch.DispIoCheckIoType3(dispIo);
}

DispIoDamage* _DispIoCheckIoType4(DispIoDamage* dispIo)
{
	return dispatch.DispIoCheckIoType4(dispIo);
}

DispIoAttackBonus* _DispIoCheckIoType5(DispIoAttackBonus* dispIo)
{
	return dispatch.DispIoCheckIoType5(dispIo);
}

DispIoD20Signal* _DispIoCheckIoType6(DispIoD20Signal* dispIo)
{
	return dispatch.DispIoCheckIoType6(dispIo);
}

DispIoD20Query* _DispIoCheckIoType7(DispIoD20Query* dispIo)
{
	return dispatch.DispIoCheckIoType7(dispIo);
}

DispIOTurnBasedStatus* _DispIoCheckIoType8(DispIOTurnBasedStatus* dispIo)
{
	return dispatch.DispIoCheckIoType8(dispIo);
}

DispIoTooltip* _DispIoCheckIoType9(DispIoTooltip* dispIo)
{
	return dispatch.DispIoCheckIoType9(dispIo);
}

DispIoBonusAndObj* _DispIoCheckIoType10(DispIoBonusAndObj* dispIo)
{
	return dispatch.DispIoCheckIoType10(dispIo);
}

DispIoDispelCheck* _DispIoCheckIoType11(DispIoDispelCheck* dispIo)
{
	return dispatch.DispIOCheckIoType11(dispIo);
}

DispIoD20ActionTurnBased* _DispIoCheckIoType12(DispIoD20ActionTurnBased* dispIo)
{
	return dispatch.DispIOCheckIoType12(dispIo);
};

DispIOBonusListAndSpellEntry* _DispIoCheckIoType14(DispIOBonusListAndSpellEntry* dispIO)
{
	return dispatch.DispIOCheckIoType14(dispIO);
}

void _PackDispatcherIntoObjFields(objHndl objHnd, Dispatcher* dispatcher)
{
	dispatch.PackDispatcherIntoObjFields(objHnd, dispatcher);
};

void _DispatcherProcessor(Dispatcher* dispatcher, enum_disp_type dispType, uint32_t dispKey, DispIO* dispIO) {
	static uint32_t dispCounter = 0;
	if (dispCounter > DISPATCHER_MAX) {
		logger->info("Dispatcher maximum recursion reached!");
		return;
	}
	dispCounter++;
	/*
	if (dispKey == DK_D20A_TOUCH_ATTACK || dispKey == DK_SIG_TouchAttack)
	{
		int asd = 1;
	}
	*/
	SubDispNode* subDispNode = dispatcher->subDispNodes[dispType];

	while (subDispNode != nullptr) {

		if ((subDispNode->subDispDef->dispKey == dispKey || subDispNode->subDispDef->dispKey == 0) && ((subDispNode->condNode->flags & 1) == 0)) {

			DispIoType21 dispIO20h;
			DispIOType21Init((DispIoType21*)&dispIO20h);
			dispIO20h.condNode = (CondNode *)subDispNode->condNode;

			if (dispKey != 10 || dispType != 62) {
				_Dispatch62(dispatcher->objHnd, (DispIO*)&dispIO20h, 10);
			}

			if (dispIO20h.interrupt == 1 && dispType != dispType63) {
				dispIO20h.interrupt = 0;
				dispIO20h.val2 = 10;
				_Dispatch63(dispatcher->objHnd, (DispIO*)&dispIO20h);
				if (dispIO20h.interrupt == 0) {
					subDispNode->subDispDef->dispCallback(subDispNode, dispatcher->objHnd, dispType, dispKey, (DispIO*)dispIO);
				}
			}
			else {
				subDispNode->subDispDef->dispCallback(subDispNode, dispatcher->objHnd, dispType, dispKey, (DispIO*)dispIO);
			}


		}

		subDispNode = subDispNode->next;
	}

	dispCounter--;
	/*
	if (dispCounter == 0)
	{
		int breakpointDummy = 1;
	}
	*/
	return;

}

int32_t _DispatchDamage(objHndl objHnd, DispIoDamage* dispIo, enum_disp_type dispType, D20DispatcherKey key)
{
	return dispatch.DispatchDamage(objHnd, dispIo, dispType, key);
}

int32_t _dispatch1ESkillLevel(objHndl objHnd, SkillEnum skill, BonusList* bonOut, objHndl objHnd2, int32_t flag)
{
	return dispatch.dispatch1ESkillLevel(objHnd, skill, bonOut, objHnd2, flag);
}


Dispatcher* _DispatcherInit(objHndl objHnd) {
	Dispatcher* dispatcherNew = (Dispatcher *)malloc(sizeof(Dispatcher));
	memset(&dispatcherNew->subDispNodes, 0, dispTypeCount * sizeof(SubDispNode*));
	CondNode* condNode = *(conds.pCondNodeGlobal);
	while (condNode != nullptr) {
		_CondNodeAddToSubDispNodeArray(dispatcherNew, condNode);
		condNode = condNode->nextCondNode;
	}
	dispatcherNew->objHnd = objHnd;
	dispatcherNew->permanentMods = nullptr;
	dispatcherNew->itemConds = nullptr;
	dispatcherNew->conditions = nullptr;
	return dispatcherNew;
};

void DispIOType21Init(DispIoType21* dispIO) {
	dispIO->dispIOType = dispIOType21;
	dispIO->interrupt = 0;
	dispIO->field_8 = 0;
	dispIO->field_C = 0;
	dispIO->SDDKey1 = 0;
	dispIO->val2 = 0;
	dispIO->okToAdd = 0;
	dispIO->condNode = nullptr;
}

void _dispatchTurnBasedStatusInit(objHndl objHnd, DispIOTurnBasedStatus* dispIOtB)
{
	dispatch.dispatchTurnBasedStatusInit(objHnd, dispIOtB);
}

int _DispatchAttackBonus(objHndl objHnd, objHndl victim, DispIoAttackBonus* dispIo, enum_disp_type dispType, int key)
{
	return dispatch.DispatchAttackBonus(objHnd, victim, dispIo, dispType, key);
};


uint32_t _Dispatch62(objHndl objHnd, DispIO* dispIO, uint32_t dispKey) {
	Dispatcher* dispatcher = (Dispatcher *)objects.GetDispatcher(objHnd);
	if (dispatcher != nullptr && (int32_t)dispatcher != -1) {
		_DispatcherProcessor(dispatcher, dispTypeImmunityTrigger, dispKey, dispIO);
	}
	return 0;
}


uint32_t _Dispatch63(objHndl objHnd, DispIO* dispIO) {
	Dispatcher* dispatcher = (Dispatcher *)objects.GetDispatcher(objHnd);
	if (dispatcher != nullptr && (int32_t)dispatcher != -1) {
		_DispatcherProcessor(dispatcher, dispType63, 0, dispIO);
	}
	return 0;
}


#pragma endregion 
DispIoAttackBonus::DispIoAttackBonus()
{
	dispIOType = dispIOTypeAttackBonus;
	bonusSys.initBonusList(&this->bonlist);
	this->attackPacket.victim = 0i64;
	this->attackPacket.attacker = 0i64;
	this->attackPacket.weaponUsed = 0i64;
	this->attackPacket.ammoItem = 0i64;
	this->attackPacket.d20ActnType = D20A_STANDARD_ATTACK;
	this->attackPacket.dispKey = 1;
	this->attackPacket.flags = (D20CAF) 0;
}