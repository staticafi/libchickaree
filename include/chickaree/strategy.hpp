#ifndef CHICKAREE_STRATEGY_HPP_INCLUDED
#   define CHICKAREE_STRATEGY_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <chickaree/path_tree.hpp>
#   include <sala/navigation_graph.hpp>

namespace chickaree {


Path  find_path(
        std::uint32_t const  target_function,
        std::uint32_t const  target_basic_block,
        sala::NavigationGraph const&  graph,
        PathTree&  tree
        );


}



#endif
