/**
 * dialScript VM Core
 * 
 * Stack-based virtual machine with cooperative multitasking
 */

#ifndef DIALOS_VM_CORE_H
#define DIALOS_VM_CORE_H

#include "vm_value.h"
#include "platform.h"
#include "bytecode.h"
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <cstring>

namespace dialos {
namespace vm {

// Call frame for function calls
struct CallFrame {
    size_t returnPC;                      // Return program counter
    std::map<uint8_t, Value> locals;      // Local variables
    size_t stackBase;                     // Base of stack for this frame
    std::string functionName;             // For debugging
};

// Exception handler
struct ExceptionHandler {
    size_t catchPC;                       // PC to jump to on exception
    size_t stackSize;                     // Stack size when handler was set
};

// VM execution result
enum class VMResult {
    OK,              // Normal execution
    YIELD,           // Cooperative yield (sleep/pause)
    FINISHED,        // Program completed
    ERROR,           // Runtime error
    OUT_OF_MEMORY    // Heap exhausted
};

// VM execution state
class VMState {
public:
    VMState(const compiler::BytecodeModule& module, ValuePool& pool, PlatformInterface& platform);
    
    // Execute instructions (returns after maxInstructions or yield)
    VMResult execute(uint32_t maxInstructions = 1000);
    
    // Stack operations
    void push(const Value& value) { stack_.push_back(value); }
    Value pop();
    Value peek(size_t offset = 0) const;
    
    // Get state info
    size_t getPC() const { return pc_; }
    size_t getStackSize() const { return stack_.size(); }
    const std::vector<CallFrame>& getCallStack() const { return callStack_; }
    size_t getCallStackDepth() const { return callStack_.size(); }
    const std::map<std::string, Value>& getGlobals() const { return globals_; }
    std::string getError() const { return error_; }
    bool isRunning() const { return running_; }
    bool hasError() const { return !error_.empty(); }
    size_t getHeapUsage() const { return pool_.getAllocated(); }
    // Available heap bytes in the VM ValuePool
    size_t getHeapAvailable() const { return pool_.getAvailable(); }
    // Total heap size configured for this VM
    size_t getHeapSize() const { return pool_.getHeapSize(); }
    
    // Sleep state
    bool isSleeping() const { return sleeping_; }
    void checkSleepState();  // Check if sleep period has ended
    
    // Garbage collection
    void garbageCollect();   // Trigger mark-and-sweep garbage collection
    
    // Reset VM
    void reset();
    
    // Callback invocation
    // Invoke a function value with given arguments (for immediate callback execution)
    bool invokeFunction(const Value& callback, const std::vector<Value>& args);
    
    // Single-step execution (for callback invocation)
    VMResult step() { return executeInstruction(); }
    
private:
    // Bytecode module
    const compiler::BytecodeModule& module_;
    
    // Memory
    ValuePool& pool_;
    
    // Platform interface
    PlatformInterface& platform_;
    
    // Execution state
    std::vector<uint8_t> code_;
    std::vector<Value> stack_;
    std::vector<CallFrame> callStack_;
    std::map<std::string, Value> globals_;
    std::vector<ExceptionHandler> exceptionHandlers_;
    
    size_t pc_;
    bool running_;
    std::string error_;
    
    // Sleep state tracking
    bool sleeping_;
    uint64_t sleepUntil_;  // Timestamp when sleep ends
    
    // Instruction execution
    VMResult executeInstruction();
    
    // Helper methods
    Value loadConstant(uint16_t index);
    Value loadGlobal(uint16_t index);
    void storeGlobal(uint16_t index, const Value& value);
    void setError(const std::string& msg);
    
    // Template formatting
    std::string formatTemplate(const std::string& template_str, const std::vector<Value>& args);
    
    // Arithmetic operations
    Value add(const Value& a, const Value& b);
    Value subtract(const Value& a, const Value& b);
    Value multiply(const Value& a, const Value& b);
    Value divide(const Value& a, const Value& b);
    Value modulo(const Value& a, const Value& b);
    Value negate(const Value& v);
    
    // Comparison operations
    Value compare_eq(const Value& a, const Value& b);
    Value compare_ne(const Value& a, const Value& b);
    Value compare_lt(const Value& a, const Value& b);
    Value compare_le(const Value& a, const Value& b);
    Value compare_gt(const Value& a, const Value& b);
    Value compare_ge(const Value& a, const Value& b);
    
    // Logical operations
    Value logical_not(const Value& v);
    Value logical_and(const Value& a, const Value& b);
    Value logical_or(const Value& a, const Value& b);
    
    // Read operands from bytecode
    uint8_t readU8() { return code_[pc_++]; }
    uint16_t readU16() {
        uint16_t value = code_[pc_] | (code_[pc_ + 1] << 8);
        pc_ += 2;
        return value;
    }
    int32_t readI32() {
        int32_t value;
        std::memcpy(&value, &code_[pc_], 4);
        pc_ += 4;
        return value;
    }
    float readF32() {
        float value;
        std::memcpy(&value, &code_[pc_], 4);
        pc_ += 4;
        return value;
    }
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_VM_CORE_H
