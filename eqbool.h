
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
#include <vector>

namespace eqbool {

class args_ref;
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

class eqbool {
private:
    enum class node_kind { none, or_node, ifelse, not_node };

    eqbool_context *context = nullptr;
    node_kind kind = node_kind::none;
    std::string term;
    std::vector<eqbool> args;
    int sat_literal = 0;

    eqbool(const char *term, int sat_literal, eqbool_context &context)
        : context(&context), term(term), sat_literal(sat_literal) {}

    eqbool(node_kind kind, args_ref args, int sat_literal,
           eqbool_context &context);

    eqbool_context &get_context() const {
        assert(context);
        return *context;
    }

    friend class eqbool_context;

public:
    eqbool() = default;

    bool is_false() const;
    bool is_true() const;
    bool is_const() const { return is_false() || is_true(); }

    bool operator == (const eqbool &other) const;

    bool operator != (const eqbool &other) const {
        return !(*this == other);
    }

    eqbool operator ~ () const;

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
    unsigned sat_solution_count = 0;
};

class eqbool_context {
private:
    int sat_literal_count = 0;
    eqbool eqfalse{"0", get_sat_literal(), *this};
    eqbool eqtrue{"1", get_sat_literal(), *this};

    eqbool_stats stats;

    int get_sat_literal() { return ++sat_literal_count; }

    void check(eqbool e) const {
        unused(&e);
        assert(&e.get_context() == this);
    }

    int skip_not(eqbool &e);

    std::ostream &dump_helper(std::ostream &s, eqbool e, bool subexpr) const;

public:
    eqbool_context();

    bool is_false(eqbool e) const { check(e); return e == eqfalse; }
    bool is_true(eqbool e) const { check(e); return e == eqtrue; }

    eqbool get_false() /* no const */ { return eqfalse; }
    eqbool get_true() /* no const */ { return eqtrue; }
    eqbool get(bool b) { return b ? get_true() : get_false(); }
    eqbool get(const char *term);

    eqbool get_or(args_ref args);
    eqbool get_and(args_ref args);
    eqbool get_eq(eqbool a, eqbool b);
    eqbool ifelse(eqbool i, eqbool t, eqbool e);
    eqbool invert(eqbool e);

    const eqbool_stats &get_stats() const { return stats; }

    bool is_unsat(eqbool e);
    bool is_equiv(eqbool a, eqbool b);

    std::ostream &dump(std::ostream &s, eqbool e) const;
};

inline eqbool::eqbool(node_kind kind, args_ref args, int sat_literal,
                      eqbool_context &context)
    : context(&context), kind(kind), args(args.begin(), args.end()),
      sat_literal(sat_literal) {}

inline bool eqbool::is_false() const {
    return get_context().is_false(*this);
}

inline bool eqbool::is_true() const {
    return get_context().is_true(*this);
}

inline eqbool eqbool::operator ~ () const {
    return get_context().invert(*this);
}

inline std::ostream &eqbool::dump(std::ostream &s) const {
    return get_context().dump(s, *this);
}

inline std::ostream &operator << (std::ostream &s, eqbool e) {
    return e.dump(s);
}

}  // namespace eqbool

#endif
