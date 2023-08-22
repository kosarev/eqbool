
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#ifndef EQBOOL_H
#define EQBOOL_H

#include <string>

namespace eqbool {

class eqbool {
private:
    std::string term;

    eqbool(const char *term) : term(term) {}

    friend class eqbool_context;

public:
    eqbool() {}

    bool operator != (const eqbool &other) const {
        return term != other.term;
    }
};

class eqbool_context {
private:
    eqbool eqfalse{"0"};
    eqbool eqtrue{"1"};

public:
    eqbool get_false() /* no const */ { return eqfalse; }
    eqbool get_true() /* no const */ { return eqtrue; }

    eqbool get(const char *term);
};

}  // namespace eqbool

#endif
