
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

    [[noreturn]] void fatal(std::string msg) {
        ::fatal(std::to_string(line_no) + ": " + msg);
    }

    void process_test_line(const std::string &line) {
        ++line_no;
        std::istringstream s(line);
        char op;
        if(!(s >> op))
            fatal("operator missed");

        bool assert = false;
        if(op == '!') {
            if(!(s >> op))
                fatal("assertion operator missed");
            assert = true;
        }

        unsigned r = 0;
        eqbool e;
        if(op == '.') {
            if(!(s >> r))
                fatal("term missed");
            e = eqbools.get(std::to_string(r).c_str());
        } else {
            fatal("unknown operator");
        }

        if(assert) {
            if(r >= nodes.size())
                fatal("undefined result node");
            if(nodes[r] != e)
                fatal("nodes do not match");
            return;
        }

        size_t expected_r = nodes.size();
        if(r != expected_r)
            fatal("expected result node " + std::to_string(expected_r));
        nodes.push_back(e);
    }

public:
    test_context() {
        nodes.push_back(eqbools.get_false());
        nodes.push_back(eqbools.get_true());
    }

    void process_test_lines() {
        std::string line;
        while(std::getline(std::cin, line))
            process_test_line(line);

        if(!std::cin.eof())
            fatal("cannot read input");
    }
};

}  // anonymous namespace

int main() {
    test_context c;
    c.process_test_lines();
}
