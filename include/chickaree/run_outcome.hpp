#ifndef CHICKAREE_RUN_OUTCOME_HPP_INCLUDED
#   define CHICKAREE_RUN_OUTCOME_HPP_INCLUDED

#   include <sala/exec_state.hpp>
#   include <cps/variable.hpp>
#   include <cps/comparator.hpp>
#   include <cps/evaluation.hpp>
#   include <cstdint>
#   include <string>
#   include <vector>
#   include <unordered_map>
#   include <memory>

namespace  chickaree {


struct  RunOutcome;
using  RunOutcomePtr = std::shared_ptr<RunOutcome>;


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

    static inline RunOutcomePtr  create(std::uint32_t const target_tree_node_index)
    {
        RunOutcomePtr outcome = std::make_shared<RunOutcome>();
        outcome->target_tree_node_index = target_tree_node_index;
        return outcome;
    }

    // Results from the sala::ExecState
    Termination  termination{ Termination::UNKNOWN };
    std::string  message{};

    // The index of path record at which the execution terminated.
    // It thus represent the count of fully executed basic blocks
    // in the path. The field is is always less that the size of
    // the path. The last basic block in the path (the target) is
    // is never executed (the execution stops when entering that
    // basic block - right before the first instruction). All fields
    // in this struct are valid only for path indices less than
    // this field.
    std::uint32_t  path_index{ 0U };

    // The index of the "target" node in the path tree which we
    // want to reach by the execution. The execution reached this
    // node only when the filed "diverged" is true. Note that
    // the execution always follows the path from the root of the
    // tree to this node. But the execution may diverge from the
    // path before reaching the node.
    std::uint32_t  target_tree_node_index{ 0U };

    // When it is false, then the last basic block in the path (the
    // target) was reached. The field 'path_index' is the index
    // of that last element in the path.
    // When it is true, then the last basic block in the path (the
    // target) was NOT reached. The field 'path_index' is the index
    // of the first element in the path, which was not executed, i.e.,
    // the sibling of that node was reached by the execution instead
    // of the one in the path.
    bool diverged{ true };

    // The input read during the execution.
    Variables  inputs{};

    // The constants (in constants segment) read during the execution.
    Variables  constants{};

    // Black box functions evaluated during the execution.
    BlackBoxFunctions  black_box_functions{};
};


}

#endif
