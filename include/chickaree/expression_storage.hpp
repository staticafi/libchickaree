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

    ExpressionStorage();

    ExpressionPtr get_or_create_input_expression(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        ExpressionBytes address_ = {}
        );
    ExpressionPtr get_or_create_constant_expression(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        ExpressionBytes address_ = {}
        );
    ExpressionPtr get_or_create_operator_expression(
        std::uint32_t tree_node_index_,
        std::uint32_t instruction_index_,
        std::vector<ExpressionBytes> children_,
        ExpressionBytes address_ = {}
        );

private:
    ExpressionStorage(ExpressionStorage const&) = delete;
    ExpressionStorage(ExpressionStorage&&) = delete;
    ExpressionStorage& operator=(ExpressionStorage const&) const = delete;
    ExpressionStorage& operator=(ExpressionStorage&&) const = delete;

    ExpressionPtr insert_temp();

    Storage m_storage;
    ExpressionPtr m_temp;
};


}

#endif
