#ifndef CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED
#   define CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <chickaree/path_tree.hpp>
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
    struct  RunOutcome
    {
        using  Termination = sala::ExecState::Termination;

        struct  Variables
        {
            // Both vectors always have the same size.

            std::vector<cps::Variable>  variables{};
            std::vector<std::uint32_t>  path_indices{};
        };

        struct  BlackBoxFunctions
        {
            // The hits are counted after the first read of a variable
            // (input or constant) and stops before the evaluation of
            // the black box function.
            using  BasicBlocksHitCounts = std::unordered_map<
                    std::uint32_t,  // Index of the basic block in the control flow graph.
                    std::uint32_t   // The number of times the basic block was executed.
                    >;

            // All vectors always have the same size.

            std::vector<std::vector<std::size_t> >  inputs_indices{}; // TODO: Here is used std::size_t due to cps::Solver.
            std::vector<std::vector<std::uint32_t> >  constants_indices{};
            std::vector<cps::Comparator>  comparators{};
            std::vector<cps::Evaluation>  output{};
            std::vector<BasicBlocksHitCounts>  basic_blocks_hit_counts{};
            std::vector<std::uint32_t>  path_indices{};
        };

        struct  Branchings
        {
            // Both vectors always have the same size.

            std::vector<std::vector<std::uint32_t> >  black_box_functions_indices{};
            std::vector<std::uint32_t>  path_indices{};
        };

        // Results from the sala::ExecState
        Termination  termination{ Termination::UNKNOWN };
        std::string  message{};

        // The index of path record at which the execution terminated. 
        std::uint32_t  path_index{ 0U };

        // The input read during the execution.
        Variables  inputs{};

        // The constants (in constants segment) read during the execution.
        Variables  constants{};

        // Black box functions evaluated during the execution.
        BlackBoxFunctions  black_box_functions{};

        // Branchings taken during the execution.
        Branchings  branchings{};
    };

    PathExecutor(
            sala::Program const&  program,
            sala::ControlFlowGraph const&  cfg,
            PathTree const&  tree,
            Path const&  path
            );

    sala::Program const&  get_program() const { return m_program; }
    sala::ControlFlowGraph const&  get_cfg() const { return m_cfg; }
    PathTree const&  get_tree() const { return m_tree; }
    Path const&  get_path() const { return m_path; }

    // Performs execution of the program for the passed input within
    // given resource limits. Outcomes from the previous call to this
    // method invalidated all data (RunOutcome) returned from the
    // previous call.
    RunOutcome const&  run(std::vector<cps::Variable> const&  input, double  timeout_seconds, std::size_t  max_memory_bytes);

    // Returns the same outcomes as those returned from the last call
    // of the run method. Data from the previous call are discarded.
    RunOutcome const&  run_outcome() const { return m_run_outcome; }

private:

    sala::Program const&  m_program;
    sala::ControlFlowGraph const&  m_cfg;
    PathTree const&  m_tree;
    Path const&  m_path;
    RunOutcome  m_run_outcome;
};


}

#endif
