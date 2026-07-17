#ifndef CHICKAREE_PATH_TREE_HPP_INCLUDED
#   define CHICKAREE_PATH_TREE_HPP_INCLUDED

#   include <cstdint>
#   include <limits>
#   include <vector>

namespace chickaree {


struct  PathTree
{
    static constexpr std::uint32_t  INVALID_INDEX = std::numeric_limits<std::uint32_t>::max();

    struct Node
    {
        // Index of node in NavigationGraph/ControlFlowGraph
        // this tree node corresponds to.
        std::uint32_t  graph_node_index;

        std::uint32_t  parent;

        // An index of another node in the tree, which
        // has the same parent. All these nodes form
        // a single-linked list, where a "link" is this
        // filed. The last node in the list has
        // sibling == INVALID_INDEX.
        std::uint32_t  sibling;

        // An index of the first child node of this node.
        // All children form a single-linked list via
        // sibling field, as described above. Look into
        // implementation of find_child method to see
        // how children can be enumerated.
        std::uint32_t  child;

        // Each node corresponds with with some call-stack frame.
        // The frame is available, when we enter the entry block
        // of the called function. This node also corresponds to
        // a call-stack frame. The field stack_entry is an index
        // of a node whose graph_node_index is the entry block
        // of the called function (corresponding to the same
        // call-stack frame). With this field we can reconstruct
        // stack for every node in the tree.
        std::uint32_t  stack_entry;

        struct Flags
        {
            std::uint32_t  unreachable : 1;
        };

        Flags  flags;
    };

    explicit PathTree(std::uint32_t  start_graph_node_index);

    std::vector<Node> const&  nodes() const { return m_nodes; }
    Node const&  node(std::uint32_t const  node_index) const { return m_nodes.at(node_index); }

    std::uint32_t  root() const { return 0U; }

    // Getters of fields of the passed node (via its index):

    std::uint32_t  graph_node_index(std::uint32_t const  node_index) const { return node(node_index).graph_node_index; }
    std::uint32_t  parent(std::uint32_t const  node_index) const { return node(node_index).parent; }
    std::uint32_t  sibling(std::uint32_t const  node_index) const { return node(node_index).sibling; }
    std::uint32_t  child(std::uint32_t const  node_index) const { return node(node_index).child; }
    std::uint32_t  stack_entry(std::uint32_t const  node_index) const { return node(node_index).stack_entry; }
    Node::Flags const& flags(std::uint32_t const  node_index) const { return node(node_index).flags; }
    Node::Flags&  flags(std::uint32_t const  node_index) { return m_nodes.at(node_index).flags; }
    bool  unreachable(std::uint32_t const  node_index) const { return flags(node_index).unreachable == 1U; }
    void  set_unreachable(std::uint32_t const  node_index) { flags(node_index).unreachable = 1U; }

    // The method allows to jump from the passed stack entry node to the next
    // one (i.e., one level deeper in the call-stack). The parameter passed to
    // this function must be result from the method stack_entry or next_stack_entry.
    // Given an index of some node, say node_index, then we can traverse the stack
    // entries on the stack for that node as follows:
    //      for (std::uint32_t  entry_index = stack_entry(node_index);
    //           entry_index != INVALID_INDEX;
    //           entry_index = next_stack_entry(entry_index))
    //      { ... }
    std::uint32_t  next_stack_entry(std::uint32_t  stack_entry_node_index) const;

    // The parameter passed to this function must be result (not INVALID_INDEX) from
    // the method stack_entry or next_stack_entry. Returns index of node representing
    // the call site of the passed stack enty node.
    std::uint32_t  call_site(std::uint32_t  stack_entry_node_index) const { return parent(stack_entry_node_index); }

    // Enumerates children of node_index and returns index of that child with the passed graph_node_index.
    // If there is no such child, then it returns INVALID_INDEX.
    std::uint32_t  find_child(std::uint32_t  node_index, std::uint32_t  graph_node_index);

    // Inserts a new node to the tree with graph_node_index. Its parent will be node_index.
    // The stack_depth_change can be -1,0,1 depending on how the call-stack should be changed
    // with insertion of the new node. Value 1 means new stack-frame was pushed, -1 means
    // the frame on the top of the stack was popped, and 0 means stack was not changed.
    // The value drives the computation of the field stack_entry of the new node.
    std::uint32_t  insert_child(std::uint32_t  node_index, std::uint32_t  graph_node_index, std::int32_t  stack_depth_change);

    // This method first calls method find_child. If the child was found, then it is returned.
    // Otherwise, there is called the insert_child method.
    std::uint32_t  get_or_insert_child(std::uint32_t  node_index, std::uint32_t  graph_node_index, std::int32_t  stack_depth_change);

    // Returns number of bytes occupied by the tree.
    std::size_t  memory() const;

private:

    std::vector<Node>  m_nodes;
};
    

}


#endif
