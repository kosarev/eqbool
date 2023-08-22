
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#ifndef EQBOOL_H
#define EQBOOL_H

#include <string>
#include <vector>

namespace eqbool {

class eqbool {
private:
    std::string term;

    eqbool(const char *term) : term(term) {}

    friend class eqbool_context;

public:
    eqbool() = default;

    bool operator != (const eqbool &other) const {
        return term != other.term;
    }
};

class args_ref {
private:
    const eqbool *ptr = nullptr;
    size_t xsize = 0;

public:
    args_ref() = default;

    args_ref(const std::vector<eqbool> &args)
        : ptr(args.data()), xsize(args.size()) {}

    size_t size() const { return xsize; }
};

class eqbool_context {
private:
    eqbool eqfalse{"0"};
    eqbool eqtrue{"1"};

public:
    eqbool get_false() /* no const */ { return eqfalse; }
    eqbool get_true() /* no const */ { return eqtrue; }

    eqbool get(const char *term);

    eqbool get_or(args_ref args);
};

}  // namespace eqbool

#endif
