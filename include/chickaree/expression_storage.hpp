#ifndef CHICKAREE_EXPRESSION_STORAGE_HPP_INCLUDED
#   define CHICKAREE_EXPRESSION_STORAGE_HPP_INCLUDED

#   include <chickaree/expression.hpp>
#   include <unordered_map>
#   include <unordered_set>

namespace chickaree {


struct ExpressionStorage
{
    struct ExpressionPtrWrapper { ExpressionPtr value{ nullptr }; bool operator==(ExpressionPtrWrapper const& other) const; };
    struct ExpressionPtrWrapperHasher { std::size_t operator()(ExpressionPtrWrapper const& w) const; };
    using StoragePerTreeNode = std::unordered_set<ExpressionPtrWrapper, ExpressionPtrWrapperHasher>;
    using Storage = std::unordered_map<std::uint32_t, StoragePerTreeNode>;

    ExpressionPtr get_or_create_input_expression(
        std::uint32_t tree_node_index_,
        ExpressionPtr address_ = nullptr
        );
    ExpressionPtr get_or_create_constant_expression(
        std::uint32_t tree_node_index_,
        std::size_t constant_segment_offset_,
        ExpressionPtr address_ = nullptr
        );
    ExpressionPtr get_or_create_operator_expression(
        std::uint32_t tree_node_index_,
        std::vector<ExpressionPtr> const& children_,
        ExpressionPtr address_ = nullptr
        );

private:

    Storage m_storage{};
    ExpressionPtrWrapper m_temp{};
};


}

#endif
