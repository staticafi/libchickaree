#ifndef CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED
#   define CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <chickaree/path_tree.hpp>
#   include <chickaree/run_outcome.hpp>
#   include <sala/control_flow_graph.hpp>
#   include <sala/program.hpp>
#   include <sala/exec_state.hpp>
#   include <cps/variable.hpp>
#   include <cps/comparator.hpp>
#   include <cps/evaluation.hpp>
#   include <cstdint>
#   include <string>
#   include <vector>
#   include <unordered_map>

namespace  chickaree {


struct  PathExecutor
{
    PathExecutor(
            sala::Program const&  program,
            sala::ControlFlowGraph const&  cfg,
            PathTree const&  tree
            );

    sala::Program const&  get_program() const { return m_program; }
    sala::ControlFlowGraph const&  get_cfg() const { return m_cfg; }
    PathTree const&  get_tree() const { return m_tree; }

    // Performs execution of the program for the passed input within
    // given resource limits. Outcomes from the previous call to this
    // method invalidated all data (RunOutcome) returned from the
    // previous call.
    RunOutcomePtr  run(
        std::vector<cps::Variable> const&  input,
        Path const&  path,
        double  timeout_seconds,
        std::size_t  max_memory_bytes
        );

    // Returns the same outcomes as those returned from the last call
    // of the run method. Data from the previous call are discarded.
    RunOutcomePtr  run_outcome() const { return m_run_outcome; }

private:

    sala::Program const&  m_program;
    sala::ControlFlowGraph const&  m_cfg;
    PathTree const&  m_tree;
    RunOutcomePtr  m_run_outcome;
};


}

#endif
