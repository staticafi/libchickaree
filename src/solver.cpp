#include <chickaree/solver.hpp>
#include <chickaree/strategy.hpp>
#include <cps/solver.hpp>
#include <cps/evaluation_cache.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>

namespace chickaree {


Solver::Solver(sala::Program const&  program, sala::NavigationGraph const&  graph, PathTree&  tree)
    : m_program{ program }
    , m_graph{ graph }
    , m_tree{ tree }

    , m_target_function{ 0U }
    , m_target_basic_block{ 0U }

    , m_start_time_point{}
    , m_timeout_seconds{ 0.0 }
    , m_remaining_seconds{ 0.0 }

    , m_memout_bytes{ 0LL }
    , m_remaining_bytes{ 0LL }

    , m_success{ false }
    , m_report{ .cps_failure = false, .timeout = false, .memout = false }
    , m_path{}
{}


bool  Solver::update_resource_limits()
{
    m_remaining_seconds = m_timeout_seconds - std::chrono::duration<double>(std::chrono::system_clock::now() - m_start_time_point).count();
    m_remaining_bytes = m_memout_bytes - m_tree.memory() - m_path.size() * sizeof(std::uint32_t);

    m_report.timeout = m_remaining_seconds <= 0.0;
    m_report.memout = m_remaining_bytes <= 0LL;

    return !m_report.timeout && !m_report.memout;
}


void  Solver::update_executed_tree_nodes(PathExecutor::RunOutcome const&  outcomes)
{
    for (std::uint32_t path_index = outcomes.path_index; path_index != 0U; --path_index)
    {
        std::uint32_t const tree_node_index{ m_path.at(path_index - 1U) };
        if (m_tree.executed(tree_node_index))
            break;
        m_tree.set_executed(tree_node_index);
    }
}


void  Solver::update_unreachable_tree_nodes(PathExecutor::RunOutcome const&  outcomes)
{
    for (std::size_t i = 0ULL; i != outcomes.black_box_functions.path_indices.size(); ++i)
        if (outcomes.black_box_functions.inputs_indices.at(i).empty())
        {
            std::uint32_t const path_index{ outcomes.black_box_functions.path_indices.at(i) };

            if (outcomes.diverged && path_index + 1U == outcomes.path_index)
            {
                m_tree.set_unreachable(m_path.at(outcomes.path_index));
                continue;
            }

            std::uint32_t const tree_branch_node_index{ m_path.at(path_index) };

            sala::NavigationGraph::Node const& graph_branch_node{ m_graph.node(m_tree.graph_node_index(tree_branch_node_index)) };
            INVARIANT(graph_branch_node.successors.size() == 2ULL);

            std::uint32_t const graph_node_index{ m_tree.graph_node_index(m_path.at(path_index + 1U)) };
            std::uint32_t const other_graph_node_index{ 
                    graph_branch_node.successors.front() == graph_node_index
                        ? graph_branch_node.successors.back()
                        : graph_branch_node.successors.front()
                    };

            std::uint32_t other_tree_node_index{ m_tree.find_child(tree_branch_node_index, other_graph_node_index) };
            if (other_tree_node_index == PathTree::INVALID_INDEX)
                other_tree_node_index = m_tree.insert_child(tree_branch_node_index, other_graph_node_index, 0);

            m_tree.set_unreachable(other_tree_node_index);
        }
}


void  Solver::run(
        std::uint32_t const  target_function,
        std::uint32_t const  target_basic_block,
        double const  timeout_seconds,
        std::int64_t const  memout_bytes
        )
{
    // First we must reset the variables used for this run.

    m_target_function = target_function;
    m_target_basic_block = target_basic_block;

    m_start_time_point = std::chrono::system_clock::now();
    m_timeout_seconds = timeout_seconds;
    m_remaining_seconds = 0.0;

    m_memout_bytes = memout_bytes;
    m_remaining_bytes = 0LL;

    m_success = false;
    m_report.cps_failure = false;
    m_report.timeout = false;
    m_report.memout = false;

    m_path.clear();

    // Here starts the actual computation.

    while (true)
    {
        if (!update_resource_limits())
            return;

        std::uint32_t const  tree_end_node_index = chickaree::find_path(
                m_target_function,
                m_target_basic_block,
                m_graph,
                m_tree,
                m_remaining_seconds,
                m_remaining_bytes
                );
        if (tree_end_node_index == PathTree::INVALID_INDEX)
            return;

        m_path.clear();
        for (std::uint32_t  tree_node_index = tree_end_node_index;
                tree_node_index != PathTree::INVALID_INDEX;
                tree_node_index = m_tree.parent(tree_node_index) )
            m_path.push_back(tree_node_index);
        std::reverse(m_path.begin(), m_path.end());

        if (!update_resource_limits())
            return;

        if (m_path.empty())
            return;

        PathExecutor  executor{ m_program, m_graph, m_tree, m_path };

        std::vector<cps::Variable>  input;

        executor.run(input, m_remaining_seconds, m_remaining_bytes);
        update_executed_tree_nodes(executor.run_outcome());
        update_unreachable_tree_nodes(executor.run_outcome());

        cps::EvaluationCache cache;
        while (true)
        {
            if (!executor.run_outcome().diverged)
            {
                m_success = true;
                return;
            }
            if (m_tree.unreachable(m_path.at(executor.run_outcome().path_index)))
                break;

            cps::Solver solver{
                    executor.run_outcome().black_box_functions.inputs_indices,
                    executor.run_outcome().black_box_functions.comparators,
                    executor.run_outcome().inputs.variables,
                    executor.run_outcome().black_box_functions.output
                    };
            while (!solver.is_finished())
            {
                if (!update_resource_limits())
                    return;

                input.clear();
                solver.compute_next_input(input);

                std::vector<cps::Evaluation> const*  output;
                cps::EvaluationCache::Output const* const cached_output{ cache.find(input) };
                if (cached_output != nullptr)
                    output = cached_output;
                else
                {
                    if (!update_resource_limits())
                        return;

                    executor.run(input, m_remaining_seconds, m_remaining_bytes);
                    update_executed_tree_nodes(executor.run_outcome());
                    update_unreachable_tree_nodes(executor.run_outcome());

                    output = &executor.run_outcome().black_box_functions.output;

                    cache.insert(input, *output);
                }

                if (!update_resource_limits())
                    return;

                solver.process_output(*output);
            }

            if (!solver.success())
            {
                m_report.cps_failure = true;
                m_tree.set_unreachable(m_path.at(executor.run_outcome().path_index));
                break;
            }
        }
    }
}


}
