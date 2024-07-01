
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

const node_def &eqbool_context::add_def(node_def def) {
    // Store node definitions as keys in a hash table and map
    // them to pointers to themselves.
    def.id = defs.size();
    return *defs.insert(def).first;
}

eqbool eqbool_context::get(const char *term) {
    return eqbool(add_def(node_def(term, *this)));
}

eqbool eqbool_context::get_or(args_ref args) {
    // Order the arguments before simplifications so we never
    // depend on the order they are specified in.
    std::vector<eqbool> sorted_args(args.begin(), args.end());
    std::sort(sorted_args.begin(), sorted_args.end());

    // TODO: We can reuse 'sorted_args'.
    std::vector<eqbool> selected_args;
    for(eqbool a : sorted_args) {
        check(a);
        a = simplify(selected_args, a);
        if(a.is_true())
            return eqtrue;
        if(!a.is_false())
            selected_args.push_back(a);
    }

    if(selected_args.empty())
        return eqfalse;
    if(selected_args.size() == 1)
        return selected_args[0];

    // (or (and A B) (and ~A C))  =>  (ifelse A B C)
    // (or ~(or ~A ~B) ~(or A ~C))  =>  (ifelse A B C)
    if(selected_args.size() == 2 &&
           selected_args[0].is_inversion() &&
           selected_args[1].is_inversion()) {
        const node_def &def0 = (~selected_args[0]).get_def();
        const node_def &def1 = (~selected_args[1]).get_def();
        if(def0.kind == node_kind::or_node && def0.args.size() == 2 &&
                def1.kind == node_kind::or_node && def1.args.size() == 2) {
            for(unsigned p = 0; p != 2; ++p) {
                for(unsigned q = 0; q != 2; ++q) {
                    if(def0.args[p] == ~def1.args[q]) {
                        eqbool i = ~def0.args[p];
                        eqbool t = ~def0.args[p ^ 1];
                        eqbool e = ~def1.args[q ^ 1];
                        return ifelse(i, t, e);
                    }
                }
            }
        }
    }

    // Order the arguments again to guarantee uniqueness.
    std::sort(selected_args.begin(), selected_args.end());

    node_def def(node_kind::or_node, selected_args, *this);
    return eqbool(add_def(def));
}

eqbool eqbool_context::get_and(args_ref args) {
    std::vector<eqbool> or_args(args.begin(), args.end());
    for(eqbool &a : or_args) {
        check(a);
        a = ~a;
    }
    return ~get_or(or_args);
}

eqbool eqbool_context::simplify(args_ref falses, eqbool e) {
    if(e.is_const())
        return e;

    for(eqbool f : falses) {
        if(f == e)
            return eqfalse;

        eqbool p = ~f;
        if(p == e)
            return eqtrue;

        // Dive into nested OR nodes.
        if(p.is_inversion()) {
            const node_def &def = (~p).get_def();
            if(def.kind == node_kind::or_node) {
                eqbool s = simplify(def.args, e);
                if(s != e)
                    return simplify(falses, s);
            }
        }

        if(!e.is_inversion()) {
            const node_def &def = e.get_def();
            if(def.kind == node_kind::or_node) {
                // e = (or A B), p = ~A/~B  =>  e = B/A
                if(def.args.size() == 2) {
                    if(def.args[0] == ~p)
                        return simplify(falses, def.args[1]);
                    if(def.args[1] == ~p)
                        return simplify(falses, def.args[0]);
                }

                // e = (or A...), p in A...  =>  e = 1
                // TODO: Can we use ordering to find matches quicker?
                for(eqbool a : def.args) {
                    if(a == p)
                        return eqtrue;
                }
            }
        } else {
            // e = (and A...), ~p in A...  =>  e = 0
            const node_def &def = (~e).get_def();
            if(def.kind == node_kind::or_node) {
                // e = (and A B), p = A/B  =>  e = B/A
                if(def.args.size() == 2) {
                    if(def.args[0] == ~p)
                        return simplify(falses, ~def.args[1]);
                    if(def.args[1] == ~p)
                        return simplify(falses, ~def.args[0]);
                }

                for(eqbool or_a : def.args) {
                    eqbool a = ~or_a;
                    if(a == f)
                        return eqfalse;
                }
            }
        }
    }

    return e;
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    t = simplify({~i}, t);
    e = simplify({i}, e);

    // (ifelse A (ifelse A B C) D) => (ifelse A B D)
    if(!t.is_inversion()) {
        const node_def &def = t.get_def();
        if(def.kind == node_kind::ifelse && def.args[0] == i)
            t = def.args[1];
    }

    // (ifelse A B (ifelse A C D)) => (ifelse A B D)
    if(!e.is_inversion()) {
        const node_def &def = e.get_def();
        if(def.kind == node_kind::ifelse && def.args[0] == i)
            e = def.args[2];
    }

    if(i == t)
        t = eqtrue;
    else if(i == ~t)
        t = eqfalse;

    if(i == e)
        e = eqfalse;
    else if(i == ~e)
        e = eqtrue;

    if(i.is_const())
        return i.is_true() ? t : e;

    if(t == e)
        return t;

    if(t.is_const() && e.is_const()) {
        assert(t != e);
        return t.is_true() ? i : ~i;
    }

    if(t.is_const())
        return t.is_false() ? (~i & e) : (i | e);

    if(e.is_const())
        return e.is_false() ? (i & t) : (~i | t);

    if(t == ~e && t < i)
        std::tie(i, t, e) = std::make_tuple(t, i, ~i);

    if(i.is_inversion())
        std::tie(i, t, e) = std::make_tuple(~i, e, t);

    node_def def(node_kind::ifelse, {i, t, e}, *this);
    return eqbool(add_def(def));
}

static int get_literal(const node_def *def,
        std::unordered_map<const node_def*, int> &literals) {
    int &lit = literals[def];
    if(lit == 0)
        lit = static_cast<int>(literals.size()) + 1;
    return lit;
}

int eqbool_context::skip_not(eqbool &e,
        std::unordered_map<const node_def*, int> &literals) {
    if(e.is_inversion()) {
        e = ~e;
        return -get_literal(&e.get_def(), literals);
    }

    return get_literal(&e.get_def(), literals);
}

bool eqbool_context::is_unsat(eqbool e) {
    if(e.is_const())
        return e.is_false();

    auto *solver = new CaDiCaL::Solver;

    {
    timer t(stats.clauses_time);

    std::unordered_map<const node_def*, int> literals;
    solver->add(skip_not(e, literals));
    solver->add(0);
    ++stats.num_clauses;

    std::vector<eqbool> worklist({e});
    std::unordered_set<const node_def*> visited;
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        const node_def &def = n.get_def();
        bool inserted = visited.insert(&def).second;
        if(!inserted)
            continue;

        int r_lit = literals[&def];
        assert(r_lit != 0);

        switch(def.kind) {
        case node_kind::none:
            if(n.is_const()) {
                assert(n.is_false());
                solver->add(-r_lit);
                solver->add(0);
                ++stats.num_clauses;
            }
            continue;
        case node_kind::or_node: {
            std::vector<int> arg_lits;
            for(eqbool a : def.args) {
                int a_lit = skip_not(a, literals);
                solver->add(-a_lit);
                solver->add(r_lit);
                solver->add(0);
                ++stats.num_clauses;

                arg_lits.push_back(a_lit);
                worklist.push_back(a);
            }

            for(int a_lit : arg_lits)
                solver->add(a_lit);
            solver->add(-r_lit);
            solver->add(0);
            ++stats.num_clauses;
            continue; }
        case node_kind::ifelse: {
            eqbool i_arg = def.args[0];
            eqbool t_arg = def.args[1];
            eqbool e_arg = def.args[2];
            int i_lit = skip_not(i_arg, literals);
            int t_lit = skip_not(t_arg, literals);
            int e_lit = skip_not(e_arg, literals);

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

            worklist.push_back(i_arg);
            worklist.push_back(t_arg);
            worklist.push_back(e_arg);
            continue; }
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

std::ostream &eqbool_context::dump_helper(
        std::ostream &s, eqbool e, bool subexpr,
        const std::unordered_map<const node_def*, unsigned> &ids,
        std::vector<eqbool> &worklist) const {
    bool is_and = false;
    if(e.is_inversion()) {
        if(e.is_true())
            return s << "1";
        if((~e).get_def().kind == node_kind::or_node) {
            is_and = true;
            e = ~e;
        } else {
            s << "~";
            dump_helper(s, ~e, /* subexpr= */ true, ids, worklist);
            return s;
        }
    }

    const node_def &def = e.get_def();
    switch(def.kind) {
    case node_kind::none:
        return s << def.term;
    case node_kind::or_node:
    case node_kind::ifelse:
        if(subexpr) {
            auto i = ids.find(&def);
            if(i != ids.end()) {
                worklist.push_back(e);
                if(is_and)
                    s << "~";
                return s << "t" << i->second;
            }
        }
        if(subexpr)
            s << "(";
        s << (is_and ? "and" :
              def.kind == node_kind::or_node ? "or" :
              "ifelse");
        for(eqbool a : def.args) {
            s << " ";
            if(is_and)
                a = ~a;
            dump_helper(s, a, /* subexpr= */ true, ids, worklist);
        }
        if(subexpr)
            s << ")";
        return s;
    }
    unreachable("unknown node kind");
}

std::ostream &eqbool_context::dump(std::ostream &s, eqbool e) const {
    // Collect common subexpressions.
    std::unordered_set<const node_def*> seen;
    std::unordered_map<const node_def*, unsigned> ids;
    std::vector<eqbool> worklist{e};
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        if(n.is_inversion())
            n = ~n;

        const node_def *def = &n.get_def();
        switch(def->kind) {
        case node_kind::none:
            continue;
        case node_kind::or_node:
        case node_kind::ifelse:
            bool inserted = seen.insert(def).second;
            if(!inserted) {
                unsigned &id = ids[def];
                if(!id)
                    id = static_cast<unsigned>(ids.size());
                continue;
            }

            for(eqbool a : def->args)
                worklist.push_back(a);
            continue;
        }
        unreachable("unknown node kind");
    }

    dump_helper(s, e, /* subexpr= */ false, ids, worklist);

    seen.clear();
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        const node_def *def = &n.get_def();
        bool inserted = seen.insert(def).second;
        if(!inserted)
            continue;

        s << "; t" << ids[def] << " = ";
        dump_helper(s, n, /* subexpr= */ false, ids, worklist);
    }

    return s;
}

}  // namesapce eqbool
