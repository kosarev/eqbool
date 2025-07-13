
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <Python.h>

#include <ostream>

#include "../eqbool.h"

namespace {

struct bool_instance {
    PyObject_HEAD
    eqbool::eqbool value;

    PyObject *as_pyobject() {
        return reinterpret_cast<PyObject*>(this);
    }
};

class term_set : public eqbool::term_set<PyObject*> {
public:
    std::ostream &print(std::ostream &s, uintptr_t t) const override {
        // TODO: Support printing associated objects.
        return s << t;
    }
};

struct context_instance {
    PyObject_HEAD
    term_set terms;
    eqbool::eqbool_context context;
};

static inline bool_instance *cast_bool_instance(PyObject *p) {
    return reinterpret_cast<bool_instance*>(p);
}

// TODO: Used?
#if 0
static inline eqbool::eqbool &cast_bool(PyObject *p) {
    return cast_bool_instance(p)->value;
}
#endif

static inline context_instance *cast_context_instance(PyObject *p) {
    return reinterpret_cast<context_instance*>(p);
}

static inline eqbool::eqbool_context &cast_context(PyObject *p) {
    return cast_context_instance(p)->context;
}

static PyMethodDef bool_methods[] = {
    {}  // Sentinel.
};

static PyObject *bool_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
                          PyObject *Py_UNUSED(kwds)) {
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

static PyTypeObject bool_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Bool",     // tp_name
    sizeof(bool_instance),      // tp_basicsize
    0,                          // tp_itemsize
    bool_dealloc,               // tp_dealloc
    0,                          // tp_print
    nullptr,                    // tp_getattr
    nullptr,                    // tp_setattr
    nullptr,                    // tp_reserved
    nullptr,                    // tp_repr
    nullptr,                    // tp_as_number
    nullptr,                    // tp_as_sequence
    nullptr,                    // tp_as_mapping
    nullptr,                    // tp_hash
    nullptr,                    // tp_call
    nullptr,                    // tp_str
    nullptr,                    // tp_getattro
    nullptr,                    // tp_setattro
    nullptr,                    // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                // tp_flags
    "Boolean value.",           // tp_doc
    nullptr,                    // tp_traverse
    nullptr,                    // tp_clear
    nullptr,                    // tp_richcompare
    0,                          // tp_weaklistoffset
    nullptr,                    // tp_iter
    nullptr,                    // tp_iternext
    bool_methods,               // tp_methods
    nullptr,                    // tp_members
    nullptr,                    // tp_getset
    nullptr,                    // tp_base
    nullptr,                    // tp_dict
    nullptr,                    // tp_descr_get
    nullptr,                    // tp_descr_set
    0,                          // tp_dictoffset
    nullptr,                    // tp_init
    nullptr,                    // tp_alloc
    bool_new,                   // tp_new
    nullptr,                    // tp_free
    nullptr,                    // tp_is_gc
    nullptr,                    // tp_bases
    nullptr,                    // tp_mro
    nullptr,                    // tp_cache
    nullptr,                    // tp_subclasses
    nullptr,                    // tp_weaklist
    nullptr,                    // tp_del
    0,                          // tp_version_tag
    nullptr,                    // tp_finalize
    nullptr,                    // tp_vectorcall
    0,                          // tp_watched
};

static PyObject *get_false(PyObject *self, PyObject *Py_UNUSED(args)) {
    auto eqfalse = cast_context(self).get_false();
    bool_instance *r = PyObject_New(bool_instance, &bool_type_object);
    if (r)
        r->value = eqfalse;
    return r->as_pyobject();
}

static PyMethodDef context_methods[] = {
    {"_get_false", get_false, METH_NOARGS, nullptr},
    {}  // Sentinel.
};

static PyObject *context_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
                             PyObject *Py_UNUSED(kwds)) {
    auto *self = cast_context_instance(type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    term_set &terms = self->terms;
    ::new(&terms) term_set();

    eqbool::eqbool_context &context = self->context;
    ::new(&context) eqbool::eqbool_context(terms);

    return &self->ob_base;
}

static void context_dealloc(PyObject *self) {
    auto &object = *cast_context_instance(self);
    object.context.~eqbool_context();
    object.terms.~term_set();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject context_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Context",  // tp_name
    sizeof(context_instance),   // tp_basicsize
    0,                          // tp_itemsize
    context_dealloc,            // tp_dealloc
    0,                          // tp_print
    nullptr,                    // tp_getattr
    nullptr,                    // tp_setattr
    nullptr,                    // tp_reserved
    nullptr,                    // tp_repr
    nullptr,                    // tp_as_number
    nullptr,                    // tp_as_sequence
    nullptr,                    // tp_as_mapping
    nullptr,                    // tp_hash
    nullptr,                    // tp_call
    nullptr,                    // tp_str
    nullptr,                    // tp_getattro
    nullptr,                    // tp_setattro
    nullptr,                    // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                // tp_flags
    "Context.",                 // tp_doc
    nullptr,                    // tp_traverse
    nullptr,                    // tp_clear
    nullptr,                    // tp_richcompare
    0,                          // tp_weaklistoffset
    nullptr,                    // tp_iter
    nullptr,                    // tp_iternext
    context_methods,            // tp_methods
    nullptr,                    // tp_members
    nullptr,                    // tp_getset
    nullptr,                    // tp_base
    nullptr,                    // tp_dict
    nullptr,                    // tp_descr_get
    nullptr,                    // tp_descr_set
    0,                          // tp_dictoffset
    nullptr,                    // tp_init
    nullptr,                    // tp_alloc
    context_new,                // tp_new
    nullptr,                    // tp_free
    nullptr,                    // tp_is_gc
    nullptr,                    // tp_bases
    nullptr,                    // tp_mro
    nullptr,                    // tp_cache
    nullptr,                    // tp_subclasses
    nullptr,                    // tp_weaklist
    nullptr,                    // tp_del
    0,                          // tp_version_tag
    nullptr,                    // tp_finalize
    nullptr,                    // tp_vectorcall
    0,                          // tp_watched
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

PyMODINIT_FUNC PyInit__eqbool(void);

PyMODINIT_FUNC PyInit__eqbool(void) {
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
