
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include "eqbool.h"

namespace eqbool {

eqbool eqbool_context::get(const char *term) {
    return eqbool(term);
}

}  // namesapce eqbool
