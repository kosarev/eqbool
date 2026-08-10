
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2026 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <Python.h>

#include <ostream>
#include <sstream>

#include "../eqbool.h"

namespace {

eqbool::eqbool eqbool_from_pyobject(PyObject *p) {
    unsigned long n = PyLong_AsUnsignedLong(p);
    assert(n != static_cast<unsigned long>(-1));
    eqbool::eqbool v = eqbool::eqbool::from_uintptr(n);
    v.propagate();
    return v;
}

PyObject *pyobject_from_eqbool(eqbool::eqbool v) {
    return PyLong_FromUnsignedLong(v.as_uintptr());
}

class term_set : public eqbool::term_set_base {
public:
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

static PyObject *bool_get_id(PyObject *self, PyObject *p);
static PyObject *bool_get_fp(PyObject *self, PyObject *p);
static PyObject *bool_get_kind(PyObject *self, PyObject *p);
static PyObject *bool_get_term(PyObject *self, PyObject *p);
static PyObject *bool_get_args(PyObject *self, PyObject *p);
static PyObject *bool_print(PyObject *self, PyObject *p);

static PyObject *context_get(PyObject *self, PyObject *arg);
static PyObject *context_get_or(PyObject *self, PyObject *args);
static PyObject *context_ifelse(PyObject *self, PyObject *args);
static PyObject *context_get_eq(PyObject *self, PyObject *args);
static PyObject *context_is_equiv(PyObject *self, PyObject *args);

static PyMethodDef context_methods[] = {
    {"_get_id", bool_get_id, METH_O, nullptr},
    {"_get_fp", bool_get_fp, METH_O, nullptr},
    {"_get_kind", bool_get_kind, METH_O, nullptr},
    {"_get_term", bool_get_term, METH_O, nullptr},
    {"_get_args", bool_get_args, METH_O, nullptr},
    {"_print", bool_print, METH_O, nullptr},

    {"_get", context_get, METH_O, nullptr},
    {"_get_or", context_get_or, METH_VARARGS, nullptr},
    {"_ifelse", context_ifelse, METH_VARARGS, nullptr},
    {"_get_eq", context_get_eq, METH_VARARGS, nullptr},
    {"_is_equiv", context_is_equiv, METH_VARARGS, nullptr},
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
    // Later fields, such as tp_watched, do not exist in all
    // supported Python versions and are left value-initialised.
};

struct order_context_object {
    PyObject_HEAD
    PyObject *context_holder;
    eqbool::order_context orders;

    static order_context_object *from_pyobject(PyObject *p) {
        return reinterpret_cast<order_context_object*>(p);
    }
};

static PyObject *order_context_register_order(PyObject *self,
                                              PyObject *args);
static PyObject *order_context_is_never(PyObject *self, PyObject *p);

static PyMethodDef order_context_methods[] = {
    {"_register_order", order_context_register_order, METH_VARARGS,
     nullptr},
    {"_is_never", order_context_is_never, METH_O, nullptr},
    {}  // Sentinel.
};

static PyObject *order_context_new(PyTypeObject *type, PyObject *args,
                                   PyObject *Py_UNUSED(kwds)) {
    PyObject *context;
    if(!PyArg_ParseTuple(args, "O!", &context_type_object, &context))
        return nullptr;

    auto *self = order_context_object::from_pyobject(
        type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
        return nullptr;

    Py_INCREF(context);
    self->context_holder = context;

    eqbool::order_context &orders = self->orders;
    ::new(&orders) eqbool::order_context(
        context_object::from_pyobject(context)->context);

    return &self->ob_base;
}

static void order_context_dealloc(PyObject *self) {
    auto &object = *order_context_object::from_pyobject(self);
    object.orders.~order_context();
    Py_XDECREF(object.context_holder);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject order_context_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._OrderContext",  // tp_name
    sizeof(order_context_object),  // tp_basicsize
    0,                          // tp_itemsize
    order_context_dealloc,      // tp_dealloc
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
    "OrderContext.",            // tp_doc
    nullptr,                    // tp_traverse
    nullptr,                    // tp_clear
    nullptr,                    // tp_richcompare
    0,                          // tp_weaklistoffset
    nullptr,                    // tp_iter
    nullptr,                    // tp_iternext
    order_context_methods,      // tp_methods
    nullptr,                    // tp_members
    nullptr,                    // tp_getset
    nullptr,                    // tp_base
    nullptr,                    // tp_dict
    nullptr,                    // tp_descr_get
    nullptr,                    // tp_descr_set
    0,                          // tp_dictoffset
    nullptr,                    // tp_init
    nullptr,                    // tp_alloc
    order_context_new,          // tp_new
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
    // Later fields, such as tp_watched, do not exist in all
    // supported Python versions and are left value-initialised.
};

struct equiv_session_object {
    PyObject_HEAD
    PyObject *context_holder;
    eqbool::equiv_session session;

    static equiv_session_object *from_pyobject(PyObject *p) {
        return reinterpret_cast<equiv_session_object*>(p);
    }
};

static PyObject *equiv_session_is_equiv(PyObject *self, PyObject *args);

static PyMethodDef equiv_session_methods[] = {
    {"_is_equiv", equiv_session_is_equiv, METH_VARARGS, nullptr},
    {}  // Sentinel.
};

static PyObject *equiv_session_new(PyTypeObject *type, PyObject *args,
                                   PyObject *Py_UNUSED(kwds)) {
    PyObject *context;
    if(!PyArg_ParseTuple(args, "O!", &context_type_object, &context))
        return nullptr;

    auto *self = equiv_session_object::from_pyobject(
        type->tp_alloc(type, /* nitems= */ 0));
    if(!self)
        return nullptr;

    Py_INCREF(context);
    self->context_holder = context;

    eqbool::equiv_session &session = self->session;
    ::new(&session) eqbool::equiv_session(
        context_object::from_pyobject(context)->context);

    return &self->ob_base;
}

static void equiv_session_dealloc(PyObject *self) {
    auto &object = *equiv_session_object::from_pyobject(self);
    object.session.~equiv_session();
    Py_XDECREF(object.context_holder);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject equiv_session_type_object = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "eqbool._eqbool._EquivSession",  // tp_name
    sizeof(equiv_session_object),  // tp_basicsize
    0,                          // tp_itemsize
    equiv_session_dealloc,      // tp_dealloc
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
    "EquivSession.",            // tp_doc
    nullptr,                    // tp_traverse
    nullptr,                    // tp_clear
    nullptr,                    // tp_richcompare
    0,                          // tp_weaklistoffset
    nullptr,                    // tp_iter
    nullptr,                    // tp_iternext
    equiv_session_methods,      // tp_methods
    nullptr,                    // tp_members
    nullptr,                    // tp_getset
    nullptr,                    // tp_base
    nullptr,                    // tp_dict
    nullptr,                    // tp_descr_get
    nullptr,                    // tp_descr_set
    0,                          // tp_dictoffset
    nullptr,                    // tp_init
    nullptr,                    // tp_alloc
    equiv_session_new,          // tp_new
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
    // Later fields, such as tp_watched, do not exist in all
    // supported Python versions and are left value-initialised.
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

static PyObject *bool_get_id(PyObject *Py_UNUSED(self), PyObject *p) {
    return PyLong_FromSize_t(eqbool_from_pyobject(p).get_id());
}

static PyObject *bool_get_fp(PyObject *Py_UNUSED(self), PyObject *p) {
    return PyLong_FromUnsignedLongLong(eqbool_from_pyobject(p).get_fp());
}

static const char *get_kind_name(eqbool::node_kind kind) {
    switch(kind) {
    case eqbool::node_kind::term:
        return "term";
    case eqbool::node_kind::or_node:
        return "or";
    case eqbool::node_kind::ifelse:
        return "ifelse";
    case eqbool::node_kind::eq:
        return "eq";
    }
    eqbool::unreachable("unknown node kind");
}

static PyObject *bool_get_kind(PyObject *Py_UNUSED(self), PyObject *p) {
    eqbool::eqbool v = eqbool_from_pyobject(p);
    const char *kind;
    if(v.is_const())
        kind = v.is_false() ? "false" : "true";
    else if(v.is_inversion())
        kind = "not";
    else
        kind = get_kind_name(v.get_kind());
    return PyUnicode_FromString(kind);
}

static PyObject *bool_get_term(PyObject *Py_UNUSED(self), PyObject *p) {
    eqbool::eqbool v = eqbool_from_pyobject(p);
    auto *term = reinterpret_cast<PyObject*>(v.get_term());
    Py_INCREF(term);
    return term;
}

static PyObject *bool_get_args(PyObject *Py_UNUSED(self), PyObject *p) {
    eqbool::args_ref args = eqbool_from_pyobject(p).get_args();
    std::size_t num_args = args.size();
    PyObject *list = PyList_New(static_cast<Py_ssize_t>(num_args));
    if(!list)
        return nullptr;

    for(std::size_t i = 0; i != num_args; ++i) {
        PyObject *arg = pyobject_from_eqbool(args[i]);
        if(!arg) {
            Py_DECREF(list);
            return nullptr;
        }

        PyList_SET_ITEM(list, static_cast<Py_ssize_t>(i), arg);
    }

    return list;
}

static PyObject *bool_print(PyObject *Py_UNUSED(self), PyObject *p) {
    std::ostringstream ss;
    ss << eqbool_from_pyobject(p);
    return PyUnicode_FromStringAndSize(
        ss.str().c_str(),
        static_cast<Py_ssize_t>(ss.str().size()));
}

static PyObject *context_get(PyObject *self, PyObject *arg) {
    auto &context = context_object::from_pyobject(self)->context;
    eqbool::eqbool v;
    if(arg == Py_False) {
        v = context.get_false();
    } else if(arg == Py_True) {
        v = context.get_true();
    } else {
        // The term is uniquified on the Python side.
        v = context.get(reinterpret_cast<uintptr_t>(arg));
    }

    return pyobject_from_eqbool(v);
}

static bool get_args(std::vector<eqbool::eqbool> &v, PyObject *args) {
    Py_ssize_t n = PyTuple_Size(args);
    for(Py_ssize_t i = 0; i != n; ++i) {
        PyObject *arg = PyTuple_GetItem(args, i);
        v.push_back(eqbool_from_pyobject(arg));
    }
    return true;
}

static PyObject *context_get_or(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    auto &context = context_object::from_pyobject(self)->context;
    return pyobject_from_eqbool(context.get_or(v));
}

static PyObject *context_ifelse(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    if(v.size() != 3) {
        PyErr_SetString(PyExc_TypeError, "Expected exactly 3 arguments");
        return nullptr;
    }

    auto &context = context_object::from_pyobject(self)->context;
    return pyobject_from_eqbool(context.ifelse(v[0], v[1], v[2]));
}

static PyObject *context_get_eq(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    if(v.size() != 2) {
        PyErr_SetString(PyExc_TypeError, "Expected exactly 2 arguments");
        return nullptr;
    }

    auto &context = context_object::from_pyobject(self)->context;
    return pyobject_from_eqbool(context.get_eq(v[0], v[1]));
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

static PyObject *equiv_session_is_equiv(PyObject *self, PyObject *args) {
    std::vector<eqbool::eqbool> v;
    if(!get_args(v, args))
        return nullptr;

    if(v.size() != 2) {
        PyErr_SetString(PyExc_TypeError, "Expected exactly 2 arguments");
        return nullptr;
    }

    auto &session = equiv_session_object::from_pyobject(self)->session;
    if(session.is_equiv(v[0], v[1]))
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

static PyObject *order_context_register_order(PyObject *self,
                                              PyObject *args) {
    PyObject *term, *before, *after;
    if(!PyArg_ParseTuple(args, "OOO", &term, &before, &after))
        return nullptr;

    // The sides are uniquified on the Python side.
    auto &orders = order_context_object::from_pyobject(self)->orders;
    orders.register_order(eqbool_from_pyobject(term),
                          reinterpret_cast<uintptr_t>(before),
                          reinterpret_cast<uintptr_t>(after));
    Py_RETURN_NONE;
}

static PyObject *order_context_is_never(PyObject *self, PyObject *p) {
    auto &orders = order_context_object::from_pyobject(self)->orders;
    if(orders.is_never(eqbool_from_pyobject(p)))
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

}  // anonymous namespace

PyMODINIT_FUNC PyInit__eqbool(void);

PyMODINIT_FUNC PyInit__eqbool(void) {
    PyObject *m = PyModule_Create(&module);
    if(!m)
        return nullptr;

    if(PyType_Ready(&context_type_object) < 0)
        return nullptr;

    Py_INCREF(&context_type_object);

    if (PyModule_AddObject(m, "_Context",
                           &context_type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&context_type_object);
        Py_DECREF(m);
        return nullptr;
    }

    if(PyType_Ready(&equiv_session_type_object) < 0)
        return nullptr;

    Py_INCREF(&equiv_session_type_object);

    if (PyModule_AddObject(m, "_EquivSession",
                           &equiv_session_type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&equiv_session_type_object);
        Py_DECREF(m);
        return nullptr;
    }

    if(PyType_Ready(&order_context_type_object) < 0)
        return nullptr;

    Py_INCREF(&order_context_type_object);

    if (PyModule_AddObject(m, "_OrderContext",
                           &order_context_type_object.ob_base.ob_base) < 0) {
        Py_DECREF(&order_context_type_object);
        Py_DECREF(m);
        return nullptr;
    }

    return m;
}
