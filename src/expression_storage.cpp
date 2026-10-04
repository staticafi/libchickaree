#include <chickaree/expression_storage.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace chickaree {


static inline ExpressionPtr create_temp() { return Expression::create_input(0, 0U); }


ExpressionStorage::ExpressionStorage()
    : m_storage{}
    , m_temp{ create_temp() }
{}


ExpressionPtr ExpressionStorage::get_or_create_input_expression(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    ExpressionBytes address_
    )
{
    m_temp->assign(tree_node_index_, instruction_index_, Expression::Kind::INPUT, {}, std::move(address_));
    return insert_temp();
}


ExpressionPtr ExpressionStorage::get_or_create_constant_expression(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    ExpressionBytes address_
    )
{
    m_temp->assign(tree_node_index_, instruction_index_, Expression::Kind::CONSTANT, {}, std::move(address_));
    return insert_temp();
}


ExpressionPtr ExpressionStorage::get_or_create_operator_expression(
    std::uint32_t const tree_node_index_,
    std::uint32_t const instruction_index_,
    std::vector<ExpressionBytes> children_,
    ExpressionBytes address_
    )
{
    m_temp->assign(tree_node_index_, instruction_index_, Expression::Kind::OPERATOR, std::move(children_), std::move(address_));
    return insert_temp();
}


ExpressionPtr ExpressionStorage::insert_temp()
{
    auto const it_and_state = m_storage[m_temp->tree_node_index()].insert(ExpressionPtrWrapper{ m_temp });
    if (it_and_state.second)
        m_temp = create_temp();
    return it_and_state.first->value;
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
