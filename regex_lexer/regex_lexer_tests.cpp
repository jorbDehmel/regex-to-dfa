/*
Tests the regex-based lexer
*/

#include "../regex_manager.hpp"
#include "regex_lexer.hpp"
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

void print(const RegexLexer<uint64_t, char> &_r)
{
    // Find actually used chars
    std::list<char> used;
    std::cout << "  ";
    for (char c = CHAR_MIN; c < CHAR_MAX; ++c)
    {
        for (uint64_t s = 0; s < _r.n_states; ++s)
        {
            if (_r.get(s, c) != 0)
            {
                used.push_back(c);
                std::cout << (isprint(c) ? c : '?') << ' ';
                break;
            }
        }
    }
    std::cout << '\n';

    // Body
    for (uint64_t s = 0; s < _r.n_states; ++s)
    {
        std::cout << s << "|";
        for (const auto &c : used)
        {
            std::cout << _r.get(s, c) << ' ';
        }
        std::cout << '\n';
    }
}

// Asserts that the given lexer, when reset, yields the expected
// output list when fed its concatenation.
void test_case(RegexManager &_m, const std::string &_pattern,
               const std::vector<std::string> &_exp)
{
    static uint64_t case_num = 0;
    std::cout << "Running test " << ++case_num << " ("
              << _pattern << ")\n";

    std::string inp;
    std::vector<std::string> obs;
    bool did_fail = false;
    for (const auto &substring : _exp)
    {
        inp += substring;
    }

    auto pattern = _m.create_regex(_pattern);

    pattern.graphviz(std::to_string(case_num) + ".dot");
    system(("dot -Tpng " + std::to_string(case_num) +
            ".dot -o " + std::to_string(case_num) + ".png")
               .c_str());

    RegexLexer<uint_fast16_t, char> r(pattern);

    try
    {
        for (const auto &c : (inp + '\0'))
        {
            r.next(c, [&](auto t) { obs.push_back(t.text); });
        }
    }
    catch (...)
    {
        obs.push_back("<<LEX FAILURE>>");
        did_fail = true;
    }

    if (obs.size() != _exp.size())
    {
        did_fail = true;
    }
    else if (!did_fail)
    {
        for (int i = 0; i < obs.size(); ++i)
        {
            if (obs[i] != _exp[i])
            {
                did_fail = true;
                break;
            }
        }
    }

    if (did_fail)
    {
        print(r);
        std::cout << "Input:\n'" << inp << "'\n";
        std::cout << "Expected:\n";
        for (const auto &i : _exp)
        {
            std::cout << "'" << i << "' ";
        }
        std::cout << "\nObserved:\n";
        for (const auto &i : obs)
        {
            std::cout << "'" << i << "' ";
        }
        std::cout << '\n' << std::flush;
        throw std::runtime_error("Failed test case " +
                                 std::to_string(case_num));
    }
}

int main()
{
    RegexManager m;

    // Test pattern 1: |aaaa|
    {
        test_case(m, "aaaa", {"aaaa"});
    }

    // Test pattern 2: |(\w+|4| )|
    {
        const auto p = "(\\w+|4| )";
        test_case(m, p, {"alabama"});
        test_case(m, p, {"al", "4", "bama"});
        test_case(m, p, {"alabama", " ", "football"});
    }

    // Test pattern 3: |(\w+|[]|\d+|\s+)|
    {
        const auto p = "(\\w+|\\d+|=|\\+|-| )";
        test_case(m, p, {"5", "+", "b"});
        test_case(m, p, {"let", " ", "a", "=", "5", "+", "b"});
    }

    std::cout << "All test cases passed!\n";
    return 0;
}
