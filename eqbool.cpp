
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
    std::vector<eqbool> selected_args;
    for(eqbool a : args) {
        if(a.is_false())
            continue;
        if(a.is_true())
            return eqtrue;
        selected_args.push_back(a);
    }

    if(selected_args.empty())
        return eqfalse;
    if(selected_args.size() == 1)
        return selected_args[0];

    // TODO
    assert(0);
}

eqbool eqbool_context::get_and(args_ref args) {
    std::vector<eqbool> or_args(args.begin(), args.end());
    for(eqbool &a : or_args)
        a = ~a;
    return ~get_or(or_args);
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    if(i.is_const())
        return i.is_true() ? t : e;

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
