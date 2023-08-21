
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <iostream>
#include <sstream>

#include "eqbool.h"

namespace {

[[noreturn]] static void fatal(const char *msg) {
    std::cerr << "error: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

class test_context {
private:
    void process_new_term(unsigned term) {
        std::cout << "new term: " << term << "\n";
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
            process_new_term(term);
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
