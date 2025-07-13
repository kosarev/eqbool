
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <Python.h>

#include <ostream>
#include <sstream>

#include "../eqbool.h"

namespace {

struct bool_object {
    PyObject_HEAD
    eqbool::eqbool value;

    static bool_object *from_pyobject(PyObject *p) {
        // Implicitly propagate all referenced values.
        auto *obj = reinterpret_cast<bool_object*>(p);
        if(obj->value)
            obj->value.propagate();
        return obj;
    }

    PyObject *as_pyobject() {
        return reinterpret_cast<PyObject*>(this);
    }
};

struct term_hasher {
    size_t operator () (PyObject *obj) const {
        Py_hash_t hash = PyObject_Hash(obj);
        assert(hash != -1);
        return static_cast<size_t>(hash);
    }
};

struct term_matcher {
    bool operator () (PyObject *a, PyObject *b) const {
        if(a == b)
            return true;

        int eq = PyObject_RichCompareBool(a, b, Py_EQ);
        assert(eq == 0 || eq == 1);
        return eq == 1;
    }
};

class term_set : public eqbool::term_set_base {
private:
    std::unordered_set<PyObject*, term_hasher, term_matcher> terms;

public:
    term_set() = default;

    ~term_set() override {
        for(PyObject *t : terms)
            Py_DECREF(t);
    }

    PyObject *add(PyObject *t) {
        auto r = terms.insert(t);
        bool inserted = r.second;
        if(inserted)
            Py_INCREF(t);
        return *r.first;
    }

    std::ostream &print(std::ostream &s, uintptr_t t) const override {
        auto *term = reinterpret_cast<PyObject*>(t);
        PyObject *str_obj = PyObject_Str(term);
        if(str_obj) {
            const char *str = PyUnicode_AsUTF8(str_obj);
            if(s)
                s << str;

            // TODO: Use a RAII wrapper?
            Py_DECREF(str_obj);

            if(s)
                return s;
        }

        // Just print the address as an integer on an error.
        return s << '<' << t << '>';
    }
};

struct context_object {
    PyObject_HEAD
    term_set terms;
    eqbool::eqbool_context context;

    static context_object *from_pyobject(PyObject *p) {
        return reinterpret_cast<context_object*>(p);
    }
};

static PyObject *bool_set(PyObject *self, PyObject *args);
static PyObject *bool_get_id(PyObject *self, PyObject *args);
static PyObject *bool_invert(PyObject *self, PyObject *args);
static PyObject *bool_print(PyObject *self, PyObject *args);

static PyMethodDef bool_methods[] = {
    {"_set", bool_set, METH_O, nullptr},
    {"_get_id", bool_get_id, METH_NOARGS, nullptr},
    {"_invert", bool_invert, METH_NOARGS, nullptr},
    {"_print", bool_print, METH_NOARGS, nullptr},
    {}  // Sentinel.
};

static PyObject *bool_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
                          PyObject *Py_UNUSED(kwds)) {
    auto *self = bool_object::from_pyobject(type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    eqbool::eqbool &value = self->value;
    ::new(&value) eqbool::eqbool();
    return &self->ob_base;
}

static void bool_dealloc(PyObject *self) {
    auto &object = *bool_object::from_pyobject(self);
    object.value.~eqbool();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject bool_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Bool",     // tp_name
    sizeof(bool_object),        // tp_basicsize
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

static PyObject *context_get(PyObject *self, PyObject *arg);
static PyObject *context_get_or(PyObject *self, PyObject *args);
static PyObject *context_ifelse(PyObject *self, PyObject *args);
static PyObject *context_is_equiv(PyObject *self, PyObject *args);

static PyMethodDef context_methods[] = {
    {"_get", context_get, METH_O, nullptr},
    {"_get_or", context_get_or, METH_VARARGS, nullptr},
    {"_ifelse", context_ifelse, METH_VARARGS, nullptr},
    {"is_equiv", context_is_equiv, METH_VARARGS, nullptr},
    {}  // Sentinel.
};

static PyObject *context_new(PyTypeObject *type, PyObject *Py_UNUSED(args),
                             PyObject *Py_UNUSED(kwds)) {
    auto *self = context_object::from_pyobject(
        type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
      return nullptr;

    term_set &terms = self->terms;
    ::new(&terms) term_set();

    eqbool::eqbool_context &context = self->context;
    ::new(&context) eqbool::eqbool_context(terms);

    return &self->ob_base;
}

static void context_dealloc(PyObject *self) {
    auto &object = *context_object::from_pyobject(self);
    object.context.~eqbool_context();
    object.terms.~term_set();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject context_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._Context",  // tp_name
    sizeof(context_object),    // tp_basicsize
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

static PyObject *bool_set(PyObject *self, PyObject *arg) {
    if(!PyObject_TypeCheck(arg, &bool_type_object)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _Bool object");
        return nullptr;
    }

    eqbool::eqbool v = bool_object::from_pyobject(arg)->value;
    bool_object::from_pyobject(self)->value = v;

    Py_RETURN_NONE;
}

static PyObject *bool_get_id(PyObject *self, PyObject *Py_UNUSED(args)) {
    return PyLong_FromSize_t(bool_object::from_pyobject(self)->value.get_id());
}

static PyObject *bool_invert(PyObject *self, PyObject *Py_UNUSED(args)) {
    bool_object *r = PyObject_New(bool_object, &bool_type_object);
    if (r)
        r->value = ~bool_object::from_pyobject(self)->value;
    return r->as_pyobject();
}

static PyObject *bool_print(PyObject *self, PyObject *Py_UNUSED(args)) {
    std::ostringstream ss;
    ss << bool_object::from_pyobject(self)->value;
    return PyUnicode_FromStringAndSize(
        ss.str().c_str(),
        static_cast<Py_ssize_t>(ss.str().size()));
}

static PyObject *context_get(PyObject *self, PyObject *arg) {
    auto &terms = context_object::from_pyobject(self)->terms;
    auto &context = context_object::from_pyobject(self)->context;
    eqbool::eqbool v;
    if(arg == Py_False) {
        v = context.get_false();
    } else if(arg == Py_True) {
        v = context.get_true();
    } else {
        if(!PyUnicode_CheckExact(arg) &&
                !PyLong_CheckExact(arg) &&
                !PyTuple_CheckExact(arg)) {
            PyErr_SetString(PyExc_TypeError,
                            "Only immutable types allowed as terms");
            return nullptr;
        }

        if(PyObject_Hash(arg) == -1)
            return nullptr;

        PyObject *t = terms.add(arg);
        v = context.get(reinterpret_cast<uintptr_t>(t));
    }

    bool_object *r = PyObject_New(bool_object, &bool_type_object);
    if (r)
        r->value = v;
    return r->as_pyobject();
}

static bool get_args(std::vector<eqbool::eqbool> &v, PyObject *args) {
    Py_ssize_t n = PyTuple_Size(args);
    for(Py_ssize_t i = 0; i != n; ++i) {
        PyObject *arg = PyTuple_GetItem(args, i);
        if(!PyObject_TypeCheck(arg, &bool_type_object)) {
            PyErr_SetString(PyExc_TypeError,
                            "Arguments must be _Bool objects");
            return false;
        }
        v.push_back(bool_object::from_pyobject(arg)->value);
    }
    return true;
}

static PyObject *context_get_or(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    bool_object *r = PyObject_New(bool_object, &bool_type_object);
    if (r) {
        auto &context = context_object::from_pyobject(self)->context;
        r->value = context.get_or(v);
    }
    return r->as_pyobject();
}

static PyObject *context_ifelse(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    if(v.size() != 3) {
        PyErr_SetString(PyExc_TypeError, "Expected exactly 3 arguments");
        return nullptr;
    }

    bool_object *r = PyObject_New(bool_object, &bool_type_object);
    if (r) {
        auto &context = context_object::from_pyobject(self)->context;
        r->value = context.ifelse(v[0], v[1], v[2]);
    }
    return r->as_pyobject();
}

static PyObject *context_is_equiv(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    if(v.size() != 2) {
        PyErr_SetString(PyExc_TypeError, "Expected exactly 2 arguments");
        return nullptr;
    }

    auto &context = context_object::from_pyobject(self)->context;
    if(context.is_equiv(v[0], v[1]))
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

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
