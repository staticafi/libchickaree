#include <chickaree/executor.hpp>
#include <sala/sanitizer.hpp>
#include <sala/input_flow.hpp>
#include <sala/interpreter.hpp>
#include <sala/extern_code_cstd.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/timeprof.hpp>
#include <utility/config.hpp>
#if COMPILER() == COMPILER_VC()
    struct INT128 { std::int64_t _[2]; }; 
    struct UINT128 { std::uint64_t _[2]; }; 
#else
#   define INT128 __int128 
#   define UINT128 unsigned __int128 
#endif
#include <vector>
#include <algorithm>
#include <sstream>
#include <chrono>

namespace  chickaree {


struct  InputProvider
{
    InputProvider(std::vector<cps::Variable>&  input, sala::ExternCodeCStd&  externals);

    template<typename T>
    void  read_input(std::size_t  offset = 0ULL);

private:

    std::vector<cps::Variable>&  m_input;
    std::uint32_t  m_index;
    sala::ExecState const&  m_state;
};

InputProvider::InputProvider(std::vector<cps::Variable>&  input, sala::ExternCodeCStd&  externals)
    : m_input{ input }
    , m_index{ 0U }
    , m_state{ externals.state() }
{
#   define REGISTER_INPUT_READER(FN_NAME, IMPL) externals.register_code(#FN_NAME, [this]() { IMPL; })
#   define REGISTER_INPUT_READER_VIA_TYPE(FN_NAME, TYPE) REGISTER_INPUT_READER(FN_NAME, read_input<TYPE>() )

    REGISTER_INPUT_READER_VIA_TYPE(__fizzer_private_io_model_cmdline_read_argc, std::int32_t);
    REGISTER_INPUT_READER_VIA_TYPE(__fizzer_private_io_model_cmdline_read_char, std::int8_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_bool, bool);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_char, std::int8_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_short, std::int16_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_int, std::int32_t);
    if (m_state.program().num_cpu_bits() == 32U)
        REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_long, std::int32_t);
    else
        REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_long, std::int64_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_longlong, std::int64_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_uchar, std::uint8_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_ushort, std::uint16_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_uint, std::uint32_t);
    if (m_state.program().num_cpu_bits() == 32U)
        REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_ulong, std::uint32_t);
    else
        REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_ulong, std::uint64_t);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_ulonglong, std::uint64_t);
    REGISTER_INPUT_READER(__VERIFIER_nondet_int128, read_input<std::uint64_t>(); read_input<std::uint64_t>(8ULL) );
    REGISTER_INPUT_READER(__VERIFIER_nondet_uint128, read_input<std::uint64_t>(); read_input<std::uint64_t>(8ULL) );
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_float, float);
    REGISTER_INPUT_READER_VIA_TYPE(__VERIFIER_nondet_double, double);

#   undef REGISTER_INPUT_READER_VIA_TYPE
#   undef REGISTER_INPUT_READER
}


template<typename T>
void  InputProvider::read_input(std::size_t const  offset)
{
    if ((std::size_t)m_index >= m_input.size())
        m_input.push_back((T)0);
    ASSUMPTION((std::size_t)m_index < m_input.size());
    ASSUMPTION(m_input.at(m_index).type == cps::Variable((T)0).type);
    sala::MemPtr const  ptr{ m_state.stack_top().parameters().front().read<sala::MemPtr>() + offset };
    m_input.at(m_index).write_value_to_address(ptr);
    ++m_index;
}


struct InputFlow : public sala::InputFlow
{
    InputFlow(sala::ExecState*  state);

private:

    void start_input_flow(sala::MemPtr ptr, std::size_t count);
    void start_input_flow(std::size_t const count) { start_input_flow(parameters().front().read<sala::MemPtr>(), count);}
    void do_ret() override;

    sala::InputFlow::InputDescriptor  m_fresh_descriptor;
};


InputFlow::InputFlow(sala::ExecState* const  state)
    : sala::InputFlow{ state }
    , m_fresh_descriptor{ 0U }
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__fizzer_private_io_model_cmdline_read_argc, this->start_input_flow(sizeof(natural_8_bit)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__fizzer_private_io_model_cmdline_read_char, this->start_input_flow(sizeof(char)) );

    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_bool, this->start_input_flow(sizeof(bool)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_char, this->start_input_flow(sizeof(std::int8_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_short, this->start_input_flow(sizeof(std::int16_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_int, this->start_input_flow(sizeof(std::int32_t)) );
    if (program().num_cpu_bits() == 32U)
        REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_long, this->start_input_flow(sizeof(std::int32_t)) );
    else
        REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_long, this->start_input_flow(sizeof(std::int64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_longlong, this->start_input_flow(sizeof(std::int64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_uchar, this->start_input_flow(sizeof(std::uint8_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_ushort, this->start_input_flow(sizeof(std::uint16_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_uint, this->start_input_flow(sizeof(std::uint32_t)) );
    if (program().num_cpu_bits() == 32U)
        REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_ulong, this->start_input_flow(sizeof(std::uint32_t)) );
    else
        REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_ulong, this->start_input_flow(sizeof(std::uint64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_ulonglong, this->start_input_flow(sizeof(std::uint64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_int128, this->start_input_flow(2 * sizeof(std::uint64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_uint128, this->start_input_flow(2 * sizeof(std::uint64_t)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_float, this->start_input_flow(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__VERIFIER_nondet_double, this->start_input_flow(sizeof(double)) );
}


void InputFlow::start_input_flow(sala::MemPtr const ptr, std::size_t const count)
{
    ++m_fresh_descriptor;
    for (std::size_t i = 0ULL; i != count; ++i)
        start(ptr + i, m_fresh_descriptor);
}


void InputFlow::do_ret()
{
    sala::InputFlow::do_ret();



//     if (some_input_was_read_ && state().current_function().name() == "__fizzer_process_condition")
//     {
//         INVARIANT(data().sensitive_bits.size() < data().trace_size);

//         trace_index_type const  path_index{ (trace_index_type)data().sensitive_bits.size() };
//         trace_item const&  branching{ data().trace_ptr->at(path_index) };

//         if (branching.id != parameters().front().read<location_id>())
//         {
//             auto const expected{ branching.id };
//             auto const obtained{ parameters().front().read<location_id>() };
//             state().set_stage(sala::ExecState::Stage::FINISHED);
//             state().set_termination(
//                 sala::ExecState::Termination::ERROR,
//                 "input_flow_analysis[extern_code]",
//                 "Execution diverged from the expected path in the tree."
//                     " At path index " + std::to_string(path_index) + "/" + std::to_string(data().trace_size - 1U) + ": Unexpected location ID."
//                     " [Expected: " + std::to_string(expected) +
//                     ", obtained: " + std::to_string(obtained) + "]"
//                 );
//             return;
//         }

//         if (path_index + 1U < data().trace_size && branching.direction != parameters().at(1).read<bool>())
//         {
//             bool const expected{ branching.direction };
//             std::stringstream obtained; obtained << (integer_32_bit)parameters().at(1).read<natural_8_bit>();
//             auto const loc{ branching.id };
//             state().set_stage(sala::ExecState::Stage::FINISHED);
//             state().set_termination(
//                 sala::ExecState::Termination::ERROR,
//                 "input_flow_analysis[extern_code]",
//                 "Execution diverged from the expected path in the tree."
//                     " At path index " + std::to_string(path_index) + "/" + std::to_string(data().trace_size - 1U) + ": Unexpected direction taken."
//                     " [Expected: " + std::to_string(expected) +
//                     ", obtained: " + obtained.str() + "]"
//                     "[NOTE: location ID: " + std::to_string(loc) + "]"
//                 );
//             return;
//         }

//         data().sensitive_bits.push_back({});
//         std::unordered_set<natural_32_bit>&  sensitive_bits{ data().sensitive_bits.back() };
//         sala::MemPtr ptr{ parameters().at(2).start() };
//         for (std::size_t i = 0ULL; i != sizeof(branching_value); ++i)
//             for (auto const& desc : read(ptr + i)->descriptors())
//                 for (std::size_t j = 0ULL; j != 8ULL; ++j)
//                     sensitive_bits.insert(8ULL * desc + j);

//         if (path_index + 1U == data().trace_size)
//         {
//             state().set_stage(sala::ExecState::Stage::FINISHED);
//             state().set_termination(
//                 sala::ExecState::Termination::NORMAL,
//                 "input_flow_analysis[extern_code]",
//                 "Execution reached the last node of the expected path in the tree."
//                 );

//             return;
//         }
//     }


}


struct Interpreter final : public sala::Interpreter
{
    Interpreter(
            sala::ExecState* state,
            sala::ExternCode* extern_code,
            std::vector<sala::Analyzer*> const& analyzers,
            sala::ControlFlowGraph const&  cfg,
            Path const&  path
            );

private:

    void on_basic_block_changed();

    void do_jump() override;
    void do_branch() override;
    void do_call() override;
    void do_ret() override;

    sala::ControlFlowGraph const&  m_cfg;
    Path const&  m_path;
    std::uint32_t  m_index; // Index to the m_path to be reached next in the execution.
};


Interpreter::Interpreter(
        sala::ExecState* state,
        sala::ExternCode* extern_code,
        std::vector<sala::Analyzer*> const& analyzers,
        sala::ControlFlowGraph const&  cfg,
        Path const&  path
        )
    : sala::Interpreter( state, extern_code, analyzers )
    , m_cfg{ cfg }
    , m_path{ path }
    , m_index{ 1U } // The first basic block in the path is program entry point, which is reached by default. So, we wait for next one.
{}


void Interpreter::on_basic_block_changed()
{
    if (state().stage() != sala::ExecState::Stage::EXECUTING)
        return;

    sala::ControlFlowGraph::Node const& n = m_cfg.node(m_path.at(m_index));
    if (n.function != state().stack_top().function_index() || n.basic_block != ip().block())
    {
        state().set_stage(sala::ExecState::Stage::FINISHED);
        state().set_termination(
            sala::ExecState::Termination::NORMAL,
            "chickaree::Interpreter",
            state().make_error_message("The execution has diverged from the path at index " + std::to_string(m_index) + ".")
            );
    }
    ++m_index;
    if ((std::size_t)m_index == m_path.size())
    {
        state().set_stage(sala::ExecState::Stage::FINISHED);
        state().set_termination(
            sala::ExecState::Termination::NORMAL,
            "chickaree::Interpreter",
            state().make_error_message("Reached the end of the path.")
            );
    }
}


void Interpreter::do_jump()
{
    sala::Interpreter::do_jump();
    on_basic_block_changed();
}


void Interpreter::do_branch()
{
    sala::Interpreter::do_branch();
    on_basic_block_changed();
}


void Interpreter::do_call()
{
    sala::Interpreter::do_call();
    on_basic_block_changed();
}


void Interpreter::do_ret()
{
    sala::Interpreter::do_ret();
    on_basic_block_changed();
}


std::uint32_t  execute(
        sala::Program const&  program,
        sala::ControlFlowGraph const&  cfg,
        std::vector<cps::Variable>&  input,
        Path const&  path,
        std::uint32_t  max_milliseconds,
        std::uint32_t  max_megabytes
        )
{
    std::chrono::system_clock::time_point const  start_time_point = std::chrono::system_clock::now();

    sala::ExecState  state{ &program, (std::size_t)max_megabytes * 1024ULL * 1024ULL };
    sala::Sanitizer  sanitizer{ &state };
    sala::ExternCodeCStd  externals{ &state, &sanitizer };
    InputProvider  input_provider{ input, externals };
    InputFlow  flow{ &state };
    Interpreter  interpreter{ &state, &externals, { &sanitizer, &flow }, cfg, path };

    interpreter.run(start_time_point, 0.001 * (double)max_milliseconds);

    return 0U;
}


}
