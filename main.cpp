
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <malloc.h>

#include "eqbool.h"

namespace {

using eqbool::eqbool_context;
using eqbool::eqbool;

[[noreturn]] static void fatal(std::string msg) {
    std::cerr << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

class test_context {
private:
    eqbool_context eqbools;
    std::unordered_map<std::string, eqbool> nodes;

    std::string filepath;
    unsigned line_no = 0;

    double total_time = 0;

    bool find_mismatches = false;

    [[noreturn]] void fatal(std::string msg) const {
        ::fatal(filepath + ": " + std::to_string(line_no) + ": " + msg);
    }

    [[noreturn]] void fatal(std::ostringstream msg) const {
        fatal(msg.str());
    }

    eqbool get_node(std::string id) const {
        auto it = nodes.find(id);
        if(it == nodes.end())
            fatal("undefined node '" + id + "'");
        return it->second;
    }

    void check_num_args(const std::vector<eqbool> &args, unsigned n) const {
        if(args.size() != n)
            fatal(std::to_string(n) + " arguments expected");
    }

    static bool is_id_char(int c) {
        return ('0' <= c && c <= '9') ||
               ('a' <= c && c <= 'z') ||
               ('A' <= c && c <= 'Z') ||
               c == '_';
    }

    eqbool parse_expr(std::istream &s) {
        s >> std::ws;
        int c = s.peek();
        if(c == '(') {
            s.get();
            std::string op;
            if(!(s >> op))
                fatal("operator expected");

            std::vector<eqbool> args;
            if(op.back() == ')') {
                op.resize(op.size() - 1);
            } else {
                for(;;) {
                    eqbool a = parse_expr(s);
                    if(!a)
                        break;
                    args.push_back(a);
                }
                if(s.get() != ')')
                    fatal("no matching closing parenthesis");
            }

            if(op == "not") {
                check_num_args(args, 1);
                return ~args[0];
            }
            if(op == "and")
                return eqbools.get_and(args);
            if(op == "or")
                return eqbools.get_or(args);
            if(op == "ifelse") {
                check_num_args(args, 3);
                return eqbools.ifelse(args[0], args[1], args[2]);
            }
            if(op == "eq") {
                check_num_args(args, 2);
                return eqbools.get_eq(args[0], args[1]);
            }

            fatal("unknown operator");
        }

        if(is_id_char(c)) {
            std::string id;
            while(is_id_char(c)) {
                id.push_back(static_cast<char>(c));
                s.get();
                c = s.peek();
            }
            return get_node(id);
        }

        if(c == '~') {
            s.get();
            eqbool a = parse_expr(s);
            if(!a)
                fatal("argument expected");
            return ~a;
        }

        return {};
    }

    void process_test_line(const std::string &line) {
        std::istringstream s(line);
        std::string op;
        if(!(s >> op))
            fatal("operator expected");

        if(op == "def") {
            std::string r;
            if(!(s >> r))
                fatal("result node expected");
            eqbool e = parse_expr(s);
            if(!e)
                e = eqbools.get(r.c_str());
            if(s.peek() != std::istream::traits_type::eof())
                fatal("unexpected arguments");
            eqbool &n = nodes[r];
            if(n)
                fatal("result is already defined");
            n = e;
            return;
        }

        if(op == "assert_is" ||
               op == "assert_equiv" || op == "assert_unequiv" ||
               op == "assert_sat_equiv" || op == "assert_sat_unequiv") {
            eqbool a = parse_expr(s);
            eqbool b = parse_expr(s);
            if(!a || !b)
                fatal("arguments expected");
            if(s.peek() != std::istream::traits_type::eof())
                fatal("unexpected arguments");
            if(op == "assert_is") {
                if(a != b) {
                    if(find_mismatches) {
                        std::ostringstream ss;
                        ss << "(" << a << ") vs (" << b << ")";
                        std::cout << line_no << ": " << ss.str().size() <<
                                     " " << ss.str() << "\n";
                    } else {
                        fatal(std::ostringstream() <<
                                  "nodes do not match\n"
                                  "a: " << a << "\n"
                                  "b: " << b);
                    }
                }
            } else {
                bool res = (op == "assert_equiv" || op == "assert_sat_equiv");
                bool sat = (op == "assert_sat_equiv" || op == "assert_sat_unequiv");
                unsigned long count = eqbools.get_stats().num_sat_solutions;
                if(eqbools.is_equiv(a, b) != res)
                    fatal("equivalence check failed");
                if(sat && eqbools.get_stats().num_sat_solutions == count)
                    fatal("equivlance check resolved without using SAT solver");
            }
            return;
        }

        fatal("unknown command");
    }

    template<typename T>
    static std::string format(T n) {
        // Using locale would require RTTI, which we want disabled.
        std::string unit = std::to_string(n % 1000);
        if(n < 1000)
            return unit;
        unit.insert(unit.begin(), 3 - unit.size(), '0');
        return format(n / 1000) + " " + unit;
    }

    void print_stats(std::ostream &s) const {
        const ::eqbool::eqbool_stats &stats = eqbools.get_stats();
        double other_time = total_time - (stats.sat_time + stats.clauses_time);
        s <<
             line_no << ": " <<
             format(static_cast<long>(total_time * 1000)) << " ms, " <<
             format(stats.num_sat_solutions) << " solutions " <<
             format(static_cast<long>(stats.sat_time * 1000)) << " ms, " <<
             format(stats.num_clauses) << " clauses " <<
             format(static_cast<long>(stats.clauses_time * 1000)) << " ms, " <<
             "other " << format(static_cast<long>(other_time * 1000)) << " ms, " <<
             format(::mallinfo2().uordblks / 1024) << "K allocated\n";
    }

    void print_stats() {
        print_stats(find_mismatches ? std::cerr : std::cout);
        std::cout.flush();

        std::ostringstream s;
        print_stats(s);
        total_times[line_no].push_back({total_time, s.str()});
    }

public:
    using time_and_stats_type = std::pair<double, std::string>;
    using total_times_type = std::map<unsigned, std::vector<time_and_stats_type>>;
    total_times_type &total_times;

    test_context(std::string filepath, total_times_type &total_times,
                 bool find_mismatches)
            : filepath(filepath), find_mismatches(find_mismatches),
              total_times(total_times) {
        nodes["0"] = eqbools.get_false();
        nodes["1"] = eqbools.get_true();
    }

    void process_test_lines(std::istream &f) {
        ::eqbool::timer t(total_time);

        std::string line;
        unsigned last_reported_line_no = 0;
        while(std::getline(f, line)) {
            ++line_no;
            // std::cout << std::to_string(line_no) << ": " << line << "\n";
            if(!line.empty() && line[0] != '#')
                process_test_line(line);
            if(line_no % 100000 == 0) {
                t.update();
                print_stats();
                last_reported_line_no = line_no;
            }
        }

        if(line_no != last_reported_line_no) {
            t.update();
            print_stats();
        }

        if(!f.eof())
            fatal("cannot read input");
    }
};

}  // anonymous namespace

int main(int argc, const char **argv) {
    (void) argc;  // Unused.

    bool find_mismatches = false;
    bool test_performance = false;
    int i = 1;
    for(; argv[i]; ++i) {
        std::string arg = argv[i];
        if(arg == "--find-mismatches") {
            find_mismatches = true;
            continue;
        }
        if(arg == "--test-performance") {
            test_performance = true;
            continue;
        }
        break;
    }

    int num_runs = test_performance ? 5 : 1;

    test_context::total_times_type total_times;

    for(; argv[i]; ++i) {
        std::string path = argv[i];
        std::ifstream f(path);
        if(!f)
            fatal("cannot open " + path);
        std::ostringstream input;
        if(!(input << f.rdbuf()))
            fatal("cannot read " + path);

        for(int n = 0; n != num_runs; ++n) {
            if(test_performance) {
                if(n != 0)
                    std::cout << "\n";
                std::cout << "run #" << n + 1 << "\n";
            }

            test_context c(path, total_times, find_mismatches);
            std::istringstream is(input.str());
            c.process_test_lines(is);
        }
    }

    if(test_performance) {
        std::cout << "\nmedian times:\n";
        for(auto &t : total_times) {
            std::vector<test_context::time_and_stats_type> &v = t.second;
            std::sort(v.begin(), v.end());
            std::cout << "median: " << v[v.size() / 2].second;
        }
    }
}
