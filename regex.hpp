/*
Adapts the TokEx interface for standard RegEx.
Jordan Dehmel, 2024
jdehmel@outlook.com
*/

#include <cstring>
#include <vector>

#pragma once

#ifdef SAVEFIG

#ifndef FORCE_TOKEX_SAVEFIG
#undef SAVEFIG
#endif

#include "tokex.hpp"
#include <cstdint>

#define SAVEFIG

#else

#include "tokex.hpp"

#endif

class TokexChar
{
  public:
    TokexChar(const char &_data) : data(_data)
    {
    }
    TokexChar(const TokexChar &_other) : data(_other.data)
    {
    }

    inline bool operator<(const TokexChar &_other) const
    {
        return data < _other.data;
    }
    inline bool operator>(const TokexChar &_other) const
    {
        return data > _other.data;
    }
    inline bool operator==(const TokexChar &_other) const
    {
        return data < _other.data;
    }

    char data;

    static inline bool is_subexpr_open(const TokexChar &_c)
    {
        return _c.data == '(';
    }

    static inline bool is_subexpr_close(const TokexChar &_c)
    {
        return _c.data == ')';
    }

    static inline bool is_disjunction(const TokexChar &_c)
    {
        return _c.data == '|';
    }

    static inline bool is_wildcard(const TokexChar &_c)
    {
        return _c.data == '.';
    }

    static inline bool is_optional(const TokexChar &_c)
    {
        return _c.data == '?';
    }

    static inline bool is_star(const TokexChar &_c)
    {
        return _c.data == '*';
    }

    static inline bool is_plus(const TokexChar &_c)
    {
        return _c.data == '+';
    }

    static inline bool is_escape(const TokexChar &_c)
    {
        return _c.data == '\\';
    }

    static inline bool is_mem_clear(const TokexChar &_c)
    {
        return false;
    }

    static inline bool is_mem_pipe(const TokexChar &_c)
    {
        return false;
    }

    static inline bool is_epsilon(const TokexChar &_c)
    {
        return _c.data == '\0';
    }

    static char wildcard()
    {
        return '.';
    }
    static char epsilon()
    {
        return '\0';
    }
};

inline std::ostream &operator<<(std::ostream &_strm,
                                const TokexChar &_c)
{
    return _strm << _c.data;
}

typedef Tokex<TokexChar> RegEx;

static RegEx compile_regex(const char *const _pattern)
{
    std::vector<TokexChar> v_pattern;
    v_pattern.reserve(strlen(_pattern));
    for (const char *ptr = _pattern; *ptr; ++ptr)
    {
        v_pattern.push_back(TokexChar(*ptr));
    }

    RegEx out(v_pattern);

#ifdef SAVEFIG

    static uint64_t id = 0;
    out.graphviz(std::to_string(id) + ".dot", _pattern);
    system(("dot -Tpng " + std::to_string(id) + ".dot -o " +
            std::to_string(id) + ".png")
               .c_str());

#ifdef SAVEFIGPATH
    system(("mv " + std::to_string(id) + ".* " SAVEFIGPATH)
               .c_str());
#endif

    ++id;

#endif

    return out;
}

static bool regex_match(RegEx &_pattern, const char *_text)
{
    std::list<TokexChar> l_text;
    for (const char *ptr = _text; *ptr; ++ptr)
    {
        l_text.push_back(TokexChar(*ptr));
    }

    return _pattern.match(l_text);
}
