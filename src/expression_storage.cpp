#include <chickaree/expression_storage.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace chickaree {


ExpressionPtr ExpressionStorage::get_or_create_input_expression(
    std::uint32_t const tree_node_index_,
    ExpressionPtr const address_
    )
{
    return  m_storage[tree_node_index_].insert(
                ExpressionPtrWrapper{ Expression::create_input(tree_node_index_, address_) }
            ).first->value;
}


ExpressionPtr ExpressionStorage::get_or_create_constant_expression(
    std::uint32_t const tree_node_index_,
    std::size_t const constant_segment_offset_,
    ExpressionPtr const address_
    )
{
    return  m_storage[tree_node_index_].insert(
                ExpressionPtrWrapper{ Expression::create_constant(tree_node_index_, constant_segment_offset_, address_) }
            ).first->value;
}


ExpressionPtr ExpressionStorage::get_or_create_operator_expression(
    std::uint32_t const tree_node_index_,
    std::vector<ExpressionPtr> const& children_,
    ExpressionPtr const address_
    )
{
    return  m_storage[tree_node_index_].insert(
                ExpressionPtrWrapper{ Expression::create_operator(tree_node_index_, children_, address_) }
            ).first->value;

}


bool ExpressionStorage::ExpressionPtrWrapper::operator==(ExpressionPtrWrapper const& other) const
{
    return *value == *other.value;
}


std::size_t ExpressionStorage::ExpressionPtrWrapperHasher::operator()(ExpressionPtrWrapper const& w) const
{
    return ExpressionHasher{}(*w.value);
}


}
