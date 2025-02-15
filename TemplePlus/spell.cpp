
#include "stdafx.h"
#include "util/fixes.h"
#include "common.h"
#include "spell.h"
#include "obj.h"
#include "tig/tig_mes.h"
#include "temple_functions.h"
#include "ui\ui_picker.h"
#include "temple_functions.h"
#include "radialmenu.h"
#include "critter.h"
#include <infrastructure/elfhash.h>

static_assert(sizeof(SpellStoreData) == (32U), "SpellStoreData structure has the wrong size!");

struct SpellCondListEntry {
	CondStruct *condition;
	int unknown;
};

static struct SpellAddresses : temple::AddressTable {
	SpellCondListEntry *spellConds;


	void (__cdecl *UpdateSpellPacket)(const SpellPacketBody &spellPktBody);
	uint32_t(__cdecl*GetMaxSpellSlotLevel)(objHndl objHnd, Stat classCode, int casterLvl);


	uint32_t(__cdecl * ConfigSpellTargetting)(PickerArgs* pickerArgs, SpellPacketBody* spellPacketBody);
	int(__cdecl*ParseSpellSpecString)(SpellStoreData* spellStoreData, char* spellString);

	SpellAddresses() {
		rebase(UpdateSpellPacket, 0x10075730);
		rebase(GetMaxSpellSlotLevel, 0x100765B0);
		rebase(ParseSpellSpecString, 0x10078F20);

		rebase(ConfigSpellTargetting, 0x100B9690);

		rebase(spellConds, 0x102E2600);
		
	}

	
} addresses;

IdxTableWrapper<SpellEntry> spellEntryRegistry(0x10AAF428);
IdxTableWrapper<SpellPacket> spellsCastRegistry(0x10AAF218);

class SpellFuncReplacements : public TempleFix {
public:
	const char* name() override {
		return "Expand the range of usable spellEnums. Currently walled off at 802.";
	}

	void apply() override {
		// writeHex(0x100779DE + 2, "A0 0F"); // this prevents the crash from casting from scroll, but it fucks up normal spell casting... (can't go to radial menu to cast!)
		replaceFunction(0x100FDEA0, _getWizSchool);
		replaceFunction(0x100779A0, _getSpellEnum); 
		replaceFunction(0x100762D0, _spellKnownQueryGetData); 
		replaceFunction(0x10076190, _spellMemorizedQueryGetData); 
		replaceFunction(0x1007A140, _spellCanCast); 
		replaceFunction(0x100754B0, _spellRegistryCopy); 
		replaceFunction(0x10075660, _GetSpellEnumFromSpellId); 
		replaceFunction(0x100756E0, _GetSpellPacketBody); 
		replaceFunction(0x100F1010, _SetSpontaneousCastingAltNode);
	}
} spellFuncReplacements;


SpontCastSpellLists spontCastSpellLists;

//temple::GlobalPrimitive<uint16_t>
//1028D09C


class SpellHostilityFlagFix : public TempleFix {
public:
	const char* name() override {
		return "Spell Hostility bug: fix mass cure spells triggering hostile reaction. Can be expanded to other spells.";
	}

	void apply() override {
		writeHex(0x10076EF4, "02"); // Cure Light Wounds, Mass
		writeHex(0x10076F42, "02"); // Mass Heal
		writeHex(0x10077058, "02 02 02"); // Cure Moderate + Serious + Critical Wounds, Mass
	}
} spellHostilityFlagFix;


#pragma region Spell System Implementation

SpellSystem spellSys;

uint32_t SpellSystem::spellRegistryCopy(uint32_t spellEnum, SpellEntry* spellEntry)
{
	return spellEntryRegistry.copy(spellEnum, spellEntry);
}

uint32_t SpellSystem::ConfigSpellTargetting(PickerArgs* pickerArgs, SpellPacketBody* spellPktBody)
{
	return addresses.ConfigSpellTargetting(pickerArgs, spellPktBody);
}

uint32_t SpellSystem::GetMaxSpellSlotLevel(objHndl objHnd, Stat classCode, int casterLvl)
{
	return addresses.GetMaxSpellSlotLevel(objHnd, classCode, casterLvl);
}

int SpellSystem::ParseSpellSpecString(SpellStoreData* spell, char* spellString)
{
	return addresses.ParseSpellSpecString(spell, spellString);
}

const char* SpellSystem::GetSpellMesline(uint32_t lineNumber)
{
	MesLine mesLine;
	mesLine.key = lineNumber;
	mesFuncs.GetLine_Safe(*spellMes, &mesLine);
	return mesLine.value;
}

const char* SpellSystem::GetSpellEnumTAG(uint32_t spellEnum)
{
	MesLine mesLine;
	mesLine.key = spellEnum + 20000;
	mesFuncs.GetLine_Safe(*spellEnumMesHandle, &mesLine);
	return mesLine.value;
}

void SpellSystem::SetSpontaneousCastingAltNode(objHndl obj, int nodeIdx, SpellStoreData* spellData)
{
	auto spellClassCode = spellData->classCode;
	if (isDomainSpell(spellClassCode))
		return;
	auto castingClassCode = GetCastingClass(spellClassCode);
	if (castingClassCode == stat_level_cleric)
	{
		RadialMenuEntry radEntry;
		radEntry.SetDefaults();
		d20Sys.D20ActnSetSpellData(&radEntry.d20SpellData, spellData->spellEnum, spellData->classCode, spellData->spellLevel, 0xFF, spellData->metaMagicData);

		radEntry.type = RadialMenuEntryType::Action;
		radEntry.minArg = 0;
		radEntry.maxArg = 0;
		radEntry.actualArg = 0;
		radEntry.d20ActionType = D20A_CAST_SPELL;
		radEntry.d20ActionData1 = 0;
		auto alignmentChoice = objects.getInt32(obj, obj_f_critter_alignment_choice);
		auto spellLevel = spellData->spellLevel;
		if (alignmentChoice == 1) // good aligned
		{
			if (spellLevel <= 9)
				radEntry.text = (char*)GetSpellMesline(spontCastSpellLists.spontCastSpellsGoodCleric[spellLevel]);
			d20Sys.D20ActnSetSetSpontCast(&radEntry.d20SpellData, SpontCastType::spontCastGoodCleric);
		} else
		{
			if (spellLevel <= 9)
				radEntry.text = (char*)GetSpellMesline(spontCastSpellLists.spontCastSpellsEvilCleric[spellLevel]);
			d20Sys.D20ActnSetSetSpontCast(&radEntry.d20SpellData, SpontCastType::spontCastEvilCleric);
		}
		radEntry.helpId = ElfHash::Hash("TAG_CLASS_FEATURES_CLERIC_SPONTANEOUS_CASTING");
		
		radialMenus.SetMorphsTo(obj, nodeIdx, radialMenus.AddRootNode(obj, &radEntry));
	} 
	else if (castingClassCode == stat_level_druid && spellData->spellLevel > 0)
	{
		RadialMenuEntry radEntry;
		radEntry.SetDefaults();
		auto spellLevel = spellData->spellLevel;
		if (spellLevel > 9)
			spellLevel = 9;
		auto radOptionsMesLine = spontCastSpellLists.spontCastSpellsDruidSummons[spellLevel]; // mes line inside spells_radial_menu_options
		auto druidSpontSpell = spontCastSpellLists.spontCastSpellsDruid[spellLevel];
		radEntry.text = (char*)GetSpellMesline(druidSpontSpell);
		int parentIdx = radialMenus.AddRootParentNode(obj, &radEntry );
		radialMenus.SetMorphsTo(obj, nodeIdx, parentIdx);
		MesLine mesLine;
		mesLine.key = radOptionsMesLine;
		mesFuncs.GetLine_Safe(*spellsRadialMenuOptionsMes, &mesLine);
		auto numSummons = atol(mesLine.value);
		for (int i = 1; i <= numSummons; i++ )
		{
			mesLine.key = i + radOptionsMesLine;
			mesFuncs.GetLine_Safe(*spellsRadialMenuOptionsMes, &mesLine);
			auto protoNum = atol(mesLine.value);
			auto protoHandle = objects.GetProtoHandle(protoNum);
			mesLine.key = objects.getInt32(protoHandle, obj_f_description);
			mesFuncs.GetLine_Safe(*description.descriptionMes, &mesLine);
			
			radEntry.SetDefaults();
			radEntry.text = (char*)mesLine.value;
			radEntry.d20ActionType = D20A_CAST_SPELL;
			radEntry.d20ActionData1 = 0;
			radialMenus.SetCallbackCopyEntryToSelected(&radEntry);
			radEntry.minArg = protoNum;
			d20Sys.D20ActnSetSpellData(&radEntry.d20SpellData, spellData->spellEnum, spellData->classCode, spellData->spellLevel, 0xFF, spellData->metaMagicData);
			d20Sys.D20ActnSetSetSpontCast(&radEntry.d20SpellData, SpontCastType::spontCastDruid);
			radEntry.helpId = ElfHash::Hash(GetSpellEnumTAG(spellData->spellEnum));
			radialMenus.AddChildNode(obj, &radEntry, parentIdx);
		}
	}
}

uint32_t SpellSystem::getBaseSpellCountByClassLvl(uint32_t classCode, uint32_t classLvl, uint32_t slotLvl, uint32_t unknown1)
{
	__asm{
		// ecx - classLvl
		// eax - slotLvl
		// edx - unknown?
		push esi;
		push ecx;
		mov ecx, this;
		mov esi, [ecx]._getSpellCountByClassLvl;
		mov eax, classCode;
		push eax;
		mov eax, slotLvl;
		mov ecx, classLvl;
		mov edx , unknown1
		call esi;
		add esp, 4;
		pop ecx;
		pop esi;
	}
}

uint32_t SpellSystem::getWizSchool(objHndl objHnd)
{
	return ( objects.getInt32(objHnd, obj_f_critter_school_specialization) & 0x000000FF );
}

uint32_t SpellSystem::getStatModBonusSpellCount(objHndl objHnd, uint32_t classCode, uint32_t slotLvl)
{
	uint32_t objHndLSB = (uint32_t)objHnd;
	uint32_t objHndMSB = (uint32_t)(objHnd >> 32);
	uint32_t result = 0;
	__asm{
		// esi - slotLvl
		// eax - classCode
		push edi;
		push edx;
		push esi;
		push ecx;
		mov ecx, this;
		mov edi, [ecx]._getStatModBonusSpellCount;
		mov eax, objHndMSB;
		mov edx, objHndLSB;
		push eax;
		push edx;
		mov eax, classCode;
		mov esi, slotLvl;
		call edi;
		add esp, 8;
		mov result, eax;
		pop edi;
		pop edx;
		pop esi;
		pop ecx;
	}
	return result;
}

void SpellSystem::spellPacketBodyReset(SpellPacketBody* spellPktBody)
{
	_spellPacketBodyReset(spellPktBody);
}

void SpellSystem::spellPacketSetCasterLevel(SpellPacketBody* spellPktBody)
{
	_spellPacketSetCasterLevel(spellPktBody);
}

CondStruct* SpellSystem::GetCondFromSpellIdx(int id) {
	if (id >= 3 && id < 254) {
		return addresses.spellConds[id - 1].condition;
	}
	return nullptr;
}

void SpellSystem::ForgetMemorized(objHndl handle) {
	objects.ClearArrayField(handle, obj_f_critter_spells_memorized_idx);
}

uint32_t SpellSystem::getSpellEnum(const char* spellName)
{
	MesLine mesLine;
	for (auto i = 0; i < SPELL_ENUM_MAX; i++)
	{
		mesLine.key = 5000 + i;
		mesFuncs.GetLine_Safe(*spellEnumMesHandle, &mesLine);
		if (!_stricmp(spellName, mesLine.value))
			return i;
	}
	return 0;
}

uint32_t SpellSystem::GetSpellEnumFromSpellId(uint32_t spellId)
{
	SpellPacket spellPacket;
	if (spellsCastRegistry.copy(spellId, &spellPacket))
	{
		if (spellPacket.isActive == 1)
			return spellPacket.spellPktBody.spellEnum;
	}
	return 0;
}

uint32_t SpellSystem::GetSpellPacketBody(uint32_t spellId, SpellPacketBody* spellPktBodyOut)
{
	SpellPacket spellPkt;
	if (spellsCastRegistry.copy(spellId, &spellPkt))
	{
		memcpy(spellPktBodyOut, &spellPkt.spellPktBody, sizeof(SpellPacketBody));
		return 1;
	}
	return 0;
}

void SpellSystem::UpdateSpellPacket(const SpellPacketBody& spellPktBody) {
	addresses.UpdateSpellPacket(spellPktBody);
}

uint32_t SpellSystem::spellKnownQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count)
{
	uint32_t countLocal;
	uint32_t * n = count;
	if (count == nullptr) n = &countLocal;

	*n = 0;
	uint32_t numSpellsKnown = objects.getArrayFieldNumItems(objHnd, obj_f_critter_spells_known_idx);
	for (uint32_t i = 0; i < numSpellsKnown; i++)
	{
		SpellStoreData spellData;
		objects.getArrayField(objHnd, obj_f_critter_spells_known_idx, i, &spellData);
		if (spellData.spellEnum == spellEnum)
		{
			if (classCodesOut) classCodesOut[*n] = spellData.classCode;
			if (slotLevelsOut) slotLevelsOut[*n] = spellData.spellLevel;
			++*n;
		}
	}
	return *n > 0;
}

uint32_t SpellSystem::spellCanCast(objHndl objHnd, uint32_t spellEnum, uint32_t spellClassCode, uint32_t spellLevel)
{
	uint32_t count = 0;
	uint32_t classCodes[10000];
	uint32_t spellLevels[10000];

	SpellEntry spellEntry;
	if (d20Sys.d20Query(objHnd, DK_QUE_CannotCast) 
		|| !spellEntryRegistry.copy(spellEnum, &spellEntry) ) 
		return 0;
	if (isDomainSpell(spellClassCode)) // domain spell
	{
		if (numSpellsMemorizedTooHigh(objHnd))	return 0;

		spellMemorizedQueryGetData(objHnd, spellEnum, classCodes, spellLevels, &count);
		for (uint32_t i = 0; i < count; i++)
		{
			if ( isDomainSpell(classCodes[i]) 
				&& (classCodes[i] & 0x7F) == ( spellClassCode &0x7F)
				&&  spellLevels[i] == spellLevel)
				return 1;
		}
		return 0;
	}

	if (d20Sys.d20Class->isNaturalCastingClass(spellClassCode & 0x7F))
	{
		if (numSpellsKnownTooHigh(objHnd)) return 0;

		spellKnownQueryGetData(objHnd, spellEnum, classCodes, spellLevels, &count);
		for (int32_t i = 0; i < (int32_t)count; i++)
		{
			if ( !isDomainSpell(classCodes[i])
				&& (classCodes[i] & 0x7F) == (spellClassCode & 0x7F)
				&& spellLevels[i] <= spellLevel)
			{
				if (spellLevels[i] < spellLevel)
					logger->info("Natural Spell Caster spellCanCast check - spell known is lower level than spellCanCast queried spell. Is this ok?? (this is vanilla code here...)");
				return 1;
			}
				
		}
		return 0;
	}

	if (numSpellsMemorizedTooHigh(objHnd)) return 0;

	spellMemorizedQueryGetData(objHnd, spellEnum, classCodes, spellLevels, &count);
	for (uint32_t i = 0; i < count; i++)
	{
		if ( !isDomainSpell(classCodes[i])
			&& (classCodes[i] & 0x7F) == (spellClassCode & 0x7F)
			&& spellLevels[i] == spellLevel)
			return 1;
	}
	return 0;
}

uint32_t SpellSystem::spellMemorizedQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count)
{
	uint32_t countLocal;
	uint32_t * n = count;
	if (count == nullptr) n = &countLocal;

	*n = 0;
	uint32_t numSpellsMemod = objects.getArrayFieldNumItems(objHnd, obj_f_critter_spells_memorized_idx);
	for (int32_t i = 0; i < (int32_t)numSpellsMemod; i++)
	{
		SpellStoreData spellData;
		objects.getArrayField(objHnd, obj_f_critter_spells_memorized_idx, i, &spellData);
		if (spellData.spellEnum == spellEnum && !spellData.spellStoreState.usedUp)
		{
			if (classCodesOut) classCodesOut[*n] = spellData.classCode;
			if (slotLevelsOut) slotLevelsOut[*n] = spellData.spellLevel;
			++*n;
		}
	}
	return *n > 0;
}

bool SpellSystem::numSpellsKnownTooHigh(objHndl objHnd)
{
	if (objects.getArrayFieldNumItems(objHnd, obj_f_critter_spells_known_idx) > MAX_SPELLS_KNOWN)
	{
		logger->info("spellCanCast(): ERROR! This critter knows WAAY too many spells! Returning 0.");
		return 1;
	}
	return 0;
}

bool SpellSystem::numSpellsMemorizedTooHigh(objHndl objHnd)
{
	if (objects.getArrayFieldNumItems(objHnd, obj_f_critter_spells_memorized_idx) > MAX_SPELLS_KNOWN)
	{
		logger->info("spellCanCast(): ERROR! This critter memorized WAAY too many spells! Returning 0.");
		return 1;
	}
	return 0;
}

bool SpellSystem::isDomainSpell(uint32_t spellClassCode)
{
	if (spellClassCode & 0x80) return 0;
	return 1;
}

Stat SpellSystem::GetCastingClass(uint32_t spellClassCode)
{
	return (Stat)(spellClassCode & 0x7F);
}

uint32_t SpellSystem::pickerArgsFromSpellEntry(SpellEntry* spellEntry, PickerArgs * pickArgs, objHndl objHnd, uint32_t casterLvl)
{
	return _pickerArgsFromSpellEntry(spellEntry, pickArgs, objHnd, casterLvl);
}

uint32_t SpellSystem::GetSpellRangeExact(SpellRangeType spellRangeType, uint32_t casterLevel, objHndl caster)
{
	switch (spellRangeType)
	{
	case SpellRangeType::SRT_Personal:
		return 5;
	case SpellRangeType::SRT_Touch:
		return critterSys.GetReach(caster, D20A_TOUCH_ATTACK);
	case SpellRangeType::SRT_Close:
		return (casterLevel / 2 + 5) * 5;
	case SpellRangeType::SRT_Medium:
		return 2 * (5*casterLevel + 50);
	case SpellRangeType::SRT_Long:
		return 8 * (5 * casterLevel + 50);
	case SpellRangeType::SRT_Special_Inivibility_Purge:
		return 5 * casterLevel;
	default:
		logger->warn("GetSpellRangeExact: unknown range specified for spell entry\n");
		break;
	}
	return 0;
}

uint32_t SpellSystem::GetSpellRange(SpellEntry* spellEntry, uint32_t casterLevel, objHndl caster)
{
	auto spellRangeType = spellEntry->spellRangeType;
	if (spellRangeType == SpellRangeType::SRT_Specified)
		return spellEntry->spellRange;
	return GetSpellRangeExact(spellRangeType, casterLevel, caster);
}
#pragma endregion



#pragma region Hooks

uint32_t __cdecl _getWizSchool(objHndl objHnd)
{
	return spellSys.getWizSchool(objHnd);
}

uint32_t __cdecl _getSpellEnum(const char* spellName)
{
	return spellSys.getSpellEnum(spellName);
}

uint32_t _spellKnownQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count)
{
	return spellSys.spellKnownQueryGetData(objHnd, spellEnum, classCodesOut, slotLevelsOut, count);
}

uint32_t _spellMemorizedQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count)
{
	return spellSys.spellMemorizedQueryGetData(objHnd, spellEnum, classCodesOut, slotLevelsOut, count);
}

uint32_t _spellCanCast(objHndl objHnd, uint32_t spellEnum, uint32_t spellClassCode, uint32_t spellLevel)
{
	return spellSys.spellCanCast(objHnd, spellEnum, spellClassCode, spellLevel);
}

uint32_t _spellRegistryCopy(uint32_t spellEnum, SpellEntry* spellEntry)
{
	return spellSys.spellRegistryCopy(spellEnum, spellEntry);
}

uint32_t _GetSpellEnumFromSpellId(uint32_t spellId)
{
	return spellSys.GetSpellEnumFromSpellId(spellId);
}

uint32_t _GetSpellPacketBody(uint32_t spellId, SpellPacketBody* spellPktBodyOut)
{
	return spellSys.GetSpellPacketBody(spellId, spellPktBodyOut);
}

void _SetSpontaneousCastingAltNode(objHndl obj, int nodeIdx, SpellStoreData* spellData)
{
	spellSys.SetSpontaneousCastingAltNode(obj, nodeIdx, spellData);
}
#pragma endregion 
