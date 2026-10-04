#ifndef CHICKAREE_EXPRESSION_HPP_INCLUDED
#   define CHICKAREE_EXPRESSION_HPP_INCLUDED

#   include <vector>
#   include <memory>
#   include <cstdint>

namespace chickaree {


struct Expression;
using ExpressionPtr = std::shared_ptr<Expression>;


struct ExpressionByte
{
    ExpressionPtr expression;
    std::uint32_t byte_index;
};


bool operator==(ExpressionByte const& l, ExpressionByte const& r);
inline bool operator!=(ExpressionByte const& l, ExpressionByte const& r) { return !(l == r); }
struct ExpressionByteHasher { std::size_t operator()(ExpressionByte const& e) const; };


using ExpressionBytes = std::vector<ExpressionByte>;


struct ExpressionBytesHasher { std::size_t operator()(ExpressionBytes const& ebs) const; };


// bool operator==(ExpressionBytes const& l, ExpressionBytes const& r);
// inline bool operator!=(ExpressionBytes const& l, ExpressionBytes const& r) { return !(l == r); }
// struct ExpressionBytesHasher { std::size_t operator()(ExpressionBytes const& e) const; };


struct Expression
{
    enum struct Kind : std::uint16_t
    {
        INPUT,      // Represents input (stdin, command-line, file, ...).
        CONSTANT,   // Represents constant read from constant segment.
        OPERATOR    // Instuction computing a new value from operants (ADD, ...).
    };

    //Expression();

    Expression(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        Kind kind_,
        ExpressionBytes address_,
        std::vector<ExpressionBytes> children_
        );

    static ExpressionPtr create_input(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        ExpressionBytes address_ = {}
        );
    static ExpressionPtr create_constant(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        ExpressionBytes address_ = {}
        );
    static ExpressionPtr create_operator(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        std::vector<ExpressionBytes> children_,
        ExpressionBytes address_ = {}
        );

    void assign(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        Kind kind_,
        std::vector<ExpressionBytes> children_,
        ExpressionBytes address_
        );

    // Methods common for all kinds:

    std::uint32_t tree_node_index() const { return m_tree_node_index; }
    std::uint32_t instruction_index() const { return m_instruction_index; }
    Kind kind() const { return m_kind; }
    ExpressionBytes const& address() const { return m_address; }

    bool is_input() const { return kind() == Kind::INPUT; }
    bool is_constant() const { return kind() == Kind::CONSTANT; }
    bool is_operator() const { return kind() == Kind::OPERATOR; }

    // Methods only for kind INPUT:
    /* None*/

    // Methods only for kind CONSTANT:
    /* None*/

    // Methods only for kind OPERATOR:
    std::uint32_t num_children() const { return (std::uint32_t)m_children.size(); }
    ExpressionBytes const& child(std::uint32_t const idx) const { return m_children.at(idx); }
    std::vector<ExpressionBytes> const& children() const { return m_children; }

private:

    // Data common for all kinds:
    std::uint32_t m_tree_node_index;
    std::uint32_t m_instruction_index;
    Kind m_kind;
    ExpressionBytes m_address;

    // Data only for kind INPUT:
    /* None*/

    // Data only for kind CONSTANT:
    /* None*/

    // Data only for kind OPERATOR:
    std::vector<ExpressionBytes> m_children;
};


bool operator==(Expression const& l, Expression const& r);
inline bool operator!=(Expression const& l, Expression const& r) { return !(l == r); }
struct ExpressionHasher { std::size_t operator()(Expression const& e) const; };


}

#endif
