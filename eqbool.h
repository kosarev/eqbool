
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#ifndef EQBOOL_H
#define EQBOOL_H

#include <cassert>
#include <chrono>
#include <cstdint>
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
    std::chrono::time_point<std::chrono::steady_clock> start =
        std::chrono::steady_clock::now();
    double &total;

public:
    timer(double &total) : total(total) {}
    ~timer() { update(); }

    void update() {
        std::chrono::time_point<std::chrono::steady_clock> now =
            std::chrono::steady_clock::now();
        std::chrono::duration<double> delta = now - start;
        total += delta.count();
        start = now;
    }
};

namespace detail {

enum class node_kind { term, or_node, ifelse, eq };

constexpr uintptr_t inversion_flag = 1;
constexpr uintptr_t lock_flag = 2;
constexpr uintptr_t entry_code_mask = ~(inversion_flag | lock_flag);

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
    node_kind kind = node_kind::term;
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
    using node_entry = std::pair<const node_def, eqbool>;

    // TODO: Should default to reinterpret_cast<uintptr_t>(nullptr)?
    mutable uintptr_t entry_code = 0;

    eqbool(uintptr_t entry_code)
        : entry_code(entry_code) {}

    eqbool(node_entry &entry)
            : eqbool(reinterpret_cast<uintptr_t>(&entry)) {
        assert(!(entry_code & detail::inversion_flag));
    }

    void propagate_impl() const;

    void reduce() const;

    void propagate() const {
        assert(!is_void());
        if(entry_code & detail::lock_flag)
            return;

        uintptr_t code = entry_code & detail::entry_code_mask;
        if(reinterpret_cast<node_entry*>(code)->second.entry_code != code)
            propagate_impl();

        for(eqbool a : reinterpret_cast<node_entry*>(
                entry_code & detail::entry_code_mask)->first.args) {
            uintptr_t a_code = a.entry_code & detail::entry_code_mask;
            auto &a_entry = *reinterpret_cast<node_entry*>(a_code);
            if(a_entry.second.entry_code != a_code || a_entry.first.id < 2) {
                reduce();
                break;
            }
        }
    }

    node_entry &get_entry() const {
        assert(!is_inversion());
        uintptr_t entry = entry_code & detail::entry_code_mask;
        return *reinterpret_cast<node_entry*>(entry);
    }

    const node_def &get_def() const {
        return get_entry().first;
    }

    bool is_inversion() const {
        assert(!is_void());
        return entry_code & detail::inversion_flag;
    }

    eqbool_context &get_context() const {
        assert(!is_void());
        uintptr_t entry = entry_code & detail::entry_code_mask;
        return reinterpret_cast<node_entry*>(entry)->first.get_context();
    }

    // Defines the canonical order. Nodes created earlier are
    // guaranteed to come before nodes created later. Also,
    // inversions always come immediately after their
    // non-inverted versions.
    std::size_t get_id() const {
        assert(!is_void());
        uintptr_t entry = entry_code & detail::entry_code_mask;
        return reinterpret_cast<node_entry*>(entry)->first.id * 2 +
               is_inversion();
    }

    friend class eqbool_context;
    friend struct detail::hasher;

public:
    eqbool() = default;

    bool is_void() const { return entry_code == 0; }
    explicit operator bool() const { return !is_void(); }

    bool is_false() const { return get_id() == 0; }
    bool is_true() const { return get_id() == 1; }
    bool is_const() const { return get_id() < 2; }

    bool operator == (const eqbool &other) const {
        assert(&get_context() == &other.get_context());
        return entry_code == other.entry_code;
    }

    bool operator != (const eqbool &other) const {
        return !(*this == other);
    }

    bool operator < (const eqbool &other) const {
        assert(&get_context() == &other.get_context());
        return get_id() < other.get_id();
    }

    eqbool operator ~ () const {
        assert(!is_void());
        return eqbool(entry_code ^ detail::inversion_flag);
    }

    eqbool operator | (eqbool other) const;
    eqbool operator & (eqbool other) const;

    eqbool operator ^ (bool inv) const { return inv ? ~*this : *this; }

    std::ostream &print(std::ostream &s) const;
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

    eqbool get_value(std::vector<eqbool> &eqs, eqbool assumed_false) const;

    static void add_eq(std::vector<eqbool> &eqs, eqbool e);

    eqbool evaluate(args_ref assumed_falses, const eqbool &excluded,
                    std::vector<eqbool> &eqs) const;

    eqbool evaluate(args_ref assumed_falses, const eqbool &excluded,
                    eqbool e, std::vector<eqbool> &eqs) const;

    eqbool evaluate(args_ref assumed_falses, const eqbool &excluded,
                    eqbool e) const;

    static bool contains_all(args_ref p, args_ref q);

    // Attempts to reduce e to one of its direct or indirect operands or
    // a constant, assuming all args that are not e are false.
    eqbool reduce_impl(args_ref assumed_falses, const eqbool &e) const;
    eqbool reduce(args_ref assumed_falses, eqbool e) const;

    eqbool ifelse_impl(eqbool i, eqbool t, eqbool e);

    void store_equiv(eqbool a, eqbool b);

    std::ostream &print_helper(std::ostream &s, eqbool e, bool subexpr,
        const std::unordered_map<const node_def*, unsigned> &ids,
        std::vector<eqbool> &worklist) const;

    // Dumps nodes in order of creation. Helps reproduce and debug
    // simplifications.
    std::ostream &dump(std::ostream &s, args_ref nodes) const;

    friend eqbool;

public:
    eqbool_context() = default;

    eqbool get_false() const { return eqfalse; }
    eqbool get_true() const { return eqtrue; }
    eqbool get(bool b) const { return b ? get_true() : get_false(); }
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

    const eqbool_stats &get_stats() const { return stats; }

    bool is_unsat(eqbool e);
    bool is_equiv(eqbool a, eqbool b);

    std::ostream &print(std::ostream &s, eqbool e) const;
};

inline detail::node_def::node_def(node_kind kind, args_ref args,
                                  eqbool_context &context)
    : context(&context), kind(kind), args(args.begin(), args.end())
{}

inline eqbool eqbool::operator | (eqbool other) const {
    return get_context().get_or(*this, other);
}

inline eqbool eqbool::operator & (eqbool other) const {
    return get_context().get_and(*this, other);
}

inline std::ostream &eqbool::print(std::ostream &s) const {
    return get_context().print(s, *this);
}

inline std::ostream &operator << (std::ostream &s, eqbool e) {
    return e.print(s);
}

}  // namespace eqbool

#endif
