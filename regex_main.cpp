/*
Tests the TokEx implementation of RegEx.
Jordan Dehmel, 2024
jdehmel@outlook.com
*/

#define SAVEFIGPATH "regex_dots/"
#define SAVEFIG

#include "regex.hpp"
#include <chrono>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
namespace clk = std::chrono;

////////////////////////////////////////////////////////////////
// Regex Macros

#define D "(0|1|2|3|4|5|6|7|8|9)"
#define W                                                      \
    "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|A|"  \
    "B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z)"
#define S "( |\t|\n)"

#define H "(a|b|c|d|e|f|A|B|C|D|E|F|0|1|2|3|4|5|6|7|8|9)"
#define O "(0|1|2|3|4|5|6|7)"

#define HEX_RE "0(x|X)(" H "+')*" H "+"
#define OCTAL_RE "(0|0(" O "+')*" O "+)"
#define BINARY_RE "0(b|B)((0|1)+')*(0|1)+"
#define DECIMAL_RE "-?(1|2|3|4|5|6|7|8|9)(" D "+')*" D "+"
#define INT_RE                                                 \
    "(" HEX_RE "|" DECIMAL_RE "|" OCTAL_RE "|" BINARY_RE ")"

////////////////////////////////////////////////////////////////
// Helper function(s)

/*
Asserts that all test cases pass.
*/
void test_regex(
    const char *const _pattern,
    const std::initializer_list<const char *> &_should_pass,
    const std::initializer_list<const char *> &_should_fail)
{
    // Timing objects
    clk::high_resolution_clock::time_point start, end;
    uint64_t compilation_us, case_sum_us = 0, n = 0;
    std::list<const char *> failures;
    bool r;

    // Compile
    start = clk::high_resolution_clock::now();
    RegEx pattern = compile_regex(_pattern);
    end = clk::high_resolution_clock::now();
    compilation_us =
        clk::duration_cast<clk::microseconds>(end - start)
            .count();

    // Run positive
    for (const auto &item : _should_pass)
    {
        start = clk::high_resolution_clock::now();
        r = regex_match(pattern, item);
        end = clk::high_resolution_clock::now();
        case_sum_us +=
            clk::duration_cast<clk::microseconds>(end - start)
                .count();

        ++n;

        if (!r)
        {
            failures.push_back(item);
        }
    }

    // Run negative
    for (const auto &item : _should_fail)
    {
        start = clk::high_resolution_clock::now();
        r = regex_match(pattern, item);
        end = clk::high_resolution_clock::now();
        case_sum_us +=
            clk::duration_cast<clk::microseconds>(end - start)
                .count();

        ++n;

        if (r)
        {
            failures.push_back(item);
        }
    }

    std::cout << "In RegEx pattern /" << _pattern << "/:\n"
              << "Success rate: "
              << (100.0 * (n - failures.size()) / (double)n)
              << "%\n";

    // If failures happened, output them
    if (!failures.empty())
    {
        std::cout << failures.size() << " errors ocurred!\n";
        for (const auto &f : failures)
        {
            std::cout << "\t" << f << "\n";
        }

        throw std::runtime_error("Not all test cases passed!");
    }

    // Output timing data
    std::cout << "Compilation us: " << compilation_us << "\n"
              << "Average run us: " << case_sum_us / (double)n
              << "\n\n";
}

////////////////////////////////////////////////////////////////
// Main function

int main()
{
    test_regex("a*b+c?d", {"bbd", "aaaabcd"}, {"aaacd", "abc"});

    test_regex(D "+", {"123", "09876"}, {"", "123abc"});

    test_regex(W "+", {"foobar", "BobErt"}, {"greg123"});

    test_regex(W "+" S W "+", {"foo bbbar", "BobErt ROCKS"},
               {"foobar", "foo ", " foo", "greg 123"});

    // Email example
    test_regex("(" W "|" D ")+@" W "+\\." W "+",
               {"jdehmel@outlook.com", "a@b.c"},
               {"jdehmel@foobar@outlook.com", "1@2.c.d",
                "jedehmel@ outlook. com"});

    // Testing basics used for int literals
    test_regex("(0+1)+", {"01001000101001"}, {"0100110011"});
    test_regex("((0|1)+')*", {"11001100'1010'"},
               {"11001100'101''"});
    test_regex("(1+')*0+", {"1'1'11'11'00"}, {"'11'00", "11'"});

    // Parshal int literal testing
    test_regex(BINARY_RE,
               {"0b1111'0000'1111'0000", "0B01011010101",
                "0b101010'1'1"},
               {"b1111'0000", "0v1111'0000", "0b1000'2011"});

    test_regex(OCTAL_RE, {"01'234'567'654", "0", "0'1'2'3"},
               {"012345678", "01234567'"});

    test_regex(DECIMAL_RE,
               {"10", "-123", "516", "-9999", "-19'92"},
               {"0", "-0", "12349A"});

    test_regex(HEX_RE, {"0x12'34'56'67'9A'bC'dd'ee'FF", "0x0"},
               {"0xG", "0x"});

    // Int literal example
    test_regex(INT_RE,
               {"123", "0123", "0x123", "0B1010'1010'1", "100",
                "0x0", "201", "200"},
               {"foo", "0xGorilla", "'0101010'", "0x", "0b", "",
                "char", "0b1010'1002", "0xx0", "0xG", "10.0",
                "100 0"});

    std::cout << "All tests of RegEx via TokEx passed.\n";

    return 0;
}
