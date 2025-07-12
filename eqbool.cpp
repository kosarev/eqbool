
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

void detail::hasher::flatten_or_impl(std::vector<eqbool> &flattened,
                                     args_ref args) {
    for(eqbool a : args) {
        if(!a.is_inversion()) {
            const node_def &def = a.get_def();
            if(def.kind == node_kind::or_node) {
                flatten_or_impl(flattened, def.args);
                continue;
            }
        }

        flattened.push_back(a);
    }
}

void detail::hasher::flatten_or(std::vector<eqbool> &flattened,
                                args_ref args) {
    flatten_or_impl(flattened, args);
    std::sort(flattened.begin(), flattened.end());
}

void detail::hasher::flatten_eq_impl(std::vector<eqbool> &flattened,
                                     args_ref args) {
    for(eqbool a : args) {
        if(!a.is_inversion()) {
            const node_def &def = a.get_def();
            if(def.kind == node_kind::eq) {
                flatten_eq_impl(flattened, def.args);
                continue;
            }
        }

        flattened.push_back(a);
    }
}

void detail::hasher::flatten_eq(std::vector<eqbool> &flattened,
                                args_ref args) {
    flatten_eq_impl(flattened, args);
    std::sort(flattened.begin(), flattened.end());
}

std::size_t detail::hasher::operator () (const node_def &def) const {
    std::size_t h = 0;
    hash(h, def.kind);
    hash(h, def.term);

    if(def.kind == node_kind::eq) {
        std::vector<eqbool> args;
        flatten_eq(args, def.args);
        for(eqbool a : args)
            hash(h, a.entry_code);
    } else if(def.kind == node_kind::ifelse) {
        hash(h, def.args[0].entry_code);
        hash(h, def.args[1].entry_code);
        hash(h, def.args[2].entry_code);
    } else if(def.kind == node_kind::or_node) {
        std::vector<eqbool> args;
        flatten_or(args, def.args);
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
    if(a.kind != b.kind)
        return false;

    if(a.kind == node_kind::term)
        return a.term == b.term;

    if(a.kind == node_kind::ifelse)
        return a.args == b.args;

    if(a.kind == node_kind::eq) {
        std::vector<eqbool> a_args, b_args;
        hasher::flatten_eq(a_args, a.args);
        hasher::flatten_eq(b_args, b.args);
        return a_args == b_args;
    }

    assert(a.kind == node_kind::or_node);
    std::vector<eqbool> a_args, b_args;
    hasher::flatten_or(a_args, a.args);
    hasher::flatten_or(b_args, b.args);
    return a_args == b_args;
}

term_set_base::~term_set_base()
{}

void eqbool::propagate_impl() {
    uintptr_t inv = 0;
    uintptr_t code = entry_code;
    for(;;) {
        inv ^= code;
        code &= detail::entry_code_mask;
        auto &entry = *reinterpret_cast<node_entry*>(code);
        eqbool s = entry.second;
        if(s.entry_code == code)
            break;
        code = s.entry_code;
    }
    entry_code = code | (inv & detail::inversion_flag);
}

void eqbool::reduce() {
    entry_code |= detail::lock_flag;
    entry_code = get_context().reduce({}, *this).entry_code;
    entry_code &= ~detail::lock_flag;
}

eqbool eqbool_context::add_def(node_def def) {
    def.id = defs.size();
    auto r = defs.insert({def, eqbool()});
    auto &i = r.first;
    eqbool &value = i->second;
    bool inserted = r.second;
    if(inserted)
        value = eqbool(*i);
    else
        value.propagate();
    return value;
}

eqbool eqbool_context::get(uintptr_t term) {
    return add_def(node_def(term, *this));
}

eqbool eqbool_context::get_or(args_ref args, bool invert_args) {
    // Order the arguments before simplifications so we never
    // depend on the order they are specified in.
    std::vector<eqbool> sorted_args(args.begin(), args.end());
    for(eqbool &a : sorted_args) {
        check(a);
        a = a ^ invert_args;
    }
    std::sort(sorted_args.begin(), sorted_args.end());

    for(;;) {
        bool repeat = false;
        for(eqbool &a : sorted_args) {
            eqbool s = reduce_impl(sorted_args, a);
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

eqbool eqbool_context::get_value(std::vector<eqbool> &eqs,
                                 eqbool assumed_false) const {
    if(contains(eqs, assumed_false))
        return eqfalse;
    if(contains(eqs, ~assumed_false))
        return eqtrue;
    return {};
}

void eqbool_context::add_eq(std::vector<eqbool> &eqs, eqbool e) {
    if(!contains(eqs, e))
        eqs.push_back(e);
}

eqbool eqbool_context::evaluate(args_ref assumed_falses,
                                const eqbool &excluded,
                                std::vector<eqbool> &eqs) const {
   for(const eqbool &a : assumed_falses) {
        if(&a == &excluded)
            continue;

        if(eqbool v = get_value(eqs, a))
            return v;

        bool inv = a.is_inversion();
        const node_def &def = (a ^ inv).get_def();
        if(def.kind == node_kind::eq) {
            if(eqbool v = get_value(eqs, def.args[0]))
                add_eq(eqs, def.args[1] ^ (inv ^ v.is_false()));
            if(eqbool v = get_value(eqs, def.args[1]))
                add_eq(eqs, def.args[0] ^ (inv ^ v.is_false()));
        } else if(!inv && def.kind == node_kind::or_node) {
            if (eqbool r = evaluate(def.args, excluded, eqs))
                return r;
        }
    }

    return {};
}

eqbool eqbool_context::evaluate(args_ref assumed_falses,
                                const eqbool &excluded,
                                eqbool e, std::vector<eqbool> &eqs) const {
    e.propagate();

    eqs = {e};
    for(;;) {
        std::size_t num_eqs = eqs.size();
        if(eqbool r = evaluate(assumed_falses, excluded, eqs))
            return r;

        if(eqs.size() == num_eqs)
            break;
    }

    if(contains(eqs, eqfalse))
        return eqfalse;
    if(contains(eqs, eqtrue))
        return eqtrue;

    return {};
}

eqbool eqbool_context::evaluate(args_ref assumed_falses,
                                const eqbool &excluded, eqbool e) const {
    std::vector<eqbool> eqs;
    return evaluate(assumed_falses, excluded, e, eqs);
}

bool eqbool_context::contains_all(args_ref p, args_ref q) {
    if(p.size() < q.size())
        return false;

    auto pi = p.begin();
    for(eqbool qa : q) {
        for(;;) {
            if(pi == p.end() || qa < *pi)
                return false;
            if(*pi == qa)
                break;
            ++pi;
        }
    }
    return true;
}

eqbool eqbool_context::reduce_impl(args_ref assumed_falses, eqbool &e) {
    e.propagate();

    if(e.is_const())
        return e;

    const eqbool &excluded = e;
    if(eqbool v = evaluate(assumed_falses, excluded, e))
        return v;

    // TODO: Can we get find all false / true nodes here first rather
    // than to collect them multiple times?
    bool inv = e.is_inversion();
    const node_def &def = (e ^ inv).get_def();
    switch(def.kind) {
    case node_kind::term:
        return e;
    case node_kind::eq:
        if(eqbool v = evaluate(assumed_falses, excluded, def.args[0]))
            return def.args[1] ^ (inv ^ v.is_false());
        if(eqbool v = evaluate(assumed_falses, excluded, def.args[1]))
            return def.args[0] ^ (inv ^ v.is_false());
        return e;
    case node_kind::ifelse: {
        if(eqbool v = evaluate(assumed_falses, excluded, def.args[0]))
            return def.args[v.is_true() ? 1 : 2] ^ inv;
        eqbool iv = evaluate(assumed_falses, excluded, def.args[1]);
        eqbool ev = evaluate(assumed_falses, excluded, def.args[2]);
        if(iv && ev) {
            if(iv == ev)
                return iv ^ inv;
            return  def.args[0] ^ (inv ^ ev.is_true());
        }
        return e;
    }
    case node_kind::or_node:
        eqbool s = eqfalse;
        std::vector<eqbool> eq_args;
        for(const eqbool &a : def.args) {
            std::vector<eqbool> eqs;
            if(eqbool r = evaluate(assumed_falses, excluded, a, eqs)) {
                if(r.is_true())
                    return get(!inv);
                continue;
            }
            if(contains(eq_args, ~a))
                return get(!inv);
            if(!s || contains(eq_args, a))
                continue;
            eq_args.insert(eq_args.end(), eqs.begin(), eqs.end());
            s = s.is_false() ? a : eqbool();
        }
        if(s)
            return s ^ inv;
        // (or (and A...) (and A... B...) C...) => (or (and A...) C...)
        for(const eqbool &a : assumed_falses) {
            if(&a == &excluded)
                continue;
            if(!a.is_inversion())
                continue;
            const node_def &a_def = (~a).get_def();
            if(a_def.kind != node_kind::or_node)
                continue;
            if(contains_all(def.args, a_def.args))
                return get(!inv);
        }
        return e;
    }
    unreachable("unknown node kind");
}

eqbool eqbool_context::reduce(args_ref assumed_falses, eqbool e) {
    for(;;) {
        eqbool r = reduce_impl(assumed_falses, e);
        if(r == e)
            break;
        e = r;
    }
    return e;
}

eqbool eqbool_context::ifelse_impl(eqbool i, eqbool t, eqbool e) {
    check(i);
    check(t);
    check(e);

    i = reduce({}, i);
    t = reduce({~i}, t);
    e = reduce({i}, e);

    if(t == ~e) {
        std::tie(i, t, e) = std::make_tuple(t, i, ~i);
        t = reduce({~i}, t);
        e = reduce({i}, e);
    }

    if(i.is_const())
        return i.is_true() ? t : e;

    if(t.is_const())
        return t.is_false() ? (~i & e) : (i | e);

    if(e.is_const())
        return e.is_false() ? (i & t) : (~i | t);

    if(t == e)
        return t;

    if(t == ~e && t < i)
        std::tie(i, t, e) = std::make_tuple(t, i, ~i);

    if(i.is_inversion())
        std::tie(i, t, e) = std::make_tuple(~i, e, t);

    if(t == ~e) {
        assert(!i.is_inversion());
        bool inv = t.is_inversion();
        node_def def(node_kind::eq, {i, t ^ inv}, *this);
        return add_def(def) ^ inv;
    }

    bool inv = t.is_inversion() && e.is_inversion();
    node_def def(node_kind::ifelse, {i, t ^ inv, e ^ inv}, *this);
    return add_def(def) ^ inv;
}

eqbool eqbool_context::ifelse(eqbool i, eqbool t, eqbool e) {
    eqbool r = ifelse_impl(i, t, e);

    if(r.is_const() && t == ~e)
        store_equiv(i, t ^ r.is_false());

    return r;
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
    e.propagate();

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
        case node_kind::term:
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

void eqbool_context::store_equiv(eqbool a, eqbool b) {
    // Assume that the node created earlier is the simpler one.
    if(a < b)
        std::swap(a, b);

    if(a.is_inversion()) {
        a = ~a;
        b = ~b;
    }

    a.get_entry().second = b;
}

bool eqbool_context::is_equiv(eqbool a, eqbool b) {
    eqbool eq = get_eq(a, b);
    if(eq.is_const())
        return eq.is_true();

    bool equiv = is_unsat(~eq);

    if(equiv)
        store_equiv(a, b);

    return equiv;
}

std::ostream &eqbool_context::print_helper(
        std::ostream &s, eqbool e, bool subexpr,
        const std::unordered_map<const node_def*, unsigned> &ids,
        std::vector<eqbool> &worklist) const {
    if (e.is_const())
        return s << (e.is_false() ? "0" : "1");

    bool is_and = false;
    if(e.is_inversion()) {
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
    case node_kind::term:
        return terms.print(s, def.term);
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
        case node_kind::term:
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
        case node_kind::term:
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
