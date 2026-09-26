#ifndef CHICKAREE_EXPRESSION_FLOW_HPP_INCLUDED
#   define CHICKAREE_EXPRESSION_FLOW_HPP_INCLUDED

#   include <chickaree/expression.hpp>
#   include <chickaree/expression_storage.hpp>
#   include <sala/analyzer.hpp>
#   include <vector>
#   include <memory>
#   include <cstdint>

namespace chickaree {


struct ExpressionFlow : public sala::Analyzer
{
    explicit ExpressionFlow(sala::ExecState* exec_state, ExpressionStorage& storage_);

private:

    ExpressionStorage& m_storage;
};


}

#endif
