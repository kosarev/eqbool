
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <Python.h>

#include "../eqbool.h"

namespace {

struct bool_instance {
    PyObject_HEAD
    eqbool::eqbool value;
};

struct context_instance {
    PyObject_HEAD
    eqbool::eqbool_context context;
};

static inline bool_instance *cast_bool_instance(PyObject *p) {
    return reinterpret_cast<bool_instance*>(p);
}

// TODO: Used?
static inline eqbool::eqbool &cast_bool(PyObject *p) {
    return cast_bool_instance(p)->value;
}

static inline context_instance *cast_context_instance(PyObject *p) {
    return reinterpret_cast<context_instance*>(p);
}

// TODO: Used?
static inline eqbool::eqbool_context &cast_context(PyObject *p) {
    return cast_context_instance(p)->context;
}

static PyMethodDef bool_methods[] = {
    { nullptr }  // Sentinel.
};

static PyMethodDef context_methods[] = {
    { nullptr }  // Sentinel.
};

static PyObject *bool_new(PyTypeObject *type, PyObject *args,
                          PyObject *kwds) {
    auto *self = cast_bool_instance(type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    eqbool::eqbool &value = self->value;
    ::new(&value) eqbool::eqbool();
    return &self->ob_base;
}

static void bool_dealloc(PyObject *self) {
    auto &object = *cast_bool_instance(self);
    object.value.~eqbool();
    Py_TYPE(self)->tp_free(self);
}

static PyObject *context_new(PyTypeObject *type, PyObject *args,
                             PyObject *kwds) {
    auto *self = cast_context_instance(type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    eqbool::eqbool_context &context = self->context;
    ::new(&context) eqbool::eqbool_context();
    return &self->ob_base;
}

static void context_dealloc(PyObject *self) {
    auto &object = *cast_context_instance(self);
    object.context.~eqbool_context();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject bool_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Bool",     // tp_name
    sizeof(bool_instance),      // tp_basicsize
    0,                          // tp_itemsize
    bool_dealloc,               // tp_dealloc
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
    "Boolean value.",           // tp_doc
    0,                          // tp_traverse
    0,                          // tp_clear
    0,                          // tp_richcompare
    0,                          // tp_weaklistoffset
    0,                          // tp_iter
    0,                          // tp_iternext
    bool_methods,               // tp_methods
    nullptr,                    // tp_members
    0,                          // tp_getset
    0,                          // tp_base
    0,                          // tp_dict
    0,                          // tp_descr_get
    0,                          // tp_descr_set
    0,                          // tp_dictoffset
    0,                          // tp_init
    0,                          // tp_alloc
    bool_new,                   // tp_new
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

static PyTypeObject context_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Context",  // tp_name
    sizeof(context_instance),   // tp_basicsize
    0,                          // tp_itemsize
    context_dealloc,            // tp_dealloc
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
    "Context.",                 // tp_doc
    0,                          // tp_traverse
    0,                          // tp_clear
    0,                          // tp_richcompare
    0,                          // tp_weaklistoffset
    0,                          // tp_iter
    0,                          // tp_iternext
    context_methods,            // tp_methods
    nullptr,                    // tp_members
    0,                          // tp_getset
    0,                          // tp_base
    0,                          // tp_dict
    0,                          // tp_descr_get
    0,                          // tp_descr_set
    0,                          // tp_dictoffset
    0,                          // tp_init
    0,                          // tp_alloc
    context_new,                // tp_new
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

    if(PyType_Ready(&bool_type_object) < 0)
        return nullptr;
    if(PyType_Ready(&context_type_object) < 0)
        return nullptr;

    Py_INCREF(&bool_type_object);
    Py_INCREF(&context_type_object);

    if (PyModule_AddObject(m, "_Bool",
                           &bool_type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&bool_type_object);
        Py_DECREF(&context_type_object);
        Py_DECREF(m);
        return nullptr;
    }

    if (PyModule_AddObject(m, "_Context",
                           &context_type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&bool_type_object);
        Py_DECREF(&context_type_object);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}
