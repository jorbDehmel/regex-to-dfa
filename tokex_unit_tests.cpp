/*
Unit testing for tokex engine. Each test function should contain
one pattern and at least two test strings, at least one of which
should match and at least one of which should fail. If this file
does not execute without error, the current TokEx build should
be considered invalid.

Jordan Dehmel, 2024
jdehmel@outlook.com
*/

#define SAVEFIG
#define SAVEFIGPATH "./test_dots/"

#include "lexer.hpp"
#include "tokex.hpp"
#include <cassert>
#include <iostream>
#include <string>

static Lexer l;

#define assert_match(_pattern, _what)                          \
    {                                                          \
        assert(_pattern.match(l.lex_l(_what)));                \
        std::cout << "Success!\n";                             \
    }
#define assert_not_match(_pattern, _what)                      \
    {                                                          \
        assert(!_pattern.match(l.lex_l(_what)));               \
        std::cout << "Success!\n";                             \
    }

// Test a simple sequential pattern in TokEx.
void test_sequential()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a b c"));

    // Match case
    assert_match(pattern, "a b c");

    // Failure case
    assert_not_match(pattern, "a c c");
}

// Tests a simple sequential pattern with a star glob in TokEx
void test_star_glob()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern = l.lex_v("a b $* c");

    // Match cases
    assert_match(pattern, "a c");
    assert_match(pattern, "a b c");
    assert_match(pattern, "a b b b b b c");

    // Failure cases
    assert_not_match(pattern, "a b b b");
    assert_not_match(pattern, "a b b b d");
}

// Tests a simple sequential pattern with a plus glob in TokEx
void test_plus_glob()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a b $+ c"));

    // Match cases
    assert_match(pattern, "a b c");
    assert_match(pattern, "a b b b b b c");

    // Failure cases
    assert_not_match(pattern, "a c");
    assert_not_match(pattern, "a b b b");
    assert_not_match(pattern, "a b b b d");
}

// Tests a simple sequential pattern with a optional glob in
// TokEx
void test_optional()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a b $? c"));

    // Match cases
    assert_match(pattern, "a b c");
    assert_match(pattern, "a c");

    // Failure case
    assert_not_match(pattern, "a b b c");
}

// Tests a basic sequential wildcard for TokEx
void test_wildcard()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a $. b"));

    // Match cases
    assert_match(pattern, "a a b");
    assert_match(pattern, "a b b");

    // Failure case
    assert_not_match(pattern, "a b");
}

// Tests globs w/ wildcards in TokEx
void test_wildcard_globs()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern_1;
    Tokex pattern_2;
    Tokex pattern_3;

    pattern_1.compile(l.lex_v("a $. $* b"));
    pattern_2.compile(l.lex_v("a $. $+ b"));
    pattern_3.compile(l.lex_v("a $. $? b"));

    // Test pattern 1
    {
        // Match cases
        assert_match(pattern_1, "a c d e f g b");
        assert_match(pattern_1, "a b");

        // Failure cases
        assert_not_match(pattern_1, "a c d e f g");
    }

    // Test pattern 2
    {
        // Match cases
        assert_match(pattern_2, "a c d e f g b");

        // Failure cases
        assert_not_match(pattern_2, "a b");
        assert_not_match(pattern_2, "a c d e f g");
    }

    // Test pattern 3
    {
        // Match cases
        assert_match(pattern_3, "a c b");
        assert_match(pattern_3, "a b");

        // Failure cases
        assert_not_match(pattern_3, "a c");
    }
}

// Tests parenthesized subexpressions in TokEx
void test_subexpression()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a $( b c d $) z"));

    // Match cases
    assert_match(pattern, "a b c d z");

    // Failure cases
    assert_not_match(pattern, "a b z");
}

// Tests parenthesized branch subexpressions in TokEx
void test_branch_subexpression_1()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(l.lex_v("a $( b c $| d e $) z"));

    // Match cases
    assert_match(pattern, "a b c z");
    assert_match(pattern, "a d e z");

    // Failure cases
    assert_not_match(pattern, "a b c d e z");
}

// Tests parenthesized branch subexpressions in TokEx
void test_branch_subexpression_2()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern(
        l.lex_v("a $( b c $| d e $| f g $| f h i j $)"));

    // Match cases
    assert_match(pattern, "a b c");
    assert_match(pattern, "a d e");
    assert_match(pattern, "a f g");
    assert_match(pattern, "a f h i j");

    // Failure cases
    assert_not_match(pattern, "a b c d e z");
}

// Tests parenthesized subexpressions with globs in TokEx
void test_subexpression_glob()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern_1;
    Tokex pattern_2;
    Tokex pattern_3;

    pattern_1.compile(l.lex_v("a $( b c d $) $+ z"));
    pattern_2.compile(l.lex_v("a $( b c d $) $* z"));
    pattern_3.compile(l.lex_v("a $( b c d $) $? z"));

    // Test pattern 1
    {
        // Match cases
        assert_match(pattern_1, "a b c d b c d b c d z");

        // Failure cases
        assert_not_match(pattern_1, "a z");
    }

    // Test pattern 2
    {
        // Match cases
        assert_match(pattern_2, "a b c d b c d b c d z");
        assert_match(pattern_2, "a z");

        // Failure cases
        assert_not_match(pattern_2, "a b c d b z");
    }

    // Test pattern 3
    {
        // Match cases
        assert_match(pattern_3, "a z");
        assert_match(pattern_3, "a b c d z");

        // Failure cases
        assert_not_match(pattern_3, "a b c d b c d z");
    }
}

// Tests parenthesized branch subexpressions with globs in TokEx
void test_branch_subexpression_glob_1()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';
    Tokex pattern_1;
    Tokex pattern_2;
    Tokex pattern_3;

    pattern_1.compile(l.lex_v("a $( b c $| d e $) $* z"));
    pattern_2.compile(l.lex_v("a $( b c $| d e $) $+ z"));
    pattern_3.compile(l.lex_v("a $( b c $| d e $) $? z"));

    // Test pattern 1
    {
        // Match cases
        assert_match(pattern_1, "a z");
        assert_match(pattern_1, "a b c z");
        assert_match(pattern_1, "a b c d e b c z");
        assert_match(pattern_1, "a d e z");

        // Failure cases
        assert_not_match(pattern_1, "a b e z");
        assert_not_match(pattern_1, "a b c d e d e d c z");
    }

    // Test pattern 2
    {
        // Match cases
        assert_match(pattern_2, "a b c z");
        assert_match(pattern_2, "a b c d e b c z");
        assert_match(pattern_2, "a d e z");

        // Failure cases
        assert_not_match(pattern_2, "a z");
        assert_not_match(pattern_2, "a b e z");
        assert_not_match(pattern_2, "a b c d e d e d c z");
    }

    // Test pattern 3
    {
        // Match cases
        assert_match(pattern_3, "a b c z");
        assert_match(pattern_3, "a d e z");
        assert_match(pattern_3, "a z");

        // Failure cases
        assert_not_match(pattern_3, "a b c d e b c z");
        assert_not_match(pattern_3, "a b e z");
        assert_not_match(pattern_3, "a b c d e d e d c z");
    }
}

void test_branch_subexpression_glob_2()
{
    std::cout << "\n"
              << __PRETTY_FUNCTION__ << ":" << __LINE__ << '\n';

    Tokex pattern_0, pattern_1, pattern_2, pattern_3;

    pattern_0.compile(l.lex_v("$( a $| b $| c $)"));
    pattern_1.compile(l.lex_v("$( a $| b $| c $) $+"));
    pattern_2.compile(l.lex_v("$( a $| b $| c $) $*"));
    pattern_3.compile(l.lex_v("$( a $| b $| c $) $?"));

    assert_match(pattern_0, "a");
    assert_match(pattern_0, "b");
    assert_match(pattern_0, "c");

    assert_match(pattern_1, "a b a c b a a c");
    assert_match(pattern_2, "a b a c b a a c");
    assert_match(pattern_3, "a");
    assert_match(pattern_3, "");
}

// Tests variable control symbols in TokEx
void test_variables()
{
    assert(false);
}

// Run all unit tests sequentially, return 0 if and only if all
// tests have passed.
int main()
{
    // Basic tests
    test_sequential();
    test_wildcard();

    // Basic glob tests
    test_optional();
    test_star_glob();
    test_plus_glob();

    // Test wildcard globs
    test_wildcard_globs();

    // Test subexpressions
    test_subexpression();
    test_branch_subexpression_1();
    test_branch_subexpression_2();

    // Test subexpressions with globs
    test_subexpression_glob();
    test_branch_subexpression_glob_1();
    test_branch_subexpression_glob_2();

    // Test variable control symbols
    // test_variables();

    std::cout << "All unit tests passed without error.\n";

    return 0;
}
