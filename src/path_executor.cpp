#include <chickaree/path_executor.hpp>
#include <sala/sanitizer.hpp>
#include <sala/input_flow_with_updates.hpp>
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
    InputProvider(
            std::vector<cps::Variable> const&  input,
            sala::ExternCodeCStd&  externals,
            PathExecutor::RunOutcome&  outcomes
            );

    template<typename T>
    void  read_input(std::size_t  offset = 0ULL);

private:

    std::vector<cps::Variable> const&  m_input;
    std::uint32_t  m_index;
    sala::ExecState const&  m_state;
    PathExecutor::RunOutcome&  m_outcomes;
};


InputProvider::InputProvider(
        std::vector<cps::Variable> const&  input,
        sala::ExternCodeCStd&  externals,
        PathExecutor::RunOutcome&  outcomes
        )
    : m_input{ input }
    , m_index{ 0U }
    , m_state{ externals.state() }
    , m_outcomes{ outcomes }
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
    if ((std::size_t)m_index < m_input.size())
    {
        ASSUMPTION(m_input.at(m_index).type == cps::Variable((T)0).type);
        m_outcomes.inputs.variables.push_back(m_input.at(m_index));
        ++m_index;
    }
    else
        m_outcomes.inputs.variables.push_back((T)0);
    m_outcomes.inputs.path_indices.push_back(m_outcomes.path_index + 1U);   // We add 1U because this method is called before
                                                                            // Interpreter::on_basic_block_changed(). See method
                                                                            // Interpreter::do_call().

    sala::MemPtr const  ptr{ m_state.stack_top().parameters().front().read<sala::MemPtr>() + offset };
    m_outcomes.inputs.variables.back().write_value_to_address(ptr);
}


struct InputFlow : public sala::InputFlowWithUpdates
{
    InputFlow(sala::ExecState*  state, sala::ControlFlowGraph const&  cfg, PathExecutor::RunOutcome&  outcomes);

private:

    union TaintDescriptor
    {
        std::uint32_t  value;
        struct {
            std::uint32_t
                // Meaning of values:
                // 0 - Taint source is input.
                // 1 - Taint source is a constant (from constant segment).
                // 2 - Taint source is bool value resulting from evaluation of a black box function.
                // 3 - Not used (invalid value).
                type  :  2,

                // Index to the vector in outcomes, depending on the type:
                // type == 0 -> m_outcomes.black_box_functions.inputs_indices.
                // type == 1 -> m_outcomes.black_box_functions.constants_indices.
                // type == 2 -> A common index to all vectors in m_outcomes.black_box_functions.black_box_functions.
                index : 30;
        } fields;
    };

    sala::ControlFlowGraph const&  m_cfg;
    PathExecutor::RunOutcome&  m_outcomes;

    void start_input_flow(sala::MemPtr ptr, std::size_t count);
    void start_input_flow(std::size_t const count) { start_input_flow(parameters().front().read<sala::MemPtr>(), count);}

    template<typename T>
    void insert_black_box_function_unary();
    template<typename T>
    void insert_black_box_function_binary(cps::Comparator  comparator);
    template<typename T>
    void insert_black_box_function(cps::Comparator  comparator, double  function_value);

    void do_truncate_u64_u8() override;
    void do_truncate_u32_u8() override;
    void do_truncate_u16_u8() override;

    void do_f2i_f32_s8() override;
    void do_f2i_f32_u8() override;
    void do_f2i_f64_s8() override;
    void do_f2i_f64_u8() override;

    void do_p2i_8() override;

    void do_less_s8() override;
    void do_less_s16() override;
    void do_less_s32() override;
    void do_less_s64() override;
    void do_less_u8() override;
    void do_less_u16() override;
    void do_less_u32() override;
    void do_less_u64() override;
    void do_less_f32() override;
    void do_less_f64() override;
    void do_less_w32() override;
    void do_less_w64() override;

    void do_less_equal_s8() override;
    void do_less_equal_s16() override;
    void do_less_equal_s32() override;
    void do_less_equal_s64() override;
    void do_less_equal_u8() override;
    void do_less_equal_u16() override;
    void do_less_equal_u32() override;
    void do_less_equal_u64() override;
    void do_less_equal_f32() override;
    void do_less_equal_f64() override;
    void do_less_equal_w32() override;
    void do_less_equal_w64() override;

    void do_greater_s8() override;
    void do_greater_s16() override;
    void do_greater_s32() override;
    void do_greater_s64() override;
    void do_greater_u8() override;
    void do_greater_u16() override;
    void do_greater_u32() override;
    void do_greater_u64() override;
    void do_greater_f32() override;
    void do_greater_f64() override;
    void do_greater_w32() override;
    void do_greater_w64() override;

    void do_greater_equal_s8() override;
    void do_greater_equal_s16() override;
    void do_greater_equal_s32() override;
    void do_greater_equal_s64() override;
    void do_greater_equal_u8() override;
    void do_greater_equal_u16() override;
    void do_greater_equal_u32() override;
    void do_greater_equal_u64() override;
    void do_greater_equal_f32() override;
    void do_greater_equal_f64() override;
    void do_greater_equal_w32() override;
    void do_greater_equal_w64() override;

    void do_equal_u8() override;
    void do_equal_u16() override;
    void do_equal_u32() override;
    void do_equal_u64() override;
    void do_equal_f32() override;
    void do_equal_f64() override;
    void do_equal_w32() override;
    void do_equal_w64() override;

    void do_unequal_u8() override;
    void do_unequal_u16() override;
    void do_unequal_u32() override;
    void do_unequal_u64() override;
    void do_unequal_f32() override;
    void do_unequal_f64() override;
    void do_unequal_w32() override;
    void do_unequal_w64() override;

    void do_isnan_w32() override;
    void do_isnan_w64() override;

    void do_branch() override;
};


InputFlow::InputFlow(sala::ExecState* const  state, sala::ControlFlowGraph const&  cfg, PathExecutor::RunOutcome&  outcomes)
    : sala::InputFlowWithUpdates{ state }
    , m_cfg{ cfg }
    , m_outcomes{ outcomes }
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
    TaintDescriptor const  descriptor{
        .fields = {
            .type = 0U, // Tracking the input.
            .index = (std::uint32_t)m_outcomes.inputs.variables.size() - 1U
        }
    };
    start(ptr, count, descriptor.value);
}


template<typename T>
void InputFlow::insert_black_box_function_unary()
{
    insert_black_box_function<T>(cps::Comparator::UNEQUAL, (double)operands().back()->read<T>());
}


template<typename T>
void InputFlow::insert_black_box_function_binary(cps::Comparator const  comparator)
{
    insert_black_box_function<T>(comparator, (double)operands().at(1)->read<T>() - (double)operands().at(2)->read<T>());
}


template<typename T>
void InputFlow::insert_black_box_function(cps::Comparator const  comparator, double const  function_value)
{
    m_outcomes.black_box_functions.inputs_indices.push_back({});
    m_outcomes.black_box_functions.constants_indices.push_back({});
    for (auto const  raw_descriptor : read(operands().front()->start())->descriptors())
    {
        TaintDescriptor const  descriptor{ .value = raw_descriptor };
        if (descriptor.fields.type == 0)
        {
            INVARIANT((std::size_t)descriptor.fields.index < m_outcomes.inputs.variables.size());
            m_outcomes.black_box_functions.inputs_indices.back().push_back(descriptor.fields.index);
        }
        else
        {
            INVARIANT((std::size_t)descriptor.fields.index < m_outcomes.constants.variables.size());
            m_outcomes.black_box_functions.constants_indices.back().push_back(descriptor.fields.index);
        }
    }
    m_outcomes.black_box_functions.comparators.push_back(comparator);
    m_outcomes.black_box_functions.output.push_back(cps::Evaluation{
        .function = function_value,
        .predicate = operands().front()->read<std::uint8_t>() != (std::uint8_t)0
    });
    m_outcomes.black_box_functions.basic_blocks_hit_counts.push_back({});
    {
        UpdatesHitCountsPtr const updates_hit_counts = read_updates(operands().front()->start());
        if (updates_hit_counts != nullptr)
            for (auto const& id_and_count : *updates_hit_counts)
            {
                std::uint32_t const  node_index{
                    m_cfg.bb_of_instruction(id_and_count.first.func, id_and_count.first.block, id_and_count.first.instr)
                };
                m_outcomes.black_box_functions.basic_blocks_hit_counts.back().insert({ node_index, id_and_count.second });
            }
    }
    m_outcomes.black_box_functions.path_indices.push_back(m_outcomes.path_index);

    // We must start taint-flow for the bool value of the evaluated black-box function.
    TaintDescriptor const  descriptor{
        .fields = {
            .type = 2U, // Tracking bool value.
            .index = (std::uint32_t)m_outcomes.black_box_functions.comparators.size() - 1U
        }
    };
    start(operands().front()->start(), descriptor.value);
}


void InputFlow::do_truncate_u64_u8()
{
    sala::InputFlow::do_truncate_u64_u8();
    set_post_operation([this]() { insert_black_box_function_unary<std::uint8_t>(); });
}


void InputFlow::do_truncate_u32_u8()
{
    sala::InputFlow::do_truncate_u32_u8();
    set_post_operation([this]() { insert_black_box_function_unary<std::uint32_t>(); });
}


void InputFlow::do_truncate_u16_u8()
{
    sala::InputFlow::do_truncate_u16_u8();
    set_post_operation([this]() { insert_black_box_function_unary<std::uint16_t>(); });
}


void InputFlow::do_f2i_f32_s8()
{
    sala::InputFlow::do_f2i_f32_s8();
    set_post_operation([this]() { insert_black_box_function_unary<float>(); });
}


void InputFlow::do_f2i_f32_u8()
{
    sala::InputFlow::do_f2i_f32_u8();
    set_post_operation([this]() { insert_black_box_function_unary<float>(); });
}


void InputFlow::do_f2i_f64_s8()
{
    sala::InputFlow::do_f2i_f64_s8();
    set_post_operation([this]() { insert_black_box_function_unary<double>(); });
}


void InputFlow::do_f2i_f64_u8()
{
    sala::InputFlow::do_f2i_f64_u8();
    set_post_operation([this]() { insert_black_box_function_unary<double>(); });
}


void InputFlow::do_p2i_8()
{
    sala::InputFlow::do_p2i_8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_s8()
{
    sala::InputFlow::do_less_s8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_s16()
{
    sala::InputFlow::do_less_s16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_s32()
{
    sala::InputFlow::do_less_s32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_s64()
{
    sala::InputFlow::do_less_s64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_u8()
{
    sala::InputFlow::do_less_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint8_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_u16()
{
    sala::InputFlow::do_less_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint16_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_u32()
{
    sala::InputFlow::do_less_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint32_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_u64()
{
    sala::InputFlow::do_less_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint64_t>(cps::Comparator::LESS); });
}


void InputFlow::do_less_f32()
{
    sala::InputFlow::do_less_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::LESS); });
}


void InputFlow::do_less_f64()
{
    sala::InputFlow::do_less_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::LESS); });
}


void InputFlow::do_less_w32()
{
    sala::InputFlow::do_less_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::LESS); });
}


void InputFlow::do_less_w64()
{
    sala::InputFlow::do_less_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::LESS); });
}


void InputFlow::do_less_equal_s8()
{
    sala::InputFlow::do_less_equal_s8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_s16()
{
    sala::InputFlow::do_less_equal_s16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_s32()
{
    sala::InputFlow::do_less_equal_s32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_s64()
{
    sala::InputFlow::do_less_equal_s64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_u8()
{
    sala::InputFlow::do_less_equal_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint8_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_u16()
{
    sala::InputFlow::do_less_equal_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint16_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_u32()
{
    sala::InputFlow::do_less_equal_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint32_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_u64()
{
    sala::InputFlow::do_less_equal_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint64_t>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_f32()
{
    sala::InputFlow::do_less_equal_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_f64()
{
    sala::InputFlow::do_less_equal_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_w32()
{
    sala::InputFlow::do_less_equal_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_less_equal_w64()
{
    sala::InputFlow::do_less_equal_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::LESS_EQUAL); });
}


void InputFlow::do_greater_s8()
{
    sala::InputFlow::do_greater_s8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_s16()
{
    sala::InputFlow::do_greater_s16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_s32()
{
    sala::InputFlow::do_greater_s32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_s64()
{
    sala::InputFlow::do_greater_s64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_u8()
{
    sala::InputFlow::do_greater_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint8_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_u16()
{
    sala::InputFlow::do_greater_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint16_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_u32()
{
    sala::InputFlow::do_greater_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint32_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_u64()
{
    sala::InputFlow::do_greater_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint64_t>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_f32()
{
    sala::InputFlow::do_greater_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_f64()
{
    sala::InputFlow::do_greater_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_w32()
{
    sala::InputFlow::do_greater_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_w64()
{
    sala::InputFlow::do_greater_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::GREATER); });
}


void InputFlow::do_greater_equal_s8()
{
    sala::InputFlow::do_greater_equal_s8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_s16()
{
    sala::InputFlow::do_greater_equal_s16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_s32()
{
    sala::InputFlow::do_greater_equal_s32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_s64()
{
    sala::InputFlow::do_greater_equal_s64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_u8()
{
    sala::InputFlow::do_greater_equal_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint8_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_u16()
{
    sala::InputFlow::do_greater_equal_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint16_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_u32()
{
    sala::InputFlow::do_greater_equal_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint32_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_u64()
{
    sala::InputFlow::do_greater_equal_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::uint64_t>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_f32()
{
    sala::InputFlow::do_greater_equal_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_f64()
{
    sala::InputFlow::do_greater_equal_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_w32()
{
    sala::InputFlow::do_greater_equal_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_greater_equal_w64()
{
    sala::InputFlow::do_greater_equal_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::GREATER_EQUAL); });
}


void InputFlow::do_equal_u8()
{
    sala::InputFlow::do_equal_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::EQUAL); }); // indeed signed int!
}


void InputFlow::do_equal_u16()
{
    sala::InputFlow::do_equal_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::EQUAL); }); // indeed signed int!
}


void InputFlow::do_equal_u32()
{
    sala::InputFlow::do_equal_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::EQUAL); }); // indeed signed int!
}


void InputFlow::do_equal_u64()
{
    sala::InputFlow::do_equal_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::EQUAL); }); // indeed signed int!
}


void InputFlow::do_equal_f32()
{
    sala::InputFlow::do_equal_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::EQUAL); });
}


void InputFlow::do_equal_f64()
{
    sala::InputFlow::do_equal_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::EQUAL); });
}


void InputFlow::do_equal_w32()
{
    sala::InputFlow::do_equal_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::EQUAL); });
}


void InputFlow::do_equal_w64()
{
    sala::InputFlow::do_equal_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::EQUAL); });
}


void InputFlow::do_unequal_u8()
{
    sala::InputFlow::do_unequal_u8();
    set_post_operation([this]() { insert_black_box_function_binary<std::int8_t>(cps::Comparator::UNEQUAL); }); // indeed signed int!
}


void InputFlow::do_unequal_u16()
{
    sala::InputFlow::do_unequal_u16();
    set_post_operation([this]() { insert_black_box_function_binary<std::int16_t>(cps::Comparator::UNEQUAL); }); // indeed signed int!
}


void InputFlow::do_unequal_u32()
{
    sala::InputFlow::do_unequal_u32();
    set_post_operation([this]() { insert_black_box_function_binary<std::int32_t>(cps::Comparator::UNEQUAL); }); // indeed signed int!
}


void InputFlow::do_unequal_u64()
{
    sala::InputFlow::do_unequal_u64();
    set_post_operation([this]() { insert_black_box_function_binary<std::int64_t>(cps::Comparator::UNEQUAL); }); // indeed signed int!
}


void InputFlow::do_unequal_f32()
{
    sala::InputFlow::do_unequal_f32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_unequal_f64()
{
    sala::InputFlow::do_unequal_f64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_unequal_w32()
{
    sala::InputFlow::do_unequal_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_unequal_w64()
{
    sala::InputFlow::do_unequal_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_isnan_w32()
{
    sala::InputFlow::do_isnan_w32();
    set_post_operation([this]() { insert_black_box_function_binary<float>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_isnan_w64()
{
    sala::InputFlow::do_isnan_w64();
    set_post_operation([this]() { insert_black_box_function_binary<double>(cps::Comparator::UNEQUAL); });
}


void InputFlow::do_branch()
{
    m_outcomes.branchings.black_box_functions_indices.push_back({});
    m_outcomes.branchings.path_indices.push_back(m_outcomes.path_index);
    for (auto const  raw_descriptor : read(operands().front()->start())->descriptors())
    {
        TaintDescriptor const  descriptor{ .value = raw_descriptor };
        if (descriptor.fields.type == 2)
            m_outcomes.branchings.black_box_functions_indices.back().push_back(descriptor.fields.index);
    }
    sala::InputFlow::do_branch();
}


struct Interpreter final : public sala::Interpreter
{
    Interpreter(
            sala::ExecState* state,
            sala::ExternCode* extern_code,
            std::vector<sala::Analyzer*> const& analyzers,
            sala::ControlFlowGraph const&  cfg,
            PathTree const&  tree,
            Path const&  path,
            PathExecutor::RunOutcome&  outcomes
            );

private:

    void on_basic_block_changed();

    void do_jump() override;
    void do_branch() override;
    void do_call() override;
    void do_ret() override;

    sala::ControlFlowGraph const&  m_cfg;
    PathTree const&  m_tree;
    Path const&  m_path;
    PathExecutor::RunOutcome&  m_outcomes;
    std::uint32_t  m_index; // Index to the m_path to be reached next in the execution.
};


Interpreter::Interpreter(
        sala::ExecState* state,
        sala::ExternCode* extern_code,
        std::vector<sala::Analyzer*> const& analyzers,
        sala::ControlFlowGraph const&  cfg,
        PathTree const&  tree,
        Path const&  path,
        PathExecutor::RunOutcome&  outcomes
        )
    : sala::Interpreter( state, extern_code, analyzers )
    , m_cfg{ cfg }
    , m_tree{ tree }
    , m_path{ path }
    , m_outcomes{ outcomes }
    , m_index{ 1U } // The first basic block in the path is program entry point, which is reached by default. So, we wait for next one.
{}


void Interpreter::on_basic_block_changed()
{
    if (state().stage() != sala::ExecState::Stage::EXECUTING)
        return;

    sala::ControlFlowGraph::Node const& n = m_cfg.node(m_tree.graph_node_index(m_path.at(m_index)));
    if (n.function != state().stack_top().function_index() || n.basic_block != ip().block())
    {
        state().set_stage(sala::ExecState::Stage::FINISHED);
        state().set_termination(
            sala::ExecState::Termination::NORMAL,
            "chickaree::Interpreter",
            state().make_error_message("The execution has diverged from the path at index " + std::to_string(m_index) + ".")
            );
    }
    m_outcomes.path_index = m_index;
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


PathExecutor::PathExecutor(
        sala::Program const&  program,
        sala::ControlFlowGraph const&  cfg,
        PathTree const&  tree,
        Path const&  path
        )
    : m_program{ program }
    , m_cfg{ cfg }
    , m_tree{ tree }
    , m_path{ path }
    , m_run_outcome{}
{}


PathExecutor::RunOutcome const&  PathExecutor::run(
        std::vector<cps::Variable> const&  input,
        double const  timeout_seconds,
        std::size_t const  max_memory_bytes
        )
{
    std::chrono::system_clock::time_point const  start_time_point = std::chrono::system_clock::now();

    m_run_outcome = {}; // Clear data - preparation for the next execution.

    sala::ExecState  state{ &m_program, max_memory_bytes };
    sala::Sanitizer  sanitizer{ &state };
    sala::ExternCodeCStd  externals{ &state, &sanitizer };
    InputProvider  input_provider{ input, externals, m_run_outcome };
    InputFlow  flow{ &state, m_cfg, m_run_outcome };
    Interpreter  interpreter{ &state, &externals, { &sanitizer, &flow }, m_cfg, m_tree, m_path, m_run_outcome };

    interpreter.run(timeout_seconds - std::chrono::duration<double>(std::chrono::system_clock::now() - start_time_point).count());

    m_run_outcome.termination = state.termination();
    m_run_outcome.message = state.error_message();

    return m_run_outcome;
}


}
