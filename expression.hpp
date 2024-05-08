/*
Definitions for the Tokex system.

`sapling2` notation:

$[ $]           - Suite
$[^ $]          - Negated suite
$( $)           - Subexpression
$( a b $| c $)  - Either `a b` or `c`
$.              - Wildcard
$*              - Zero or more times
$+              - One or more times
$?              - Zero or one times
$~              - Clear memory
$>name          - Pipe memory onto variable
*/

#pragma once

#include <cassert>
#include <list>
#include <map>
#include <queue>
#include <set>

// Ensure constraints will work
static_assert(__cplusplus >= 2020'00L, "Please use -std=c++20");

// IMPORTANT: Constraints on generics.

// Each of the methods herein must be valid under the given type
// T in order for that type to be "Expressionable" and thus
// valid in later templates. This is the closure of all
// discernable tokens needed to construct a GPMA. Any token for
// which all of these is false is taken to be a literal.
template <typename T> class ExpressionableClass
{
  public:
    static bool is_subexpr_open(const T &);
    static bool is_subexpr_close(const T &);
    static bool is_disjunction(const T &);
    static bool is_wildcard(const T &);
    static bool is_optional(const T &);
    static bool is_star(const T &);
    static bool is_plus(const T &);
    static bool is_escape(const T &);

    // These can be contraditions if no such concept exists in
    // the desired expression type (IE RegEx).
    static bool is_mem_clear(const T &);
    static bool is_mem_pipe(const T &);

    // This is used to mark epsilon transitions
    static T wildcard();
    static T epsilon();
    static bool is_epsilon(const T &);
};

// For use later on. Used by saying `requires Expressionable<T>`
template <class T>
concept Expressionable =
    requires(T) { ExpressionableClass<T>(); };

////////////////////////////////////////////////////////////////

enum NodeType
{
    normal,    // A normal node
    scripting, // A code-only node (IE variable control)
    end,       // A terminal good node
    error,     // A terminal bad node
};

// Returns true if the machine is completed, false otherwise.
static inline const bool state_to_bool(const NodeType &state)
{
    return state == end;
}

// A single node in a pattern
template <typename T>
    requires Expressionable<T>
class Node
{
  public:
    // Transitions out of this node.
    std::map<T, Node *> next;

    std::list<T> script;

    // The type of the current node. Defaults to normal.
    NodeType type = normal;
};

////////////////////////////////////////////////////////////////

/*
Helper type used for compilation. This does NOT take ownership
of its nodes, and may not even have a complete list of them!
*/
template <typename T> class Expression
{
  public:
    Node<T> *first;

    // Appends another expression onto the end of this one. In
    // order for this not to cause a memory leak, `last` should
    // be nullptr. A partially constructed expression will use
    // nullptr ONLY to denote end.
    void knit_other_onto_end(const Expression<T> &_other);

    // Union
    void add_other_as_suit(const Expression<T> &_other);

    // Removes epsilon transitions via closure
    void remove_epsilons();
};

////////////////////////////////////////////////////////////////

/*
Helper function for knitting two expressions together.
*/
template <typename T>
void knit_recursive(Node<T> *_target, Node<T> *_cur,
                    std::set<Node<T> *> &_visited)
{
    for (auto &link : _cur->next)
    {
        if (link.second == nullptr)
        {
            _cur->next[link.first] = _target;
        }
        else if (!_visited.contains(link.second))
        {
            _visited.insert(link.second);
            knit_recursive(_target, link.second, _visited);
        }
    }
}

/*
Recursively replaces all instances of `nullptr` with the root
of the other expression, causing the other to be knitted onto
the end of this expression.
*/
template <typename T>
void Expression<T>::knit_other_onto_end(
    const Expression<T> &_other)
{
    std::set<Node<T> *> used = {_other.first};
    knit_recursive(_other.first, first, used);
}

/*
Recursive disjunction call.

If _mine.next and _theirs.next is nullptr:
    Both are already success
    No change needed; Exit

If _mine.next is nullptr but _theirs.next is not:
    Existing graph finishes here, other does not
    Epsilon into rest of _theirs
    Exit

If _theirs.next is nullptr but _mine.next is not:
    Existing graph goes on, other ends here
    Epsilon into success

Else
    Traditional recursion here
*/
template <typename T>
void suit_add_recursive(Node<T> *_mine, Node<T> *_theirs)
{
    assert(_mine != nullptr && _theirs != nullptr);

    // For each transition in _theirs->next
    for (const auto &t : _theirs->next)
    {
        // If already has this transition
        if (_mine->next.contains(t.first))
        {
            // `m` is my next transition, `o` is theirs.
            Node<T> *m = _mine->next[t.first], *o = t.second;

            if (m == nullptr && o == nullptr)
            {
                // No change needed; Exit
                return;
            }
            else if (m == nullptr || o == nullptr)
            {
                /*
                Epsilon to the remainder of _theirs
                */

                Node<T> *cur = _mine;
                while (cur->next.contains(T::epsilon()))
                {
                    cur = cur->next[T::epsilon()];
                }

                cur->next[T::epsilon()] = o;
                return;
            }

            // Neither are nullptr; Traditional recursion
            else
            {
                // Advance recursively
                suit_add_recursive(m, o);
            }
        }

        // Otherwise, trivial case
        else
        {
            // Add remaining of _theirs directly as next
            _mine->next[t.first] = t.second;
            return;
        }
    }
}

/*
Perform the union of the two expressions, such that either this
or other will lead to the next expression.
*/
template <typename T>
void Expression<T>::add_other_as_suit(
    const Expression<T> &_other)
{
    suit_add_recursive(first, _other.first);
}

/*
Performs epsilon closure on the given graph.
*/
template <typename T> void Expression<T>::remove_epsilons()
{
    std::set<Node<T> *> all_nodes;
    std::queue<Node<T> *> to_visit;
    all_nodes.insert(first);
    to_visit.push(first);

    // Collect nodes
    while (!to_visit.empty())
    {
        Node<T> *cur = to_visit.front();
        to_visit.pop();

        for (const auto &p : cur->next)
        {
            if (!all_nodes.contains(p.second))
            {
                all_nodes.insert(p.second);
                to_visit.push(p.second);
            }
        }
    }

    // Close each node
    for (const auto &node : all_nodes)
    {
        close_node(node);
    }
}

////////////////////////////////////////////////////////////////

/*
This will erase epsilon transitions and return a set of links
which should be added out of a given node in order to maintain
functionality.
*/
template <typename T> void close_node(Node<T> *_cur)
{
    if (_cur != nullptr && _cur->next.contains(T::epsilon()))
    {
        // Find all nodes reachable by only epsilons from this
        // node. Also erase epsilon transitions while we are
        // here.
        std::set<Node<T> *> closure;
        std::queue<Node<T> *> to_visit;
        to_visit.push(_cur);

        while (!to_visit.empty())
        {
            Node<T> *cur = to_visit.front();
            to_visit.pop();

            // If has not been visited so far
            if (cur != nullptr &&
                cur->next.contains(T::epsilon()) &&
                !closure.contains(cur->next[T::epsilon()]))
            {
                // Add to closure and visiting list
                closure.insert(cur->next[T::epsilon()]);
                to_visit.push(cur->next[T::epsilon()]);

                // Remove epsilon link SAFELY

                // Adopt type
                if (cur->next[T::epsilon()]->type != normal)
                {
                    cur->type = cur->next[T::epsilon()]->type;
                }

                cur->next.erase(T::epsilon());
            }
        }

        // Merge everything
        for (const auto &node : closure)
        {
            for (const auto &edge : node->next)
            {
                if (_cur->next.contains(edge.first))
                {
                    // Prevents infinite loop
                    if (_cur->next[edge.first] == _cur &&
                        edge.second == node)
                    {
                        continue;
                    }

                    // Recursive case
                    // Add epsilon linkage between the two
                    // candidates, then recurse.

                    // Add linkage
                    Node<T> *cursor = _cur->next[edge.first];
                    while (cursor->next.contains(T::epsilon()))
                    {
                        cursor = cursor->next[T::epsilon()];
                    }
                    cursor->next[T::epsilon()] = edge.second;

                    // Recurse
                    close_node(_cur->next[edge.first]);
                }
                else
                {
                    // Normal case
                    _cur->next[edge.first] = edge.second;
                }
            }
        }
    }
}
