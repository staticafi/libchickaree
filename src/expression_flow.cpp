#include <chickaree/expression_flow.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace chickaree {


ExpressionFlow::ExpressionFlow(sala::ExecState* exec_state, ExpressionStorage& storage_)
    : sala::Analyzer{ exec_state }
    , m_storage{ storage_ }
{}


}
