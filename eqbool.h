
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#ifndef EQBOOL_H
#define EQBOOL_H

#include <cassert>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace eqbool {

class args_ref;
class eqbool;
class eqbool_context;

static inline void unused(...) {}

[[noreturn]] static inline void unreachable(const char *msg) {
#if !defined(NDEBUG)
     std::fprintf(stderr, "%s\n", msg);
    std::abort();
#elif defined(_MSC_VER)
    unused(msg);
    __assume(0);
#else
    unused(msg);
    __builtin_unreachable();
#endif
}

namespace detail {

enum class node_kind { none, or_node, ifelse };

constexpr uintptr_t inversion_flag = 1;

struct node_def {
    eqbool_context *context = nullptr;
    node_kind kind = node_kind::none;
    std::string term;
    std::vector<eqbool> args;

    node_def(const char *term, eqbool_context &context)
        : context(&context), term(term) {}

    node_def(node_kind kind, args_ref args, eqbool_context &context);

    eqbool_context &get_context() const {
        assert(context);
        return *context;
    }

    struct hasher {
        template <class T>
        static void hash(std::size_t &seed, const T &v) {
            std::hash<T> h;
            seed ^= h(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        std::size_t operator () (const node_def &def) const;
    };

    struct matcher {
        bool operator () (const node_def &a, const node_def &b) const;
    };
};

}  // namespace detail

class eqbool {
private:
    using node_def = detail::node_def;

    uintptr_t def_code = 0;

    eqbool(uintptr_t def_code)
        : def_code(def_code) {}

    eqbool(const node_def &def)
            : eqbool(reinterpret_cast<uintptr_t>(&def)) {
        assert(!is_inversion());
    }

    const node_def &get_def() const {
        assert(!is_inversion());
        assert(def_code);
        return *reinterpret_cast<node_def*>(def_code);
    }

    bool is_inversion() const {
        return def_code & detail::inversion_flag;
    }

    eqbool_context &get_context() const {
        uintptr_t def = def_code & ~detail::inversion_flag;
        assert(def);
        return reinterpret_cast<node_def*>(def)->get_context();
    }

    friend class eqbool_context;
    friend struct node_def::hasher;

public:
    eqbool() = default;

    bool is_false() const;
    bool is_true() const;
    bool is_const() const { return is_false() || is_true(); }

    bool operator == (const eqbool &other) const {
        assert(&get_context() == &other.get_context());
        return def_code == other.def_code;
    }

    bool operator != (const eqbool &other) const {
        return !(*this == other);
    }

    // Defines an (implementation-defined) canonical order.
    bool operator < (const eqbool &other) const {
        assert(&get_context() == &other.get_context());
        return def_code < other.def_code;
    }

    eqbool operator ~ () const;
    eqbool operator & (eqbool other) const;

    std::ostream &dump(std::ostream &s) const;
};

class args_ref {
private:
    const eqbool *ptr = nullptr;
    size_t xsize = 0;

    args_ref(const eqbool *ptr, size_t size) : ptr(ptr), xsize(size) {}

public:
    args_ref(const std::vector<eqbool> &args)
        : args_ref(args.data(), args.size()) {}
    args_ref(std::initializer_list<eqbool> args)
        : args_ref(args.begin(), args.size()) {}

    const eqbool *data() const { return ptr; }
    size_t size() const { return xsize; }
    bool empty() const { return size() == 0; }

    eqbool operator [] (size_t i) const {  return ptr[i]; }

    const eqbool *begin() const { return data(); }
    const eqbool *end() const { return data() + size(); }
};

struct eqbool_stats {
    long sat_time = 0;
    unsigned long sat_solution_count = 0;
    unsigned long num_clauses = 0;
};

class eqbool_context {
private:
    using node_def = detail::node_def;

    std::unordered_map<node_def, node_def*,
                       node_def::hasher, node_def::matcher> defs;

    eqbool_stats stats;

    eqbool eqfalse{add_def(node_def("0", *this))};
    eqbool eqtrue = ~eqfalse;

    node_def &add_def(const node_def &def);

    void check(eqbool e) const {
        unused(&e);
        assert(&e.get_context() == this);
    }

    int skip_not(eqbool &e,
                 std::unordered_map<const node_def*, int> &literals);

    // Attempts to simplify e given p is true.
    eqbool simplify(eqbool p, eqbool e);

    std::ostream &dump_helper(std::ostream &s, eqbool e, bool subexpr,
        const std::unordered_map<const node_def*, unsigned> &ids,
        std::vector<eqbool> &worklist) const;

public:
    eqbool_context() = default;

    bool is_false(eqbool e) const { check(e); return e == eqfalse; }
    bool is_true(eqbool e) const { check(e); return e == eqtrue; }

    eqbool get_false() /* no const */ { return eqfalse; }
    eqbool get_true() /* no const */ { return eqtrue; }
    eqbool get(bool b) { return b ? get_true() : get_false(); }
    eqbool get(const char *term);

    eqbool get_or(args_ref args);
    eqbool get_and(args_ref args);
    eqbool get_and(eqbool a, eqbool b) { return get_and({a, b}); }
    eqbool ifelse(eqbool i, eqbool t, eqbool e);

    eqbool get_eq(eqbool a, eqbool b) {
        // XOR gates take the same number of clauses with the
        // same number of literals as IFELSE gates, so it doesn't
        // make sense to have special support for them.
        return ifelse(a, b, ~b);
    }

    eqbool invert(eqbool e) {
        check(e);
        return eqbool(e.def_code ^ detail::inversion_flag);
    }

    const eqbool_stats &get_stats() const { return stats; }

    bool is_unsat(eqbool e);
    bool is_equiv(eqbool a, eqbool b);

    std::ostream &dump(std::ostream &s, eqbool e) const;
};

inline detail::node_def::node_def(node_kind kind, args_ref args,
                                  eqbool_context &context)
    : context(&context), kind(kind), args(args.begin(), args.end())
{}

inline std::size_t
detail::node_def::hasher::operator () (const node_def &def) const {
    std::size_t h = 0;
    hash(h, def.kind);
    hash(h, def.term);
    for(eqbool a : def.args)
        hash(h, a.def_code);
    return h;
}

inline bool detail::node_def::matcher::operator () (const node_def &a,
                                                    const node_def &b) const {
    assert(&a.get_context() == &b.get_context());
    return a.kind == b.kind && a.term == b.term && a.args == b.args;
}

inline bool eqbool::is_false() const {
    return get_context().is_false(*this);
}

inline bool eqbool::is_true() const {
    return get_context().is_true(*this);
}

inline eqbool eqbool::operator ~ () const {
    return get_context().invert(*this);
}

inline eqbool eqbool::operator & (eqbool other) const {
    return get_context().get_and(*this, other);
}

inline std::ostream &eqbool::dump(std::ostream &s) const {
    return get_context().dump(s, *this);
}

inline std::ostream &operator << (std::ostream &s, eqbool e) {
    return e.dump(s);
}

}  // namespace eqbool

#endif
