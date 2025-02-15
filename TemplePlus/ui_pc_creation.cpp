#include "stdafx.h"
#include "common.h"
#include "util/config.h"
#include "d20.h"
#include "feat.h"
#include "ui_char_editor.h"
#include "obj.h"
#include "ui/ui.h"
#include "util/fixes.h"
#include "tig/tig_texture.h"
#include "gamesystems/gamesystems.h"

#define MAX_PC_CREATION_PORTRAITS 8
#include "party.h"
#include "tig/tig_msg.h"

struct TigMsg;
int PcCreationFeatUiPrereqCheckUsercallWrapper();
int(__cdecl * OrgFeatMultiselectSub_101822A0)();
int HookedFeatMultiselectSub_101822A0(feat_enums feat);
int HookedUsercallFeatMultiselectSub_101822A0();


class PcCreationUiSystem : TempleFix
{
public:
	static WidgetType1 pcPortraitsMain;
	static int pcPortraitsMainId;
	static TigRect pcPortraitRects[MAX_PC_CREATION_PORTRAITS];
	static TigRect pcPortraitBoxRects[MAX_PC_CREATION_PORTRAITS];
	

	static BOOL PcPortraitsInit(GameSystemConf * conf);
	static BOOL PcPortraitWidgetsInit(int height);
	static BOOL PcPortraitsExit();
	static BOOL PcPortraitsResize(UiResizeArgs * resizeArgs);
	static BOOL PcPortraitsMainHideAndGet();

	static void PcPortraitsButtonActivateNext();
	static void PcPortraitsRefresh();
	static void PcPortraitsDisable();
	static int return0()
	{
		return 0;
	};
	static int PcPortraitsMsgFunc(int widgetId, TigMsg* tigMsg);
	static UiMsgFunc orgPcPortraitsMsgFunc;

	static int pcPortraitWidgIds[MAX_PC_CREATION_PORTRAITS];

	const char* name() override { return "PC Creation UI Fixes"; }
	void apply() override
	{
		replaceFunction(0x10182E80, PcCreationFeatUiPrereqCheckUsercallWrapper);
		OrgFeatMultiselectSub_101822A0 = (int(__cdecl*)()) replaceFunction(0x101822A0, HookedUsercallFeatMultiselectSub_101822A0);
		//replaceFunction(0x101634D0, PcPortraitWidgetsInit);
		replaceFunction(0x10163030, PcPortraitsMainHideAndGet);
		replaceFunction(0x10163660, PcPortraitsInit);
		replaceFunction(0x101636E0, PcPortraitsResize);
		replaceFunction(0x10163410, PcPortraitsExit);

		replaceFunction(0x10163060, PcPortraitsDisable);
		replaceFunction(0x10163090, PcPortraitsButtonActivateNext);
		replaceFunction(0x10163440, PcPortraitsRefresh);
		orgPcPortraitsMsgFunc = replaceFunction(0x101633A0, PcPortraitsMsgFunc);

		// 1016312F
		int writeVal = (int)&pcPortraitWidgIds[-1];
		write(0x1016312F + 3, &writeVal, 4);

		// ui_msg_pc_creation_portraits
		writeVal = MAX_PC_CREATION_PORTRAITS;
		write(0x101633B5 + 1, &writeVal, 1);
		writeVal = (int)&pcPortraitWidgIds;
		write(0x101633B7 + 1, &writeVal, 4);

		// ui_render_pc_creation_portraits
		writeVal = MAX_PC_CREATION_PORTRAITS;
		write(0x10163279 + 1, &writeVal, 1);
		writeVal = (int)&pcPortraitWidgIds;
		write(0x1016327B + 1, &writeVal, 4);

		writeVal = (int)&pcPortraitsMainId;
		write(0x101632E6 + 2, &writeVal, 4);
		write(0x1016337A + 2, &writeVal, 4);
		writeVal = (int)&pcPortraitBoxRects;
		write(0x101632F6 + 2, &writeVal, 4);
		writeVal = (int)&pcPortraitRects;
		write(0x10163329 + 2, &writeVal, 4);

		
		
	}
} pcCreationSys;
WidgetType1 PcCreationUiSystem::pcPortraitsMain;
int PcCreationUiSystem::pcPortraitsMainId = -1;
int PcCreationUiSystem::pcPortraitWidgIds[MAX_PC_CREATION_PORTRAITS] = { -1, };
TigRect PcCreationUiSystem::pcPortraitRects[MAX_PC_CREATION_PORTRAITS];
TigRect PcCreationUiSystem::pcPortraitBoxRects[MAX_PC_CREATION_PORTRAITS];
UiMsgFunc PcCreationUiSystem::orgPcPortraitsMsgFunc;

struct PcCreationUiAddresses : temple::AddressTable
{
	int * pcCreationIdx;

	WidgetType1 * pcCreationPortraitsMainWidget;
	int * mainWindowWidgetId; // 10BF0ED4
	int * pcPortraitsWidgetIds; // 10BF0EBC  array of 5 entries


	int * uiPccPortraitTexture;
	int *uiPccPortraitHoverTexture;
	int *uiPccPortraitClickTexture;
	int * uiPccPortraitDisabledTexture;
	int * uiPcPortraitsFullMaybe;



	int * dword_10C75F30;
	int * featsMultiselectNum_10C75F34;
	feat_enums * featMultiselect_10C75F38;
	int *dword_10C76AF0;
	Widget* widg_10C77CD0;
	int * dword_10C77D50;
	int * dword_10C77D54;
	int *widIdx_10C77D80;
	feat_enums * featsMultiselectList;
	feat_enums * feat_10C79344;
	int * widgId_10C7AE14;
	char* (__cdecl*sub_10182760)(feat_enums featEnums);
	int(__cdecl* j_CopyWidget_101F87A0)(int widIdx, Widget* widg);
	int(__cdecl*sub_101F87B0)(int widIdx, Widget* widg);
	int(__cdecl*sub_101F8E40)(int);
	int(__cdecl*sub_101F9100)(int widId, int);
	int(__cdecl*sub_101F9510)(int, int);

	CharEditorSelectionPacket * charEdSelPkt;
	MesHandle* pcCreationMes;
	int (__cdecl*ui_render_pc_creation_portraits)(int widId);
	int(__cdecl*ui_msg_pc_creation_portraits)(int widId);

	PcCreationUiAddresses()
	{

		rebase(ui_render_pc_creation_portraits, 0x10163270);
		rebase(ui_msg_pc_creation_portraits   , 0x101633A0);
		rebase(pcCreationIdx,					0x10BF0BC8);
		rebase(uiPccPortraitTexture,			0x10BF0BCC);
		rebase(uiPccPortraitHoverTexture,		0x10BF0C20);
		rebase(pcCreationPortraitsMainWidget,	0x10BF0C28);
		rebase(uiPccPortraitClickTexture,		0x10BF1354);
		rebase(uiPccPortraitDisabledTexture,	0x10BF1358);
		rebase(uiPcPortraitsFullMaybe,			0x10BF0ED0);


		rebase(dword_10C75F30, 0x10C75F30);
		rebase(featsMultiselectNum_10C75F34, 0x10C75F34);
		rebase(featMultiselect_10C75F38, 0x10C75F38);
		rebase(dword_10C76AF0, 0x10C76AF0);
		rebase(widg_10C77CD0, 0x10C77CD0);
		rebase(dword_10C77D50, 0x10C77D50);
		rebase(dword_10C77D54, 0x10C77D54);
		rebase(widIdx_10C77D80, 0x10C77D80);
		rebase(featsMultiselectList, 0x10C78920);
		rebase(feat_10C79344, 0x10C79344);
		rebase(widgId_10C7AE14, 0x10C7AE14);

		rebase(sub_10182760, 0x10182760);
		rebase(j_CopyWidget_101F87A0, 0x101F87A0);
		rebase(sub_101F87B0, 0x101F87B0);
		rebase(sub_101F8E40, 0x101F8E40);
		rebase(sub_101F9100, 0x101F9100);
		rebase(sub_101F9510, 0x101F9510);

		rebase(pcCreationMes, 0x11E72EF0);
		rebase(charEdSelPkt, 0x11E72F00);
		

		
	}


} addresses;




int __declspec(naked) HookedUsercallFeatMultiselectSub_101822A0()
{
	{ __asm push ecx __asm push esi __asm push ebx __asm push edi}
	__asm{
		push eax;
		call HookedFeatMultiselectSub_101822A0;
		add esp, 4;
	}
	{ __asm pop edi __asm pop ebx __asm pop esi __asm pop ecx}
	__asm retn;
}

BOOL PcCreationUiSystem::PcPortraitsInit(GameSystemConf* conf)
{
	if (textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait.tga", addresses.uiPccPortraitTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_click.tga", addresses.uiPccPortraitClickTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_hover.tga", addresses.uiPccPortraitHoverTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_disabled.tga", addresses.uiPccPortraitDisabledTexture)
		)
	{
		return 0;
	}
	return PcPortraitWidgetsInit(conf->height);
}

BOOL PcCreationUiSystem::PcPortraitWidgetsInit(int height)
{
	pcPortraitsMain.WidgetType1Init(10, height - 80, 650, 63);
	pcPortraitsMain.widgetFlags = 1;
	pcPortraitsMain.render = (int)return0;
	pcPortraitsMain.handleMessage = (int)return0;

	if (ui.AddWindow(&pcPortraitsMain, sizeof(WidgetType1), &pcPortraitsMainId, "pc_creation_portraits.c", 275) )
		return 0;

	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS; i++)
	{
		WidgetType2 button;
		ui.ButtonInit(&button, 0, pcPortraitsMainId, pcPortraitsMain.x + 81 * i, pcPortraitsMain.y, 76, 63);

		pcPortraitBoxRects[i].x = button.x;
		pcPortraitRects[i].x = button.x + 4;

		pcPortraitBoxRects[i].y = button.y;
		pcPortraitRects[i].y = button.y + 4;


		pcPortraitBoxRects[i].width = button.width;
		pcPortraitBoxRects[i].height = button.height;

		pcPortraitRects[i].width = 51;
		pcPortraitRects[i].height = 45;

		button.render = (uint32_t) addresses.ui_render_pc_creation_portraits;
		button.handleMessage = (uint32_t) addresses.ui_msg_pc_creation_portraits;

		if (ui.AddButton(&button, sizeof(WidgetType2), &pcPortraitWidgIds[i], "pc_creation_portraits.c", 299)
			|| ui.BindButton(pcPortraitsMainId, pcPortraitWidgIds[i])
			|| ui.ButtonSetButtonState(pcPortraitWidgIds[i], 4))
			return 0;

	}

	return 1;
}

BOOL PcCreationUiSystem::PcPortraitsExit()
{
	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS; i++)
	{
		ui.WidgetRemoveRegardParent(pcPortraitWidgIds[i]);
	}
	return ui.WidgetAndWindowRemove(pcPortraitsMainId);
}

BOOL PcCreationUiSystem::PcPortraitsResize(UiResizeArgs* resizeArgs)
{
	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS;i++)
	{
		ui.WidgetRemoveRegardParent(pcPortraitWidgIds[i]);
	}
	ui.WidgetAndWindowRemove(pcPortraitsMainId);
	return PcPortraitWidgetsInit(resizeArgs->rect1.height);
}

BOOL PcCreationUiSystem::PcPortraitsMainHideAndGet()
{
	ui.WidgetSetHidden(pcPortraitsMainId, 1);
	return ui.WidgetCopy(pcPortraitsMainId, &pcPortraitsMain);
}

void PcCreationUiSystem::PcPortraitsButtonActivateNext()
{
	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS; i++)
	{
		int state;
		ui.GetButtonState(pcPortraitWidgIds[i], &state);
		if (state == 4)
		{
			ui.ButtonSetButtonState(pcPortraitWidgIds[i],0);
			if (i == MAX_PC_CREATION_PORTRAITS -1)
				*addresses.uiPcPortraitsFullMaybe = 1;
			return ;
		}
	}
	*addresses.uiPcPortraitsFullMaybe = 1;
}

void PcCreationUiSystem::PcPortraitsRefresh()
{
	ui.WidgetSetHidden(pcPortraitsMainId,0);
	ui.WidgetCopy(pcPortraitsMainId, &pcPortraitsMain);
	ui.WidgetBringToFront(pcPortraitsMainId);
	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS;i++)
	{
		ui.ButtonSetButtonState(pcPortraitWidgIds[i], 4);
	}
	*addresses.uiPcPortraitsFullMaybe = 0;

	if (party.GroupPCsLen())
	{
		for (int i = 0; i < MAX_PC_CREATION_PORTRAITS;i++)
		{
			ui.ButtonSetButtonState(pcPortraitWidgIds[i], 0);
		}
	}
	*addresses.pcCreationIdx = -1;
}

void PcCreationUiSystem::PcPortraitsDisable()
{
	for (int i = 0; i < MAX_PC_CREATION_PORTRAITS;i++)
	{
		ui.ButtonSetButtonState(pcPortraitWidgIds[i], 4);
	}
	*addresses.uiPcPortraitsFullMaybe = 0;
}

int PcCreationUiSystem::PcPortraitsMsgFunc(int widgetId, TigMsg* tigMsg)
{
	int i;
	if (tigMsg->type != TigMsgType::WIDGET
		|| tigMsg->arg2 != 1
		|| (i = ui.WidgetlistIndexof(widgetId, pcPortraitWidgIds, MAX_PC_CREATION_PORTRAITS), i == -1))
		return 0;

	return orgPcPortraitsMsgFunc(widgetId, tigMsg);
};


int HookedFeatMultiselectSub_101822A0(feat_enums feat)
{
	if ((feat >= FEAT_EXOTIC_WEAPON_PROFICIENCY && feat <= FEAT_WEAPON_SPECIALIZATION) || feat == FEAT_WEAPON_FINESSE_DAGGER)
	{
		__asm mov eax, feat;
		return OrgFeatMultiselectSub_101822A0();
	}
	return 0;
}



int __cdecl PcCreationFeatUiPrereqCheck(feat_enums feat)
{
	int featArrayLen = 0;
	feat_enums featArray[10];
	if (addresses.charEdSelPkt->feat0 != FEAT_NONE)
		featArray[featArrayLen++] = addresses.charEdSelPkt->feat0;
	if (addresses.charEdSelPkt->feat1 != FEAT_NONE)
		featArray[featArrayLen++] = addresses.charEdSelPkt->feat1;
	if (addresses.charEdSelPkt->feat2 != FEAT_NONE)
		featArray[featArrayLen++] = addresses.charEdSelPkt->feat2;
	if (feat == FEAT_IMPROVED_TRIP)
	{
		if (objects.StatLevelGet(*feats.charEditorObjHnd, stat_level_monk) == 5
			&& addresses.charEdSelPkt->classCode == stat_level_monk)
			return 1;
	}
	if (feat <= FEAT_NONE || feat > FEAT_MONK_PERFECT_SELF)
		return feats.FeatPrereqsCheck(*feats.charEditorObjHnd, feat, featArray, featArrayLen, addresses.charEdSelPkt->classCode, addresses.charEdSelPkt->statBeingRaised);

	// the vanilla multiselect range

	return feats.FeatPrereqsCheck(*feats.charEditorObjHnd, feat, featArray, featArrayLen, addresses.charEdSelPkt->classCode, addresses.charEdSelPkt->statBeingRaised);
}


int __declspec(naked) PcCreationFeatUiPrereqCheckUsercallWrapper()
{
	{ __asm push ecx __asm push esi __asm push ebx __asm push edi}

	__asm
	{
		push ecx;
		call PcCreationFeatUiPrereqCheck;
		pop edi;
	}
	{ __asm pop edi __asm pop ebx __asm pop esi __asm pop ecx }
	__asm retn;
}