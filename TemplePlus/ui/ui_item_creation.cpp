
#include "stdafx.h"
#include "ui.h"
#include "util/config.h"
#include "tig/tig_mes.h"
#include "tig/tig_font.h"
#include "temple_functions.h"
#include "tig/tig_tokenizer.h"
#include "ui_item_creation.h"
#include "combat.h"
#include "obj.h"
//#include "radial_menu.h"
#include "radialmenu.h"
#include "common.h"
#include "ui_render.h"
#include <python/python_integration_obj.h>
#include <python/python_object.h>
#include <condition.h>
#include <critter.h>
#include <util/fixes.h>
//#include <condition.h>

temple::GlobalPrimitive<ItemCreationType, 0x10BEDF50> itemCreationType;
temple::GlobalPrimitive<objHndl, 0x10BECEE0> globObjHndCrafter;

temple::GlobalPrimitive<int32_t, 0x10BEE3A4> craftInsufficientXP;
temple::GlobalPrimitive<int32_t, 0x10BEE3A8> craftInsufficientFunds;
temple::GlobalPrimitive<int32_t, 0x10BEE3AC> craftSkillReqNotMet;
temple::GlobalPrimitive<int32_t, 0x10BEE3B0> dword_10BEE3B0;

temple::GlobalPrimitive<char *, 0x10BED6D4> itemCreationUIStringSkillRequired;
temple::GlobalPrimitive<char *, 0x10BEDB50> itemCreationUIStringItemCost;
temple::GlobalPrimitive<char *, 0x10BED8A4> itemCreationUIStringXPCost;
temple::GlobalPrimitive<char *, 0x10BED8A8> itemCreationUIStringValue;
temple::GlobalPrimitive<TigTextStyle, 0x10BEE338> itemCreationTextStyle; // so far used by "Item Cost: %d" and "Experience Cost: %d"
temple::GlobalPrimitive<TigTextStyle, 0x10BED938> itemCreationTextStyle2; // so far used by "Value: %d"
int WandCraftCostCp=0;

struct UiItemCreationAddresses : temple::AddressTable
{
	int (__cdecl* Sub_100F0200)(objHndl a1, RadialMenuEntry *entry);
	UiItemCreationAddresses()
	{
		rebase(Sub_100F0200, 0x100F0200);
	}
	
} addresses;




static MesHandle mesItemCreationText;
static MesHandle mesItemCreationRules;
static MesHandle mesItemCreationNamesText;
static ImgFile *background = nullptr;
//static ButtonStateTextures acceptBtnTextures;
//static ButtonStateTextures declineBtnTextures;
static int disabledBtnTexture;



int CraftedWandSpellLevel(objHndl objHndItem)
{
	SpellStoreData spellData;
	objects.getArrayField(objHndItem, obj_f_item_spell_idx, 0, &spellData);
	uint32_t spellLevelBasic = spellData.spellLevel;
	uint32_t spellLevelFinal = spellData.spellLevel;

	if (spellData.spellEnum == 25)
	{
		int dummy = 1;
	}
	int casterLevelSet = (int) d20Sys.d20QueryReturnData(globObjHndCrafter, DK_QUE_Craft_Wand_Spell_Level, 0, 0);
	casterLevelSet = 2 * ((casterLevelSet + 1) / 2) - 1;
	if (casterLevelSet < 1)
		casterLevelSet = 1;

	int slotLevelSet = 1 + (casterLevelSet -1)/ 2;
	if (spellLevelBasic == 0 && casterLevelSet <= 1)
		slotLevelSet = 0;
		
	

	// get data from caster - make this optional!

	uint32_t classCodes[SPELL_ENUM_MAX] = { 0, };
	uint32_t spellLevels[SPELL_ENUM_MAX] = { 0, };
	uint32_t spellFoundNum = 0;
	int casterKnowsSpell = spellSys.spellKnownQueryGetData(globObjHndCrafter, spellData.spellEnum, classCodes, spellLevels, &spellFoundNum);
	if (casterKnowsSpell){
		uint32_t spellClassFinal = classCodes[0];
		spellLevelFinal = 0;
		uint32_t isClassSpell = classCodes[0] & (0x80);

		if (isClassSpell){
			spellLevelFinal = spellSys.GetMaxSpellSlotLevel(globObjHndCrafter, static_cast<Stat>(classCodes[0] & 0x7F), 0);
		};
		if (spellFoundNum > 1){
			for (uint32_t i = 1; i < spellFoundNum; i++){
				if (spellLevels[i] > spellLevelFinal){
					spellData.classCode = classCodes[i];
					spellLevelFinal = spellLevels[i];
				}
			}
			spellData.spellLevel = spellLevelFinal;

		}
		spellData.spellLevel = spellLevelFinal; // that's the max possible at this point
		if (slotLevelSet && slotLevelSet <= spellLevelFinal && slotLevelSet >= spellLevelBasic)
			spellData.spellLevel = slotLevelSet;
		else if (slotLevelSet  > spellLevelFinal)
			spellData.spellLevel = spellLevelFinal;
		else if (slotLevelSet < spellLevelBasic)
			spellData.spellLevel = spellLevelBasic;
		else if (spellLevelBasic == 0)
		{
			spellData.spellLevel = spellLevelBasic;
		} 

		//templeFuncs.Obj_Set_IdxField_byPtr(objHndItem, obj_f_item_spell_idx, 0, &spellData);
		spellLevelFinal = spellData.spellLevel;

	}
	return spellLevelFinal;
}

int CraftedWandCasterLevel(objHndl item)
{
	int result = CraftedWandSpellLevel(item);
	if (result <= 1)
		return 1;
	return (result * 2) - 1;
}

int32_t CreateItemResourceCheck(objHndl obj, objHndl objHndItem){
	bool canCraft = 1;
	bool xpCheck = 0;
	int32_t * globInsuffXP = craftInsufficientXP.ptr();
	int32_t * globInsuffFunds = craftInsufficientFunds.ptr();
	int32_t *globSkillReqNotMet = craftSkillReqNotMet.ptr();
	int32_t *globB0 = dword_10BEE3B0.ptr();
	uint32_t crafterLevel = objects.StatLevelGet(obj, stat_level);
	uint32_t minXPForCurrentLevel = templeFuncs.XPReqForLevel(crafterLevel); 
	uint32_t crafterXP = templeFuncs.Obj_Get_Field_32bit(obj, obj_f_critter_experience);
	uint32_t surplusXP = crafterXP - minXPForCurrentLevel;
	uint32_t craftingCostCP = 0;
	uint32_t partyMoney = templeFuncs.PartyMoney();

	*globInsuffXP = 0;
	*globInsuffFunds = 0;
	*globSkillReqNotMet = 0;
	*globB0 = 0;
	int itemWorth = objects.getInt32(objHndItem, obj_f_item_worth);

	
	// Check GP Section
	if (itemCreationType == ItemCreationType::CraftMagicArmsAndArmor){
		craftingCostCP = templeFuncs.ItemWorthFromEnhancements( 41 );
	}
	else
	{
		// current method for crafting stuff:
		craftingCostCP =  itemWorth / 2;

		// TODO: create new function
		if (itemCreationType == ItemCreationType::CraftWand)
		{

			itemWorth = ItemWorthAdjustedForCasterLevel(objHndItem, CraftedWandCasterLevel(objHndItem));
			craftingCostCP = itemWorth / 2;
		}
			
	};

	if ( ( (uint32_t)partyMoney ) < craftingCostCP){
		*globInsuffFunds = 1;
		canCraft = 0;
	};


	// Check XP section (and maybe spell prerequisites too? explore sub_10152280)
	if ( itemCreationType != CraftMagicArmsAndArmor){
		if ( templeFuncs.sub_10152280(obj, objHndItem) == 0){ //TODO explore function
			*globB0 = 1;
			canCraft = 0;
		};

		// TODO make XP cost calculation take applied caster level into account
		uint32_t itemXPCost = itemWorth / 2500;
		xpCheck = surplusXP > itemXPCost;
	} else 
	{
		uint32_t magicArmsAndArmorXPCost = templeFuncs.CraftMagicArmsAndArmorSthg(41);
		xpCheck = surplusXP > magicArmsAndArmorXPCost;
	}
		
	if (xpCheck){
		return canCraft;
	} else
	{
		*globInsuffXP = 1;
		return 0;
	};

};

void CraftScrollWandPotionSetItemSpellData(objHndl objHndItem, objHndl objHndCrafter){

	// the new and improved Wands/Scroll Property Setting Function

	if (itemCreationType == CraftWand){
		
		SpellStoreData spellData;
		objects.getArrayField(objHndItem, obj_f_item_spell_idx, 0, &spellData);
		uint32_t spellLevelBasic = spellData.spellLevel;
		uint32_t spellLevelFinal = CraftedWandSpellLevel(objHndItem);
		spellData.spellLevel = spellLevelFinal;

		templeFuncs.Obj_Set_IdxField_byPtr(objHndItem, obj_f_item_spell_idx, 0, &spellData);
		
		auto args = PyTuple_New(3);
			
		int casterLevelFinal = spellLevelFinal * 2 - 1;
		if (casterLevelFinal < 1)
			casterLevelFinal = 1;
		PyTuple_SET_ITEM(args, 0, PyObjHndl_Create(objHndItem));
		PyTuple_SET_ITEM(args, 1, PyObjHndl_Create(objHndCrafter));
		PyTuple_SET_ITEM(args, 2, PyInt_FromLong(casterLevelFinal));

		auto wandNameObj = pythonObjIntegration.ExecuteScript("crafting", "craft_wand_new_name", args);
		char * wandNewName = PyString_AsString(wandNameObj);
			
		objects.setInt32(objHndItem, obj_f_description, objects.description.CustomNameNew(wandNewName)); // TODO: create function that appends effect of caster level boost			
		objects.setInt32(objHndItem, obj_f_item_spell_charges_idx, 50); // as god intended it to be!
		Py_DECREF(wandNameObj);
		Py_DECREF(args);
		
		return;

	}
	if (itemCreationType == ScribeScroll){
		// do scroll specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Scroll of LOL"));
	};

	if (itemCreationType == BrewPotion){
		// do potion specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Potion of Commotion"));
		// TODO: change it so it's 0xBAAD F00D just like spawned / mobbed potions
	};
	
	int numItemSpells = objects.getArrayFieldNumItems(objHndItem, obj_f_item_spell_idx);

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	// Current code - change this at will...
	for (int n = 0; n < numItemSpells; n++){
		SpellStoreData spellData;
		objects.getArrayField(objHndItem, obj_f_item_spell_idx, n, &spellData);

		// get data from caster - make this optional!

		uint32_t classCodes[SPELL_ENUM_MAX] = { 0, };
		uint32_t spellLevels[SPELL_ENUM_MAX] = { 0, };
		uint32_t spellFoundNum = 0;
		int casterKnowsSpell = spellSys.spellKnownQueryGetData(objHndCrafter, spellData.spellEnum, classCodes, spellLevels, &spellFoundNum);
		if (casterKnowsSpell){
			uint32_t spellClassFinal = classCodes[0];
			uint32_t spellLevelFinal = 0;
			uint32_t isClassSpell = classCodes[0] & (0x80);

			if (isClassSpell){
				spellLevelFinal = spellSys.GetMaxSpellSlotLevel(objHndCrafter, static_cast<Stat>(classCodes[0] & 0x7F), 0);
			};
			if (spellFoundNum > 1){
				for (uint32_t i = 1; i < spellFoundNum; i++){
					if (spellLevels[i] > spellLevelFinal){
						spellData.classCode = classCodes[i];
						spellLevelFinal = spellLevels[i];
					}
				}
				spellData.spellLevel = spellLevelFinal;

			}
			spellData.spellLevel = spellLevelFinal;
			templeFuncs.Obj_Set_IdxField_byPtr(objHndItem, obj_f_item_spell_idx, n, &spellData);

		}

	}
};


void CreateItemDebitXPGP(objHndl objHndCrafter, objHndl objHndItem){
	uint32_t crafterXP = objects.getInt32(objHndCrafter, obj_f_critter_experience);
	uint32_t craftingCostCP = 0;
	uint32_t craftingCostXP = 0;

	if (itemCreationType == CraftMagicArmsAndArmor){ // magic arms and armor
		craftingCostCP = templeFuncs.ItemWorthFromEnhancements(41);
		craftingCostXP = templeFuncs.CraftMagicArmsAndArmorSthg(41);
	}
	else
	{
		// TODO make crafting costs take applied caster level into account
		// currently this is what ToEE does	
		int itemWorth;
		if (itemCreationType == ItemCreationType::CraftWand)
			itemWorth = ItemWorthAdjustedForCasterLevel(objHndItem, CraftedWandCasterLevel(objHndItem));
		else
			itemWorth = objects.getInt32(objHndItem, obj_f_item_worth);
		craftingCostCP = itemWorth / 2;
		craftingCostXP = itemWorth / 2500;

	};

	templeFuncs.DebitPartyMoney(0, 0, 0, craftingCostCP);
	objects.setInt32(objHndCrafter, obj_f_critter_experience, crafterXP - craftingCostXP);
};

void __cdecl UiItemCreationCraftingCostTexts(objHndl objHndItem){
	// prolog
	int32_t widgetId;
	int32_t * globInsuffXP;
	int32_t * globInsuffFunds;
	int32_t *globSkillReqNotMet;
	int32_t *globB0;
	uint32_t craftingCostCP;
	uint32_t craftingCostXP;
	TigRect rect;
	char * prereqString;
	__asm{
		mov widgetId, ebx; // widgetId is passed in ebx
	};


	uint32_t casterLevelNew = -1; // h4x!
	

	if (itemCreationType == CraftWand)
	{
		casterLevelNew = CraftedWandCasterLevel(objHndItem);
	}
	

	rect.x = 212;
	rect.y = 157;
	rect.width = 159;
	rect.height = 10;

	globInsuffXP = craftInsufficientXP.ptr();
	globInsuffFunds = craftInsufficientFunds.ptr();
	globSkillReqNotMet = craftSkillReqNotMet.ptr();
	globB0 = dword_10BEE3B0.ptr();

	//old method
	/* 
	craftingCostCP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2;
	craftingCostXP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2500;
	*/

	craftingCostCP = ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 2;
	craftingCostXP = ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 2500;
	
	string text;
	// "Item Cost: %d"
	if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
		
		text = format("{} @{}{}", *itemCreationUIStringItemCost.ptr(), *(globInsuffFunds)+1, craftingCostCP / 100);
	} else {
		//_snprintf(text, 128, "%s @3%d", itemCreationUIStringItemCost.ptr(), craftingCostCP / 100);
		text = format("{} @3{}", *itemCreationUIStringItemCost.ptr(), craftingCostCP / 100);
	};


	UiRenderer::DrawTextInWidget(widgetId, text, rect, itemCreationTextStyle);
	rect.y += 11;



	// "Experience Cost: %d"  (or "Skill Req: " for alchemy - WIP)

	if (itemCreationType == IC_Alchemy){ 
		// placeholder - they do similar bullshit in the code :P but I guess it can be modified easily enough!
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			text = format("{} @{}{}", *itemCreationUIStringSkillRequired.ptr(), *globSkillReqNotMet + 1, craftingCostXP);
		}
		else {
			text = format("{} @3{}", *itemCreationUIStringSkillRequired.ptr(), craftingCostXP);
		};
	}
	else
	{
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			text = format("{} @{}{}", *itemCreationUIStringXPCost.ptr(), *(globInsuffXP) + 1, craftingCostXP);
		}
		else {
			text = format("{} @3{}", *itemCreationUIStringXPCost.ptr(), craftingCostXP);
		};
	};

	UiRenderer::DrawTextInWidget(widgetId, text, rect, itemCreationTextStyle);
	rect.y += 11;

	// "Value: %d"
	//_snprintf(text, 128, "%s @1%d", * (itemCreationUIStringValue.ptr() ), templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 100);
	text = format("{} @1{}", *itemCreationUIStringValue.ptr(), ItemWorthAdjustedForCasterLevel(objHndItem, casterLevelNew) / 100);

	UiRenderer::DrawTextInWidget(widgetId, text, rect, itemCreationTextStyle2);

	// Prereq: %s
	rect.x = 210;
	rect.y = 200;
	rect.width = 150;
	rect.height = 105;
	prereqString = templeFuncs.ItemCreationPrereqSthg_sub_101525B0(globObjHndCrafter, objHndItem);
	if (prereqString){
		UiRenderer::DrawTextInWidget(widgetId, prereqString, rect, itemCreationTextStyle);
	}
	
	if (itemCreationType == ItemCreationType::CraftWand)
	{
		rect.x = 210;
		rect.y = 250;
		rect.width = 150;
		rect.height = 105;
		char asdf[1000];
		sprintf(asdf, "Crafted Caster Level: " );
		
		text = format("{} @3{}", asdf, casterLevelNew);
		if (prereqString){
			UiRenderer::DrawTextInWidget(widgetId, text, rect, itemCreationTextStyle);
		}
	}
	
	
};


uint32_t ItemWorthAdjustedForCasterLevel(objHndl objHndItem, uint32_t casterLevelNew){
	uint32_t numItemSpells = objects.getArrayFieldNumItems(objHndItem, obj_f_item_spell_idx);
	uint32_t itemWorthBase = objects.getInt32(objHndItem, obj_f_item_worth);
	if (casterLevelNew == -1){
		return itemWorthBase;
	}

	uint32_t itemSlotLevelBase = 0;

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	for (uint32_t n = 0; n < numItemSpells; n++){
		SpellStoreData spellData;
		templeFuncs.Obj_Get_ArrayElem_Generic(objHndItem, obj_f_item_spell_idx, n, &spellData);
		if (spellData.spellLevel > itemSlotLevelBase){
			itemSlotLevelBase = spellData.spellLevel;
		}
	};

	int casterLevelOld = itemSlotLevelBase * 2 - 1;
	if (casterLevelOld < 1)
		casterLevelOld = 1;

	if (itemSlotLevelBase == 0 && casterLevelNew > casterLevelOld){
		return itemWorthBase * casterLevelNew;
	}
	if (casterLevelNew > casterLevelOld)
	{
		return itemWorthBase * casterLevelNew / casterLevelOld;
	}
	return itemWorthBase;

	
}

static vector<uint64_t> craftingProtoHandles[8];

const char *getProtoName(uint64_t protoHandle) {
	/*
	 // gets item creation proto id
  if ( sub_1009C950((objHndl)protoHandle) )
    v1 = sub_100392E0(protoHandle);
  else
    v1 = sub_10039320((objHndl)protoHandle);

  line.key = v1;
  if ( tig_mes_get_line(ui_itemcreation_names, &line) )
    result = line.value;
  else
    result = objects.description._getDisplayName((objHndl)protoHandle, (objHndl)protoHandle);
  return result;
  */

	return objects.description._getDisplayName(protoHandle, protoHandle);
}

static void loadProtoIds(MesHandle mesHandle) {

	for (uint32_t i = 0; i < 8; ++i) {
		auto protoLine = mesFuncs.GetLineById(mesHandle, i);
		if (!protoLine) {
			continue;
		}

		auto &protoHandles = craftingProtoHandles[i];

		StringTokenizer tokenizer(protoLine);
		while (tokenizer.next()) {
			auto handle = templeFuncs.GetProtoHandle(tokenizer.token().numberInt);
			protoHandles.push_back(handle);
		}

		// Sort by prototype name
		sort(protoHandles.begin(), protoHandles.end(), [](uint64_t a, uint64_t b)
		{
			auto nameA = getProtoName(a);
			auto nameB = getProtoName(b);
			return _strcmpi(nameA, nameB);
		});
		logger->info("Loaded {} prototypes for crafting type {}", craftingProtoHandles[i].size(), i);
	}
	
}




uint32_t ItemCreationBuildRadialMenuEntry(DispatcherCallbackArgs args, ItemCreationType itemCreationType, char* helpSystemString, MesHandle combatMesLine)
{
	if (combatSys.isCombatActive()) { return 0; }
	MesLine mesLine;
	RadialMenuEntry radmenu;
	radmenu.SetDefaults();
	mesLine.key = combatMesLine;
	mesFuncs.GetLine_Safe(*combatSys.combatMesfileIdx, &mesLine);
	//RadialMenuStructInit(&radmenu);
	radmenu.text = (char*)mesLine.value;
	//radmenu.field0 = (void*)mesLine.value;
	
	radmenu.d20ActionType = D20A_ITEM_CREATION;
	//radmenu.field20 = 37;

	radmenu.d20ActionData1 = itemCreationType;
	//radmenu.field24 = itemCreationType;

	radmenu.helpId = templeFuncs.StringHash(helpSystemString);
	//radmenu.field40 = templeFuncs.StringHash(helpSystemString);

	radialMenus.AddChildNode(args.objHndCaller, &radmenu, radialMenus.GetStandardNode(RadialMenuStandardNode::Feats));
	//radialFuncs.RadialMenuCreateEntry(args.objHndCaller,  &radmenu , radialFuncs.RadialMenuArgMap_sub_100F12B0(3) ) ;

	return 0;
};

#pragma region ItemCreation Radial Menu Dispatcher Callbacks

uint32_t BrewPotionRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, BrewPotion, "TAG_BREW_POTION", 5066);
};

uint32_t ScribeScrollRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, ScribeScroll, "TAG_SCRIBE_SCROLL", 5067);
};

uint32_t CraftWandRadialMenu(DispatcherCallbackArgs args)
{
	
	if (combatSys.isCombatActive()) { return 0; }
	MesLine mesLine;
	RadialMenuEntry radMenuCraftWand;
	radMenuCraftWand.SetDefaults();
	mesLine.key = 5068;
	mesFuncs.GetLine_Safe(*combatSys.combatMesfileIdx, &mesLine);
	radMenuCraftWand.text = (char*)mesLine.value;
	radMenuCraftWand.d20ActionType = D20A_ITEM_CREATION;
	radMenuCraftWand.d20ActionData1 = CraftWand;
	radMenuCraftWand.helpId = templeFuncs.StringHash("TAG_CRAFT_WAND");
	
	int newParent = radialMenus.AddParentChildNode(args.objHndCaller, &radMenuCraftWand, radialMenus.GetStandardNode(RadialMenuStandardNode::Feats));

	RadialMenuEntry useCraftWand;
	RadialMenuEntry setWandLevel;
	
	
	setWandLevel.SetDefaults();
	setWandLevel.minArg = 1;
	setWandLevel.maxArg = min(20, critterSys.GetCasterLevel(args.objHndCaller));
	
	setWandLevel.field4 = (int)combatSys.GetCombatMesLine(6019);
	setWandLevel.type = RadialMenuEntryType::Slider;
	setWandLevel.actualArg = (int)conds.CondNodeGetArgPtr(args.subDispNode->condNode, 0);
	setWandLevel.callback = (void (__cdecl*)(objHndl, RadialMenuEntry*))addresses.Sub_100F0200;
	setWandLevel.text = combatSys.GetCombatMesLine(6017);
	setWandLevel.helpId = templeFuncs.StringHash("TAG_CRAFT_WAND");
	radialMenus.AddChildNode(args.objHndCaller, &setWandLevel, newParent);

	useCraftWand.SetDefaults();
	useCraftWand.type = RadialMenuEntryType::Action;
	useCraftWand.text = combatSys.GetCombatMesLine(6018);
	useCraftWand.helpId = templeFuncs.StringHash("TAG_CRAFT_WAND");
	useCraftWand.d20ActionType = D20A_ITEM_CREATION;
	useCraftWand.d20ActionData1 = CraftWand;
	radialMenus.AddChildNode(args.objHndCaller, &useCraftWand, newParent);



	return 0;
	
	//return ItemCreationBuildRadialMenuEntry(args, CraftWand, "TAG_CRAFT_WAND", 5068);
};

uint32_t CraftWandOnAdd(DispatcherCallbackArgs args)
{
	//vector< int> condArgs(2);
	//condArgs[0] = 1;
	//condArgs[1] = 0;
	conds.AddTo(args.objHndCaller, "Craft Wand Level Set", { 1, 0 });
	//Dispatcher * dispatcher = objects.GetDispatcher(args.objHndCaller);
	//CondStruct * condStruct = conds.hashmethods.GetCondStruct(conds.hashmethods.StringHash("Craft Wand Level Set") );
	//conds.ConditionAddDispatchArgs(dispatcher, &dispatcher->conditions, condStruct, condArgs);
	return 0;
}

uint32_t CraftRodRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftRod, "TAG_CRAFT_ROD", 5069);
};

uint32_t CraftWondrousRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftWondrous, "TAG_CRAFT_WONDROUS", 5070);
};

uint32_t CraftStaffRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftStaff, "TAG_CRAFT_STAFF", 5103);
};

uint32_t ForgeRingRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, ForgeRing, "TAG_FORGE_RING", 5104);
};

uint32_t CraftMagicArmsAndArmorRadialMenu(DispatcherCallbackArgs args)
{
	return ItemCreationBuildRadialMenuEntry(args, CraftMagicArmsAndArmor, "TAG_CRAFT_MAA", 5071);
};

#pragma endregion
/*
static int __cdecl systemInit(const GameSystemConf *conf) {

	mesFuncs.Open("mes\\item_creation.mes", &mesItemCreationText);
	mesFuncs.Open("mes\\item_creation_names.mes", &mesItemCreationNamesText);
	mesFuncs.Open("rules\\item_creation.mes", &mesItemCreationRules);
	loadProtoIds(mesItemCreationRules);

	acceptBtnTextures.loadAccept();
	declineBtnTextures.loadDecline();
	ui.GetAsset(UiAssetType::Generic, UiGenericAsset::DisabledNormal, disabledBtnTexture);

	background = ui.LoadImg("art\\interface\\item_creation_ui\\item_creation.img");

	// TODO !sub_10150F00("rules\\item_creation.mes")

	/*
	tig_texture_register("art\\interface\\item_creation_ui\\craftarms_0.tga", &dword_10BEE38C)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_1.tga", &dword_10BECEE8)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_2.tga", &dword_10BED988)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_3.tga", &dword_10BECEEC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot_selected.tga", &dword_10BECDAC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot.tga", &dword_10BEE038)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button.tga", &dword_10BEE334)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_grey.tga", &dword_10BED990)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_hover.tga", &dword_10BEE2D8)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_press.tga", &dword_10BED79C) )
	

	return 0;
}
*/
static void __cdecl systemReset() {
}

static void __cdecl systemExit() {
}

class ItemCreation : public TempleFix {
public:
	const char* name() override {
		return "Item Creation UI";
	}
	void apply() override {
		// auto system = UiSystem::getUiSystem("ItemCreation-UI");		
		// system->init = systemInit;
		void* ptrToBrewPotion = &BrewPotionRadialMenu;
		void* ptrToScribeScroll = &ScribeScrollRadialMenu;
		void* ptrToCraftWand = &CraftWandRadialMenu;
		void* ptrToCraftRod = &CraftRodRadialMenu;

		void* ptrToCraftWondrous = &CraftWondrousRadialMenu;
		void* ptrToCraftStaff = &CraftStaffRadialMenu;
		void* ptrToForgeRing = &ForgeRingRadialMenu;
		void* ptrToMAA = &CraftMagicArmsAndArmorRadialMenu;

		replaceFunction(0x10150DA0, CraftScrollWandPotionSetItemSpellData);
		replaceFunction(0x10152690, CreateItemResourceCheck);
		replaceFunction(0x10151F60, CreateItemDebitXPGP);
		replaceFunction(0x10152930, UiItemCreationCraftingCostTexts);


		write(0x102EE250, &ptrToBrewPotion, sizeof(ptrToBrewPotion));
		
		write(0x102EE280, &ptrToScribeScroll, sizeof(ptrToScribeScroll));
		//write(0x102EE2B0, &ptrToCraftWand, sizeof(ptrToCraftWand));
		write(0x102EE2E0, &ptrToCraftRod, sizeof(ptrToCraftRod));

		write(0x102EE310, &ptrToCraftWondrous, sizeof(ptrToCraftWondrous));
		if (config.newFeatureTestMode)
		{
			char * testbuf[100];
			read(0x102EE310, testbuf, 4);
			logger->info("New Feature Test Mode: Testing Item Creation Function Replacement");
		}

		write(0x102AAE28, &ptrToCraftStaff, sizeof(ptrToCraftStaff));
		write(0x102AADF8, &ptrToForgeRing, sizeof(ptrToForgeRing));
		write(0x102EE340, &ptrToMAA, sizeof(ptrToMAA));
	}
} itemCreation;
