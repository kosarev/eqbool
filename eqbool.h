
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
};

class eqbool_context {
public:
    eqbool get(const char *term);
};

}  // namespace eqbool

#endif
