/*
Tests the TokEx implementation of RegEx.
Jordan Dehmel, 2024
jdehmel@outlook.com
*/

// #define SAVEFIG

#include "regex.hpp"
#include <cassert>
#include <iostream>

#define D "(0|1|2|3|4|5|6|7|8|9)"
#define W                                                      \
    "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|A|"  \
    "B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z)"

int main()
{
    RegEx basic = compile_regex("a*b+c?d");
    assert(regex_match(basic, "bbd"));

    RegEx d_plus = compile_regex(D "+");
    assert(regex_match(d_plus, "987234"));

    RegEx w_plus = compile_regex(W "+");
    assert(regex_match(w_plus, "foobar"));

    RegEx email =
        compile_regex("(" W "|" D ")+@" W "+\\." W "+");
    assert(regex_match(email, "jdehmel@outlook.com"));

    std::cout << "All tests of RegEx via TokEx passed.\n";

    return 0;
}
