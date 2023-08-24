
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <algorithm>
#include <ostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wextra-semi"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#include "cadical/src/cadical.hpp"
#pragma GCC diagnostic pop

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

eqbool_context::eqbool_context() {
    assert(eqfalse.sat_literal == 1);
    assert(eqtrue.sat_literal == 2);
    assert(sat_literal_count == 2);
}

eqbool eqbool_context::get(const char *term) {
    return eqbool(term, get_sat_literal(), *this);
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

    return eqbool(eqbool::node_kind::or_node, selected_args,
                  get_sat_literal(), *this);
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

    // XOR gates take the same number of clauses with the same
    // number of literals as IFELSE gates, so it doesn't make
    // sense to have special support for them.
    return eqbool(eqbool::node_kind::ifelse, {a, b, ~b},
                  get_sat_literal(), *this);
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    if(i.is_const())
        return i.is_true() ? t : e;

    return eqbool(eqbool::node_kind::ifelse, {i, t, e},
                  get_sat_literal(), *this);
}

eqbool eqbool_context::invert(eqbool e) {
    check(e);
    if(e.is_const())
        return get(!e.is_true());
    if(e.kind == eqbool::node_kind::not_node)
        return e.args[0];

    return eqbool(eqbool::node_kind::not_node, {e},
                  get_sat_literal(), *this);
}

int eqbool_context::skip_not(eqbool &e) {
    if(e.kind != eqbool::node_kind::not_node)
        return e.sat_literal;

    e = e.args[0];
    assert(e.kind != eqbool::node_kind::not_node);
    return -e.sat_literal;
}

bool eqbool_context::is_unsat(eqbool e) {
    if(e.is_const())
        return e.is_false();

    auto *solver = new CaDiCaL::Solver;

    solver->add(skip_not(e));
    solver->add(0);

    std::vector<eqbool> worklist({e});
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        int r_lit = n.sat_literal;

        switch(n.kind) {
        case eqbool::node_kind::none:
            if(n.is_const()) {
                solver->add(n.is_true() ? r_lit : -r_lit);
                solver->add(0);
            }
            continue;
        case eqbool::node_kind::or_node: {
            std::vector<int> arg_lits;
            for(eqbool a : n.args) {
                int a_lit = skip_not(a);
                solver->add(-a_lit);
                solver->add(r_lit);
                solver->add(0);

                arg_lits.push_back(a_lit);
                worklist.push_back(a);
            }

            for(int a_lit : arg_lits)
                solver->add(a_lit);
            solver->add(-r_lit);
            solver->add(0);
            continue;
        }
        case eqbool::node_kind::ifelse: {
            eqbool i_arg = n.args[0], t_arg = n.args[1], e_arg = n.args[2];
            int i_lit = skip_not(i_arg), t_lit = skip_not(t_arg),
                e_lit = skip_not(e_arg);

            solver->add(-i_lit);
            solver->add(t_lit);
            solver->add(-r_lit);
            solver->add(0);

            solver->add(-i_lit);
            solver->add(-t_lit);
            solver->add(r_lit);
            solver->add(0);

            solver->add(i_lit);
            solver->add(e_lit);
            solver->add(-r_lit);
            solver->add(0);

            solver->add(i_lit);
            solver->add(-e_lit);
            solver->add(r_lit);
            solver->add(0);

            worklist.push_back(i_arg);
            worklist.push_back(t_arg);
            worklist.push_back(e_arg);
            continue;
        }
        case eqbool::node_kind::not_node:
            // Inversions should all be translated to negative
            // literals by now.
            assert(0);
        }
        assert(0);
    }

    bool unsat = solver->solve() == 20;

    ++sat_solve_count;

    delete solver;

    return unsat;
}

bool eqbool_context::is_equiv(eqbool a, eqbool b) {
    return is_unsat(~get_eq(a, b));
}

std::ostream &eqbool_context::dump_helper(std::ostream &s, eqbool e,
                                          bool subexpr) const {
    switch(e.kind) {
    case eqbool::node_kind::none:
        return s << e.term;
    case eqbool::node_kind::or_node:
    case eqbool::node_kind::ifelse:
        if(subexpr)
            s << "(";
        s << (e.kind == eqbool::node_kind::or_node ? "or" : "ifelse");
        for(eqbool a : e.args) {
            s << " ";
            dump_helper(s, a, /* subexpr= */ true);
        }
        if(subexpr)
            s << ")";
        return s;
    case eqbool::node_kind::not_node:
        s << "~";
        dump_helper(s, e.args[0], /* subexpr= */ true);
        return s;
    }

    // TODO
    assert(0);
}

std::ostream &eqbool_context::dump(std::ostream &s, eqbool e) const {
    return dump_helper(s, e, /* subexpr= */ false);
}

}  // namesapce eqbool
