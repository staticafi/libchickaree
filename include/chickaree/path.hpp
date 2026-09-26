#ifndef CHICKAREE_PATH_HPP_INCLUDED
#   define CHICKAREE_PATH_HPP_INCLUDED

#   include <vector>
#   include <cstdint>

namespace chickaree {


// Elements are indices of nodes in the PathTree.
// A Path instance thus represents a path in the tree.
using  Path = std::vector<std::uint32_t>;


struct  PathTree;


Path get_path(PathTree const& tree, std::uint32_t target_tree_node);


}

#endif
