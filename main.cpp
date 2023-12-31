
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <ctime>
#include <fstream>
#include <iostream>
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
        char op;
        if(!(s >> op))
            fatal("operator expected");

        bool assert = false;
        if(op == '!') {
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

        eqbool e;
        if(op == '.') {
            check_num_args(args, 0);
            e = eqbools.get(r.c_str());
        } else if(op == '|') {
            e = eqbools.get_or(args);
        } else if(op == '&') {
            e = eqbools.get_and(args);
        } else if(op == '=') {
            check_num_args(args, 2);
            e = eqbools.get_eq(args[0], args[1]);
        } else if(op == '?') {
            check_num_args(args, 3);
            e = eqbools.ifelse(args[0], args[1], args[2]);
        } else if(op == '~') {
            check_num_args(args, 1);
            e = ~args[0];
        } else if(op == 'q') {
            check_num_args(args, 2);
            if(r != "0" && r != "1")
                fatal("constant result expected");
            unsigned long count = eqbools.get_stats().sat_solution_count;
            if(eqbools.is_equiv(args[0], args[1]) != (r == "1"))
                fatal("equivalence check failed");
            if(assert && eqbools.get_stats().sat_solution_count == count)
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

    void print_stats() const {
        long cpu_time = 1000 * std::clock() / CLOCKS_PER_SEC;
        const ::eqbool::eqbool_stats &stats = eqbools.get_stats();
        std::cout <<
            "line " << line_no << ": " <<
            format(cpu_time) << " CPU ms, " <<
            format(stats.sat_time) << " SAT ms, " <<
            format(stats.sat_solution_count) << " solutions, " <<
            format(stats.num_clauses) << " clauses, " <<
            format(::mallinfo2().uordblks / 1024) << "K allocated\n";
    }

public:
    test_context(std::string filepath) : filepath(filepath) {
        nodes["0"] = eqbools.get_false();
        nodes["1"] = eqbools.get_true();
    }

    void process_test_lines(std::istream &f) {
        std::string line;
        while(std::getline(f, line)) {
            ++line_no;
            // std::cout << std::to_string(line_no) << ": " << line << "\n";
            if(!line.empty() && line[0] != '#')
                process_test_line(line);
            if(line_no % 50000 == 0)
                print_stats();
        }

        if(!f.eof())
            fatal("cannot read input");
    }
};

}  // anonymous namespace

int main(int argc, const char **argv) {
    for(int i = 1; i != argc; ++i) {
        std::string path = argv[i];
        std::ifstream f(path);
        if(!f)
            fatal("cannot open " + path);
        test_context c(path);
        c.process_test_lines(f);
    }
}
