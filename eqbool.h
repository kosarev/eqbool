
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

class eqbool {
private:
    enum class node_kind { none, not_node };

    eqbool_context *context = nullptr;
    node_kind kind = node_kind::none;
    std::string term;
    std::vector<eqbool> args;

    eqbool(const char *term, eqbool_context &context)
        : context(&context), term(term) {}

    eqbool(node_kind kind, args_ref args, eqbool_context &context);

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
        return !(term == other.term);
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

class eqbool_context {
private:
    eqbool eqfalse{"0", *this};
    eqbool eqtrue{"1", *this};

    void check(eqbool e) const { assert(&e.get_context() == this); }

public:
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

    bool is_unsat(eqbool e);
    bool is_equiv(eqbool a, eqbool b);

    std::ostream &dump(std::ostream &s, eqbool e) const;
};

inline eqbool::eqbool(node_kind kind, args_ref args, eqbool_context &context)
    : context(&context), kind(kind), args(args.begin(), args.end()) {}

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
