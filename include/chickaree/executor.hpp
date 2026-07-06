#ifndef CHICKAREE_EXECUTOR_HPP_INCLUDED
#   define CHICKAREE_EXECUTOR_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <sala/control_flow_graph.hpp>
#   include <cps/variable.hpp>
#   include <sala/program.hpp>
#   include <cstdint>

namespace  chickaree {


std::uint32_t  execute(
        sala::Program const&  program,
        sala::ControlFlowGraph const&  cfg,
        std::vector<cps::Variable>&  input,
        Path const&  path,
        double  timeout_seconds,
        std::size_t  max_megabytes
        );


}

#endif
