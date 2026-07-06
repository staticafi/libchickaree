#include <chickaree/strategy.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <queue>
#include <algorithm>

namespace chickaree {


using  Cost = sala::NavigationGraph::Cost;


struct  QueueRecord
{
    std::uint32_t  tree_node_index;
    Cost  g;
    Cost  h;
};


bool operator<(QueueRecord const&  l, QueueRecord const&  r)
{
    // We want min-heap => we need inverse order.
    return sala::NavigationGraph::sum_costs(l.g, l.h) > sala::NavigationGraph::sum_costs(r.g, r.h);
}


Cost  estimate_cost(
        std::uint32_t const  from_graph_node_index,
        std::uint32_t const  to_graph_node_index,
        sala::NavigationGraph const&  graph,
        std::uint32_t const  tree_stack_entry_index,
        PathTree const&  tree
        )
{
    if (from_graph_node_index == to_graph_node_index)
        return 0U;

    sala::NavigationGraph::Node const&  n = graph.node(from_graph_node_index);
    sala::NavigationGraph::Node const&  t = graph.node(to_graph_node_index);

    if (n.function == t.function)
        return graph.intra_cost(from_graph_node_index, to_graph_node_index);

    Cost  best_cost_via_calls = sala::NavigationGraph::INFINITY_COST;
    {
        for (std::uint32_t const  call_node_index : graph.calls(n.function))
        {
            Cost const  cost = sala::NavigationGraph::sum_costs(
                    graph.intra_cost_stationary(from_graph_node_index, call_node_index),
                    graph.inter_cost_from_call(call_node_index, t.function)
                    );
            if (cost < best_cost_via_calls)
                best_cost_via_calls = cost;
        }
        if (best_cost_via_calls != sala::NavigationGraph::INFINITY_COST)
            best_cost_via_calls = sala::NavigationGraph::sum_costs(
                    best_cost_via_calls,
                    graph.intra_cost_stationary(graph.entry(t.function), to_graph_node_index)
                    );
    }

    Cost  best_cost_via_rets = sala::NavigationGraph::INFINITY_COST;
    {
        for (std::uint32_t const  ret_node_index : graph.rets(n.function))
        {
            Cost const  cost = graph.intra_cost_stationary(from_graph_node_index, ret_node_index);
            if (cost < best_cost_via_rets)
                best_cost_via_rets = cost;
        }
        if (best_cost_via_rets != sala::NavigationGraph::INFINITY_COST)
        {
            std::uint32_t const  call_site_tree_index = tree.call_site(tree_stack_entry_index);
            if (call_site_tree_index == PathTree::INVALID_INDEX)
                best_cost_via_rets = sala::NavigationGraph::INFINITY_COST;
            else
            {
                std::uint32_t const  succ_graph_node_index = graph.bb_next(tree.graph_node_index(call_site_tree_index));
                best_cost_via_rets = sala::NavigationGraph::sum_costs(
                    best_cost_via_rets,
                    estimate_cost(succ_graph_node_index, to_graph_node_index, graph, tree.next_stack_entry(tree_stack_entry_index), tree)
                    );
            }
        }
    }

    return std::min(best_cost_via_calls, best_cost_via_rets);
}


Path  find_path(
        std::uint32_t const  target_function,
        std::uint32_t const  target_basic_block,
        sala::NavigationGraph const&  graph,
        PathTree&  tree,
        double const  timeout_seconds,
        std::size_t const  max_memory_bytes
        )
{
    std::chrono::system_clock::time_point const  start_time_point = std::chrono::system_clock::now();

    std::uint32_t const  target_graph_node_index{ graph.bb_entry(target_function, target_basic_block) };
    std::priority_queue<QueueRecord>  queue;
    queue.push(QueueRecord{
        .tree_node_index = tree.root(),
        .g = 0U,
        .h = 0U // We do not need to compute cost estimate, since it is only record in the queue and will be popped immediately.
    });
    do
    {
        double const num_passed_seconds = std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_point).count();
        if (num_passed_seconds > timeout_seconds)
            return {}; // Failed to find the path due to timeout.
        std::size_t const  num_used_bytes = tree.memory() + queue.size() * sizeof(QueueRecord);
        if (num_used_bytes > max_memory_bytes)
            return {}; // Failed to find the path due to out of memory.

        QueueRecord  record = queue.top();
        queue.pop();

        std::uint32_t const  graph_node_index{ tree.graph_node_index(record.tree_node_index) };

        if (target_graph_node_index == graph_node_index)
        {
            Path  result;
            for (std::uint32_t  tree_node_index = record.tree_node_index;
                    tree_node_index != PathTree::INVALID_INDEX;
                    tree_node_index = tree.parent(tree_node_index) )
                result.push_back(tree.graph_node_index(tree_node_index));
            std::reverse(result.begin(), result.end());
            return result;
        }

        if (sala::NavigationGraph::sum_costs(record.g, record.h) == sala::NavigationGraph::INFINITY_COST)
            continue;

        sala::NavigationGraph::Node const&  n = graph.node(graph_node_index);
        if (graph.is_ret(graph_node_index))
        {
            std::uint32_t const  call_site_tree_index = tree.call_site(tree.stack_entry(record.tree_node_index));
            if (call_site_tree_index != PathTree::INVALID_INDEX)
            {
                std::uint32_t const  succ_graph_node_index = graph.bb_next(tree.graph_node_index(call_site_tree_index));
                std::uint32_t const  succ_tree_node_index = tree.get_or_insert_child(record.tree_node_index, succ_graph_node_index, -1);
                queue.push( QueueRecord {
                    .tree_node_index = succ_tree_node_index,
                    .g = sala::NavigationGraph::sum_costs(record.g, graph.num_instructions(graph_node_index)),
                    .h = estimate_cost(succ_graph_node_index, target_graph_node_index, graph, tree.stack_entry(succ_tree_node_index), tree)
                });
            }
        }
        else
        {
            std::int32_t const  stack_change = graph.is_call(graph_node_index) ? 1 : 0;
            for (std::uint32_t const  succ_graph_node_index : n.successors)
            {
                std::uint32_t const  succ_tree_node_index =
                    tree.get_or_insert_child(record.tree_node_index, succ_graph_node_index, stack_change);
                queue.push( QueueRecord {
                    .tree_node_index = succ_tree_node_index,
                    .g = sala::NavigationGraph::sum_costs(record.g, graph.intra_cost(graph_node_index, succ_graph_node_index)),
                    .h = estimate_cost(succ_graph_node_index, target_graph_node_index, graph, tree.stack_entry(succ_tree_node_index), tree)
                });
            }
        }
    }
    while (!queue.empty());
    return {}; // No path exists.
}


}
