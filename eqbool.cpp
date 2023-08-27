
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <algorithm>
#include <ctime>
#include <ostream>
#include <unordered_set>

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

using detail::node_def;
using detail::node_kind;

namespace {

template<typename C, typename E>
bool contains(const C &c, const E &e) {
    return std::find(c.begin(), c.end(), e) != c.end();
}

}  // anonymous namespace

eqbool_context::eqbool_context() {
    assert(eqfalse.get_def().sat_literal == 1);
    assert(eqtrue.get_def().sat_literal == 2);
    assert(sat_literal_count == 2);
}

node_def &eqbool_context::get_def(const node_def &def) {
    // Store node definitions as keys in a hash table and map
    // them to pointers to themselves.
    auto r = defs.insert({def, nullptr});
    auto &i = r.first;
    node_def &key = const_cast<node_def&>(i->first);
    node_def *&value = i->second;
    bool inserted = r.second;
    if(inserted)
        value = &key;
    return *value;
}

eqbool eqbool_context::get(const char *term) {
    return eqbool(get_def(node_def(term, get_sat_literal(), *this)));
}

eqbool eqbool_context::get_or(args_ref args) {
    std::vector<eqbool> selected_args;
    for(eqbool a : args) {
        check(a);
        if(a.is_false() || contains(selected_args, a))
            continue;
        if(a.is_true() || contains(selected_args, ~a))
            return eqtrue;
        selected_args.push_back(a);
    }

    if(selected_args.empty())
        return eqfalse;
    if(selected_args.size() == 1)
        return selected_args[0];

    node_def def(node_kind::or_node, selected_args,
                 get_sat_literal(), *this);
    return eqbool(get_def(def));
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

    if(a.is_true())
        return b;
    if(b.is_true())
        return a;
    if(a.is_false())
        return ~b;
    if(b.is_false())
        return ~a;

    if(a == b)
        return eqtrue;

    // XOR gates take the same number of clauses with the same
    // number of literals as IFELSE gates, so it doesn't make
    // sense to have special support for them.
    node_def def(node_kind::ifelse, {a, b, ~b},
                 get_sat_literal(), *this);
    return eqbool(get_def(def));
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    if(i.is_const())
        return i.is_true() ? t : e;

    if(t.is_true() && e.is_false())
        return i;

    if(t == e)
        return t;

    node_def def(node_kind::ifelse, {i, t, e},
                  get_sat_literal(), *this);
    return eqbool(get_def(def));
}

eqbool eqbool_context::invert(eqbool e) {
    check(e);
    if(e.is_const())
        return get(!e.is_true());
    if(e.get_def().kind == node_kind::not_node)
        return e.get_def().args[0];

    node_def def(node_kind::not_node, {e},
                  get_sat_literal(), *this);
    return eqbool(get_def(def));
}

int eqbool_context::skip_not(eqbool &e) {
    if(e.get_def().kind != node_kind::not_node)
        return e.get_def().sat_literal;

    e = e.get_def().args[0];
    assert(e.get_def().kind != node_kind::not_node);
    return -e.get_def().sat_literal;
}

bool eqbool_context::is_unsat(eqbool e) {
    if(e.is_const())
        return e.is_false();

    auto *solver = new CaDiCaL::Solver;

    {
    timer t(stats.clauses_time);

    solver->add(skip_not(e));
    solver->add(0);

    std::vector<eqbool> worklist({e});
    std::unordered_set<const node_def*> visited;
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();
        visited.insert(&n.get_def());

        int r_lit = n.get_def().sat_literal;

        switch(n.get_def().kind) {
        case node_kind::none:
            if(n.is_const()) {
                solver->add(n.is_true() ? r_lit : -r_lit);
                solver->add(0);
            }
            continue;
        case node_kind::or_node: {
            std::vector<int> arg_lits;
            for(eqbool a : n.get_def().args) {
                int a_lit = skip_not(a);
                solver->add(-a_lit);
                solver->add(r_lit);
                solver->add(0);
                ++stats.num_clauses;

                arg_lits.push_back(a_lit);
                if(!contains(visited, &a.get_def()))
                    worklist.push_back(a);
            }

            for(int a_lit : arg_lits)
                solver->add(a_lit);
            solver->add(-r_lit);
            solver->add(0);
            ++stats.num_clauses;
            continue;
        }
        case node_kind::ifelse: {
            eqbool i_arg = n.get_def().args[0];
            eqbool t_arg = n.get_def().args[1];
            eqbool e_arg = n.get_def().args[2];
            int i_lit = skip_not(i_arg);
            int t_lit = skip_not(t_arg);
            int e_lit = skip_not(e_arg);

            solver->add(-i_lit);
            solver->add(t_lit);
            solver->add(-r_lit);
            solver->add(0);
            ++stats.num_clauses;

            solver->add(-i_lit);
            solver->add(-t_lit);
            solver->add(r_lit);
            solver->add(0);
            ++stats.num_clauses;

            solver->add(i_lit);
            solver->add(e_lit);
            solver->add(-r_lit);
            solver->add(0);
            ++stats.num_clauses;

            solver->add(i_lit);
            solver->add(-e_lit);
            solver->add(r_lit);
            solver->add(0);
            ++stats.num_clauses;

            if(!contains(visited, &i_arg.get_def()))
                worklist.push_back(i_arg);
            if(!contains(visited, &t_arg.get_def()))
                worklist.push_back(t_arg);
            if(!contains(visited, &e_arg.get_def()))
                worklist.push_back(e_arg);
            continue;
        }
        case node_kind::not_node:
            unreachable("unskipped NOT node encountered");
        }
        unreachable("unknown node kind");
    }
    }

    bool unsat;
    {
        timer t(stats.sat_time);
        unsat = solver->solve() == 20;
    }

    ++stats.num_sat_solutions;

    delete solver;

    return unsat;
}

bool eqbool_context::is_equiv(eqbool a, eqbool b) {
    return is_unsat(~get_eq(a, b));
}

std::ostream &eqbool_context::dump_helper(std::ostream &s, eqbool e,
                                          bool subexpr) const {
    switch(e.get_def().kind) {
    case node_kind::none:
        return s << e.get_def().term;
    case node_kind::or_node:
    case node_kind::ifelse:
        if(subexpr)
            s << "(";
        s << (e.get_def().kind == node_kind::or_node ? "or" : "ifelse");
        for(eqbool a : e.get_def().args) {
            s << " ";
            dump_helper(s, a, /* subexpr= */ true);
        }
        if(subexpr)
            s << ")";
        return s;
    case node_kind::not_node:
        s << "~";
        dump_helper(s, e.get_def().args[0], /* subexpr= */ true);
        return s;
    }
    unreachable("unknown node kind");
}

std::ostream &eqbool_context::dump(std::ostream &s, eqbool e) const {
    return dump_helper(s, e, /* subexpr= */ false);
}

}  // namesapce eqbool
