#ifndef CHICKAREE_EXPRESSION_HPP_INCLUDED
#   define CHICKAREE_EXPRESSION_HPP_INCLUDED

#   include <vector>
#   include <memory>
#   include <cstdint>

namespace chickaree {


struct Expression;
using ExpressionPtr = std::shared_ptr<Expression>;


struct Expression
{
    enum Kind : std::uint16_t
    {
        INPUT,      // Represents input (stdin, command-line, file, ...).
        CONSTANT,   // Represents constant read from constant segment.
        OPERATOR    // Instuction computing a new value from operants (ADD, ...).
    };

    Expression(
        std::uint32_t tree_node_index,
        Kind kind_,
        ExpressionPtr address_,
        std::size_t constant_segment_offset_,
        std::vector<ExpressionPtr> const& children_
        );

    static ExpressionPtr create_input(
        std::uint32_t tree_node_index_,
        ExpressionPtr address_ = nullptr
        );
    static ExpressionPtr create_constant(
        std::uint32_t tree_node_index_,
        std::size_t constant_segment_offset_,
        ExpressionPtr address_ = nullptr
        );
    static ExpressionPtr create_operator(
        std::uint32_t tree_node_index_,
        std::vector<ExpressionPtr> const& children_,
        ExpressionPtr address_ = nullptr
        );

    // Methods common for all kinds:

    std::uint32_t tree_node_index() const { return m_tree_node_index; }
    Kind kind() const { return m_kind; }
    ExpressionPtr address() const { return m_address; }

    bool is_input() const { return kind() == INPUT; }
    bool is_constant() const { return kind() == CONSTANT; }
    bool is_operator() const { return kind() == OPERATOR; }

    // Methods only for kind INPUT:
    /* None*/

    // Methods only for kind CONSTANT:
    std::size_t constant_segment_offset() const { return m_constant_segment_offset; }

    // Methods only for kind OPERATOR:
    std::uint32_t num_children() const { return (std::uint32_t)m_children.size(); }
    ExpressionPtr child(std::uint32_t const idx) const { return m_children.at(idx); }
    std::vector<ExpressionPtr> const& children() const { return m_children; }

private:

    // Data common for all kinds:
    std::uint32_t m_tree_node_index;
    Kind m_kind;
    ExpressionPtr m_address;

    // Data only for kind INPUT:
    /* None*/

    // Data only for kind CONSTANT:
    std::size_t m_constant_segment_offset; // This is the offset to constant segment from which the constant was read.

    // Data only for kind OPERATOR:
    std::vector<ExpressionPtr> m_children; // Pointers to operands of the operator instruction (ADD, ...). 
};


bool operator==(Expression const& l, Expression const& r);
inline bool operator!=(Expression const& l, Expression const& r) { return !(l == r); }
struct ExpressionHasher { std::size_t operator()(Expression const& e) const; };


}

#endif
