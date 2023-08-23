
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <algorithm>
#include <ostream>

#include "eqbool.h"

namespace eqbool {

namespace {

template<typename C, typename E>
bool contains(const C &c, const E &e) {
    return std::find(c.begin(), c.end(), e) != c.end();
}

}  // anonymous namespace

bool eqbool::operator == (const eqbool &other) const {
    assert(&get_context() == &other.get_context());

    if(kind != other.kind || term != other.term)
        return false;
    if(args.size() != other.args.size())
        return false;
    for(size_t i = 0, e = args.size(); i != e; ++i) {
        if(args[i] != other.args[i])
            return false;
    }
    return true;
}

eqbool eqbool_context::get(const char *term) {
    return eqbool(term, *this);
}

eqbool eqbool_context::get_or(args_ref args) {
    std::vector<eqbool> selected_args;
    for(eqbool a : args) {
        check(a);
        if(a.is_false())
            continue;
        if(a.is_true() || contains(selected_args, ~a))
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
    for(eqbool &a : or_args) {
        check(a);
        a = ~a;
    }
    return ~get_or(or_args);
}

eqbool eqbool_context::get_eq(eqbool a, eqbool b) {
    check(a);
    check(b);

    if(b.is_false())
        return ~a;
    if(a.is_const() && b.is_const())
        return get(a.is_false() == b.is_false());

    // TODO
    assert(0);
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    if(i.is_const())
        return i.is_true() ? t : e;

    // TODO
    assert(0);
}

eqbool eqbool_context::invert(eqbool e) {
    check(e);
    if(e.is_const())
        return get(!e.is_true());
    if(e.kind == eqbool::node_kind::not_node)
        return e.args[0];

    return eqbool(eqbool::node_kind::not_node, {e}, *this);
}

bool eqbool_context::is_unsat(eqbool e) {
    if(e.is_const())
        return e.is_false();

    // TODO
    assert(0);
}

bool eqbool_context::is_equiv(eqbool a, eqbool b) {
    return is_unsat(~get_eq(a, b));
}

std::ostream &eqbool_context::dump(std::ostream &s, eqbool e) const {
    switch(e.kind) {
    case eqbool::node_kind::none:
        return s << e.term;
    case eqbool::node_kind::not_node:
        return s << "not " << e.args[0];
    }

    // TODO
    assert(0);
}

}  // namesapce eqbool
