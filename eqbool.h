
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

class timer {
private:
    static double now() {
        timespec t;
        ::clock_gettime(CLOCK_MONOTONIC, &t);
        return static_cast<double>(t.tv_sec) +
               static_cast<double>(t.tv_nsec) / 1e9;
    }

    double start = now();
    double &total;

public:
    timer(double &total) : total(total) {}
    ~timer() { update(); }

    void update() {
        double t = now();
        total += t - start;
        start = t;
    }
};

namespace detail {

enum class node_kind { none, or_node, ifelse, eq };

constexpr uintptr_t inversion_flag = 1;

struct node_def;

struct hasher {
    template <class T>
    static void hash(std::size_t &seed, const T &v) {
        std::hash<T> h;
        seed ^= h(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    static void flatten_args(std::vector<eqbool> &flattened, args_ref args);

    std::size_t operator () (const node_def &def) const;
};

struct matcher {
    bool operator () (const node_def &a, const node_def &b) const;
};

struct node_def {
    eqbool_context *context = nullptr;
    std::size_t id = 0;
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
};

}  // namespace detail

class eqbool {
private:
    using node_def = detail::node_def;

    // TODO: Should default to reinterpret_cast<uintptr_t>(nullptr)?
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

    // Defines the canonical order. Nodes created earlier are
    // guaranteed to come before nodes created later. Also,
    // inversions always come immediately after their
    // non-inverted versions.
    std::size_t get_id() const {
        uintptr_t def = def_code & ~detail::inversion_flag;
        assert(def);
        return reinterpret_cast<node_def*>(def)->id * 2 + is_inversion();
    }

    friend class eqbool_context;
    friend struct detail::hasher;

public:
    eqbool() = default;

    bool is_void() const { return def_code == 0; }
    operator bool() const { return !is_void(); }

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

    bool operator < (const eqbool &other) const {
        assert(&get_context() == &other.get_context());
        return get_id() < other.get_id();
    }

    eqbool operator ~ () const;
    eqbool operator | (eqbool other) const;
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

    const eqbool &operator [] (size_t i) const {  return ptr[i]; }

    const eqbool *begin() const { return data(); }
    const eqbool *end() const { return data() + size(); }
};

struct eqbool_stats {
    double sat_time = 0;
    double clauses_time = 0;
    unsigned long num_sat_solutions = 0;
    unsigned long num_clauses = 0;
};

class eqbool_context {
private:
    using node_def = detail::node_def;

    std::unordered_map<node_def, eqbool, detail::hasher, detail::matcher> defs;

    eqbool_stats stats;

    eqbool eqfalse = get("0");
    eqbool eqtrue = ~eqfalse;

    eqbool add_def(node_def def);

    void check(eqbool e) const {
        unused(&e);
        assert(&e.get_context() == this);
    }

    int skip_not(eqbool &e,
                 std::unordered_map<const node_def*, int> &literals);

    bool contains_another(args_ref args, const eqbool &e) const;

    // Attempts to simplify e given falses are all false.
    eqbool simplify(args_ref falses, const eqbool &e) const;

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

    eqbool get_or(args_ref args, bool invert_args = false);
    eqbool get_or(eqbool a, eqbool b) { return get_or({a, b}); }
    eqbool get_and(args_ref args, bool invert_args = false) {
        return ~get_or(args, !invert_args);
    }
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

inline bool eqbool::is_false() const {
    return get_context().is_false(*this);
}

inline bool eqbool::is_true() const {
    return get_context().is_true(*this);
}

inline eqbool eqbool::operator ~ () const {
    return get_context().invert(*this);
}

inline eqbool eqbool::operator | (eqbool other) const {
    return get_context().get_or(*this, other);
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
