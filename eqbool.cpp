
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023-2025 Ivan Kosarev.
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

}

void detail::hasher::flatten_args(std::vector<eqbool> &flattened, args_ref args) {
    for(eqbool a : args) {
        if(!a.is_inversion()) {
            const node_def &def = a.get_def();
            if(def.kind == node_kind::or_node) {
                flatten_args(flattened, def.args);
                continue;
            }
        }

        flattened.push_back(a);
    }
}

std::size_t detail::hasher::operator () (const node_def &def) const {
    std::size_t h = 0;
    hash(h, def.kind);
    hash(h, def.term);

    if(def.kind == node_kind::eq) {
        hash(h, def.args[0].entry_code);
        hash(h, def.args[1].entry_code);
    } else if(def.kind == node_kind::ifelse) {
        hash(h, def.args[0].entry_code);
        hash(h, def.args[1].entry_code);
        hash(h, def.args[2].entry_code);
    } else if(def.kind == node_kind::or_node) {
        std::vector<eqbool> args;
        flatten_args(args, def.args);
        std::sort(args.begin(), args.end());
        for(eqbool a : args)
            hash(h, a.entry_code);
    } else {
        assert(def.args.size() == 0);
    }

    return h;
}

inline bool detail::matcher::operator () (const node_def &a,
                                          const node_def &b) const {
    assert(&a.get_context() == &b.get_context());
    if(a.kind != b.kind || a.term != b.term)
        return false;

    if(a.kind == node_kind::none)
        return true;

    if(a.kind == node_kind::ifelse || a.kind == node_kind::eq)
        return a.args == b.args;

    assert(a.kind == node_kind::or_node);
    std::vector<eqbool> a_args, b_args;
    hasher::flatten_args(a_args, a.args);
    hasher::flatten_args(b_args, b.args);
    std::sort(a_args.begin(), a_args.end());
    std::sort(b_args.begin(), b_args.end());
    return a_args == b_args;
}

void eqbool::propagate_impl() const {
    uintptr_t inv = 0;
    uintptr_t code = entry_code;
    for(;;) {
        inv ^= code;
        code &= ~detail::inversion_flag;
        auto &entry = *reinterpret_cast<node_entry*>(code);
        eqbool s = entry.second;
        if(s.entry_code == code)
            break;
        code = s.entry_code;
    }
    entry_code = code | (inv & detail::inversion_flag);
}

eqbool eqbool_context::add_def(node_def def) {
    def.id = defs.size();
    auto r = defs.insert({def, eqbool()});
    auto &i = r.first;
    eqbool &value = i->second;
    bool inserted = r.second;
    if(inserted)
        value = eqbool(*i);
    return value;
}

eqbool eqbool_context::get(const char *term) {
    return add_def(node_def(term, *this));
}

eqbool eqbool_context::get_or(args_ref args, bool invert_args) {
    // Order the arguments before simplifications so we never
    // depend on the order they are specified in.
    std::vector<eqbool> sorted_args(args.begin(), args.end());
    for(eqbool &a : sorted_args) {
        check(a);
        a = invert_args ? ~a : a;
    }
    std::sort(sorted_args.begin(), sorted_args.end());

    for(;;) {
        bool repeat = false;
        for(eqbool &a : sorted_args) {
            eqbool s = simplify(sorted_args, a);
            if(s != a) {
                a = s;
                if(!a.is_const())
                    repeat = true;
            }
        }

        if(!repeat)
            break;
    }

    std::size_t num_args = 0;
    for(eqbool a : sorted_args) {
        if(a.is_true())
            return eqtrue;
        if(!a.is_false())
            sorted_args[num_args++] = a;
    }

    sorted_args.resize(num_args);

    if(num_args == 0)
        return eqfalse;

    if(num_args == 1)
        return sorted_args[0];

    // (or (and A B) (and ~A C))  =>  (ifelse A B C)
    // (or ~(or ~A ~B) ~(or A ~C))  =>  (ifelse A B C)
    if(num_args == 2 &&
           sorted_args[0].is_inversion() &&
           sorted_args[1].is_inversion()) {
        const node_def &def0 = (~sorted_args[0]).get_def();
        const node_def &def1 = (~sorted_args[1]).get_def();
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

    node_def def(node_kind::or_node, sorted_args, *this);
    return add_def(def);
}

void eqbool_context::add_eq(std::vector<eqbool> &eqs, eqbool e) {
    if(!contains(eqs, e))
        eqs.push_back(e);
}

// TODO: Rename to get_false_nodes?
// TODO: Generalise to find both false and true nodes?
eqbool eqbool_context::get_eqs(args_ref args, const eqbool &excluded,
                               std::vector<eqbool> &eqs) const {
   for(const eqbool &a : args) {
        if(&a == &excluded)
            continue;

        if(contains(eqs, a))
            return eqfalse;
        if(contains(eqs, ~a))
            return eqtrue;

        bool inv = a.is_inversion();
        const node_def &def = (inv ? ~a : a).get_def();
        if(def.kind == node_kind::eq) {
            if(contains(eqs, def.args[0]))
                add_eq(eqs, inv ? def.args[1] : ~def.args[1]);
            if(contains(eqs, ~def.args[0]))
                add_eq(eqs, inv ? ~def.args[1] : def.args[1]);
            if(contains(eqs, def.args[1]))
                add_eq(eqs, inv ? def.args[0] : ~def.args[0]);
            if(contains(eqs, ~def.args[1]))
                add_eq(eqs, inv ? ~def.args[0] : def.args[0]);
        } else if(!inv && def.kind == node_kind::or_node) {
            if (eqbool r = get_eqs(def.args, excluded, eqs))
                return r;
        }

        // TODO: Consider IFELSE nodes as well.
    }

    return {};
}

eqbool eqbool_context::get_eqs(args_ref args, const eqbool &excluded,
                               eqbool e, std::vector<eqbool> &eqs) const {
    eqs = {e};
    for(;;) {
        std::size_t num_eqs = eqs.size();
        if(eqbool r = get_eqs(args, excluded, eqs))
            return r;

        if(eqs.size() == num_eqs)
            break;
    }

    return {};
}

bool eqbool_context::is_known_false(args_ref args, const eqbool &excluded,
                                    eqbool e) const {
    std::vector<eqbool> eqs;
    if(eqbool r = get_eqs(args, excluded, e, eqs))
        return r.is_false();

    return false;
}

bool eqbool_context::contains_all(args_ref p, args_ref q) {
    if(p.size() < q.size())
        return false;

    auto pi = p.begin();
    for(eqbool qa : q) {
        for(;;) {
            if(pi == p.end() || *pi > qa)
                return false;
            if(*pi == qa)
                break;
            ++pi;
        }
    }
    return true;
}

eqbool eqbool_context::simplify(args_ref args, const eqbool &e) const {
    if(e.is_const())
        return e;

    const eqbool &excluded = e;
    if(is_known_false(args, excluded, e))
        return eqfalse;
    if(is_known_false(args, excluded, ~e))
        return eqtrue;

    // TODO: Can we get find all false / true nodes here first rather
    // than to collect them multiple times?
    bool inv = e.is_inversion();
    const node_def &def = (inv ? ~e : e).get_def();
    if(def.kind == node_kind::eq) {
        if(is_known_false(args, excluded, def.args[0]))
            return inv ? def.args[1] : ~def.args[1];
        if(is_known_false(args, excluded, ~def.args[0]))
            return inv ? ~def.args[1] : def.args[1];
        if(is_known_false(args, excluded, def.args[1]))
            return inv ? def.args[0] : ~def.args[0];
        if(is_known_false(args, excluded, ~def.args[1]))
            return inv ? ~def.args[0] : def.args[0];
    } else if(def.kind == node_kind::ifelse) {
        if(is_known_false(args, excluded, ~def.args[0]))
            return inv ? ~def.args[1] : def.args[1];
        if(is_known_false(args, excluded, def.args[0]))
            return inv ? ~def.args[2] : def.args[2];
    } else if(def.kind == node_kind::or_node) {
        eqbool s = eqfalse;
        std::vector<eqbool> eq_args;
        for(const eqbool &a : def.args) {
            std::vector<eqbool> eqs;
            if(eqbool r = get_eqs(args, excluded, a, eqs)) {
                if(r.is_true())
                    return inv ? eqfalse : eqtrue;
                continue;
            }
            if(contains(eq_args, ~a))
                return inv ? eqfalse : eqtrue;
            if(!s || contains(eq_args, a))
                continue;
            eq_args.insert(eq_args.end(), eqs.begin(), eqs.end());
            s = s.is_false() ? a : eqbool();
        }
        if(s)
            return inv ? ~s : s;
        // (or (and A...) (and A... B...) C...) => (or (and A...) C...)
        for(const eqbool &a : args) {
            if(&a == &excluded)
                continue;
            if(!a.is_inversion())
                continue;
            const node_def &a_def = (~a).get_def();
            if(a_def.kind != node_kind::or_node)
                continue;
            if(contains_all(def.args, a_def.args))
                return inv ? eqfalse : eqtrue;
        }
    }
    return e;
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    for(;;) {
        for(;;) {
            eqbool s = simplify({~i}, t);
            if(s == t)
                break;
            t = s;
        }

        for(;;) {
            eqbool s = simplify({i}, e);
            if(s == e)
                break;
            e = s;
        }

        if(i.is_const())
            return i.is_true() ? t : e;

        if(t.is_const())
            return t.is_false() ? (~i & e) : (i | e);

        if(e.is_const())
            return e.is_false() ? (i & t) : (~i | t);

        if(t == e)
            return t;

        if(t == ~e) {
            if(t < i) {
                std::tie(i, t, e) = std::make_tuple(t, i, ~i);
                continue;
            }

            bool inv = false;
            if(i.is_inversion()) {
                i = ~i;
                inv = !inv;
            }
            if(t.is_inversion()) {
                t = ~t;
                inv = !inv;
            }

            // We only consider the case when t contains i, because we
            // know i was created before t (i < t).
            const node_def &t_def = t.get_def();
            if(t_def.kind == node_kind::eq) {
                if(t_def.args[0] == i)
                    return inv ? ~t_def.args[1] : t_def.args[1];
                if(t_def.args[1] == i)
                    return inv ? ~t_def.args[0] : t_def.args[0];
            }

            node_def def(node_kind::eq, {i, t}, *this);
            eqbool r = add_def(def);
            return inv ? ~r : r;
        }

        break;
    }

    if(i.is_inversion())
        std::tie(i, t, e) = std::make_tuple(~i, e, t);

    bool inv = false;
    if(t.is_inversion() && e.is_inversion()) {
        std::tie(t, e) = std::make_tuple(~t, ~e);
        inv = !inv;
    }

    node_def def(node_kind::ifelse, {i, t, e}, *this);
    eqbool r = add_def(def);
    return inv ? ~r : r;
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
        case node_kind::ifelse:
        case node_kind::eq: {
            eqbool i_arg = def.args[0];
            eqbool t_arg = def.args[1];
            eqbool e_arg = def.kind == node_kind::ifelse ? def.args[2] : ~def.args[1];
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
    eqbool eq = get_eq(a, b);
    if(eq.is_const())
        return eq.is_true();

    bool equiv = is_unsat(~eq);

    if(equiv) {
        if(a < b)
            std::swap(a, b);

        if(a.is_inversion()) {
            a = ~a;
            b = ~b;
        }

        defs[a.get_def()] = b;
    }

    return equiv;
}

std::ostream &eqbool_context::print_helper(
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
            print_helper(s, ~e, /* subexpr= */ true, ids, worklist);
            return s;
        }
    }

    const node_def &def = e.get_def();
    switch(def.kind) {
    case node_kind::none:
        return s << def.term;
    case node_kind::or_node:
    case node_kind::ifelse:
    case node_kind::eq:
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
              def.kind == node_kind::ifelse ? "ifelse" :
              "eq");
        for(eqbool a : def.args) {
            s << " ";
            if(is_and)
                a = ~a;
            print_helper(s, a, /* subexpr= */ true, ids, worklist);
        }
        if(subexpr)
            s << ")";
        return s;
    }
    unreachable("unknown node kind");
}

std::ostream &eqbool_context::print(std::ostream &s, eqbool e) const {
    check(e);

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
        case node_kind::eq:
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

    print_helper(s, e, /* subexpr= */ false, ids, worklist);

    seen.clear();
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        const node_def *def = &n.get_def();
        bool inserted = seen.insert(def).second;
        if(!inserted)
            continue;

        s << "; t" << ids[def] << " = ";
        print_helper(s, n, /* subexpr= */ false, ids, worklist);
    }

    return s;
}

std::ostream &eqbool_context::dump(std::ostream &s, args_ref nodes) const {
    std::vector<eqbool> temps;
    std::vector<eqbool> worklist(nodes.begin(), nodes.end());
    while(!worklist.empty()) {
        eqbool n = worklist.back();
        worklist.pop_back();

        if(contains(temps, n))
            continue;

        temps.push_back(n);

        if(n.is_inversion()) {
            worklist.push_back(~n);
            continue;
        }

        for(eqbool a : n.get_def().args)
            worklist.push_back(a);
    }

    std::sort(temps.begin(), temps.end(),
              [](eqbool a, eqbool b) { return a.get_id() < b.get_id(); });

    for(eqbool n : temps) {
        s << "def t" << n.get_id();
        if(n.is_inversion()) {
            s << " ~t" << (~n).get_id() << "\n";
            continue;
        }
        const node_def &def = n.get_def();
        switch(def.kind) {
        case node_kind::none:
            s << "\n";
            continue;
        case node_kind::or_node:
        case node_kind::ifelse:
        case node_kind::eq:
            s << " (";
            s << (def.kind == node_kind::or_node ? "or" :
                  def.kind == node_kind::ifelse ? "ifelse" :
                  "eq");
            for(eqbool a : def.args)
                s << " t" << a.get_id();
            s << ")\n";
            continue;
        }
        unreachable("unknown node kind");
    }

    return s;
}

}  // namesapce eqbool
