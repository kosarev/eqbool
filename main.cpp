
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

    void process_test_line(const std::string &line) {
        std::istringstream s(line);
        std::string op;
        if(!(s >> op))
            fatal("operator expected");

        if(op == "def") {
            std::string r;
            if(!(s >> r))
                fatal("result node expected");
            if(!s.eof())
                fatal("unexpected arguments");
            eqbool &n = nodes[r];
            if(n)
                fatal("result is already defined");
            n = eqbools.get(r.c_str());
            return;
        }

        bool assert = false;
        if(op == "!") {
            if(!(s >> op))
                fatal("assertion operator expected");
            assert = true;
        }

        std::string r;
        if(!(s >> r))
            fatal("result node expected");

        std::vector<eqbool> args;
        for(;;) {
            s >> std::ws;
            if(s.peek() == '~') {
                s.get();
                std::string a;
                if(!(s >> a))
                    fatal("argument expected after '~'");
                args.push_back(~get_node(a));
                continue;
            }

            std::string a;
            if(!(s >> a))
                break;

            args.push_back(get_node(a));
        }

        if(!s.eof())
            fatal("unexpected arguments");

        ::eqbool::timer t(total_time);

        eqbool e;
        if(assert && op == ".") {
            check_num_args(args, 0);
            e = eqbools.get(r.c_str());
        } else if(op == "|") {
            e = eqbools.get_or(args);
        } else if(op == "&") {
            e = eqbools.get_and(args);
        } else if(op == "=") {
            check_num_args(args, 2);
            e = eqbools.get_eq(args[0], args[1]);
        } else if(op == "?") {
            check_num_args(args, 3);
            e = eqbools.ifelse(args[0], args[1], args[2]);
        } else if(op == "~") {
            check_num_args(args, 1);
            e = ~args[0];
        } else if(op == "q") {
            check_num_args(args, 2);
            if(r != "0" && r != "1")
                fatal("constant result expected");
            unsigned long count = eqbools.get_stats().num_sat_solutions;
            if(eqbools.is_equiv(args[0], args[1]) != (r == "1"))
                fatal("equivalence check failed");
            if(assert && eqbools.get_stats().num_sat_solutions == count)
                fatal("equivlance check resolved without using SAT solver");
            return;
        } else {
            fatal("unknown operator");
        }

        if(assert) {
            eqbool expected = get_node(r);
            if(e != expected) {
                fatal(std::ostringstream() <<
                          "nodes do not match\n"
                          "actual: " << e << "\n"
                          "expected: " << expected);
            }
            return;
        }

        if(nodes.find(r) != nodes.end())
            fatal("result is already defined");
        nodes[r] = e;
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
        print_stats(std::cout);

        std::ostringstream s;
        print_stats(s);
        total_times[line_no].push_back({total_time, s.str()});
    }

public:
    using time_and_stats_type = std::pair<double, std::string>;
    using total_times_type = std::map<unsigned, std::vector<time_and_stats_type>>;
    total_times_type &total_times;

    test_context(std::string filepath, total_times_type &total_times)
            : filepath(filepath), total_times(total_times) {
        nodes["0"] = eqbools.get_false();
        nodes["1"] = eqbools.get_true();
    }

    void process_test_lines(std::istream &f) {
        std::string line;
        unsigned last_reported_line_no = 0;
        while(std::getline(f, line)) {
            ++line_no;
            // std::cout << std::to_string(line_no) << ": " << line << "\n";
            if(!line.empty() && line[0] != '#')
                process_test_line(line);
            if(line_no % 50000 == 0) {
                print_stats();
                last_reported_line_no = line_no;
            }
        }

        if(line_no != last_reported_line_no)
            print_stats();

        if(!f.eof())
            fatal("cannot read input");
    }
};

}  // anonymous namespace

int main(int argc, const char **argv) {
    (void) argc;  // Unused.

    bool test_performance = false;
    int i = 1;
    for(; argv[i]; ++i) {
        std::string arg = argv[i];
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
        for(int n = 0; n != num_runs; ++n) {
            if(test_performance) {
                if(n != 0)
                    std::cout << "\n";
                std::cout << "run #" << n + 1 << "\n";
            }

            std::ifstream f(path);
            if(!f)
                fatal("cannot open " + path);
            test_context c(path, total_times);
            c.process_test_lines(f);
        }
    }

    if(test_performance) {
        std::cout << "\nmeadian times:\n";
        for(auto &t : total_times) {
            std::vector<test_context::time_and_stats_type> &v = t.second;
            std::sort(v.begin(), v.end());
            std::cout << v[v.size() / 2].second;
        }
    }
}
