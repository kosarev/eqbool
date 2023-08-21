
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <iostream>
#include <sstream>

#include "eqbool.h"

[[noreturn]] static void fatal(const char *msg) {
    std::cerr << "error: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

static void process_new_term(unsigned term) {
    std::cout << "new term: " << term << "\n";
}

static void process_test_line(const std::string &line) {
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

static void process_test_lines() {
    std::string line;
    while(std::getline(std::cin, line))
        process_test_line(line);

    if(!std::cin.eof())
        fatal("cannot read input");
}

int main() {
    process_test_lines();
}
