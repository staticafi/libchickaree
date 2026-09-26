#include <chickaree/path.hpp>
#include <chickaree/path_tree.hpp>
#include <algorithm>

namespace chickaree {


Path get_path(PathTree const& tree, std::uint32_t target_tree_node)
{
    Path result;
    while (target_tree_node != PathTree::INVALID_INDEX)
    {
        result.push_back(target_tree_node);
        target_tree_node = tree.parent(target_tree_node);
    }
    std::reverse(result.begin(), result.end());
    return result;
}


}
