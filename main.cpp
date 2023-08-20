
/*  Testing boolean expressions for equivalence.
    https://github.com/kosarev/eqbool

    Copyright (C) 2023 Ivan Kosarev.
    mail@ivankosarev.com

    Published under the MIT license.
*/

#include <iostream>

#include "eqbool.h"

[[noreturn]] static void fatal(const char *msg) {
    std::cerr << "error: " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

static void process_test_lines() {
    std::string line;
    while(std::getline(std::cin, line))
        std::cout << line << "\n";

    if(!std::cin.eof())
        fatal("cannot read input");
}

int main() {
    process_test_lines();
}
