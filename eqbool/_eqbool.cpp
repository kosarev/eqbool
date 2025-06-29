
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <Python.h>

#include "../eqbool.h"

using eqbool::eqbool_context;

namespace {

// TODO: Used?
class decref_guard {
public:
    decref_guard(PyObject *object)
        : object(object)
    {}

    ~decref_guard() {
        if(object)
            Py_XDECREF(object);
    }

    explicit operator bool () const {
        return object != nullptr;
    }

    PyObject *get() const {
        return object;
    }

private:
    PyObject *object;
};

struct object_instance {
    PyObject_HEAD
    eqbool_context eqbools;
};

// TODO: Used?
static inline object_instance *cast_object(PyObject *p) {
    return reinterpret_cast<object_instance*>(p);
}

// TODO: Used?
static inline eqbool_context &cast_eqbools(PyObject *p) {
    return cast_object(p)->eqbools;
}

static PyMethodDef methods[] = {
    { nullptr }  // Sentinel.
};

static PyObject *object_new(PyTypeObject *type, PyObject *args,
                            PyObject *kwds) {
    auto *self = cast_object(type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    eqbool_context &eqbools = self->eqbools;
    ::new(&eqbools) eqbool_context();
    return &self->ob_base;
}

static void object_dealloc(PyObject *self) {
    auto &object = *cast_object(self);
    object.eqbools.~eqbool_context();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Context",  // tp_name
    sizeof(object_instance),    // tp_basicsize
    0,                          // tp_itemsize
    object_dealloc,             // tp_dealloc
    0,                          // tp_print
    0,                          // tp_getattr
    0,                          // tp_setattr
    0,                          // tp_reserved
    0,                          // tp_repr
    0,                          // tp_as_number
    0,                          // tp_as_sequence
    0,                          // tp_as_mapping
    0,                          // tp_hash
    0,                          // tp_call
    0,                          // tp_str
    0,                          // tp_getattro
    0,                          // tp_setattro
    0,                          // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                // tp_flags
    "Boolean expression",       // tp_doc
    0,                          // tp_traverse
    0,                          // tp_clear
    0,                          // tp_richcompare
    0,                          // tp_weaklistoffset
    0,                          // tp_iter
    0,                          // tp_iternext
    methods,                    // tp_methods
    nullptr,                    // tp_members
    0,                          // tp_getset
    0,                          // tp_base
    0,                          // tp_dict
    0,                          // tp_descr_get
    0,                          // tp_descr_set
    0,                          // tp_dictoffset
    0,                          // tp_init
    0,                          // tp_alloc
    object_new,                 // tp_new
    0,                          // tp_free
    0,                          // tp_is_gc
    0,                          // tp_bases
    0,                          // tp_mro
    0,                          // tp_cache
    0,                          // tp_subclasses
    0,                          // tp_weaklist
    0,                          // tp_del
    0,                          // tp_version_tag
    0,                          // tp_finalize
};

static PyModuleDef module = {
    PyModuleDef_HEAD_INIT,      // m_base
    "eqbool._eqbool",           // m_name
    "Testing boolean expressions for equivalence.",
                                // m_doc
    -1,                         // m_size
    nullptr,                    // m_methods
    nullptr,                    // m_slots
    nullptr,                    // m_traverse
    nullptr,                    // m_clear
    nullptr,                    // m_free
};

}  // anonymous namespace

extern "C" PyMODINIT_FUNC PyInit__eqbool(void) {
    PyObject *m = PyModule_Create(&module);
    if(!m)
        return nullptr;

    if(PyType_Ready(&type_object) < 0)
        return nullptr;
    Py_INCREF(&type_object);

    if (PyModule_AddObject(m, "_Context", &type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&type_object);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}
