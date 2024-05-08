/*
A very painful regular expression constructor for arbitrary
types.

Process:
1) Regex
2) Regex subexpressions
3) Partial epsilon-NFAs
4) Assembled epsilon-NFA
5) DFA with dead nodes
6) DFA

Jordan Dehmel, 2024.
*/

#pragma once

#include "expression.hpp"
#include "lexer.hpp"
#include <cassert>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef SAVEFIG
#include <cstdint>
#endif

////////////////////////////////////////////////////////////////

// A wrapper which encapsulated a series of Nodes.
template <typename T = Token> class Tokex
{
  public:
    Tokex()
    {
    }

    Tokex(const std::vector<T> &_pattern)
    {
        compile(_pattern);
    }

    ~Tokex();

    // Return the type of the current node, which is the
    // current state of the Tokex expression.
    NodeType get_state();

    // Create a new Tokex structure from a pattern.
    // The default syntax here is `sapling2`.
    void compile(const std::vector<T> &pattern);
    Expression<T> compile(const std::vector<T> &pattern,
                          const int &_begin, const int &_end);

    // Run on the given input. These work on simple DFA rules;
    // all the complexity of this system comes from compile.
    NodeType run(const std::list<T> &input,
                 const bool &allow_epsilons = false);

    // A basic wrapper to `run` which provides a RegEx-style
    // interface. Returns true if and only if the provided data
    // matches the (already compiled) pattern. This calls reset
    // before running to match expected RegEx behavior.
    bool match(const std::list<T> &input);

    // Run a single token of input. This is where the real work
    // is done.
    void run(const T &input, const bool &allow_epsilons);

    // Reset the variables and memory.
    void reset();

    // Get the contents of a given variable. A non-existant
    // variable is just an empty list.
    const std::list<T> fetch_variable(
        const T &variable_name) const;

    // Returns false if and only if there are no epsilon
    // transitions remain.
    bool has_epsilons() const;

    // Returns all REACHABLE nodes in the graph.
    std::list<Node<T> *> get_all_nodes();

    // Print the graph.
    void print();

    // Write the `*.dot` file which will represent the compiled
    // graph in GraphViz. This is similar (but not exactly the
    // same as) print.
    void graphviz(const std::string &_filepath,
                  const std::string &_title = "");

    // Erases all unreachable nodes.
    void purge();

  protected:
    // Dynamically allocate a new node on the heap. This also
    // adds the newly created node to the set of all nodes
    // internally so that it can be freed later. This avoids
    // memory leaks.
    Node<T> *create_node();

    Expression<T> duplicate_expression(
        const Expression<T> &_what);

    // Pointer to the entry node
    Node<T> *beginning = nullptr;

    // Pointer to the current node in pattern matching.
    Node<T> *current = nullptr;

    // The nodes to clean up upon deletion.
    std::set<Node<T> *> allNodes;

    std::list<T> memory;
    std::map<T, std::list<T>> variables;
};

////////////////////////////////////////////////////////////////

template <typename T>
bool Tokex<T>::match(const std::list<T> &input)
{
    reset();
    return state_to_bool(run(input));
}

// Delete a Tokex machine
template <typename T> Tokex<T>::~Tokex()
{
    current = nullptr;
    beginning = nullptr;

    // Delete all items which have been dynamically allocated
    // over time
    for (Node<T> *item : allNodes)
    {
        delete item;
    }
    allNodes.clear();
}

// Get the current state of the machine
template <typename T> NodeType Tokex<T>::get_state()
{
    if (current == nullptr)
    {
        return NodeType::error;
    }
    else
    {
        return current->type;
    }
}

// Run the machine on a series of tokens.
template <typename T>
NodeType Tokex<T>::run(const std::list<T> &input,
                       const bool &allow_epsilons)
{
    for (const auto &item : input)
    {
        run(item, allow_epsilons);
    }
    return get_state();
}

// Get the contents of a given variable. A non-existant variable
// is just an empty list.
template <typename T>
const std::list<T> Tokex<T>::fetch_variable(
    const T &variable_name) const
{
    if (variables.count(variable_name) == 0)
    {
        return std::list<T>();
    }
    else
    {
        return variables.at(variable_name);
    }
}

// Reset the running position of the machine. Does NOT clear the
// currently compiled machine.
template <typename T> void Tokex<T>::reset()
{
    memory.clear();
    variables.clear();
    current = beginning;
}

// Dynamically allocate a new node, and add it to the internal
// list of allocated nodes.
template <typename T> Node<T> *Tokex<T>::create_node()
{
    Node<T> *out = new Node<T>;
    allNodes.insert(out);
    return out;
}

template <typename T>
Expression<T> Tokex<T>::duplicate_expression(
    const Expression<T> &_what)
{
    // Get all reachable nodes
    std::map<Node<T> *, Node<T> *> old_to_new;
    std::queue<Node<T> *> to_visit;
    to_visit.push(_what.first);

    // Map existing nodes to newly allocated ones
    while (!to_visit.empty())
    {
        auto cur = to_visit.front();
        to_visit.pop();

        if (!old_to_new.contains(cur))
        {
            if (cur != nullptr)
            {
                old_to_new[cur] = create_node();

                old_to_new[cur]->type = cur->type;
                old_to_new[cur]->script = cur->script;

                for (const auto &p : cur->next)
                {
                    to_visit.push(p.second);
                }
            }
            else
            {
                old_to_new[cur] = nullptr;
            }
        }
    }

    for (const auto &node : old_to_new)
    {
        if (node.first == nullptr)
        {
            continue;
        }

        // For each node, add transitions
        for (const auto &p : node.first->next)
        {
            assert(old_to_new.contains(p.second));
            old_to_new[node.first]->next[p.first] =
                old_to_new[p.second];
        }
    }

    // Build output
    Expression<T> out;
    out.first = old_to_new[_what.first];
    return out;
}

template <typename T>
std::list<Node<T> *> Tokex<T>::get_all_nodes()
{
    std::list<Node<T> *> out_l;
    std::set<Node<T> *> out;
    std::queue<Node<T> *> to_visit;

    out_l.push_back(beginning);
    out.insert(beginning);
    to_visit.push(beginning);

    while (!to_visit.empty())
    {
        Node<T> *cur = to_visit.front();
        to_visit.pop();

        for (const auto &p : cur->next)
        {
            if (!out.contains(p.second))
            {
                out_l.push_back(p.second);
                out.insert(p.second);
                to_visit.push(p.second);
            }
        }
    }

    return out_l;
}

template <typename T> void Tokex<T>::print()
{
    std::list<Node<T> *> all_nodes = get_all_nodes();
    std::map<Node<T> *, std::string> named_nodes;

    // Pass 1: Name all nodes
    for (Node<T> *node : all_nodes)
    {
        if (node != beginning)
        {
            named_nodes[node] =
                (node->type == end ? "E" : "q") +
                std::to_string(named_nodes.size());
        }
        else
        {
            named_nodes[node] = "IN";
        }
    }

    // Pass 2: Print transitions
    for (Node<T> *node : all_nodes)
    {
        for (const std::pair<T, Node<T> *> transition :
             node->next)
        {
            std::cout << named_nodes.at(node) << " -{";

            if (T::is_epsilon(transition.first))
            {
                std::cout << "EPS";
            }
            else
            {
                std::cout << transition.first;
            }

            std::cout << "}-> "
                      << named_nodes[transition.second] << '\n';
        }
    }
}

// Write the `*.dot` file which will represent the compiled
// graph in GraphViz. This is similar (but not exactly the
// same as) print.
template <class T>
void Tokex<T>::graphviz(const std::string &_filepath,
                        const std::string &_title)
{
    // Open file
    std::ofstream file(_filepath);
    assert(file.is_open());

    // Header
    file << "digraph tokex {\n"
         << "\tlabelloc=\"t\";\n"
         << "\tlabel=\"" << _title << "\";\n"
         << "\tgraph [dpi=200];\n"
         << "\trankdir=LR;\n"
         << "\tfontname=\"Helvetica\";\n"
         << "\tedge [arrowhead=normal,arrowtail=dot];\n"
         << "\tnode [shape=circle];\n"
         << "\t# Auto-generated by Tokex.\n\n";

    // Data
    std::map<Node<T> *, std::string> named_nodes;

    // Pass 1: Name all nodes
    for (Node<T> *node : allNodes)
    {
        if (node == nullptr)
        {
            named_nodes[node] = "Null";
            file << '\t' << named_nodes[node]
                 << " [label=<&#9658;>];\n";
        }
        else
        {
            named_nodes[node] =
                "q" + std::to_string(named_nodes.size());

            if (node == beginning)
            {
                file << '\t' << named_nodes[node]
                     << " [label=\""
                     << (node->type == end ? "BegEnd" : "Beg")
                     << "\"];\n";
            }
            else if (node->type == end)
            {
                file << "\t" << named_nodes[node]
                     << " [label=\"End\"];\n";
            }
            else
            {
                file << '\t' << named_nodes[node]
                     << " [label=\"\"];\n";
            }
        }
    }

    // Pass 2: Print transitions
    for (Node<T> *node : get_all_nodes())
    {
        for (const std::pair<T, Node<T> *> transition :
             node->next)
        {
            file << '\t' << named_nodes.at(node) << " -> "
                 << named_nodes[transition.second]
                 << " [label=";

            if (T::is_epsilon(transition.first))
            {
                file << "<&epsilon;>";
            }
            else
            {
                file << '"' << transition.first << '"';
            }

            file << "];\n";
        }
    }

    // Footer
    file << "}\n";

    // Close file
    file.close();
}

// Erases all unreachable nodes.
template <typename T> void Tokex<T>::purge()
{
    // Mark reachable nodes
    auto l = get_all_nodes();
    std::set<Node<T> *> reachable(l.begin(), l.end());
    auto copy = allNodes;

    for (const auto &item : copy)
    {
        if (!reachable.contains(item))
        {
            allNodes.erase(item);
        }
    }

    for (const auto &item : allNodes)
    {
        assert(reachable.contains(item));
    }
}

// Returns true if this is an epsilon-NFA, false if it's a DFA.
template <typename T> bool Tokex<T>::has_epsilons() const
{
    auto all_nodes = get_all_nodes();
    for (const auto &node : all_nodes)
    {
        for (const auto &trans : node->next)
        {
            if (T::is_epsilon(trans.first))
            {
                return true;
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////

template <typename T>
void Tokex<T>::compile(const std::vector<T> &pattern)
{
    // Fetch compiled results
    Expression<T> res = compile(pattern, 0, pattern.size());
    beginning = res.first;

    // Append success node
    Node<T> *success = create_node();
    success->type = end;
    Expression<T> expr;
    expr.first = success;
    res.knit_other_onto_end(expr);

    // Print if set up to do so
#ifdef SAVEFIG

    static uint64_t id = 0;

    graphviz(std::to_string(id) + ".eps.dot");
    system(("dot -Tpng " + std::to_string(id) + ".eps.dot -o " +
            std::to_string(id) + ".eps.png")
               .c_str());

#endif

    // Remove epsilon transitions
    res.remove_epsilons();

    // Remove dead nodes
    purge();

    // Print if set up to do so
#ifdef SAVEFIG

    graphviz(std::to_string(id) + ".dot");
    system(("dot -Tpng " + std::to_string(id) + ".dot -o " +
            std::to_string(id) + ".png")
               .c_str());

#ifdef SAVEFIGPATH
    system(("mv " + std::to_string(id) + ".* " SAVEFIGPATH)
               .c_str());
#endif

    ++id;

#endif
}

// Compiles the given pattern into a directed graph to be used
// as a tokex machine
template <typename T>
Expression<T> Tokex<T>::compile(const std::vector<T> &pattern,
                                const int &_begin,
                                const int &_end)
{
    static Node<T> *const NEXT_NODE = nullptr;

    // Break into subexpressions on a pseudo-stack
    // Compile subexpressions individually according to rules
    std::vector<Expression<T>> expressions;
    for (int i = _begin; i < _end; ++i)
    {
        if (T::is_escape(pattern[i]))
        {
            // ...
            // (beg) -...-> _
            ++i;

            Expression<T> to_add;
            to_add.first = create_node();

            to_add.first->next[pattern[i]] = NEXT_NODE;

            expressions.push_back(to_add);
        }

        else if (T::is_subexpr_open(pattern[i]))
        {
            int count = 1;
            ++i;
            std::vector<int> delims;

            // Gather breakpoints, raise error if unclosed
            delims.push_back(i - 1);
            while (true)
            {
                if (i >= _end)
                {
                    throw std::runtime_error(
                        "Unmatched opening subexpression "
                        "token.");
                }
                else if (T::is_subexpr_open(pattern[i]))
                {
                    ++count;
                }
                else if (T::is_disjunction(pattern[i]))
                {
                    if (count == 1)
                    {
                        delims.push_back(i);
                    }
                }
                else if (T::is_subexpr_close(pattern[i]))
                {
                    --count;

                    if (count == 0)
                    {
                        delims.push_back(i);
                        break;
                    }
                    else if (count < 0)
                    {
                        throw std::runtime_error(
                            "Unmatched closing subexpression "
                            "token.");
                    }
                }

                ++i;
            }

            // Compile breakpoints into expressions
            std::vector<Expression<T>> subexpressions;
            Expression<T> final_expr;
            for (int j = 0; j + 1 < delims.size(); ++j)
            {
                subexpressions.push_back(compile(
                    pattern, delims[j] + 1, delims[j + 1]));
            }

            // Merge expressions if need be
            if (subexpressions.size() > 1)
            {
                final_expr = subexpressions[0];
                for (int j = 1; j < subexpressions.size(); ++j)
                {
                    final_expr.add_other_as_suit(
                        subexpressions[j]);
                }
            }
            else if (subexpressions.size() == 1)
            {
                final_expr = subexpressions[0];
            }

            // Append merged and compiled expression
            expressions.push_back(final_expr);
        }

        else if (T::is_wildcard(pattern[i]))
        {
            // .
            // (beg) -any-> _

            Expression<T> to_add;
            to_add.first = create_node();

            to_add.first->next[T::wildcard()] = NEXT_NODE;

            expressions.push_back(to_add);
        }

        else if (T::is_optional(pattern[i]))
        {
            // ...?
            // (beg) -...-> _
            // (beg) -eps-> _

            assert(!expressions.empty());
            expressions.back().first->next[T::epsilon()] =
                NEXT_NODE;
        }
        else if (T::is_star(pattern[i]))
        {
            // ...*
            // (beg) -eps-> _
            // (beg) -...-> (beg)

            assert(!expressions.empty());
            expressions.back().knit_other_onto_end(
                expressions.back());
            expressions.back().first->next[T::epsilon()] =
                NEXT_NODE;
        }
        else if (T::is_plus(pattern[i]))
        {
            // ...+
            // (beg) -...-> (A)
            // (A) -...-> (A)
            // (A) -eps-> _

            assert(!expressions.empty());

            expressions.push_back(
                duplicate_expression(expressions.back()));

            expressions.back().knit_other_onto_end(
                expressions.back());

            expressions.back().first->next[T::epsilon()] =
                NEXT_NODE;
        }

        // Literal
        else
        {
            // ...
            // (beg) -...-> _

            Expression<T> to_add;
            to_add.first = create_node();

            to_add.first->next[pattern[i]] = NEXT_NODE;

            expressions.push_back(to_add);
        }
    }

    // Assemble subexpressions
    Expression<T> expression = expressions.front();
    for (int i = 1; i < expressions.size(); ++i)
    {
        expression.knit_other_onto_end(expressions[i]);
    }

    // Return result
    return expression;
}

// Run the machine on a single token.
template <typename T>
void Tokex<T>::run(const T &input, const bool &allow_epsilons)
{
    if (current == nullptr)
    {
        return;
    }
    else if (current->next.contains(input))
    {
        current = current->next[input];
    }
    else if (current->next.contains(T::wildcard()))
    {
        current = current->next[T::wildcard()];
    }
    else if (allow_epsilons &&
             current->next.contains(T::epsilon()))
    {
        current = current->next[T::epsilon()];
    }
    else
    {
        current = nullptr;
    }
}
