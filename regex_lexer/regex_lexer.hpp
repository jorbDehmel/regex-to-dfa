/*
Uses a custom RegEx DFA compiler to generate runtime lexers.
Jordan Dehmel, 2024

Breaks an input string into a series of substrings which, when
concatenated, equal the input. The output series is said to be
the "lexed" version of the input. This series follows the
following restrictions:

- Each token will be a maximally-sized string which matches the
    regular expression that the lexer is constructed around
- No token shall be the empty string
- A token will be constructed for as long as its text matches
    the pattern. Upon the processing of a character which causes
    the match to fail, the token will be yielded. At this time,
    the state machine will instead advance to the position it
    would be in if the breaking character were the only
    character.
*/

#include "../regex.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>

// Assert viable C++ version
static_assert(__cplusplus >= 2020'00ULL);

template <typename DFAState, typename CharType> class RegexLexer
{
  public:
    static_assert(std::is_unsigned<DFAState>(),
                  "DFA state type must be unsigned");

    // Passed to callback function during lexing
    struct Token
    {
        // The matched text
        std::basic_string<CharType> text;

        // The state history of the text
        std::list<DFAState> states;

        // The index where this match began
        uint64_t starting_index;
    };

    RegexLexer(const RegEx &_r)
        : n_states(_r.get_all_nodes().size() + 1)
    {
        if (n_states + 1 > pow(2, sizeof(DFAState) * 8))
        {
            throw std::runtime_error("Invalid DFAState type");
        }

        dfa.assign(n_states * n_chars, 0);
        const auto entry_node = _r.get_entry_node();
        const auto all_nodes = _r.get_all_nodes();

        // Establish canonical mapping
        std::map<Node<TokexChar> *, uint64_t> names;
        delim_state = names.size();
        for (const auto &node : all_nodes)
        {
            names[node] = names.size();
        }

        // Translate to DFA
        for (auto node : all_nodes)
        {
            const auto cur_state = names.at(node);
            for (const auto &p : node->next)
            {
                if (p.second == entry_node && node->type == end)
                {
                    // This is a match-ending transition
                    set(cur_state, p.first.data, delim_state);
                }
                else
                {
                    // Normal transition
                    set(cur_state, p.first.data,
                        names.at(p.second));
                }
            }
        }

        state = names.at(entry_node);
    }

    // Gets the DFA entry at the state _s and the input
    // character _c.
    inline DFAState get(const DFAState &_s,
                        const CharType &_c) const
    {
        uint64_t safe_char = _c + ctype_offset;

        if (_s * n_chars + safe_char < dfa.size() &&
            _s * n_chars + safe_char >= 0)
        {
            return dfa[_s * n_chars + safe_char];
        }
        else
        {
            std::cerr << "Invalid state/char combo (" << _s
                      << ", " << _c << ")\n";
            throw std::runtime_error("Invalid DFA 'get'");
        }
    }

    // Sets the DFA entry at the given location to the given
    // new state.
    inline void set(const DFAState &_s, const CharType &_c,
                    const DFAState &_to)
    {
        uint64_t safe_char = _c + ctype_offset;

        if (_s * n_chars + safe_char < dfa.size() &&
            _s * n_chars + safe_char >= 0)
        {
            dfa[_s * n_chars + safe_char] = _to;
        }
        else
        {
            throw std::runtime_error("Invalid DFA 'set' call");
        }
    }

    // Resets the state machine, as if nothing had ever been
    // given to it
    inline void reset()
    {
        state = 0;
        index = 0;
        cur_tok.text.clear();
        cur_tok.starting_index = index;
    }

    // Process a single character. If the given character
    // completes a token, _callback will be called
    // appropriately. NOTE: Since the completion of a token
    // cannot be detected until the following character, a null
    // terminator character is required to be passed upon the
    // completion of the input text.
    inline void next(
        const CharType &_c,
        std::function<void(const Token &)> _callback)
    {
        // End of string
        if (_c == '\0')
        {
            _callback(cur_tok);
            cur_tok.starting_index = index;
            cur_tok.text.clear();
        }

        // Not end of string
        else
        {
            state = get(state, _c);
            if (state == delim_state)
            {
                _callback(cur_tok);
                cur_tok.starting_index = index;
                cur_tok.text.clear();

                state = get(state, _c);
                if (state == delim_state)
                {
                    throw std::runtime_error("Lex failure!");
                }
            }
            cur_tok.text.push_back(_c);
            cur_tok.states.push_back(state);
        }

        // Increment index
        ++index;
    }

    const uint64_t n_states;
    const uint64_t n_chars = pow(2, sizeof(CharType) * 8);
    const uint64_t ctype_offset =
        -std::numeric_limits<CharType>::lowest();

  protected:
    uint64_t index = 0;
    DFAState state = 0;
    std::vector<DFAState> dfa;
    DFAState delim_state;
    Token cur_tok = {std::basic_string<CharType>(), {}, index};
};
