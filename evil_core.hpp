#ifndef EVIL_CORE_HPP
#define EVIL_CORE_HPP

#include <iostream>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <immintrin.h>
#include <chrono>
#include <ctime>
#include <exception>
#include <utility>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/arithmetic/xor.hpp>

#define DEP_GRAPH_RESOLVE(x, y) x ## y
#define EVALUATE_NODE(node_id) DEP_GRAPH_RESOLVE(NODE_, node_id)
#define NODE_0 0x1
#define NODE_1 (NODE_0 + 0x2)
#define NODE_2 (NODE_1 * NODE_0 + 0x3)
#define NODE_3 (NODE_2 - NODE_1 + NODE_0)
#define NODE_4 (NODE_3 * EVALUATE_NODE(2) - NODE_1)
#define NODE_5 (NODE_4 + NODE_3 - NODE_2 * EVALUATE_NODE(1))
#define FINAL_GRAPH_VALUE (NODE_5 * 0 + NODE_4 * 0 + NODE_3 * 0 + EVALUATE_NODE(2) * 0)

#define KEY_H BOOST_PP_XOR(NODE_1, 0x2A)
#define KEY_e BOOST_PP_XOR(NODE_2, 0x1F)
#define KEY_l BOOST_PP_XOR(NODE_3, 0x4B)
#define KEY_o BOOST_PP_XOR(NODE_4, 0x0D)
#define KEY_comma BOOST_PP_XOR(NODE_5, 0x72)
#define KEY_space BOOST_PP_XOR(NODE_0, 0x3C)
#define KEY_W BOOST_PP_XOR(NODE_2, 0x54)
#define KEY_r BOOST_PP_XOR(NODE_3, 0x11)
#define KEY_d BOOST_PP_XOR(NODE_4, 0x6E)
#define KEY_excl  BOOST_PP_XOR(NODE_1, 0x1A)

#define HEX_MEM_PROTECTION_RW 0x3  
#define HEX_MEM_PROTECTION_RX 0x5  
#define HEX_MEM_FLAGS 0x22 
#define SYS_MMAP_HEX 0x9
#define SYS_MPROTECT_HEX 0xA       
#define SYS_EXIT_HEX 0x3C 
#define SYS_KILL_HEX 0x3E          
#define SIGUSR1_HEX 0xAU           
#define SYS_FORK_HEX 0x39          
#define SYS_PTRACE_HEX 0x65        

#define GENERATE_NOP_WASTE(z, n, text) asm volatile("nop");

template <int ID> struct Literal {
    static constexpr int id = ID;
    static constexpr bool sign = ID > 0;
    static constexpr int var = ID > 0 ? ID : -ID;
};

template <typename... Lits> struct Clause { static constexpr size_t size = sizeof...(Lits); };
template <typename... Clauses> struct Formula { static constexpr size_t size = sizeof...(Clauses); };

enum class TriState : char { Undefined, False, True };

template <size_t Size>
struct Env {
    TriState data[Size + 1] = { TriState::Undefined };
    constexpr TriState get(int var) const { return data[var]; }
    constexpr Env set(int var, bool val) const {
        Env copy = *this;
        copy.data[var] = val ? TriState::True : TriState::False;
        return copy;
    }
};

template <typename Lit, size_t EnvSize>
constexpr TriState eval_literal(Env<EnvSize> env) {
    TriState v = env.get(Lit::var);
    if (v == TriState::Undefined) return TriState::Undefined;
    if constexpr (Lit::sign) return v; else return v == TriState::True ? TriState::False : TriState::True;
}

template <typename ClauseType, size_t EnvSize> struct EvalClause;
template <size_t EnvSize, typename... Lits>
struct EvalClause<Clause<Lits...>, EnvSize> {
    static constexpr TriState call(Env<EnvSize> env) {
        bool has_undef = false; bool is_true = false;
        auto eval = [&](auto lit) {
            using L = decltype(lit); TriState res = eval_literal<L>(env);
            if (res == TriState::True) is_true = true; if (res == TriState::Undefined) has_undef = true;
        };
        (eval(Lits{}), ...);
        if (is_true) return TriState::True; if (has_undef) return TriState::Undefined; return TriState::False;
    }
};

template <typename ClauseType, size_t EnvSize> struct FindUnitLiteral {
    static constexpr int call(Env<EnvSize> env) {
        int undef_lit = 0; int undef_count = 0; bool is_true = false;
        auto check = [&](auto lit) {
            using L = decltype(lit); TriState res = eval_literal<L>(env);
            if (res == TriState::True) is_true = true;
            if (res == TriState::Undefined) { undef_count++; undef_lit = L::id; }
        };
        unpack_clause(check, ClauseType{});
        return (!is_true && undef_count == 1) ? undef_lit : 0;
    }
private:
    template <typename F, typename... Lits> static constexpr void unpack_clause(F f, Clause<Lits...>) { (f(Lits{}), ...); }
};

template <typename FormulaType, size_t VarsCount>
struct CompileTimeDPLL {
    template <typename... Clauses>
    static constexpr TriState eval_formula(Env<VarsCount> env, Formula<Clauses...>) {
        bool has_undef = false; bool is_false = false;
        auto eval = [&](auto cl) {
            using C = decltype(cl); TriState res = EvalClause<C, VarsCount>::call(env);
            if (res == TriState::False) is_false = true; if (res == TriState::Undefined) has_undef = true;
        };
        (eval(Clauses{}), ...);
        if (is_false) return TriState::False; if (has_undef) return TriState::Undefined; return TriState::True;
    }

    template <typename... Clauses>
    static constexpr int propagate_units(Env<VarsCount> env, Formula<Clauses...>) {
        int found_lit = 0;
        auto scan = [&](auto cl) { if (found_lit != 0) return; using C = decltype(cl); found_lit = FindUnitLiteral<C, VarsCount>::call(env); };
        (scan(Clauses{}), ...);
        return found_lit;
    }

    static constexpr std::pair<bool, Env<VarsCount>> solve(Env<VarsCount> env) {
        while (true) {
            int unit_lit = propagate_units(env, FormulaType{}); if (unit_lit == 0) break;
            env = env.set(unit_lit > 0 ? unit_lit : -unit_lit, unit_lit > 0);
        }
        TriState status = eval_formula(env, FormulaType{});
        if (status == TriState::True)  return { true, env }; if (status == TriState::False) return { false, env };
        int next_var = 0;
        for (size_t i = 1; i <= VarsCount; ++i) { if (env.get(i) == TriState::Undefined) { next_var = i; break; } }
        auto try_true = solve(env.set(next_var, true)); if (try_true.first) return try_true;
        return solve(env.set(next_var, false));
    }
};

template <unsigned char... Bytes>
struct CompileTimeCodeStorage {
    [[deprecated("Obfuscated JIT Table")]] static constexpr unsigned char data[sizeof...(Bytes)] = { Bytes... };
    [[nodiscard]] static constexpr size_t size = sizeof...(Bytes);
};
template <unsigned char... Bytes> constexpr unsigned char CompileTimeCodeStorage<Bytes...>::data[];

template <typename T, unsigned char... Extracted> struct JitBuilder;
template <unsigned char B, unsigned char... Tail, unsigned char... Extracted>
struct JitBuilder<CompileTimeCodeStorage<B, Tail...>, Extracted...> {
    using Type = typename JitBuilder<CompileTimeCodeStorage<Tail...>, Extracted..., B>::Type;
};
template <unsigned char... Extracted>
struct JitBuilder<CompileTimeCodeStorage<>, Extracted...> { using Type = CompileTimeCodeStorage<Extracted...>; };

using ValidOpcodes = typename JitBuilder<CompileTimeCodeStorage<
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, 0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00, 
    0x48, 0x8d, 0x35, 0x0a, 0x00, 0x00, 0x00, 0x48, 0xc7, 0xc2, 0x01, 0x00, 0x00, 0x00, 
    0x0f, 0x05, 0xc3, 0x0a, 0x00               
>>::Type;

using InvalidOpcodes = CompileTimeCodeStorage<0x0F, 0x0B>;

template <bool IsSat>
struct OpcodeSelector { using Type = ValidOpcodes; };
template <>
struct OpcodeSelector<false> { using Type = InvalidOpcodes; };

template<int N> struct Explosion { static const int BAM = Explosion<N - 1>::BAM + Explosion<N - 1>::BAM; };
template<> struct Explosion<0> { static const int BAM = 1; };
template<int N, typename T> struct DeepChain : public DeepChain<N - 1, typename DeepChain<N - 1, T>::NestedType> {
    using NestedType = typename DeepChain<N - 1, T>::NestedType; static constexpr int lookup() { return DeepChain<N - 1, T>::lookup() + 1; }
};
template <typename T> struct DeepChain<0, T> { using NestedType = T; static constexpr int lookup() { return 0; } };

[[gnu::always_inline]] [[nodiscard]] inline void* evil_hex_alloc(volatile size_t size) {
    void* volatile allocated_ptr = nullptr;
    asm volatile ("movq %[sys_num], %%rax; movq $0, %%rdi; movq %[bytes], %%rsi; movq %[prot], %%rdx; movq %[flags], %%r10; movq $-1, %%r8; movq $0, %%r9; syscall; movq %%rax, %[result];" : [result] "=r" (allocated_ptr) : [sys_num] "g" ((long)SYS_MMAP_HEX), [bytes] "g" ((long)size), [prot] "g" ((long)HEX_MEM_PROTECTION_RW), [flags] "g" ((long)HEX_MEM_FLAGS) : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9", "rcx", "r11", "memory");
    return allocated_ptr;
}

[[gnu::always_inline]] inline int evil_hex_mprotect(void* volatile addr, volatile size_t len, volatile int prot) {
    long volatile ret;
    asm volatile (".intel_syntax noprefix;\n\t" "mov rax, %[sys_num];\n\t" "mov rdi, %[address];\n\t" "mov rsi, %[length];\n\t" "mov rdx, %[protection];\n\t" "syscall;\n\t" "mov %[result], rax;\n\t" ".att_syntax;\n\t" : [result] "=r" (ret) : [sys_num] "g" ((long)SYS_MPROTECT_HEX), [address] "r" (addr), [length] "g" (len), [protection] "g" ((long)prot) : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory");
    return (int)ret;
}

inline volatile long global_my_pid = 0;

template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
class CharacterCapsule {
private:
    volatile char encrypted_data; volatile char xor_key;
public:
    [[maybe_unused]] CharacterCapsule() : encrypted_data('\0'), xor_key(0) {}
    template<typename U> [[gnu::hot]] CharacterCapsule(volatile U enc_c, volatile char key) : encrypted_data(static_cast<char>(enc_c)), xor_key(key) {
        [[maybe_unused]] void* volatile dummy = evil_hex_alloc(1 + Explosion<5 + FINAL_GRAPH_VALUE>::BAM * 0);
    }
    [[gnu::cold]] ~CharacterCapsule() volatile {
        if (encrypted_data == '\0') return;
        volatile char decrypted_char = '\0';
        __m128i vector_result = _mm_xor_si128(_mm_set1_epi8(encrypted_data), _mm_set1_epi8(xor_key));
        decrypted_char = static_cast<char>(_mm_cvtsi128_si32(vector_result) & 0xFF);
        if (decrypted_char != '\0') {
            volatile long syscall_number = 0x1; volatile long file_descriptor = 0x2; volatile long length = 0x1;         
            asm volatile (".intel_syntax noprefix;\n\t" "mov rax, %[sys];\n\t" "mov rdi, %[fd];\n\t" "mov rsi, %[buf];\n\t" "mov rdx, %[len];\n\t" "syscall;\n\t" ".att_syntax;\n\t" : : [sys] "g" (syscall_number), [fd] "g" (file_descriptor), [buf] "r" (&decrypted_char), [len] "g" (length) : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory");
        }
    }
};

namespace {
    inline int get_lunar_phase_shift() {
        std::time_t now = std::time(nullptr); std::tm* utc = std::gmtime(&now);
        int Y = utc->tm_year + 1900; int M = utc->tm_mon + 1; int D = utc->tm_mday;
        if (M <= 2) { Y -= 1; M += 12; }
        double JD = static_cast<int>(365.25 * (Y + 4716)) + static_cast<int>(30.6001 * (M + 1)) + (2 - (Y / 100) + ((Y / 100) / 4)) + D - 1524.5;
        double new_moons = (JD - 2451549.5) / 29.530588853;
        return ((static_cast<int>((new_moons - static_cast<long>(new_moons)) * 8.0 + 0.5) & 7) == 4) ? 0x1337 : 0; 
    }

    [[noreturn]] void evil_terminate_logger() {
        {
            volatile CharacterCapsule<int> c13('!' ^ KEY_excl,  KEY_excl); volatile CharacterCapsule<int> c12('d' ^ KEY_d,     KEY_d); volatile CharacterCapsule<int> c11('l' ^ KEY_l,     KEY_l);
            volatile CharacterCapsule<int> c10('r' ^ KEY_r,     KEY_r);    volatile CharacterCapsule<int> c9 ('o' ^ KEY_o,     KEY_o); volatile CharacterCapsule<int> c8 ('W' ^ KEY_W,     KEY_W);
            volatile CharacterCapsule<int> c7 (' ' ^ KEY_space, KEY_space);volatile CharacterCapsule<int> c6 (',' ^ KEY_comma, KEY_comma);volatile CharacterCapsule<int> c5 ('o' ^ KEY_o,     KEY_o);
            volatile CharacterCapsule<int> c4 ('l' ^ KEY_l,     KEY_l);    volatile CharacterCapsule<int> c3 ('l' ^ KEY_l,     KEY_l); volatile CharacterCapsule<int> c2 ('e' ^ KEY_e,     KEY_e);
            volatile CharacterCapsule<int> c1 ('H' ^ KEY_H,     KEY_H);
        }
        asm volatile (".intel_syntax noprefix;\n\t" "mov rax, %[sys_exit];\n\t" "xor rdi, rdi;\n\t" "syscall;\n\t" ".att_syntax;" :: [sys_exit] "g" ((long)SYS_EXIT_HEX));
        while(true) {}
    }
}

[[gnu::naked]] inline volatile int evil_ub_missing_return() { asm volatile (".intel_syntax noprefix;\n\t" "xor eax, eax;\n\t" "ret;\n\t" ".att_syntax;\n\t"); }

enum class ExecutionState { Init, AntiDebug, Unwind, Fork, Jit, FakeTrap, Halt };

template <ExecutionState State, bool SatPass>
struct PipelineStage { [[gnu::flatten]] static inline void dispatch(); };

struct ApocalypseNode {
    ExecutionState current_state; size_t runtime_payload; bool sat_validation;
    inline ApocalypseNode() : current_state(ExecutionState::Init), runtime_payload(0), sat_validation(false) {}

    [[nodiscard]] inline ApocalypseNode& operator+(ExecutionState next_state) { current_state = next_state; return *this; }
    [[nodiscard]] inline ApocalypseNode& operator<<(size_t payload) { runtime_payload ^= payload; return *this; }
    [[nodiscard]] inline ApocalypseNode& operator*=(bool sat_flag) { sat_validation = sat_flag; return *this; }

    [[deprecated("State Gate")]] inline void operator++(int) {
        if (current_state == ExecutionState::Init) {
            PipelineStage<ExecutionState::Init, true>::dispatch(); 
        } else if (current_state == ExecutionState::AntiDebug) {
            if (runtime_payload == 0x1337) { PipelineStage<ExecutionState::FakeTrap, true>::dispatch(); } 
            else {
                if (sat_validation) { PipelineStage<ExecutionState::Fork, true>::dispatch(); } 
                else { PipelineStage<ExecutionState::Fork, false>::dispatch(); }
            }
        }
    }
};

template <bool SatPass>
struct PipelineStage<ExecutionState::Init, SatPass> {
    static inline void dispatch() {
        BOOST_PP_REPEAT(5, GENERATE_NOP_WASTE, _)
        asm volatile (".intel_syntax noprefix;\n\t" "mov rax, 0x27;\n\t" "syscall;\n\t" "mov %[pid], rax;\n\t" ".att_syntax;\n\t" : [pid] "=r" (global_my_pid) :: "rax", "rcx", "r11");
        asm volatile ("movq %[sys_num], %%rax; movq %[pid], %%rdi; movq %[sig], %%rsi; syscall;" : : [sys_num] "g" ((long)SYS_KILL_HEX), [pid] "g" (global_my_pid), [sig] "g" ((long)SIGUSR1_HEX) : "rax", "rdi", "rsi", "rcx", "r11", "memory");
        
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        ApocalypseNode() + ExecutionState::AntiDebug << get_lunar_phase_shift() *= SatPass ++;
#pragma GCC diagnostic pop
    }
};

template <bool SatPass>
struct PipelineStage<ExecutionState::AntiDebug, SatPass> {
    static inline void dispatch() {
        long volatile ptrace_ret = 0;
        asm volatile (".intel_syntax noprefix;\n\t" "mov rax, %[sys_ptrace];\n\t" "xor rdi, rdi;\n\t" "xor rsi, rsi;\n\t" "mov rdx, 1;\n\t" "xor r10, r10;\n\t" "syscall;\n\t" "mov %[result], rax;\n\t" ".att_syntax;\n\t" : [result] "=r" (ptrace_ret) : [sys_ptrace] "g" ((long)SYS_PTRACE_HEX) : "rax", "rdi", "rsi", "rdx", "r10", "rcx", "r11", "memory");
        
        if (ptrace_ret < 0) { PipelineStage<ExecutionState::FakeTrap, SatPass>::dispatch(); } 
        else { PipelineStage<ExecutionState::Fork, SatPass>::dispatch(); }
    }
};

template <bool SatPass>
struct PipelineStage<ExecutionState::Fork, SatPass> {
    static inline void dispatch() {
        volatile long fork_pid = -1;
        asm volatile (".intel_syntax noprefix;\n\t" "mov rax, 0x39;\n\t" "syscall;\n\t" "mov %[result], rax;\n\t" ".att_syntax;\n\t" : [result] "=r" (fork_pid) :: "rax", "rcx", "r11");
        if (fork_pid == 0) { PipelineStage<ExecutionState::Jit, SatPass>::dispatch(); } 
        else { PipelineStage<ExecutionState::Unwind, SatPass>::dispatch(); }
    }
};

template <bool SatPass>
struct PipelineStage<ExecutionState::Unwind, SatPass> { [[noreturn]] static inline void dispatch() { std::terminate(); } };

template <bool SatPass>
struct PipelineStage<ExecutionState::Jit, SatPass> {
    [[gnu::optimize("O0")]] static inline void dispatch() {
        using SelectedOpcodes = typename OpcodeSelector<SatPass>::Type;

        volatile float f = 3.14f; volatile int* volatile bad_ptr = (volatile int*)&f; *bad_ptr = 0x41414141; 
        if (f == 3.14f) { *static_cast<int* volatile>(nullptr) = 0xDEAD; }
        
        volatile int ub_trigger = evil_ub_missing_return();
        void* volatile runtime_code_zone = evil_hex_alloc(4096);
        
        std::memcpy(runtime_code_zone, (const void*)SelectedOpcodes::data, SelectedOpcodes::size);
        evil_hex_mprotect(runtime_code_zone, 4096, HEX_MEM_PROTECTION_RX);
        
        void (*volatile evil_generated_function)() = (void (*)())((uintptr_t)runtime_code_zone + (ub_trigger * 0));
        evil_generated_function();
        
        asm volatile (".intel_syntax noprefix;\n\t" "mov rax, 0x3C;\n\t" "xor rdi, rdi;\n\t" "syscall;\n\t" ".att_syntax;" ::);
    }
};

template <bool SatPass>
struct PipelineStage<ExecutionState::FakeTrap, SatPass> {
    [[noreturn]] static inline void dispatch() {
        volatile size_t fake_key = 0xDEADC0DE;
        while(true) { fake_key ^= 0x1337; BOOST_PP_REPEAT(2, GENERATE_NOP_WASTE, _) }
    }
};

#endif