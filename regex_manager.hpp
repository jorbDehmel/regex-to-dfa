/*
Defines the RegexManager class.
*/

#pragma once

#include "regex.hpp"
#include <map>
#include <string>

/*
Performs substitutions and composition for regular expressions.
This is a factory for regular expressions which keeps an
internal bank of named substitutions; When a regular expression
is requested, it performs any necessary substitutions.
*/
class RegexManager
{
  public:
    RegexManager()
    {
        register_substitution("\\d", "(0|1|2|3|4|5|6|7|8|9)");
        register_substitution(
            "\\w", "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|"
                   "v|w|x|y|z|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|"
                   "Q|R|S|T|U|V|W|X|Y|Z)");
        register_substitution("\\s", "( |\t|\n)");
    }

    // Registers a substituion under the given name. Any time a
    // future pattern uses the name, it will be replaced by the
    // value.
    void register_substitution(const std::string &_name,
                               const std::string &_value)
    {
        substitutions[_name] = perform_substitutions(_value);
    }

    // Compile a regular expression.
    RegEx create_regex(const std::string &_pattern)
    {
        return compile_regex(
            perform_substitutions(_pattern).c_str());
    }

    // Compiler a regular expression and register it as a
    // substitution.
    RegEx create_regex(const std::string &_name,
                       const std::string &_pattern)
    {
        register_substitution(_name, _pattern);
        return create_regex(substitutions[_name]);
    }

    const std::map<const std::string, std::string>
    get_substitutions() const
    {
        return substitutions;
    }

  protected:
    std::string perform_substitutions(
        const std::string &_on) const
    {
        std::string out = _on;
        bool done = false;
        size_t it;

        while (!done)
        {
            done = true;
            for (const auto &p : substitutions)
            {
                it = out.find(p.first);
                if (it != std::string::npos)
                {
                    done = false;
                    out.replace(it, p.first.size(), p.second);
                }
            }
        }

        return out;
    }

    std::map<const std::string, std::string> substitutions;
};
