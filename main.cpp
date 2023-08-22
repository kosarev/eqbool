
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
    eqbool_context context;
    std::vector<eqbool> nodes;

    void add_term(unsigned term) {
        size_t expected_term = nodes.size();
        if(term != expected_term)
            fatal("expected term " + std::to_string(expected_term));
        nodes.push_back(context.get(std::to_string(term).c_str()));
    }

    void process_test_line(const std::string &line) {
        std::istringstream s(line);
        char op;
        if(!(s >> op))
            fatal("operator missed");
        if(op == '.') {
            unsigned term;
            if(!(s >> term))
                fatal("term missed");
            add_term(term);
            return;
        }

        fatal("unknown operator");
    }

public:
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
