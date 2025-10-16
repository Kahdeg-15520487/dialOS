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
    LOAD_FUNCTION = 0x83,  // Push function reference (function index u16)
    CALL_INDIRECT = 0x84,  // Call function from stack (arg count u8)
    CALL_METHOD = 0x85,  // Call method with implicit receiver (u8 argCount, u16 methodNameIdx?)
    
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
    // Metadata
    struct Metadata {
        uint16_t version;           // Bytecode format version
        uint32_t heapSize;          // Required heap size in bytes
        std::string appName;        // Application name
        std::string appVersion;     // Application version
        std::string author;         // Author name
        uint32_t timestamp;         // Compilation timestamp
        uint32_t hashCode;          // Simple hash of metadata content
        uint16_t checksum;          // Error correcting checksum
        
        Metadata() : version(1), heapSize(8192), appName("untitled"), 
                     appVersion("1.0.0"), author(""), timestamp(0), 
                     hashCode(0), checksum(0) {}
        
        // Calculate hash code for metadata integrity (includes checksum)
        uint32_t calculateHash() const {
            uint32_t hash = 0x811C9DC5; // FNV-1a offset basis
            const uint32_t prime = 0x01000193; // FNV-1a prime
            
            // Hash version and heap size
            hash ^= version;
            hash *= prime;
            hash ^= heapSize;
            hash *= prime;
            hash ^= timestamp;
            hash *= prime;
            
            // Hash the bytecode checksum (this links metadata to bytecode integrity)
            hash ^= checksum;
            hash *= prime;
            
            // Hash strings
            for (char c : appName) {
                hash ^= static_cast<uint8_t>(c);
                hash *= prime;
            }
            for (char c : appVersion) {
                hash ^= static_cast<uint8_t>(c);
                hash *= prime;
            }
            for (char c : author) {
                hash ^= static_cast<uint8_t>(c);
                hash *= prime;
            }
            
            return hash;
        }

    };
    
    Metadata metadata;
    std::vector<uint8_t> code;           // Bytecode instructions
    std::vector<uint32_t> debugLines;    // Source line number for each bytecode byte (optional)
    std::vector<std::string> constants;  // String constant pool
    std::vector<std::string> globals;    // Global variable names
    std::vector<std::string> functions;  // Function names
    std::vector<uint32_t> functionEntryPoints;  // PC for each function
    std::vector<uint8_t> functionParamCounts;   // Parameter count for each function
    uint32_t mainEntryPoint;             // Entry point for main code
    
    BytecodeModule() : mainEntryPoint(0) {}
    
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
    uint16_t addFunction(const std::string& name, uint8_t paramCount = 0) {
        for (size_t i = 0; i < functions.size(); i++) {
            if (functions[i] == name) {
                return static_cast<uint16_t>(i);
            }
        }
        functions.push_back(name);
        functionEntryPoints.push_back(0); // Will be set later
        functionParamCounts.push_back(paramCount); // Store param count
        return static_cast<uint16_t>(functions.size() - 1);
    }
    
    // Set function entry point
    void setFunctionEntryPoint(uint16_t funcIndex, uint32_t pc) {
        if (funcIndex < functionEntryPoints.size()) {
            functionEntryPoints[funcIndex] = pc;
        }
    }
    
    // Emit instruction (with optional line number for debugging)
    void emit(const Instruction& instr, uint32_t lineNumber) {
        size_t startPos = code.size();
        
        code.push_back(static_cast<uint8_t>(instr.opcode));
        code.insert(code.end(), instr.operands.begin(), instr.operands.end());
        
        // Add line number mapping for each bytecode byte if debug info is enabled
        // Only add debug info if debugLines has been initialized via enableDebugInfo()
        if (!debugLines.empty()) {
            // If debugLines has a dummy element at position 0 (from enableDebugInfo when code was empty),
            // and this is the first real code, remove it
            if (debugLines.size() == 1 && startPos == 0 && code.size() > 0) {
                debugLines.clear();
            }
            
            // Ensure debugLines is the same size as code before this instruction
            while (debugLines.size() < startPos) {
                debugLines.push_back(0); // Fill gaps with line 0 (unknown)
            }
            
            // Map all bytes of this instruction to the source line
            size_t instructionSize = code.size() - startPos;
            for (size_t i = 0; i < instructionSize; i++) {
                debugLines.push_back(lineNumber);
            }
        }
    }
    
    // Get current code position (for jumps)
    size_t getCurrentPosition() const {
        return code.size();
    }
    
    // Enable debug line tracking
    void enableDebugInfo() {
        // Mark debug info as enabled by ensuring debugLines is not empty
        // Even if no code exists yet, we need to mark it so emit() knows to add lines
        debugLines.resize(code.size() > 0 ? code.size() : 1, 0);
    }
    
    // Disable debug line tracking (to save memory)
    void disableDebugInfo() {
        debugLines.clear();
        debugLines.shrink_to_fit();
    }
    
    // Check if debug info is enabled
    bool hasDebugInfo() const {
        return !debugLines.empty();
    }
    
    // Get source line for a specific bytecode position
    uint32_t getSourceLine(size_t pc) const {
        if (pc < debugLines.size()) {
            return debugLines[pc];
        }
        return 0; // Unknown line
    }
    
    // Get debug info for current instruction (finds start of instruction)
    struct DebugInfo {
        uint32_t lineNumber;
        size_t instructionStart;
        Opcode opcode;
    };
    
    DebugInfo getDebugInfo(size_t pc) const {
        DebugInfo info = {0, pc, Opcode::NOP};
        
        if (pc >= code.size()) {
            return info;
        }
        
        // Find the start of the instruction containing this PC
        size_t instrStart = pc;
        while (instrStart > 0) {
            // Check if this looks like an opcode (simple heuristic)
            uint8_t byte = code[instrStart];
            if (byte <= static_cast<uint8_t>(Opcode::HALT)) {
                break;
            }
            instrStart--;
        }
        
        info.instructionStart = instrStart;
        info.opcode = static_cast<Opcode>(code[instrStart]);
        
        if (hasDebugInfo() && instrStart < debugLines.size()) {
            info.lineNumber = debugLines[instrStart];
        }
        
        return info;
    }
    
    // Patch jump offset at position
    void patchJump(size_t position, int32_t offset) {
        code[position] = offset & 0xFF;
        code[position + 1] = (offset >> 8) & 0xFF;
        code[position + 2] = (offset >> 16) & 0xFF;
        code[position + 3] = (offset >> 24) & 0xFF;
    }
    
    // Calculate checksum of bytecode content
    uint16_t calculateBytecodeChecksum() const {
        uint16_t sum = 0;
        
        // Checksum all bytecode bytes
        for (uint8_t byte : code) {
            sum += byte;
        }
        
        // Optionally include debug info in checksum for integrity
        if (hasDebugInfo()) {
            for (uint32_t line : debugLines) {
                sum += (line & 0xFF);
                sum += ((line >> 8) & 0xFF);
                sum += ((line >> 16) & 0xFF);
                sum += ((line >> 24) & 0xFF);
            }
        }
        
        return sum;
    }
    
    // Update integrity: first calculate bytecode checksum, then metadata hash
    void updateIntegrity() {
        // Step 1: Calculate checksum from bytecode content
        metadata.checksum = calculateBytecodeChecksum();
        
        // Step 2: Calculate hash from metadata (including the checksum)
        metadata.hashCode = metadata.calculateHash();
    }
    
    // Verify integrity of both bytecode and metadata
    bool verifyIntegrity() const {
        // Verify bytecode checksum
        if (metadata.checksum != calculateBytecodeChecksum()) {
            return false;
        }
        
        // Verify metadata hash (which includes the checksum)
        if (metadata.hashCode != metadata.calculateHash()) {
            return false;
        }
        
        return true;
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
//         Flags (2 bytes) - bit 0: has debug info
// Constants section: count (4 bytes), [length (2 bytes), string data]...
// Globals section: count (4 bytes), [length (2 bytes), name]...
// Functions section: count (4 bytes), [length (2 bytes), name]...
// Code section: length (4 bytes), bytecode...
// Debug section (optional): length (4 bytes), [line number (4 bytes)]... (one per bytecode byte)

} // namespace compiler
} // namespace dialos

#endif // DIALOS_COMPILER_BYTECODE_H
