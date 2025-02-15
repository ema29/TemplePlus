
#include "stdafx.h"
#include "common.h"
#include "util\fixes.h"
#include "python\pythonglobal.h"
#include "python\python_header.h"
#include "obj.h"
#include "d20.h"


void PyPerformTouchAttack_PatchedCallToHitProcessing(D20Actn * pd20A, D20Actn d20A, uint32_t savedesi, uint32_t retaddr, PyObject * pyObjCaller, PyObject * pyTupleArgs);
void enlargeSpellRestoreModelScaleHook(objHndl objHnd);
void enlargeSpellIncreaseModelScaleHook(objHndl objHnd);

class SpellConditionFixes : public TempleFix {
public:
	const char* name() override {
		return "Spell Condition Fixes (for buggy spell effects)";
	}

	void VampiricTouchFix();
	void enlargePersonModelScaleFix(); // fixes ambiguous float point calculations that resulted in cumulative roundoff errors
	
	void apply() override {
		
		VampiricTouchFix();
		enlargePersonModelScaleFix();
	}
} spellConditionFixes;


void SpellConditionFixes::VampiricTouchFix()
{
	writeHex(0x102E0A24, "EA 00");
	writeHex(0x102E0A28, "D0 43 0C 10");
	writeHex(0x102E0A2C, "F0 09  2E 10");

	writeHex(0x102E0A8C, "D0 8F 0E 10");

	writeHex(0x102E0AC4, "A6");
	writeHex(0x102E0AC8, "20 76 0D 10");

	writeHex(0x102E0B00, "A6");
	writeHex(0x102E0B04, "B0 BA 0C 10");


	//Perform Touch Attack mod:
	redirectCall(0x100B2CC9, PyPerformTouchAttack_PatchedCallToHitProcessing);
	return;
}

void SpellConditionFixes::enlargePersonModelScaleFix()
{
	redirectCall(0x100CD45C, enlargeSpellIncreaseModelScaleHook);
	redirectCall(0x100D84DE, enlargeSpellRestoreModelScaleHook); // sp152 enlarge 
	redirectCall(0x100D9C22, enlargeSpellRestoreModelScaleHook); // sp404 righteous might

};

void PyPerformTouchAttack_PatchedCallToHitProcessing( D20Actn * pd20A, D20Actn d20A, uint32_t savedesi, uint32_t retaddr, PyObject * pyObjCaller, PyObject * pyTupleArgs)
{
	auto tupSize = PyTuple_Size(pyTupleArgs);
	uint32_t shouldPerformMeleeTouchAttack = 0;
	if (tupSize > 1)
	{
		PyObject * pyarg2 = PyTuple_GetItem(pyTupleArgs, 1);
		if (PyType_IsSubtype(pyarg2->ob_type, &PyInt_Type))
		{
			if (PyLong_AsLong( pyarg2) != 0)
			{
				pd20A->d20Caf = D20CAF_TOUCH_ATTACK; // sans D20CAF_RANGED
			}
		}
	}
	

	d20Sys.ToHitProc(pd20A);
	return;
	
}

void __cdecl enlargeSpellRestoreModelScaleHook(objHndl objHnd)
{
	// patches for spellRemove function (disgusting hardcoded shit! bah!)
	uint32_t modelScale = objects.getInt32(objHnd, obj_f_model_scale);
	modelScale *= 5;
	modelScale /= 9;
	objects.setInt32(objHnd, obj_f_model_scale, modelScale);
}

void enlargeSpellIncreaseModelScaleHook(objHndl objHnd)
{
	uint32_t modelScale = objects.getInt32(objHnd, obj_f_model_scale);
	modelScale *= 9;
	modelScale /= 5;
	objects.setInt32(objHnd, obj_f_model_scale, modelScale);
}