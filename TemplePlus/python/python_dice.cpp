
#include "stdafx.h"
#include "python_dice.h"
#include <structmember.h>
#include "../dice.h"

struct PyDice {
	PyObject_HEAD;
	int number;
	int size;
	int bonus;
};

PyObject *PyDice_Repr(PyObject *obj) {
	auto self = (PyDice*) obj;
	string text;
	if (self->bonus == 0) {
		text = format("{}d{}", self->number, self->size);
	} else {
		text = format("{}d{}{:+d}", self->number, self->size, self->bonus);
	}
	return PyString_FromString(text.c_str());
}

PyObject *PyDice_Clone(PyObject *obj, PyObject *args) {
	auto self = (PyDice*)obj;
	auto result = PyObject_New(PyDice, obj->ob_type);
	result->number = self->number;
	result->size = self->size;
	result->bonus = self->bonus;
	return (PyObject*) result;
}

PyObject *PyDice_Roll(PyObject *obj, PyObject *args) {
	auto self = (PyDice*)obj;
	auto result = Dice::Roll(self->number, self->size, self->bonus);
	return PyInt_FromLong(result);
}

int PyDice_Init(PyObject *obj, PyObject *args, PyObject *kwds) {
	auto self = (PyDice*)obj;
	if (PyTuple_Size(args) == 1) {
		// Form: PyDice("1d2+3") etc.
		const char *diceSpec;
		if (!PyArg_ParseTuple(args, "s:PyDice", &diceSpec)) {
			return -1;
		}
		if (!Dice::Parse(diceSpec, self->number, self->size, self->bonus)) {
			PyErr_SetString(PyExc_RuntimeError, "The given dice string is invalid");
			return -1;
		}
	} else {
		self->bonus = 0; // Initialize since it's optional
		// Form: PyDice(1, 2[, 3]) for 1d2+3 etc.
		if (!PyArg_ParseTuple(args, "ii|:PyDice", &self->number, &self->size, &self->bonus)) {
			return -1;
		}
	}
	return 0;
}

PyObject *PyDice_New(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
	auto self = PyObject_New(PyDice, &PyDiceType);
	self->bonus = 0;
	self->number = 0;
	self->size = 0;
	return (PyObject*) self;
}

static PyMemberDef PyDice_Members[] = {
	{ "number", T_INT, offsetof(PyDice, number), 0, NULL },
	{ "size", T_INT, offsetof(PyDice, size), 0, NULL },
	{ "bonus", T_INT, offsetof(PyDice, bonus), 0, NULL },
	{NULL, NULL, NULL, NULL, NULL }
};

static PyMethodDef PyDice_Methods[] = {
	{ "clone", PyDice_Clone, METH_VARARGS, NULL },
	{ "roll", PyDice_Roll, METH_VARARGS, NULL },
	{ NULL, NULL, NULL, NULL }
};

PyTypeObject PyDiceType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"PyDice",                  /*tp_name*/
	sizeof(PyDice),     	   /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)PyObject_Del,  /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	PyDice_Repr,               /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	PyObject_GenericGetAttr,   /*tp_getattro*/
	PyObject_GenericSetAttr,   /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	0,			               /* tp_doc */
	0,						   /* tp_traverse */
	0,						   /* tp_clear */
	0,						   /* tp_richcompare */
	0,						   /* tp_weaklistoffset */
	0,						   /* tp_iter */
	0,						   /* tp_iternext */
	PyDice_Methods,            /* tp_methods */
	PyDice_Members,            /* tp_members */
	0,                   	   /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	PyDice_Init,               /* tp_init */
	0,                         /* tp_alloc */
	PyDice_New,                /* tp_new */
};

PyObject* PyDice_FromDice(const Dice& dice) {
	auto self = PyObject_New(PyDice, &PyDiceType);
	self->number = dice.GetCount();
	self->size = dice.GetSides();
	self->bonus = dice.GetModifier();
	return (PyObject*) self;
}

bool ConvertDice(PyObject* obj, Dice* pDiceOut) {
	if (obj->ob_type != &PyDiceType) {
		PyErr_SetString(PyExc_TypeError, "Expected a dice.");
		return false;
	}

	auto pyDice = (PyDice*)obj;
	*pDiceOut = Dice(pyDice->number, pyDice->size, pyDice->bonus);
	return true;
}
