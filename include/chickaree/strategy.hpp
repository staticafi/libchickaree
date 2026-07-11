#ifndef CHICKAREE_STRATEGY_HPP_INCLUDED
#   define CHICKAREE_STRATEGY_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <chickaree/path_tree.hpp>
#   include <sala/navigation_graph.hpp>
#   include <cstdint>

namespace chickaree {


// Extends the passed tree, until the path to the target
// is found or the function fails to find any such path.
// If the path is found, the function returns the index
// of a node in the tree representing the last node in the
// path from the root to the node. Otherwise, the value
// PathTree::INVALID_INDEX is returned.
std::uint32_t  find_path(
        std::uint32_t const  target_function,
        std::uint32_t const  target_basic_block,
        sala::NavigationGraph const&  graph,
        PathTree&  tree, 
        double const  timeout_seconds,
        std::size_t const  max_memory_bytes
        );


}

#endif
