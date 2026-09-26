#include <chickaree/expression.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace chickaree {


Expression::Expression(
        std::uint32_t const tree_node_index_,
        Kind const kind_,
        ExpressionPtr const address_,
        std::size_t const constant_segment_offset_,
        std::vector<ExpressionPtr> const& children_
        )
    : m_tree_node_index{ tree_node_index_ }
    , m_kind{ kind_ }
    , m_address{ address_ }
    , m_constant_segment_offset{ constant_segment_offset_ }
    , m_children{ children_ }
{}


ExpressionPtr Expression::create_input(
    std::uint32_t const tree_node_index_,
    ExpressionPtr const address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, INPUT, address_, 0U, std::vector<ExpressionPtr>{});
}


ExpressionPtr Expression::create_constant(
    std::uint32_t const tree_node_index_,
    std::size_t const constant_segment_offset_,
    ExpressionPtr const address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, CONSTANT, address_, constant_segment_offset_, std::vector<ExpressionPtr>{});
}


ExpressionPtr Expression::create_operator(
    std::uint32_t const tree_node_index_,
    std::vector<ExpressionPtr> const& children_,
    ExpressionPtr const address_
    )
{
    return std::make_shared<Expression>(tree_node_index_, OPERATOR, address_, 0U, children_);
}


bool operator==(Expression const& l, Expression const& r)
{
    if (l.tree_node_index() != r.tree_node_index() || l.kind() != r.kind() || l.address() != r.address())
        return false;
    switch (l.kind()) {
        case Expression::INPUT:
            return true;
        case Expression::CONSTANT:
            return l.constant_segment_offset() == r.constant_segment_offset();
        case Expression::OPERATOR:
            return l.children() == r.children();
        default: UNREACHABLE(); break;
    }
}


std::size_t ExpressionHasher::operator()(Expression const& e) const
{
    std::size_t seed{ std::hash<std::uint32_t>{}(e.tree_node_index()) };
    hash_combine(seed, std::hash<std::uint16_t>{}(e.kind()));
    hash_combine(seed, std::hash<void*>{}(e.address().get()));
    switch (e.kind()) {
        case Expression::INPUT:
            break;
        case Expression::CONSTANT:
            hash_combine(seed, std::hash<std::size_t>{}(e.constant_segment_offset()));
            break;
        case Expression::OPERATOR:
            for (auto const& ptr : e.children())
                hash_combine(seed, std::hash<void*>{}(ptr.get()));
            break;
        default: UNREACHABLE(); break;
    }
    return seed;
}


}
