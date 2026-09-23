#include <chickaree/data_table.hpp>

namespace  chickaree {


bool DataTable::Variable::operator<(Variable const& other) const
{
    return graph_node_index == other.graph_node_index ? value < other.value :
                                                        graph_node_index < other.graph_node_index;
}


bool DataTable::Row::operator<(Row const& other) const
{
    if (evaluation != other.evaluation)
        return evaluation < other.evaluation;
    if (inputs != other.inputs)
        return inputs < other.inputs;
    if (constants != other.constants)
        return constants < other.constants;
    return counters < other.counters;
}


DataTable::DataTable(cps::Comparator cmp)
    : m_comparator{ cmp }
    , m_rows{}
{}


}
