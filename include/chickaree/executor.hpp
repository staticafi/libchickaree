#ifndef CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED
#   define CHICKAREE_PATH_EXECUTOR_HPP_INCLUDED

#   include <chickaree/path.hpp>
#   include <sala/control_flow_graph.hpp>
#   include <cps/variable.hpp>
#   include <sala/program.hpp>
#   include <sala/exec_state.hpp>
#   include <cstdint>
#   include <string>

namespace  chickaree {


struct  PathExecutor
{
    using  Termination = sala::ExecState::Termination;

    PathExecutor(sala::Program const&  program, sala::ControlFlowGraph const&  cfg, Path const&  path);

    // You can fill in this vector be desired input to be read during
    // the execution. Missing variables will be inserted automatically
    // to the vector during the execution (so you can even keep the
    // vector empty). You can also insert more data than there will be
    // used during the execution.
    std::vector<cps::Variable>&  input() { return m_input; }

    void run(double  timeout_seconds, std::size_t  max_memory_bytes);

    // The following methods return data from the last call of the run method above.
    // Data from the previous call are lost.

    std::vector<cps::Variable> const&  input() const { return m_input; }
    std::uint32_t  path_index() const { return m_path_index; }
    Termination  termination() const { return m_termination; }
    std::string const&  message() const { return m_message; }

    // This method deletes all data from the last call to run method.
    // It is called automatically at the begin of the run method. So,
    // you do not have to call it explicitly.
    void clear();

private:

    sala::Program const&  m_program;
    sala::ControlFlowGraph const&  m_cfg;
    Path const&  m_path;
    std::vector<cps::Variable>  m_input;
    std::uint32_t  m_path_index;
    Termination  m_termination;
    std::string  m_message;
};


}

#endif
