#include <chickaree/expression.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace chickaree {


bool operator==(ExpressionByte const& l, ExpressionByte const& r)
{
    return l.byte_index == r.byte_index && l.expression == r.expression;
}


std::size_t ExpressionByteHasher::operator()(ExpressionByte const& e) const
{
    std::size_t hash{ (std::size_t)e.byte_index };
    hash_combine(hash, e.expression.get()); // hash_combine(hash, ExpressionHasher{}(*e.expression));
    return hash;
}


std::size_t ExpressionBytesHasher::operator()(ExpressionBytes const& ebs) const
{
    std::size_t hash{ 0UL };
    for (auto const& eb : ebs)
        hash_combine(hash, ExpressionByteHasher{}(eb));
    return hash;
}


// Expression::Expression()
//     : Expression(0U, 0U, Kind::INPUT, {}, {})
// {}


Expression::Expression(
        std::uint32_t const tree_node_index_,
        std::uint32_t const instruction_index_,
        Kind const kind_,
        ExpressionBytes address_,
        std::vector<ExpressionBytes> children_
        )
    : m_tree_node_index{ tree_node_index_ }
    , m_instruction_index{ instruction_index_ }
    , m_kind{ kind_ }
    , m_address{ std::move(address_) }
    , m_children{ std::move(children_) }
{}


ExpressionPtr Expression::create_input(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    ExpressionBytes address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, instruction_index_, Kind::INPUT, std::move(address_), std::vector<ExpressionBytes>{});
}


ExpressionPtr Expression::create_constant(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    ExpressionBytes address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, instruction_index_, Kind::CONSTANT, std::move(address_), std::vector<ExpressionBytes>{});
}


ExpressionPtr Expression::create_operator(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    std::vector<ExpressionBytes> children_,
    ExpressionBytes address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, instruction_index_, Kind::OPERATOR, std::move(address_), std::move(children_));
}


void Expression::assign(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    Kind const kind_,
    std::vector<ExpressionBytes> children_,
    ExpressionBytes address_
    )
{
    m_tree_node_index = tree_node_index_;
    m_instruction_index = instruction_index_;
    m_kind = kind_;
    m_address = std::move(address_);
    m_children = std::move(children_);
}


bool operator==(Expression const& l, Expression const& r)
{
    if (l.tree_node_index() != r.tree_node_index()
            || l.instruction_index() != r.instruction_index()
            || l.kind() != r.kind()
            || l.address() != r.address()
            )
        return false;
    switch (l.kind()) {
        case Expression::Kind::INPUT:
            return true;
        case Expression::Kind::CONSTANT:
            return true;
        case Expression::Kind::OPERATOR:
            return l.children() == r.children();
        default: UNREACHABLE(); break;
    }
}


std::size_t ExpressionHasher::operator()(Expression const& e) const
{
    std::size_t seed{ (std::size_t)e.tree_node_index() };
    hash_combine(seed, e.instruction_index());
    hash_combine(seed, e.kind());
    hash_combine(seed, ExpressionBytesHasher{}(e.address()));
    switch (e.kind()) {
        case Expression::Kind::INPUT:
            break;
        case Expression::Kind::CONSTANT:
            break;
        case Expression::Kind::OPERATOR:
            for (auto const& ebs : e.children())
                hash_combine(seed, ExpressionBytesHasher{}(ebs));
            break;
        default: UNREACHABLE(); break;
    }
    return seed;
}


}
