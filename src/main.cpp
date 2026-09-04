#include <iostream>

#include <stepseq/pattern.hpp>
#include <stepseq/repl.hpp>

int main() {
    stepseq::Pattern pattern = stepseq::makeDefaultPattern();
    stepseq::runRepl(std::cin, std::cout, pattern);

    return 0;
}
