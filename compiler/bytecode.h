/**
 * dialScript Bytecode Instruction Set
 * 
 * Virtual Machine Design:
 * - Stack-based VM with local variables
 * - 32-bit values (int, float, pointers)
 * - Call stack for function frames
 * - Heap for objects and arrays
 */

#ifndef DIALOS_COMPILER_BYTECODE_H
#define DIALOS_COMPILER_BYTECODE_H

#include <cstdint>
#include <vector>
#include <string>

namespace dialos {
namespace compiler {

// Bytecode instruction opcodes
enum class Opcode : uint8_t {
    // Stack operations
    NOP         = 0x00,  // No operation
    POP         = 0x01,  // Pop top of stack
    DUP         = 0x02,  // Duplicate top of stack
    SWAP        = 0x03,  // Swap top two stack items
    
    // Constants
    PUSH_NULL   = 0x10,  // Push null
    PUSH_TRUE   = 0x11,  // Push boolean true
    PUSH_FALSE  = 0x12,  // Push boolean false
    PUSH_I8     = 0x13,  // Push 8-bit signed integer (next byte)
    PUSH_I16    = 0x14,  // Push 16-bit signed integer (next 2 bytes)
    PUSH_I32    = 0x15,  // Push 32-bit signed integer (next 4 bytes)
    PUSH_F32    = 0x16,  // Push 32-bit float (next 4 bytes)
    PUSH_STR    = 0x17,  // Push string constant (index in constant pool)
    
    // Local variables
    LOAD_LOCAL  = 0x20,  // Load local variable (index)
    STORE_LOCAL = 0x21,  // Store to local variable (index)
    
    // Global variables
    LOAD_GLOBAL = 0x30,  // Load global variable (name index)
    STORE_GLOBAL= 0x31,  // Store to global variable (name index)
    
    // Arithmetic operations
    ADD         = 0x40,  // Add two values
    SUB         = 0x41,  // Subtract
    MUL         = 0x42,  // Multiply
    DIV         = 0x43,  // Divide
    MOD         = 0x44,  // Modulo
    NEG         = 0x45,  // Negate (unary minus)
    
    // String operations
    STR_CONCAT  = 0x46,  // String concatenation
    
    // Comparison operations
    EQ          = 0x50,  // Equal (=)
    NE          = 0x51,  // Not equal (!=)
    LT          = 0x52,  // Less than (<)
    LE          = 0x53,  // Less or equal (<=)
    GT          = 0x54,  // Greater than (>)
    GE          = 0x55,  // Greater or equal (>=)
    
    // Logical operations
    NOT         = 0x60,  // Logical NOT
    AND         = 0x61,  // Logical AND
    OR          = 0x62,  // Logical OR
    
    // Control flow
    JUMP        = 0x70,  // Unconditional jump (offset)
    JUMP_IF     = 0x71,  // Jump if true (offset)
    JUMP_IF_NOT = 0x72,  // Jump if false (offset)
    
    // Function calls
    CALL        = 0x80,  // Call function (function index, arg count)
    CALL_NATIVE = 0x81,  // Call native function (native index, arg count)
    RETURN      = 0x82,  // Return from function
    
    // Object/Member access
    GET_FIELD   = 0x90,  // Get object field (field name index)
    SET_FIELD   = 0x91,  // Set object field (field name index)
    GET_INDEX   = 0x92,  // Get array element
    SET_INDEX   = 0x93,  // Set array element
    
    // Object creation
    NEW_OBJECT  = 0xA0,  // Create new object (class index)
    NEW_ARRAY   = 0xA1,  // Create new array (size on stack)
    
    // Exception handling
    TRY         = 0xB0,  // Set exception handler (catch offset)
    END_TRY     = 0xB1,  // Remove exception handler
    THROW       = 0xB2,  // Throw exception (value on stack)
    
    // Special
    PRINT       = 0xF0,  // Debug print (temporary)
    HALT        = 0xFF,  // Halt execution
};

// Bytecode instruction
struct Instruction {
    Opcode opcode;
    std::vector<uint8_t> operands;
    
    Instruction(Opcode op) : opcode(op) {}
    
    template<typename T>
    void addOperand(T value) {
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
        for (size_t i = 0; i < sizeof(T); i++) {
            operands.push_back(bytes[i]);
        }
    }
    
    void addOperandU8(uint8_t value) {
        operands.push_back(value);
    }
    
    void addOperandU16(uint16_t value) {
        operands.push_back(value & 0xFF);
        operands.push_back((value >> 8) & 0xFF);
    }
    
    void addOperandU32(uint32_t value) {
        operands.push_back(value & 0xFF);
        operands.push_back((value >> 8) & 0xFF);
        operands.push_back((value >> 16) & 0xFF);
        operands.push_back((value >> 24) & 0xFF);
    }
};

// Bytecode module (compilation unit)
class BytecodeModule {
public:
    std::vector<uint8_t> code;           // Bytecode instructions
    std::vector<std::string> constants;  // String constant pool
    std::vector<std::string> globals;    // Global variable names
    std::vector<std::string> functions;  // Function names
    
    // Add string to constant pool, return index
    uint16_t addConstant(const std::string& str) {
        // Check if already exists
        for (size_t i = 0; i < constants.size(); i++) {
            if (constants[i] == str) {
                return static_cast<uint16_t>(i);
            }
        }
        constants.push_back(str);
        return static_cast<uint16_t>(constants.size() - 1);
    }
    
    // Add global variable name, return index
    uint16_t addGlobal(const std::string& name) {
        for (size_t i = 0; i < globals.size(); i++) {
            if (globals[i] == name) {
                return static_cast<uint16_t>(i);
            }
        }
        globals.push_back(name);
        return static_cast<uint16_t>(globals.size() - 1);
    }
    
    // Add function name, return index
    uint16_t addFunction(const std::string& name) {
        for (size_t i = 0; i < functions.size(); i++) {
            if (functions[i] == name) {
                return static_cast<uint16_t>(i);
            }
        }
        functions.push_back(name);
        return static_cast<uint16_t>(functions.size() - 1);
    }
    
    // Emit instruction
    void emit(const Instruction& instr) {
        code.push_back(static_cast<uint8_t>(instr.opcode));
        code.insert(code.end(), instr.operands.begin(), instr.operands.end());
    }
    
    // Get current code position (for jumps)
    size_t getCurrentPosition() const {
        return code.size();
    }
    
    // Patch jump offset at position
    void patchJump(size_t position, int32_t offset) {
        code[position] = offset & 0xFF;
        code[position + 1] = (offset >> 8) & 0xFF;
        code[position + 2] = (offset >> 16) & 0xFF;
        code[position + 3] = (offset >> 24) & 0xFF;
    }
    
    // Serialize to binary format
    std::vector<uint8_t> serialize() const;
    
    // Deserialize from binary format
    static BytecodeModule deserialize(const std::vector<uint8_t>& data);
    
    // Disassemble for debugging
    std::string disassemble() const;
};

// Bytecode file format (.dsb)
// Header: "DSBC" (4 bytes magic)
//         Version (2 bytes)
//         Flags (2 bytes)
// Constants section: count (4 bytes), [length (2 bytes), string data]...
// Globals section: count (4 bytes), [length (2 bytes), name]...
// Functions section: count (4 bytes), [length (2 bytes), name]...
// Code section: length (4 bytes), bytecode...

} // namespace compiler
} // namespace dialos

#endif // DIALOS_COMPILER_BYTECODE_H
