#ifndef CHICKAREE_SOLVER_HPP_INCLUDED
#   define CHICKAREE_SOLVER_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/navigation_graph.hpp>
#   include <chickaree/path_tree.hpp>
#   include <chickaree/path.hpp>
#   include <chickaree/path_executor.hpp>
#   include <cstdint>
#   include <chrono>

namespace chickaree {


struct Solver
{
    struct Report
    {
        bool  cps_failure{ false };
        bool  timeout{ false };
        bool  memout{ false };
    };

    Solver(sala::Program const&  program, sala::NavigationGraph const&  graph, PathTree&  tree);

    void  run(
            std::uint32_t  target_function,
            std::uint32_t  target_basic_block,
            double  timeout_seconds,
            std::int64_t  memout_bytes
            );

    bool  success() const { return m_success; }
    Report const&  report() const { return m_report; }
    Path const&  path() const { return m_path; }

private:

    bool  update_resource_limits();
    void  update_unreachable_tree_nodes(PathExecutor::RunOutcome const&  outcomes);

    // Data common for all calls of run method.

    sala::Program const&  m_program;
    sala::NavigationGraph const&  m_graph;
    PathTree&  m_tree;

    // Data related to the current call of run method.
    // They are cleared in the begin of the run method.

    std::uint32_t  m_target_function;
    std::uint32_t  m_target_basic_block;

    std::chrono::system_clock::time_point  m_start_time_point;
    double  m_timeout_seconds;
    double  m_remaining_seconds;

    std::int64_t  m_memout_bytes;
    std::int64_t  m_remaining_bytes;

    bool  m_success;
    Report  m_report;
    Path  m_path;
};


}

#endif
