
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include "eqbool.h"

namespace eqbool {

eqbool eqbool_context::get(const char *term) {
    return eqbool(term, *this);
}

eqbool eqbool_context::get_or(args_ref args) {
    if(args.size() == 0)
        return eqfalse;

    // TODO
    assert(0);
}

eqbool eqbool_context::invert(eqbool e) {
    if(e.is_const())
        return e.is_false() ? eqtrue : eqfalse;

    // TODO
    assert(0);
}

}  // namesapce eqbool
