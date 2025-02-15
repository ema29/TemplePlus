
#include "stdafx.h"
#include "python_cheats.h"
#include <temple/dll.h>

// Returns 1 if it can handle the command
typedef int (__cdecl *CheatFn)(const char *command);
typedef void (__cdecl *CheatSetStateFn)(const char *command, int state);

const int cheatCount = 40;

#pragma pack(push, 1)
struct Cheat {
	char *name;
	int unk1;
	CheatSetStateFn setState;
	CheatFn handler;
};
#pragma pack(pop)

static struct PythonCheatsAddresses : temple::AddressTable {
	Cheat *cheats;

	PythonCheatsAddresses() {
		rebase(cheats, 0x102B02E8);
	}
} addresses;

static PyObject *PyCheats_GetAttr(PyObject*, char *name) {
	for (int i = 0; i < cheatCount; ++i) {
		auto cheat = addresses.cheats[i];
		if (!_stricmp(name, cheat.name) && cheat.handler) {
			return PyInt_FromLong(cheat.handler(name));
		}
	}
	PyErr_Format(PyExc_AttributeError, "Unknown attribute: %s", name);
	return 0;
}

static int PyCheats_SetAttr(PyObject*, char *name, PyObject *value) {
	auto intVal = PyInt_AsLong(value);

	for (int i = 0; i < cheatCount; ++i) {
		auto cheat = addresses.cheats[i];
		if (!_stricmp(name, cheat.name) && cheat.setState) {
			cheat.setState(name, intVal);
			return 0;
		}
	}

	PyErr_Format(PyExc_AttributeError, "Unknown attribute: %s", name);
	return -1;
}

static PyTypeObject PyCheatsType = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"toee.PyCheats", /*tp_name*/
	sizeof(PyObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	(destructor) PyObject_Del, /*tp_dealloc*/
	0, /*tp_print*/
	PyCheats_GetAttr, /*tp_getattr*/
	PyCheats_SetAttr, /*tp_setattr*/
	0, /*tp_compare*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0, /*tp_hash */
	0, /*tp_call*/
	0, /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /*tp_flags*/
	0, /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	0, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	0, /* tp_init */
	0, /* tp_alloc */
	0, /* tp_new */
};

PyObject* PyCheats_Create() {
	return PyObject_New(PyObject, &PyCheatsType);
}
