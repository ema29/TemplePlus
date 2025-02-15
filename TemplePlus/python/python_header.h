
#pragma once

#include <temple/dll.h>
#include "obj.h"

/*
Define all type objects used by ToEE.
*/
// ObjHandle
static temple::GlobalStruct<PyTypeObject, 0x102CF3B8> pyObjHandleType;
static getattrfunc pyObjHandleTypeGetAttr; // Original getattr of pyObjHandleType
static setattrfunc pyObjHandleTypeSetAttr; // Original setattr of pyObjHandleType
static temple::GlobalStruct<PyMethodDef, 0x102CE9A8> pyObjHandleMethods;


struct TemplePyObjHandle : public PyObject {
	ObjectId objId;
	uint64_t objHandle;
};


enum SAN : uint32_t
{
	san_examine = 0x0,
	san_use = 0x1,
	san_destroy = 0x2,
	san_unlock = 0x3,
	san_get = 0x4,
	san_drop = 0x5,
	san_throw = 0x6,
	san_hit = 0x7,
	san_miss = 0x8,
	san_dialog = 0x9,
	san_first_heartbeat = 0xA,
	san_catching_thief_pc = 0xB,
	san_dying = 0xC,
	san_enter_combat = 0xD,
	san_exit_combat = 0xE,
	san_start_combat = 0xF,
	san_end_combat = 0x10,
	san_buy_object = 0x11,
	san_resurrect = 0x12,
	san_heartbeat = 0x13,
	san_leader_killing = 0x14,
	san_insert_item = 0x15,
	san_will_kos = 0x16,
	san_taking_damage = 0x17,
	san_wield_on = 0x18,
	san_wield_off = 0x19,
	san_critter_hits = 0x1A,
	san_new_sector = 0x1B,
	san_remove_item = 0x1C,
	san_leader_sleeping = 0x1D,
	san_bust = 0x1E,
	san_dialog_override = 0x1F,
	san_transfer = 0x20,
	san_caught_thief = 0x21,
	san_critical_hit = 0x22,
	san_critical_miss = 0x23,
	san_join = 0x24,
	san_disband = 0x25,
	san_new_map = 0x26,
	san_trap = 0x27,
	san_true_seeing = 0x28,
	san_spell_cast = 0x29,
	san_unlock_attempt = 0x2A,
};


struct objHndl_Plus_PartsysID{
	objHndl objHnd;
	uint32_t nPartsysID;
};
