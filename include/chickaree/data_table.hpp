#ifndef CHICKAREE_DATA_TABLE_HPP_INCLUDED
#   define CHICKAREE_DATA_TABLE_HPP_INCLUDED

#   include <chickaree/path_executor.hpp>
#   include <cps/variable.hpp>
#   include <cps/evaluation.hpp>
#   include <cps/comparator.hpp>
#   include <cstdint>
#   include <set>

namespace  chickaree {


struct DataTable
{
    struct Variable
    {
        bool operator==(Variable const& other) const { return graph_node_index == other.graph_node_index && value == other.value; }
        bool operator!=(Variable const& other) const { return !(*this == other); }
        bool operator<(Variable const& other) const;

        std::uint32_t graph_node_index;
        cps::Variable value;
    };

    struct Row
    {
        bool operator==(Row const& other) const
        { return evaluation == other.evaluation && inputs == other.inputs && constants == other.constants && counters == other.counters; }
        bool operator!=(Row const& other) const { return !(*this == other); }
        bool operator<(Row const& other) const;

        cps::Evaluation evaluation;
        std::vector<Variable> inputs;
        std::vector<Variable> constants;
        std::vector<Variable> counters;
    };

    DataTable(cps::Comparator cmp);

    void extend(Row const& row) { m_rows.insert(row); }

    cps::Comparator comparator() const { return m_comparator; }
    std::set<Row> const& rows() const { return m_rows; }

private:

    cps::Comparator m_comparator;
    std::set<Row> m_rows{};
};


}

#endif
