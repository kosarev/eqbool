
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <iostream>
#include <sstream>
#include <vector>

#include "eqbool.h"

namespace {

using eqbool::eqbool_context;
using eqbool::eqbool;

[[noreturn]] static void fatal(std::string msg) {
    std::cerr << "error: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

class test_context {
private:
    eqbool_context eqbools;
    std::vector<eqbool> nodes;
    unsigned line_no = 0;

    [[noreturn]] void fatal(std::string msg) const {
        ::fatal(std::to_string(line_no) + ": " + msg);
    }

    eqbool get_node(unsigned i) const {
        if(i >= nodes.size())
            fatal("undefined node");
        return nodes[i];
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

        unsigned r = 0;
        if(!(s >> r))
            fatal("result node expected");

        std::vector<eqbool> args;
        unsigned a;
        while(s >> a)
            args.push_back(get_node(a));
        if(!s.eof())
            fatal("unexpected arguments");

        eqbool e;
        if(op == '.') {
            if(args.size() != 0)
                fatal("no arguments expected");
            e = eqbools.get(std::to_string(r).c_str());
        } else if(op == '|') {
            e = eqbools.get_or(args);
        } else if(op == '&') {
            e = eqbools.get_and(args);
        } else if(op == '=') {
            if(args.size() != 2)
                fatal("2 arguments expected");
            e = eqbools.get_eq(args[0], args[1]);
        } else if(op == '?') {
            if(args.size() != 3)
                fatal("3 arguments expected");
            e = eqbools.ifelse(args[0], args[1], args[2]);
        } else if(op == '~') {
            if(args.size() != 1)
                fatal("one argument expected");
            e = ~args[0];
        } else {
            fatal("unknown operator");
        }

        if(assert) {
            if(get_node(r) != e)
                fatal("nodes do not match");
            return;
        }

        size_t expected_r = nodes.size();
        if(r != expected_r)
            fatal("result node " + std::to_string(expected_r) + "expected");
        nodes.push_back(e);
    }

public:
    test_context() {
        nodes.push_back(eqbools.get_false());
        nodes.push_back(eqbools.get_true());
    }

    void process_test_lines() {
        std::string line;
        while(std::getline(std::cin, line)) {
            ++line_no;
            std::cout << std::to_string(line_no) << ": " << line << "\n";
            if(!line.empty())
                process_test_line(line);
        }

        if(!std::cin.eof())
            fatal("cannot read input");
    }
};

}  // anonymous namespace

int main() {
    test_context c;
    c.process_test_lines();
}
