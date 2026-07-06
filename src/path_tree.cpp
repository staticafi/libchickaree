#include <chickaree/path_tree.hpp>

namespace chickaree {


PathTree::PathTree(std::uint32_t  start_graph_node_index)
    : m_nodes{ Node {
        .graph_node_index = start_graph_node_index,
        .parent = INVALID_INDEX,
        .sibling = INVALID_INDEX,
        .child = INVALID_INDEX,
        .stack_entry = 0U   // This node must correspond to the initial stack frame.
    }}
{}


std::uint32_t  PathTree::next_stack_entry(std::uint32_t const  stack_entry_node_index) const
{
    std::uint32_t const  cs = call_site(stack_entry_node_index);
    return (cs == INVALID_INDEX) ? INVALID_INDEX : stack_entry(cs);
}


std::uint32_t  PathTree::find_child(std::uint32_t const  node_index, std::uint32_t const  graph_node_index)
{
    for (std::uint32_t  child_node_index = child(node_index); child_node_index != INVALID_INDEX; )
    {
        Node const&  c = node(child_node_index);
        if (c.graph_node_index == graph_node_index)
            return child_node_index;
        child_node_index = c.sibling;
    }
    return INVALID_INDEX;
}


std::uint32_t  PathTree::insert_child(
        std::uint32_t const  node_index,
        std::uint32_t const  graph_node_index,
        std::int32_t const  stack_depth_change
        )
{
    std::uint32_t const  child_node_index{ (std::uint32_t)m_nodes.size() };

    std::uint32_t  stack_entry_index;
    {
        if (stack_depth_change == 0)
            stack_entry_index = stack_entry(node_index);
        else if (stack_depth_change > 0)
            stack_entry_index = child_node_index;
        else
            stack_entry_index = next_stack_entry(node_index);
    }
    
    m_nodes.push_back(Node {
        .graph_node_index = graph_node_index,
        .parent = node_index,
        .sibling = INVALID_INDEX,
        .child = INVALID_INDEX,
        .stack_entry = stack_entry_index
    });

    Node&  n = m_nodes.at(node_index);
    if (n.child == INVALID_INDEX)
        n.child = child_node_index;
    else
    {
        std::uint32_t  prev_child_node_index = n.child;
        while (true)
        {
            Node&  c = m_nodes.at(prev_child_node_index);
            if (c.sibling == INVALID_INDEX)
            {
                c.sibling = child_node_index;
                break;
            }
            prev_child_node_index = c.sibling;
        }
    }

    return child_node_index;
}


std::uint32_t  PathTree::get_or_insert_child(
        std::uint32_t const  node_index,
        std::uint32_t const  graph_node_index,
        std::int32_t const  stack_depth_change
        )
{
    std::uint32_t const  child_index = find_child(node_index, graph_node_index);
    if (child_index != INVALID_INDEX)
        return child_index;
    return insert_child(node_index, graph_node_index, stack_depth_change);
}


std::size_t  PathTree::memory() const
{
    return m_nodes.size() * sizeof(Node);
}


}
