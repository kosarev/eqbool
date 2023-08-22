
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#ifndef EQBOOL_H
#define EQBOOL_H

#include <cassert>
#include <string>
#include <vector>

namespace eqbool {

class eqbool_context;

class eqbool {
private:
    eqbool_context *context = nullptr;
    std::string term;

    eqbool(const char *term, eqbool_context &context)
        : context(&context), term(term) {}

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

    bool operator == (const eqbool &other) const {
        return term == other.term;
    }

    bool operator != (const eqbool &other) const {
        return !(term == other.term);
    }

    eqbool operator ~ () const;
};

class args_ref {
private:
    const eqbool *ptr = nullptr;
    size_t xsize = 0;

public:
    args_ref(const std::vector<eqbool> &args)
        : ptr(args.data()), xsize(args.size()) {}

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

public:
    eqbool get_false() /* no const */ { return eqfalse; }
    eqbool get_true() /* no const */ { return eqtrue; }

    eqbool get(const char *term);

    eqbool get_or(args_ref args);
    eqbool get_and(args_ref args);

    eqbool invert(eqbool e);

    friend class eqbool;
};

inline bool eqbool::is_false() const {
    return *this == get_context().eqfalse;
}

inline bool eqbool::is_true() const {
    return *this == get_context().eqtrue;
}

inline eqbool eqbool::operator ~ () const {
    return get_context().invert(*this);
}

}  // namespace eqbool

#endif
