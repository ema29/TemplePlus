#pragma once
#include "common.h"

#define NUM_ASSIGNABLE_HOTKEYS 39 

struct GameSystemSaveFile;
struct TioFile;

class HotkeySystem
{
public:
	int SaveHotkeys(TioFile * file);
	int SaveHotkeys(FILE * file);
	int LoadHotkeys(GameSystemSaveFile * file);
	void HotkeyInit();
	void HotkeyExit();
	void HotkeyAssignCallback(int cancelFlag);
};


extern HotkeySystem hotkeys;

void __cdecl HotkeyInit();
void __cdecl HotkeyExit();
int __cdecl SaveHotkeys(TioFile* file);
int __cdecl LoadHotkeys(GameSystemSaveFile* file);
void __cdecl HotkeyAssignCallback( int cancelFlag);