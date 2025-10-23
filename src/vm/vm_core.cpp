/**
 * dialScript VM Core Implementation
 * 
 * Stack-based virtual machine with cooperative multitasking
 */

#include "../../include/vm/vm_core.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>

namespace dialos {
namespace vm {

VMState::VMState(const compiler::BytecodeModule& module, ValuePool& pool, PlatformInterface& platform)
    : module_(module), pool_(pool), platform_(platform), pc_(module.mainEntryPoint), running_(false), 
      sleeping_(false), sleepUntil_(0) {
    
    // Set VM reference in platform for callback invocation
    platform_.setVM(this);
    
    // Copy code from module
    code_ = module_.code;
    
    // Initialize globals with null
    for (const auto& name : module_.globals) {
        globals_[name] = Value::Null();
    }
    
    // Initialize built-in 'os' object if present
    auto osIt = globals_.find("os");
    if (osIt != globals_.end()) {
        vm::Object* osObj = pool_.allocateObject("OS");
        if (osObj) {
            // Add console object
            vm::Object* consoleObj = pool_.allocateObject("Console");
            if (consoleObj) {
                osObj->fields["console"] = Value::Object(consoleObj);
            }
            
            // Add system object
            vm::Object* systemObj = pool_.allocateObject("System");
            if (systemObj) {
                osObj->fields["system"] = Value::Object(systemObj);
            }
            
            // Add display object
            vm::Object* displayObj = pool_.allocateObject("Display");
            if (displayObj) {
                osObj->fields["display"] = Value::Object(displayObj);
            }
            
            // Add encoder object
            vm::Object* encoderObj = pool_.allocateObject("Encoder");
            if (encoderObj) {
                osObj->fields["encoder"] = Value::Object(encoderObj);
            }
            
            // Add touch object
            vm::Object* touchObj = pool_.allocateObject("Touch");
            if (touchObj) {
                osObj->fields["touch"] = Value::Object(touchObj);
            }
            
            // Add app object
            vm::Object* appObj = pool_.allocateObject("App");
            if (appObj) {
                osObj->fields["app"] = Value::Object(appObj);
            }
            
            // Add rfid object
            vm::Object* rfidObj = pool_.allocateObject("RFID");
            if (rfidObj) {
                osObj->fields["rfid"] = Value::Object(rfidObj);
            }
            
            // Add file object
            vm::Object* fileObj = pool_.allocateObject("File");
            if (fileObj) {
                osObj->fields["file"] = Value::Object(fileObj);
            }
            
            // Add gpio object
            vm::Object* gpioObj = pool_.allocateObject("GPIO");
            if (gpioObj) {
                osObj->fields["gpio"] = Value::Object(gpioObj);
            }
            
            // Add i2c object
            vm::Object* i2cObj = pool_.allocateObject("I2C");
            if (i2cObj) {
                osObj->fields["i2c"] = Value::Object(i2cObj);
            }
            
            // Add buzzer object
            vm::Object* buzzerObj = pool_.allocateObject("Buzzer");
            if (buzzerObj) {
                osObj->fields["buzzer"] = Value::Object(buzzerObj);
            }
            
            globals_["os"] = Value::Object(osObj);
        }
    }
}

void VMState::reset() {
    pc_ = module_.mainEntryPoint;
    running_ = true;
    sleeping_ = false;
    sleepUntil_ = 0;
    stack_.clear();
    callStack_.clear();
    exceptionHandlers_.clear();
    error_.clear();
    
    // Reset globals except "os" which contains the platform interface
    for (auto& pair : globals_) {
        if (pair.first != "os") {
            pair.second = Value::Null();
        }
    }
}

void VMState::checkSleepState() {
    if (sleeping_) {
        uint64_t currentTime = platform_.system_getTime();
        if (currentTime >= sleepUntil_) {
            sleeping_ = false;
        }
    }
}

bool VMState::invokeFunction(const Value& callback, const std::vector<Value>& args) {
    // Validate callback is a function
    if (!callback.isFunction()) {
        return false;
    }
    
    const Function* func = callback.asFunction();
    if (func == nullptr) {
        return false;
    }
    
    // Validate argument count
    if (args.size() != func->paramCount) {
        // Set detailed error message for parameter count mismatch
        std::string functionName = (func->functionIndex < module_.functions.size()) ? 
                                 module_.functions[func->functionIndex] : "<callback>";
        error_ = "Parameter count mismatch: function '" + functionName + 
                "' expects " + std::to_string(func->paramCount) + 
                " parameter(s), but " + std::to_string(args.size()) + " provided";
        return false;
    }
    
    // Get function entry point
    uint32_t functionIndex = func->functionIndex;
    if (functionIndex >= module_.functionEntryPoints.size()) {
        return false;
    }
    
    uint32_t entryPC = module_.functionEntryPoints[functionIndex];
    
    // Save current state
    uint32_t savedPC = static_cast<uint32_t>(pc_);
    size_t stackSizeBefore = stack_.size();
    
    // Create call frame
    CallFrame frame;
    frame.returnPC = savedPC;
    frame.stackBase = stack_.size();
    frame.functionName = (functionIndex < module_.functions.size()) ? 
                         module_.functions[functionIndex] : "<callback>";
    
    // Copy arguments to call frame locals (indexed by parameter number)
    for (uint8_t i = 0; i < args.size(); i++) {
        frame.locals[i] = args[i];
    }
    
    // Push call frame
    callStack_.push_back(frame);
    
    // Jump to function
    pc_ = entryPC;
    
    // Execute the function until it returns
    // Track call stack depth to know when callback completes
    size_t callDepthBefore = callStack_.size();
    
    // Temporarily enable running state for callback execution
    // (callbacks can run even if main script has finished)
    bool wasRunning = running_;
    running_ = true;
    
    while (callStack_.size() >= callDepthBefore && !hasError() && running_) {
        VMResult result = step();
        if (result == VMResult::ERROR) {
            platform_.console_log("[VM] ERROR in callback execution!");
            if (!error_.empty()) {
                platform_.console_log("[VM] Error: " + error_);
            }
            // Determine if this is a fatal error
            bool isFatalError = error_.find("Out of memory") != std::string::npos;

            // Unwind any frames pushed for the callback so VM state is consistent
            while (callStack_.size() >= callDepthBefore) {
                callStack_.pop_back();
            }

            // Restore PC and stack to the state before the callback
            pc_ = savedPC;
            while (stack_.size() > stackSizeBefore) pop();

                // Treat any error during a callback as fatal. Do not clear the error.
                platform_.console_log("[VM] ERROR during callback - halting VM");
                // Ensure VM is stopped so host/emulator can inspect state and report diagnostics
                running_ = false;
                return false;
        }
        if (result == VMResult::FINISHED) {
            break;
        }
    }
    
    // Check if callback execution left an error
    bool hadError = hasError();
    if (hadError) {
        platform_.console_log("[VM] Callback left error state!");
        platform_.console_log("[VM] Error: " + error_);
        
        // Treat any error during a callback as fatal. Do not clear the error.
        platform_.console_log("[VM] ERROR during callback - halting VM");
        running_ = false;  // Stop the VM completely
        return false;
    }
    
    // Restore original running state only if no error occurred
    running_ = wasRunning;
    
    // Restore PC to where we were before callback
    pc_ = savedPC;
    
    // Restore stack to exact state before callback
    // The callback's RETURN cleans up to its stackBase and pushes return value
    // We need to remove everything the callback added (including return value)
    while (stack_.size() > stackSizeBefore) {
        pop();
    }
    
    size_t stackSizeAfter = stack_.size();
    
    if (stackSizeAfter != stackSizeBefore) {
        platform_.console_warn("[VM] WARNING: Stack imbalance! Delta: " + 
                            std::to_string((int)stackSizeAfter - (int)stackSizeBefore));
    }
    
    // Clean up unreferenced strings after callback completes
    pool_.garbageCollectStrings();
    
    return !hasError();
}

Value VMState::pop() {
    if (stack_.empty()) {
        setError("Stack underflow");
        return Value::Null();
    }
    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value VMState::peek(size_t offset) const {
    if (offset >= stack_.size()) {
        return Value::Null();
    }
    return stack_[stack_.size() - 1 - offset];
}

void VMState::setError(const std::string& msg) {
    error_ = msg;
    // If stack underflow, ask platform to dump VM state (platform-level I/O requested)
    if (msg == "Stack underflow") {
        try {
            platform_.dumpVMState(*this, pc_, msg);
        } catch (...) {
            platform_.console_log("[VM] Failed to run platform dumpVMState()");
        }
    }
    // mark VM as not running after recording the error
    running_ = false;
}

Value VMState::loadConstant(uint16_t index) {
    if (index >= module_.constants.size()) {
        setError("Invalid constant index");
        return Value::Null();
    }
    
    // Allocate string in heap
    std::string* str = pool_.allocateString(module_.constants[index]);
    if (!str) {
        setError("Out of memory allocating string constant");
        return Value::Null();
    }
    
    return Value::StringFromPool(str);
}

Value VMState::loadGlobal(uint16_t index) {
    if (index >= module_.globals.size()) {
        setError("Invalid global index");
        return Value::Null();
    }
    
    const std::string& name = module_.globals[index];
    auto it = globals_.find(name);
    if (it != globals_.end()) {
        Value v = it->second;
        if (v.isNull()) {
            platform_.console_log(std::string("loadGlobal: global '") + name + " is null");
        }
        return v;
    }
    
    platform_.console_log(std::string("loadGlobal: global '") + name + " not found, returning null");
    return Value::Null();
}

void VMState::storeGlobal(uint16_t index, const Value& value) {
    if (index >= module_.globals.size()) {
        setError("Invalid global index");
        return;
    }
    
    const std::string& name = module_.globals[index];
    globals_[name] = value;
    // Logging removed to reduce verbosity during normal operation
}

VMResult VMState::execute(uint32_t maxInstructions) {
    if (!running_) {
        return VMResult::ERROR;
    }
    
    // Check if we're sleeping
    checkSleepState();
    if (sleeping_) {
        return VMResult::YIELD;
    }
    
    uint32_t executed = 0;
    
    while (running_ && executed < maxInstructions && pc_ < code_.size()) {
        VMResult result = executeInstruction();
        
        if (result != VMResult::OK) {
            return result;
        }
        
        // Check sleep state after each instruction
        checkSleepState();
        if (sleeping_) {
            return VMResult::YIELD;
        }
        
        executed++;
    }
    
    if (pc_ >= code_.size()) {
        running_ = false;
        return VMResult::FINISHED;
    }
    
    return VMResult::OK;
}

VMResult VMState::executeInstruction() {
    if (pc_ >= code_.size()) {
        running_ = false;
        return VMResult::FINISHED;
    }
    
    compiler::Opcode op = static_cast<compiler::Opcode>(code_[pc_++]);
    
    switch (op) {
        // ===== Stack Operations =====
        case compiler::Opcode::NOP:
            // No operation
            break;
            
        case compiler::Opcode::POP:
            pop();
            break;
            
        case compiler::Opcode::DUP:
            if (!stack_.empty()) {
                push(stack_.back());
            }
            break;
            
        case compiler::Opcode::SWAP:
            if (stack_.size() >= 2) {
                Value a = pop();
                Value b = pop();
                push(a);
                push(b);
            }
            break;
            
        // ===== Constants =====
        case compiler::Opcode::PUSH_NULL:
            push(Value::Null());
            break;
            
        case compiler::Opcode::PUSH_TRUE:
            push(Value::Bool(true));
            break;
            
        case compiler::Opcode::PUSH_FALSE:
            push(Value::Bool(false));
            break;
            
        case compiler::Opcode::PUSH_I8: {
            int8_t value = static_cast<int8_t>(readU8());
            push(Value::Int32(value));
            break;
        }
        
        case compiler::Opcode::PUSH_I16: {
            int16_t value = static_cast<int16_t>(readU16());
            push(Value::Int32(value));
            break;
        }
        
        case compiler::Opcode::PUSH_I32: {
            int32_t value = readI32();
            push(Value::Int32(value));
            break;
        }
        
        case compiler::Opcode::PUSH_F32: {
            float value = readF32();
            push(Value::Float32(value));
            break;
        }
        
        case compiler::Opcode::PUSH_STR: {
            uint16_t index = readU16();
            push(loadConstant(index));
            break;
        }
        
        // ===== Local Variables =====
        case compiler::Opcode::LOAD_LOCAL: {
            uint8_t index = readU8();
            if (callStack_.empty()) {
                setError("No active call frame");
                return VMResult::ERROR;
            }
            
            auto& frame = callStack_.back();
            auto it = frame.locals.find(index);
            if (it != frame.locals.end()) {
                push(it->second);
            } else {
                push(Value::Null());
            }
            break;
        }
        
        case compiler::Opcode::STORE_LOCAL: {
            uint8_t index = readU8();
            Value value = pop();
            
            if (callStack_.empty()) {
                setError("No active call frame");
                return VMResult::ERROR;
            }
            
            callStack_.back().locals[index] = value;
            break;
        }
        
        // ===== Global Variables =====
        case compiler::Opcode::LOAD_GLOBAL: {
            uint16_t index = readU16();
            push(loadGlobal(index));
            break;
        }
        
        case compiler::Opcode::STORE_GLOBAL: {
            uint16_t index = readU16();
            Value value = pop();
            storeGlobal(index, value);
            break;
        }
        
        // ===== Arithmetic Operations =====
        case compiler::Opcode::ADD: {
            Value b = pop();
            Value a = pop();
            push(add(a, b));
            break;
        }
        
        case compiler::Opcode::SUB: {
            Value b = pop();
            Value a = pop();
            push(subtract(a, b));
            break;
        }
        
        case compiler::Opcode::MUL: {
            Value b = pop();
            Value a = pop();
            push(multiply(a, b));
            break;
        }
        
        case compiler::Opcode::DIV: {
            Value b = pop();
            Value a = pop();
            Value result = divide(a, b);
            if (!running_) {
                pc_--; // Set PC back to the instruction that caused the error
                return VMResult::ERROR; // Check for division by zero error
            }
            push(result);
            break;
        }
        
        case compiler::Opcode::MOD: {
            Value b = pop();
            Value a = pop();
            push(modulo(a, b));
            break;
        }
        
        case compiler::Opcode::NEG: {
            Value v = pop();
            push(negate(v));
            break;
        }
        
        case compiler::Opcode::STR_CONCAT: {
            Value b = pop();
            Value a = pop();
            std::string result = a.toString() + b.toString();
            std::string* str = pool_.allocateString(result);
            if (!str) {
                // Try garbage collection and retry
                pool_.garbageCollectStrings();
                str = pool_.allocateString(result);
                if (!str) {
                    setError("Out of memory in string concatenation");
                    return VMResult::OUT_OF_MEMORY;
                }
            }
            push(Value::StringFromPool(str));
            break;
        }
        
        case compiler::Opcode::TEMPLATE_FORMAT: {
            uint8_t argCount = readU8();
            
            // Pop template string
            Value templateVal = pop();
            if (!templateVal.isString()) {
                setError("TEMPLATE_FORMAT: template must be string");
                return VMResult::ERROR;
            }
            
            // Pop arguments (in reverse order due to stack)
            std::vector<Value> args(argCount);
            for (int i = argCount - 1; i >= 0; i--) {
                args[i] = pop();
            }
            
            // Format template string with arguments
            std::string result = formatTemplate(templateVal.toString(), args);
            
            // Intern the result
            std::string* str = pool_.allocateString(result);
            if (!str) {
                // Try garbage collection and retry
                pool_.garbageCollectStrings();
                str = pool_.allocateString(result);
                if (!str) {
                    setError("Out of memory in template formatting");
                    return VMResult::OUT_OF_MEMORY;
                }
            }
            push(Value::StringFromPool(str));
            break;
        }
        
        // ===== Comparison Operations =====
        case compiler::Opcode::EQ: {
            Value b = pop();
            Value a = pop();
            push(compare_eq(a, b));
            break;
        }
        
        case compiler::Opcode::NE: {
            Value b = pop();
            Value a = pop();
            push(compare_ne(a, b));
            break;
        }
        
        case compiler::Opcode::LT: {
            Value b = pop();
            Value a = pop();
            push(compare_lt(a, b));
            break;
        }
        
        case compiler::Opcode::LE: {
            Value b = pop();
            Value a = pop();
            push(compare_le(a, b));
            break;
        }
        
        case compiler::Opcode::GT: {
            Value b = pop();
            Value a = pop();
            push(compare_gt(a, b));
            break;
        }
        
        case compiler::Opcode::GE: {
            Value b = pop();
            Value a = pop();
            push(compare_ge(a, b));
            break;
        }
        
        // ===== Logical Operations =====
        case compiler::Opcode::NOT: {
            Value v = pop();
            push(logical_not(v));
            break;
        }
        
        case compiler::Opcode::AND: {
            Value b = pop();
            Value a = pop();
            push(logical_and(a, b));
            break;
        }
        
        case compiler::Opcode::OR: {
            Value b = pop();
            Value a = pop();
            push(logical_or(a, b));
            break;
        }
        
        // ===== Control Flow =====
        case compiler::Opcode::JUMP: {
            int32_t offset = readI32();
            pc_ = static_cast<size_t>(static_cast<int32_t>(pc_) + offset);
            break;
        }
        
        case compiler::Opcode::JUMP_IF: {
            int32_t offset = readI32();
            Value condition = pop();
            if (condition.isTruthy()) {
                pc_ = static_cast<size_t>(static_cast<int32_t>(pc_) + offset);
            }
            break;
        }
        
        case compiler::Opcode::JUMP_IF_NOT: {
            int32_t offset = readI32();
            Value condition = pop();
            if (!condition.isTruthy()) {
                pc_ = static_cast<size_t>(static_cast<int32_t>(pc_) + offset);
            }
            break;
        }
        
        // ===== Function Calls =====
        case compiler::Opcode::CALL: {
            uint16_t funcIndex = readU16();
            uint8_t argCount = readU8();
            
            if (funcIndex >= module_.functions.size()) {
                setError("Invalid function index: " + std::to_string(funcIndex));
                return VMResult::ERROR;
            }
            
            // Get function entry point
            if (funcIndex >= module_.functionEntryPoints.size()) {
                setError("Function entry point not found for: " + module_.functions[funcIndex]);
                return VMResult::ERROR;
            }
            
            uint32_t entryPoint = module_.functionEntryPoints[funcIndex];
            if (entryPoint == 0 && funcIndex != 0) { // Allow PC 0 for first function
                setError("Function not defined: " + module_.functions[funcIndex]);
                return VMResult::ERROR;
            }
            
            // Create new call frame
            CallFrame frame;
            frame.returnPC = pc_;
            frame.stackBase = stack_.size() - argCount; // Arguments start here
            frame.functionName = module_.functions[funcIndex];
            
            // Store arguments in local variables (0, 1, 2, ...)
            // Arguments are on stack in order: arg0, arg1, arg2, ...
            for (uint8_t i = 0; i < argCount; i++) {
                if (frame.stackBase + i < stack_.size()) {
                    frame.locals[i] = stack_[frame.stackBase + i];
                }
            }
            
            // Remove arguments from stack but keep stack base for locals
            stack_.resize(frame.stackBase);
            
            // Push call frame and jump to function
            callStack_.push_back(frame);
            pc_ = entryPoint;
            
            break;
        }
        
        case compiler::Opcode::CALL_NATIVE: {
            uint16_t funcIndex = readU16();
            uint8_t argCount = readU8();
            
            if (funcIndex >= module_.functions.size()) {
                setError("Invalid native function index");
                return VMResult::ERROR;
            }
            
            // Stack layout: [..., receiver, arg1, arg2, ..., argN]
            // Arguments are on top, receiver is below them
            
            const std::string& funcName = module_.functions[funcIndex];
            
            // Get native function ID from name
            NativeFunctionID funcID = getNativeFunctionID(funcName);
            
            // Dispatch to native functions using switch (compiler can optimize to jump table)
            switch (funcID) {
                // ===== Console Functions =====
                case NativeFunctionID::CONSOLE_LOG: {
                    if (argCount < 1) {
                        setError("log() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    // Pop arguments in reverse order (last arg is on top)
                    Value arg = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop(); // Pop remaining args
                    platform_.console_log(arg.toString());
                    
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::CONSOLE_WARN: {
                    if (argCount < 1) {
                        setError("warn() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    Value arg = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    platform_.console_warn(arg.toString());
                    
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::CONSOLE_ERROR: {
                    if (argCount < 1) {
                        setError("error() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    Value arg = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    platform_.console_error(arg.toString());
                    
                    push(Value::Null());
                    break;
                }
                
                // ===== Display Functions =====
                case NativeFunctionID::DISPLAY_CLEAR: {
                    if (argCount < 1) {
                        setError("clear() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    Value colorVal = pop();
                    uint32_t color = colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0;
                    platform_.display_clear(color);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_TEXT: {
                    if (argCount < 5) {
                        setError("drawText() requires 5 arguments");
                        return VMResult::ERROR;
                    }
                    Value sizeVal = pop();
                    Value colorVal = pop();
                    Value textVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    
                    platform_.display_drawText(
                        xVal.isInt32() ? xVal.int32Val : 0,
                        yVal.isInt32() ? yVal.int32Val : 0,
                        textVal.toString(),
                        colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF,
                        sizeVal.isInt32() ? sizeVal.int32Val : 1
                    );
                    
                    for (uint8_t i = 5; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_RECT: {
                    if (argCount < 6) {
                        setError("drawRect() requires 6 arguments");
                        return VMResult::ERROR;
                    }
                    Value filledVal = pop();
                    Value colorVal = pop();
                    Value hVal = pop();
                    Value wVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    
                    platform_.display_drawRect(
                        xVal.isInt32() ? xVal.int32Val : 0,
                        yVal.isInt32() ? yVal.int32Val : 0,
                        wVal.isInt32() ? wVal.int32Val : 0,
                        hVal.isInt32() ? hVal.int32Val : 0,
                        colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF,
                        filledVal.isTruthy()
                    );
                    
                    for (uint8_t i = 6; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_CIRCLE: {
                    if (argCount < 5) {
                        setError("drawCircle() requires 5 arguments");
                        return VMResult::ERROR;
                    }
                    
                    // Pop arguments first (they're on top of stack)
                    Value filledVal = pop();
                    Value colorVal = pop();
                    Value rVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    // Then pop receiver (it's below the arguments)
                    
                    // Debug logging
                    int x = xVal.isInt32() ? xVal.int32Val : 0;
                    int y = yVal.isInt32() ? yVal.int32Val : 0;
                    int r = rVal.isInt32() ? rVal.int32Val : 0;
                    uint32_t color = colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF;
                    bool filled = filledVal.isTruthy();
                    
                    platform_.display_drawCircle(x, y, r, color, filled);
                    
                    for (uint8_t i = 5; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_LINE: {
                    if (argCount < 5) {
                        setError("drawLine() requires 5 arguments");
                        return VMResult::ERROR;
                    }
                    Value colorVal = pop();
                    Value y2Val = pop();
                    Value x2Val = pop();
                    Value y1Val = pop();
                    Value x1Val = pop();
                    
                    platform_.display_drawLine(
                        x1Val.isInt32() ? x1Val.int32Val : 0,
                        y1Val.isInt32() ? y1Val.int32Val : 0,
                        x2Val.isInt32() ? x2Val.int32Val : 0,
                        y2Val.isInt32() ? y2Val.int32Val : 0,
                        colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF
                    );
                    
                    for (uint8_t i = 5; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_PIXEL: {
                    if (argCount < 3) {
                        setError("drawPixel() requires 3 arguments");
                        return VMResult::ERROR;
                    }
                    Value colorVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    
                    platform_.display_drawPixel(
                        xVal.isInt32() ? xVal.int32Val : 0,
                        yVal.isInt32() ? yVal.int32Val : 0,
                        colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF
                    );
                    
                    for (uint8_t i = 3; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_SET_BRIGHTNESS: {
                    if (argCount < 1) {
                        setError("setBrightness() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value levelVal = pop();
                    
                    platform_.display_setBrightness(levelVal.isInt32() ? levelVal.int32Val : 128);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_GET_WIDTH: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.display_getWidth()));
                    break;
                }
                
                case NativeFunctionID::DISPLAY_GET_HEIGHT: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.display_getHeight()));
                    break;
                }
                
                // ===== Encoder Functions =====
                case NativeFunctionID::ENCODER_GET_BUTTON: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Bool(platform_.encoder_getButton()));
                    break;
                }
                
                case NativeFunctionID::ENCODER_GET_DELTA: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.encoder_getDelta()));
                    break;
                }
                
                case NativeFunctionID::ENCODER_GET_POSITION: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.encoder_getPosition()));
                    break;
                }
                
                case NativeFunctionID::ENCODER_RESET: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.encoder_reset();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::ENCODER_ON_TURN: {
                    if (argCount < 1) {
                        setError("onTurn() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onTurn() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("encoder.onTurn", callback);
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::ENCODER_ON_BUTTON: {
                    if (argCount < 1) {
                        setError("onButton() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onButton() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("encoder.onButton", callback);
                    push(Value::Null());
                    break;
                }
                
                // ===== System Functions =====
                case NativeFunctionID::SYSTEM_GET_TIME: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(static_cast<int32_t>(platform_.system_getTime())));
                    break;
                }
                
                case NativeFunctionID::SYSTEM_SLEEP: {
                    if (argCount < 1) {
                        setError("sleep() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value msVal = pop();
                    
                    uint32_t sleepMs = msVal.isInt32() ? static_cast<uint32_t>(msVal.int32Val) : 0;
                    if (sleepMs > 0) {
                        // Set sleep state - VM will yield until time is up
                        uint64_t currentTime = platform_.system_getTime();
                        sleepUntil_ = currentTime + sleepMs;
                        sleeping_ = true;
                    }
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::SYSTEM_GET_RTC: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(static_cast<int32_t>(platform_.system_getRTC())));
                    break;
                }
                
                case NativeFunctionID::SYSTEM_SET_RTC: {
                    if (argCount < 1) {
                        setError("setRTC() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value timestampVal = pop();
                    
                    platform_.system_setRTC(timestampVal.isInt32() ? static_cast<uint32_t>(timestampVal.int32Val) : 0);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== Touch Functions =====
                case NativeFunctionID::TOUCH_GET_X: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.touch_getX()));
                    break;
                }
                
                case NativeFunctionID::TOUCH_GET_Y: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.touch_getY()));
                    break;
                }
                
                case NativeFunctionID::TOUCH_IS_PRESSED: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Bool(platform_.touch_isPressed()));
                    break;
                }
                
                // ===== RFID Functions =====
                case NativeFunctionID::RFID_READ: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::String(platform_.rfid_read()));
                    break;
                }
                
                case NativeFunctionID::RFID_IS_PRESENT: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Bool(platform_.rfid_isPresent()));
                    break;
                }
                
                // ===== File Functions =====
                case NativeFunctionID::FILE_OPEN: {
                    if (argCount < 2) {
                        setError("open() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value modeVal = pop();
                    Value pathVal = pop();
                    
                    int handle = platform_.file_open(pathVal.toString(), modeVal.toString());
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Int32(handle));
                    break;
                }
                
                case NativeFunctionID::FILE_READ: {
                    if (argCount < 2) {
                        setError("read() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value sizeVal = pop();
                    Value handleVal = pop();
                    
                    std::string data = platform_.file_read(
                        handleVal.isInt32() ? handleVal.int32Val : -1,
                        sizeVal.isInt32() ? sizeVal.int32Val : 0
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::String(data));
                    break;
                }
                
                case NativeFunctionID::FILE_WRITE: {
                    if (argCount < 2) {
                        setError("write() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value dataVal = pop();
                    Value handleVal = pop();
                    
                    int written = platform_.file_write(
                        handleVal.isInt32() ? handleVal.int32Val : -1,
                        dataVal.toString()
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Int32(written));
                    break;
                }
                
                case NativeFunctionID::FILE_CLOSE: {
                    if (argCount < 1) {
                        setError("close() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value handleVal = pop();
                    
                    platform_.file_close(handleVal.isInt32() ? handleVal.int32Val : -1);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::FILE_EXISTS: {
                    if (argCount < 1) {
                        setError("exists() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pathVal = pop();
                    
                    bool exists = platform_.file_exists(pathVal.toString());
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Bool(exists));
                    break;
                }
                
                case NativeFunctionID::FILE_DELETE: {
                    if (argCount < 1) {
                        setError("delete() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pathVal = pop();
                    
                    bool deleted = platform_.file_delete(pathVal.toString());
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Bool(deleted));
                    break;
                }
                
                case NativeFunctionID::FILE_SIZE: {
                    if (argCount < 1) {
                        setError("size() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pathVal = pop();
                    
                    int size = platform_.file_size(pathVal.toString());
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Int32(size));
                    break;
                }
                
                // ===== GPIO Functions =====
                case NativeFunctionID::GPIO_PIN_MODE: {
                    if (argCount < 2) {
                        setError("pinMode() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value modeVal = pop();
                    Value pinVal = pop();
                    
                    platform_.gpio_pinMode(
                        pinVal.isInt32() ? pinVal.int32Val : 0,
                        modeVal.isInt32() ? modeVal.int32Val : 0
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::GPIO_DIGITAL_WRITE: {
                    if (argCount < 2) {
                        setError("digitalWrite() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value valueVal = pop();
                    Value pinVal = pop();
                    
                    platform_.gpio_digitalWrite(
                        pinVal.isInt32() ? pinVal.int32Val : 0,
                        valueVal.isInt32() ? valueVal.int32Val : 0
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::GPIO_DIGITAL_READ: {
                    if (argCount < 1) {
                        setError("digitalRead() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pinVal = pop();
                    
                    int value = platform_.gpio_digitalRead(pinVal.isInt32() ? pinVal.int32Val : 0);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Int32(value));
                    break;
                }
                
                case NativeFunctionID::GPIO_ANALOG_WRITE: {
                    if (argCount < 2) {
                        setError("analogWrite() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value valueVal = pop();
                    Value pinVal = pop();
                    
                    platform_.gpio_analogWrite(
                        pinVal.isInt32() ? pinVal.int32Val : 0,
                        valueVal.isInt32() ? valueVal.int32Val : 0
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::GPIO_ANALOG_READ: {
                    if (argCount < 1) {
                        setError("analogRead() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pinVal = pop();
                    
                    int value = platform_.gpio_analogRead(pinVal.isInt32() ? pinVal.int32Val : 0);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Int32(value));
                    break;
                }
                
                // ===== I2C Functions =====
                case NativeFunctionID::I2C_SCAN: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    std::vector<int> addresses = platform_.i2c_scan();
                    // Convert to JSON-like string: "[0x20, 0x21, ...]"
                    std::string result = "[";
                    for (size_t i = 0; i < addresses.size(); i++) {
                        if (i > 0) result += ", ";
                        result += "0x" + std::to_string(addresses[i]);
                    }
                    result += "]";
                    
                    push(Value::String(result));
                    break;
                }
                
                case NativeFunctionID::I2C_WRITE: {
                    if (argCount < 2) {
                        setError("write() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value dataVal = pop();
                    Value addressVal = pop();
                    
                    // Convert string to vector<uint8_t>
                    std::string dataStr = dataVal.toString();
                    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());
                    
                    bool success = platform_.i2c_write(
                        addressVal.isInt32() ? addressVal.int32Val : 0,
                        data
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Bool(success));
                    break;
                }
                
                case NativeFunctionID::I2C_READ: {
                    if (argCount < 2) {
                        setError("read() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value lengthVal = pop();
                    Value addressVal = pop();
                    
                    std::vector<uint8_t> data = platform_.i2c_read(
                        addressVal.isInt32() ? addressVal.int32Val : 0,
                        lengthVal.isInt32() ? lengthVal.int32Val : 0
                    );
                    
                    // Convert vector<uint8_t> to string
                    std::string result(data.begin(), data.end());
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::String(result));
                    break;
                }
                
                // ===== Buzzer Functions =====
                case NativeFunctionID::BUZZER_BEEP: {
                    if (argCount < 2) {
                        setError("beep() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value durationVal = pop();
                    Value frequencyVal = pop();
                    
                    platform_.buzzer_beep(
                        frequencyVal.isInt32() ? frequencyVal.int32Val : 1000,
                        durationVal.isInt32() ? durationVal.int32Val : 100
                    );
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::BUZZER_STOP: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.buzzer_stop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::BUZZER_PLAY_MELODY: {
                    if (argCount < 1) {
                        setError("playMelody() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    
                    Value notesVal = pop();
                    
                    // Convert notes array to vector of integers
                    std::vector<int> notes;
                    if (notesVal.isArray()) {
                        Array* arr = notesVal.arrayVal;
                        for (const Value& v : arr->elements) {
                            if (v.isInt32()) {
                                notes.push_back(v.int32Val);
                            }
                        }
                    }
                    
                    platform_.buzzer_playMelody(notes);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== Timer Functions =====
                case NativeFunctionID::TIMER_SET_TIMEOUT: {
                    if (argCount < 1) {
                        setError("setTimeout() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    // Stack layout: [..., receiver, callbackArgs..., arg0]
                    // Compiler pushes arguments in order so the top of the stack is the last argument (ms)
                    Value msVal = pop();
                    // After popping arguments, the next value is the receiver

                    int timerId = platform_.timer_setTimeout(msVal.isInt32() ? msVal.int32Val : 0);

                    // Pop any additional args (if more than expected)
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Int32(timerId));
                    break;
                }
                
                case NativeFunctionID::TIMER_SET_INTERVAL: {
                    // Expect: receiver, callback (function), ms (int)
                    if (argCount < 2) {
                        setError("setInterval() requires 2 arguments (callback, ms)");
                        return VMResult::ERROR;
                    }

                    // Pop ms (top), then callback, then receiver
                    Value msVal = pop();
                    Value callback = pop();

                    if (!callback.isFunction()) {
                        setError("setInterval() first argument must be a function");
                        return VMResult::ERROR;
                    }

                    int timerId = platform_.timer_setInterval(callback, msVal.isInt32() ? msVal.int32Val : 0);

                    // Pop any additional args beyond the expected two
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Int32(timerId));
                    break;
                }
                
                case NativeFunctionID::TIMER_CLEAR_TIMEOUT:
                case NativeFunctionID::TIMER_CLEAR_INTERVAL: {
                    if (argCount < 1) {
                        setError("clearTimeout/clearInterval() requires 1 argument");
                        return VMResult::ERROR;
                    }

                    // Pop id (top) then receiver
                    Value idVal = pop();

                    if (funcID == NativeFunctionID::TIMER_CLEAR_TIMEOUT) {
                        platform_.timer_clearTimeout(idVal.isInt32() ? idVal.int32Val : -1);
                    } else {
                        platform_.timer_clearInterval(idVal.isInt32() ? idVal.int32Val : -1);
                    }

                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== Memory Functions =====
                case NativeFunctionID::MEMORY_GET_AVAILABLE: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.memory_getAvailable()));
                    break;
                }
                
                case NativeFunctionID::MEMORY_GET_USAGE: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.memory_getUsage()));
                    break;
                }
                
                case NativeFunctionID::MEMORY_ALLOCATE: {
                    if (argCount < 1) {
                        setError("allocate() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value sizeVal = pop();
                    
                    int handle = platform_.memory_allocate(sizeVal.isInt32() ? sizeVal.int32Val : 0);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Int32(handle));
                    break;
                }
                
                case NativeFunctionID::MEMORY_FREE: {
                    if (argCount < 1) {
                        setError("free() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value handleVal = pop();
                    
                    platform_.memory_free(handleVal.isInt32() ? handleVal.int32Val : -1);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== Console Functions =====
                case NativeFunctionID::CONSOLE_PRINT: {
                    if (argCount < 1) {
                        setError("print() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    Value arg = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    platform_.console_print(arg.toString());
                    
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::CONSOLE_PRINTLN: {
                    if (argCount < 1) {
                        setError("println() requires at least 1 argument");
                        return VMResult::ERROR;
                    }
                    Value arg = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    platform_.console_println(arg.toString());
                    
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::CONSOLE_CLEAR: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.console_clear();
                    push(Value::Null());
                    break;
                }
                
                // ===== Display Functions =====
                case NativeFunctionID::DISPLAY_SET_TITLE: {
                    if (argCount < 1) {
                        setError("setTitle() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value titleVal = pop();
                    
                    platform_.display_setTitle(titleVal.toString());
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::DISPLAY_GET_SIZE: {
                    
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    // Create object with width and height
                    Object* sizeObj = pool_.allocateObject("Size");
                    if (sizeObj) {
                        sizeObj->fields["width"] = Value::Int32(platform_.display_getWidth());
                        sizeObj->fields["height"] = Value::Int32(platform_.display_getHeight());
                        push(Value::Object(sizeObj));
                    } else {
                        push(Value::Null());
                    }
                    break;
                }
                
                case NativeFunctionID::DISPLAY_DRAW_IMAGE: {
                    if (argCount < 3) {
                        setError("drawImage() requires 3 arguments (x, y, imageData)");
                        return VMResult::ERROR;
                    }
                    
                    Value imageDataVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    
                    // Convert image data to byte vector
                    std::vector<uint8_t> imageData;
                    if (imageDataVal.isArray()) {
                        Array* arr = imageDataVal.arrayVal;
                        for (const Value& v : arr->elements) {
                            if (v.isInt32()) {
                                imageData.push_back(static_cast<uint8_t>(v.int32Val));
                            }
                        }
                    }
                    
                    platform_.display_drawImage(
                        xVal.isInt32() ? xVal.int32Val : 0,
                        yVal.isInt32() ? yVal.int32Val : 0,
                        imageData
                    );
                    
                    for (uint8_t i = 3; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== System Functions =====
                case NativeFunctionID::SYSTEM_YIELD: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.system_yield();
                    push(Value::Null());
                    break;
                }
                
                // ===== Touch Functions =====
                case NativeFunctionID::TOUCH_GET_POSITION: {
                    
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    // Create object with x, y, and pressed state
                    Object* posObj = pool_.allocateObject("TouchPosition");
                    if (posObj) {
                        posObj->fields["x"] = Value::Int32(platform_.touch_getX());
                        posObj->fields["y"] = Value::Int32(platform_.touch_getY());
                        posObj->fields["pressed"] = Value::Bool(platform_.touch_isPressed());
                        push(Value::Object(posObj));
                    } else {
                        push(Value::Null());
                    }
                    break;
                }
                
                case NativeFunctionID::TOUCH_ON_PRESS: {
                    if (argCount < 1) {
                        setError("onPress() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onPress() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("touch.onPress", callback);
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::TOUCH_ON_RELEASE: {
                    if (argCount < 1) {
                        setError("onRelease() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onRelease() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("touch.onRelease", callback);
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::TOUCH_ON_DRAG: {
                    if (argCount < 1) {
                        setError("onDrag() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onDrag() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("touch.onDrag", callback);
                    push(Value::Null());
                    break;
                }
                
                // ===== Directory Functions =====
                case NativeFunctionID::DIR_LIST: {
                    if (argCount < 1) {
                        setError("list() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pathVal = pop();
                    
                    // Get directory listing from platform
                    std::vector<std::string> files = platform_.dir_list(pathVal.toString());
                    
                    // Convert to DialScript array
                    Array* filesArray = pool_.allocateArray();
                    if (filesArray) {
                        for (const std::string& filename : files) {
                            std::string* pooledStr = pool_.allocateString(filename);
                            if (pooledStr) {
                                filesArray->elements.push_back(Value::StringFromPool(pooledStr));
                            }
                        }
                        push(Value::Array(filesArray));
                    } else {
                        push(Value::Null());
                    }
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    break;
                }
                
                case NativeFunctionID::DIR_CREATE:
                case NativeFunctionID::DIR_DELETE:
                case NativeFunctionID::DIR_EXISTS: {
                    if (argCount < 1) {
                        setError("Directory operation requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value pathVal = pop();
                    
                    // TODO: Implement directory operations
                    bool result = false;
                    if (funcID == NativeFunctionID::DIR_CREATE) {
                        result = platform_.dir_create(pathVal.toString());
                    } else if (funcID == NativeFunctionID::DIR_DELETE) {
                        result = platform_.dir_delete(pathVal.toString());
                    } else {
                        result = platform_.dir_exists(pathVal.toString());
                    }
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Bool(result));
                    break;
                }
                
                // ===== Power Functions =====
                case NativeFunctionID::POWER_SLEEP: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.power_sleep();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::POWER_GET_BATTERY_LEVEL: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Int32(platform_.power_getBatteryLevel()));
                    break;
                }
                
                case NativeFunctionID::POWER_IS_CHARGING: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::Bool(platform_.power_isCharging()));
                    break;
                }
                
                // ===== App Functions =====
                case NativeFunctionID::APP_EXIT: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.app_exit();
                    running_ = false;
                    return VMResult::FINISHED;
                }
                
                case NativeFunctionID::APP_GET_INFO: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::String(platform_.app_getInfo()));
                    break;
                }
                
                case NativeFunctionID::APP_ON_LOAD: {
                    if (argCount < 1) {
                        setError("onLoad() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onLoad() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("app.onLoad", callback);
                    platform_.console_log("Registered app.onLoad callback " + std::to_string(callback.asFunction()->functionIndex));
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::APP_ON_SUSPEND: {
                    if (argCount < 1) {
                        setError("onSuspend() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onSuspend() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("app.onSuspend", callback);
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::APP_ON_RESUME: {
                    if (argCount < 1) {
                        setError("onResume() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onResume() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("app.onResume", callback);
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::APP_ON_UNLOAD: {
                    if (argCount < 1) {
                        setError("onUnload() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value callback = pop();
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    
                    if (!callback.isFunction()) {
                        setError("onUnload() requires a function argument");
                        return VMResult::ERROR;
                    }
                    
                    platform_.registerCallback("app.onUnload", callback);
                    push(Value::Null());
                    break;
                }
                
                // ===== Storage Functions =====
                case NativeFunctionID::STORAGE_GET_MOUNTED: {
                    // TODO: Return array of mounted devices
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::STORAGE_GET_INFO: {
                    if (argCount < 1) {
                        setError("getInfo() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value deviceVal = pop();
                    
                    push(Value::String(platform_.storage_getInfo(deviceVal.toString())));
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    break;
                }
                
                // ===== Sensor Functions =====
                case NativeFunctionID::SENSOR_ATTACH: {
                    if (argCount < 2) {
                        setError("attach() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value typeVal = pop();
                    Value portVal = pop();
                    
                    int handle = platform_.sensor_attach(portVal.toString(), typeVal.toString());
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Int32(handle));
                    break;
                }
                
                case NativeFunctionID::SENSOR_READ: {
                    if (argCount < 1) {
                        setError("read() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value handleVal = pop();
                    
                    push(Value::String(platform_.sensor_read(handleVal.isInt32() ? handleVal.int32Val : -1)));
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    break;
                }
                
                case NativeFunctionID::SENSOR_DETACH: {
                    if (argCount < 1) {
                        setError("detach() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value handleVal = pop();
                    
                    platform_.sensor_detach(handleVal.isInt32() ? handleVal.int32Val : -1);
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== WiFi Functions =====
                case NativeFunctionID::WIFI_CONNECT: {
                    if (argCount < 2) {
                        setError("connect() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value passwordVal = pop();
                    Value ssidVal = pop();
                    
                    bool connected = platform_.wifi_connect(ssidVal.toString(), passwordVal.toString());
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Bool(connected));
                    break;
                }
                
                case NativeFunctionID::WIFI_DISCONNECT: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    platform_.wifi_disconnect();
                    push(Value::Null());
                    break;
                }
                
                case NativeFunctionID::WIFI_GET_STATUS: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::String(platform_.wifi_getStatus()));
                    break;
                }
                
                case NativeFunctionID::WIFI_GET_IP: {
                    for (uint8_t i = 0; i < argCount; i++) pop();
                    
                    push(Value::String(platform_.wifi_getIP()));
                    break;
                }
                
                // ===== IPC Functions =====
                case NativeFunctionID::IPC_SEND: {
                    if (argCount < 2) {
                        setError("send() requires 2 arguments");
                        return VMResult::ERROR;
                    }
                    Value messageVal = pop();
                    Value appIdVal = pop();
                    
                    bool sent = platform_.ipc_send(appIdVal.toString(), messageVal.toString());
                    
                    for (uint8_t i = 2; i < argCount; i++) pop();
                    push(Value::Bool(sent));
                    break;
                }
                
                case NativeFunctionID::IPC_BROADCAST: {
                    if (argCount < 1) {
                        setError("broadcast() requires 1 argument");
                        return VMResult::ERROR;
                    }
                    Value messageVal = pop();
                    
                    platform_.ipc_broadcast(messageVal.toString());
                    
                    for (uint8_t i = 1; i < argCount; i++) pop();
                    push(Value::Null());
                    break;
                }
                
                // ===== Unknown/Unimplemented Functions =====
                default: {
                    // Unknown native function - pop receiver and arguments, return null
                    pop();  // receiver
                    for (uint8_t i = 0; i < argCount; i++) {
                        pop();
                    }
                    push(Value::Null());
                    break;
                }
            }
            break;
        }
        
        case compiler::Opcode::RETURN: {
            Value returnValue = pop();
            
            if (callStack_.empty()) {
                // Top-level return, finish execution
                push(returnValue);
                running_ = false;
                return VMResult::FINISHED;
            }
            
            // Restore previous call frame
            CallFrame frame = callStack_.back();
            callStack_.pop_back();
            
            // Restore PC
            pc_ = frame.returnPC;
            
            // Clean up stack to base
            while (stack_.size() > frame.stackBase) {
                stack_.pop_back();
            }
            
            // If this function is a constructor, return the 'this' object instead of returnValue
            bool isConstructor = false;
            if (!frame.functionName.empty()) {
                const std::string ctorSuffix = "::constructor";
                if (frame.functionName.size() >= ctorSuffix.size() &&
                    frame.functionName.compare(frame.functionName.size() - ctorSuffix.size(), ctorSuffix.size(), ctorSuffix) == 0) {
                    isConstructor = true;
                }
            }

            if (isConstructor) {
                auto it = frame.locals.find(0);
                if (it != frame.locals.end()) {
                    push(it->second);
                } else {
                    // Fallback to returnValue if 'this' not found
                    push(returnValue);
                }
            } else {
                // Push normal return value
                push(returnValue);
            }
            break;
        }
        
        case compiler::Opcode::LOAD_FUNCTION: {
            uint16_t funcIndex = readU16();
            
            if (funcIndex >= module_.functions.size()) {
                setError("Invalid function index: " + std::to_string(funcIndex));
                return VMResult::ERROR;
            }
            
            // Get parameter count from module
            uint8_t paramCount = (funcIndex < module_.functionParamCounts.size()) 
                                ? module_.functionParamCounts[funcIndex] 
                                : 0;
            
            Function* fn = pool_.allocateFunction(funcIndex, paramCount);
            if (!fn) {
                setError("Out of memory allocating function");
                return VMResult::OUT_OF_MEMORY;
            }
            
            push(Value::Function(fn));
            break;
        }
        
        case compiler::Opcode::CALL_INDIRECT: {
            uint8_t argCount = readU8();
            
            // Pop function value (on top of stack, after arguments)
            Value funcVal = pop();
            if (!funcVal.isFunction()) {
                setError("CALL_INDIRECT: value is not a function");
                return VMResult::ERROR;
            }

            Function* fn = funcVal.asFunction();
            
            // Validate argument count
            if (argCount != fn->paramCount) {
                setError("Function '" + module_.functions[fn->functionIndex] + 
                        "' expects " + std::to_string(fn->paramCount) + 
                        " arguments, got " + std::to_string(argCount));
                return VMResult::ERROR;
            }
            
            // Get function entry point
            uint16_t funcIndex = fn->functionIndex;
            if (funcIndex >= module_.functionEntryPoints.size()) {
                setError("Invalid function entry point");
                return VMResult::ERROR;
            }
            
            uint32_t entryPoint = module_.functionEntryPoints[funcIndex];
            
            // Create call frame (same as CALL opcode)
            CallFrame frame;
            frame.returnPC = pc_;

            // Detect implicit receiver presence: if there's still an extra value below the function
            // (compiler emits receiver + DUP + GET_FIELD + function on top), the receiver will be
            // located at stack_.size() - argCount - 1
            bool hasReceiver = false;
            Value receiverVal;
            if (stack_.size() >= (size_t)argCount + 1) {
                // Candidate receiver position
                size_t recvPos = stack_.size() - argCount - 1;
                receiverVal = stack_[recvPos];
                if (receiverVal.isObject()) {
                    hasReceiver = true;
                }
            }

            // Stack base for arguments (arguments are currently on top of stack)
            frame.stackBase = stack_.size() - argCount;
            frame.functionName = module_.functions[funcIndex];

            // If we have a receiver, make it local 0 and shift argument locals by +1
            if (hasReceiver) {
                // Store receiver in local 0
                frame.locals[0] = receiverVal;

                // Store arguments starting at locals[1]
                for (uint8_t i = 0; i < argCount; i++) {
                    frame.locals[i + 1] = stack_[frame.stackBase + i];
                }

                // Remove arguments and the receiver from stack
                stack_.resize(frame.stackBase - 1);
            } else {
                // Store arguments in locals starting at 0
                for (uint8_t i = 0; i < argCount; i++) {
                    frame.locals[i] = stack_[frame.stackBase + i];
                }

                // Remove arguments from stack
                stack_.resize(frame.stackBase);
            }
            
            // Push call frame and jump
            callStack_.push_back(frame);
            pc_ = entryPoint;
            
            break;
        }

        case compiler::Opcode::CALL_METHOD: {
            uint8_t argCount = readU8();
            uint16_t nameIdx = readU16();

            if (nameIdx >= module_.constants.size()) {
                setError("CALL_METHOD: invalid method name index");
                return VMResult::ERROR;
            }

            const std::string& methodName = module_.constants[nameIdx];

            // Pop receiver from stack (it should be below the arguments on the stack)
            // Stack layout before CALL_METHOD: [..., receiver, arg0, arg1, ..., argN]
            if (stack_.size() < (size_t)argCount + 1) {
                setError("CALL_METHOD: stack underflow for receiver/args");
                return VMResult::ERROR;
            }

            // Receiver is located at position: stack.size() - argCount - 1
            size_t recvPos = stack_.size() - argCount - 1;
            Value receiver = stack_[recvPos];
            if (!receiver.isObject() || !receiver.objVal) {
                // Detailed debug: show receiver type, value, and source mapping when available
                std::stringstream dbg;
                dbg << "CALL_METHOD on non-object receiver at PC:" << pc_ << ": type=" << static_cast<int>(receiver.type);
                try { dbg << " value=" << receiver.toString(); } catch (...) {}

                // Include the method name being called
                dbg << " method='" << methodName << "'";

                // If debug info exists in the module, attempt to map PC to source line
                if (module_.hasDebugInfo()) {
                    uint32_t srcLine = module_.getSourceLine(pc_);
                    if (srcLine > 0) {
                        dbg << " (source line: " << srcLine << ")";
                    }
                }

                // If we have a current call frame, include its function name for context
                if (!callStack_.empty()) {
                    dbg << " in function: " << callStack_.back().functionName;
                }

                platform_.console_log(dbg.str());
                setError("CALL_METHOD on non-object receiver");
                return VMResult::ERROR;
            }

            // Lookup the method in the receiver's fields
            auto it = receiver.objVal->fields.find(methodName);
            if (it == receiver.objVal->fields.end()) {
                // Log available fields for debugging
                std::string dbg = "Method '" + methodName + "' not found on object of class " + receiver.objVal->className + ": fields=[";
                bool first = true;
                for (const auto &p : receiver.objVal->fields) {
                    if (!first) dbg += ", ";
                    dbg += p.first;
                    first = false;
                }
                dbg += "]";
                platform_.console_log(dbg);
                setError("Method '" + methodName + "' not found on object");
                return VMResult::ERROR;
            }

            Value methodVal = it->second;

            // Method must be a function value
            if (!methodVal.isFunction()) {
                // Log field type for debugging
                std::string dbg = "CALL_METHOD: field '" + methodName + "' on class " + receiver.objVal->className + " is present but not a function. Type: ";
                switch (methodVal.type) {
                    case vm::ValueType::NULL_VAL: dbg += "null"; break;
                    case vm::ValueType::BOOL: dbg += "bool"; break;
                    case vm::ValueType::INT32: dbg += "int32"; break;
                    case vm::ValueType::FLOAT32: dbg += "float32"; break;
                    case vm::ValueType::STRING: dbg += "string"; break;
                    case vm::ValueType::OBJECT: dbg += "object"; break;
                    case vm::ValueType::ARRAY: dbg += "array"; break;
                    case vm::ValueType::FUNCTION: dbg += "function"; break;
                    case vm::ValueType::NATIVE_FN: dbg += "native_fn"; break;
                    default: dbg += "unknown"; break;
                }
                platform_.console_log(dbg);
                setError("CALL_METHOD: field '" + methodName + "' is not a function");
                return VMResult::ERROR;
            }

            Function* fn = methodVal.asFunction();

            // Validate argument count against function's paramCount (method receives 'this' as implicit local 0)
            if (argCount != fn->paramCount) {
                // Note: methods compiled with 'this' as local 0 still have paramCount equal to declared params
                if (argCount != fn->paramCount) {
                    setError("Method '" + methodName + "' expects " + std::to_string(fn->paramCount) +
                             " arguments, got " + std::to_string(argCount));
                    return VMResult::ERROR;
                }
            }

            uint16_t funcIndex = fn->functionIndex;
            if (funcIndex >= module_.functionEntryPoints.size()) {
                setError("Invalid function entry point for method");
                return VMResult::ERROR;
            }

            uint32_t entryPoint = module_.functionEntryPoints[funcIndex];

            // Build call frame: local 0 = receiver, locals 1..N = args
            CallFrame frame;
            frame.returnPC = pc_;
            frame.stackBase = recvPos; // base was where receiver is
            frame.functionName = module_.functions[funcIndex];

            // Store receiver as local 0
            frame.locals[0] = receiver;

            // Store arguments into locals[1..]
            for (uint8_t i = 0; i < argCount; i++) {
                frame.locals[i + 1] = stack_[recvPos + 1 + i];
            }

            // Remove receiver and args from the stack
            stack_.resize(recvPos);

            // Push call frame and jump
            callStack_.push_back(frame);
            pc_ = entryPoint;

            break;
        }
        
        // ===== Object/Array Operations =====
        case compiler::Opcode::GET_FIELD: {
            uint16_t fieldIndex = readU16();
            Value obj = pop();
            
            if (fieldIndex >= module_.constants.size()) {
                setError("Invalid field name index");
                return VMResult::ERROR;
            }
            
            const std::string& fieldName = module_.constants[fieldIndex];
            
            if (obj.isArray() && obj.arrayVal) {
                // Handle array properties
                if (fieldName == "length") {
                    push(Value::Int32(static_cast<int32_t>(obj.arrayVal->elements.size())));
                } else {
                    push(Value::Null());
                }
            } else if (obj.isObject() && obj.objVal) {
                // Handle object properties
                auto it = obj.objVal->fields.find(fieldName);
                if (it != obj.objVal->fields.end()) {
                    push(it->second);
                } else {
                    push(Value::Null());
                }
            } else {
                setError("GET_FIELD on non-object");
                return VMResult::ERROR;
            }
            break;
        }
        
        case compiler::Opcode::SET_FIELD: {
            uint16_t fieldIndex = readU16();
            // Note: compiler emits value then object (value pushed first, then receiver)
            // So pop receiver (object) first, then the value
            Value obj = pop();
            Value value = pop();

            if (!obj.isObject() || !obj.objVal) {
                // Debug: log what we popped
                platform_.console_log(std::string("SET_FIELD on non-object; popped type: ") + std::to_string(static_cast<int>(obj.type)));
                setError("SET_FIELD on non-object");
                return VMResult::ERROR;
            }

            if (fieldIndex >= module_.constants.size()) {
                setError("Invalid field name index");
                return VMResult::ERROR;
            }

            const std::string& fieldName = module_.constants[fieldIndex];
            obj.objVal->fields[fieldName] = value;
            break;
        }
        
        case compiler::Opcode::GET_INDEX: {
            Value index = pop();
            Value array = pop();
            
            if (!array.isArray() || !array.arrayVal) {
                setError("GET_INDEX on non-array");
                return VMResult::ERROR;
            }
            
            if (!index.isInt32()) {
                setError("Array index must be integer");
                return VMResult::ERROR;
            }
            
            int32_t idx = index.int32Val;
            if (idx < 0 || idx >= static_cast<int32_t>(array.arrayVal->elements.size())) {
                push(Value::Null());
            } else {
                push(array.arrayVal->elements[idx]);
            }
            break;
        }
        
        case compiler::Opcode::SET_INDEX: {
            Value value = pop();
            Value index = pop();
            Value array = pop();
            
            if (!array.isArray() || !array.arrayVal) {
                setError("SET_INDEX on non-array");
                return VMResult::ERROR;
            }
            
            if (!index.isInt32()) {
                setError("Array index must be integer");
                return VMResult::ERROR;
            }
            
            int32_t idx = index.int32Val;
            if (idx >= 0 && idx < static_cast<int32_t>(array.arrayVal->elements.size())) {
                array.arrayVal->elements[idx] = value;
            }
            break;
        }
        
        // ===== Object Creation =====
        case compiler::Opcode::NEW_OBJECT: {
            uint16_t classIndex = readU16();
            
            std::string className = "Object";
            if (classIndex < module_.constants.size()) {
                className = module_.constants[classIndex];
            }
            
            vm::Object* obj = pool_.allocateObject(className);
            if (!obj) {
                setError("Out of memory creating object");
                return VMResult::OUT_OF_MEMORY;
            }
            
            // Look for constructor function
            std::string constructorName = className + "::constructor";
            int32_t funcIndex = -1;
            for (size_t i = 0; i < module_.functions.size(); i++) {
                if (module_.functions[i] == constructorName) {
                    funcIndex = static_cast<int32_t>(i);
                    break;
                }
            }
            
            // Populate methods on the instance: find functions named Class::method and attach
            for (size_t i = 0; i < module_.functions.size(); ++i) {
                const std::string &fname = module_.functions[i];
                std::string prefix = className + "::";
                if (fname.size() > prefix.size() && fname.compare(0, prefix.size(), prefix) == 0) {
                    std::string method = fname.substr(prefix.size());
                    if (method == "constructor") continue;
                    // Allocate function value for this method and store on the instance
                    uint8_t paramCount = (i < module_.functionParamCounts.size()) ? module_.functionParamCounts[i] : 0;
                    vm::Function* fn = pool_.allocateFunction(static_cast<uint16_t>(i), paramCount);
                    if (fn) {
                        obj->fields[method] = Value::Function(fn);
                    }
                }
            }

            // Push object onto stack (it will be 'this' for constructor)
            push(Value::Object(obj));

            // If constructor exists, call it synchronously
            // Note: Arguments are already on stack before NEW_OBJECT, and the object was just pushed
            // Stack layout now: [arg0, arg1, ..., argN-1, object]
            if (funcIndex != -1) {
                uint16_t ctorIdx = static_cast<uint16_t>(funcIndex);
                uint32_t entryPoint = module_.functionEntryPoints[ctorIdx];

                // Determine argument count as number of items below the object
                size_t total = stack_.size();
                if (total < 1) {
                    // Shouldn't happen, object just pushed
                } else {
                    size_t argCount = total - 1; // values below object are args

                    // Build call frame with local0 = this, locals[1..] = args
                    CallFrame frame;
                    frame.returnPC = pc_;
                    frame.stackBase = total - (argCount + 1); // position of first arg
                    frame.functionName = module_.functions[ctorIdx];

                    // Store 'this' as local 0
                    frame.locals[0] = stack_[frame.stackBase + argCount]; // object

                    // Store args into locals[1..]
                    for (size_t i = 0; i < argCount; ++i) {
                        frame.locals[1 + i] = stack_[frame.stackBase + i];
                    }

                    // Remove receiver and args from stack
                    stack_.resize(frame.stackBase);

                    // Push call frame and jump to constructor
                    callStack_.push_back(frame);
                    pc_ = entryPoint;
                    
                    break; // start executing constructor
                }
            }
            break;
        }
        
        case compiler::Opcode::NEW_ARRAY: {
            Value sizeVal = pop();
            
            if (!sizeVal.isInt32()) {
                setError("Array size must be integer");
                return VMResult::ERROR;
            }
            
            int32_t size = sizeVal.int32Val;
            if (size < 0) {
                size = 0;
            }
            
            vm::Array* arr = pool_.allocateArray(size);
            if (!arr) {
                setError("Out of memory creating array");
                return VMResult::OUT_OF_MEMORY;
            }
            
            // Pop elements from stack and fill array (in reverse)
            for (int32_t i = size - 1; i >= 0; i--) {
                arr->elements[i] = pop();
            }
            
            push(Value::Array(arr));
            break;
        }
        
        // ===== Exception Handling =====
        case compiler::Opcode::TRY: {
            int32_t catchOffset = readI32();
            ExceptionHandler handler;
            handler.catchPC = static_cast<size_t>(static_cast<int32_t>(pc_) + catchOffset);
            handler.stackSize = stack_.size();
            exceptionHandlers_.push_back(handler);
            break;
        }
        
        case compiler::Opcode::END_TRY: {
            if (!exceptionHandlers_.empty()) {
                exceptionHandlers_.pop_back();
            }
            break;
        }
        
        case compiler::Opcode::THROW: {
            Value exception = pop();
            
            if (exceptionHandlers_.empty()) {
                // Unhandled exception
                setError("Unhandled exception: " + exception.toString());
                return VMResult::ERROR;
            }
            
            // Jump to catch handler
            ExceptionHandler handler = exceptionHandlers_.back();
            exceptionHandlers_.pop_back();
            
            // Clean up stack
            while (stack_.size() > handler.stackSize) {
                stack_.pop_back();
            }
            
            // Push exception value
            push(exception);
            
            // Jump to catch block
            pc_ = handler.catchPC;
            break;
        }
        
        // ===== Special =====
        case compiler::Opcode::PRINT: {
            Value v = pop();
            platform_.console_print(v.toString());
            break;
        }
        
        case compiler::Opcode::HALT:
            running_ = false;
            return VMResult::FINISHED;
            
        default:
            setError("Unknown opcode: " + std::to_string(static_cast<int>(op)));
            return VMResult::ERROR;
    }
    
    return VMResult::OK;
}

// ===== Arithmetic Operations =====

Value VMState::add(const Value& a, const Value& b) {
    // Int + Int = Int
    if (a.isInt32() && b.isInt32()) {
        return Value::Int32(a.int32Val + b.int32Val);
    }
    
    // Float arithmetic
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Float32(fa + fb);
    }
    
    // String concatenation
    if (a.isString() || b.isString()) {
        std::string result = a.toString() + b.toString();
        std::string* str = pool_.allocateString(result);
        if (!str) {
            setError("Out of memory in add");
            return Value::Null();
        }
        return Value::StringFromPool(str);
    }
    
    return Value::Null();
}

Value VMState::subtract(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Int32(a.int32Val - b.int32Val);
    }
    
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Float32(fa - fb);
    }
    
    return Value::Null();
}

Value VMState::multiply(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Int32(a.int32Val * b.int32Val);
    }
    
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Float32(fa * fb);
    }
    
    return Value::Null();
}

Value VMState::divide(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        if (b.int32Val == 0) {
            setError("Division by zero");
            return Value::Null();
        }
        return Value::Int32(a.int32Val / b.int32Val);
    }
    
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        if (fb == 0.0f) {
            setError("Division by zero");
            return Value::Null();
        }
        return Value::Float32(fa / fb);
    }
    
    return Value::Null();
}

Value VMState::modulo(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        if (b.int32Val == 0) {
            setError("Modulo by zero");
            return Value::Null();
        }
        return Value::Int32(a.int32Val % b.int32Val);
    }
    
    return Value::Null();
}

Value VMState::negate(const Value& v) {
    if (v.isInt32()) {
        return Value::Int32(-v.int32Val);
    }
    if (v.isFloat32()) {
        return Value::Float32(-v.float32Val);
    }
    return Value::Null();
}

// ===== Comparison Operations =====

Value VMState::compare_eq(const Value& a, const Value& b) {
    return Value::Bool(a.equals(b));
}

Value VMState::compare_ne(const Value& a, const Value& b) {
    return Value::Bool(!a.equals(b));
}

Value VMState::compare_lt(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Bool(a.int32Val < b.int32Val);
    }
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Bool(fa < fb);
    }
    return Value::Bool(false);
}

Value VMState::compare_le(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Bool(a.int32Val <= b.int32Val);
    }
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Bool(fa <= fb);
    }
    return Value::Bool(false);
}

Value VMState::compare_gt(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Bool(a.int32Val > b.int32Val);
    }
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Bool(fa > fb);
    }
    return Value::Bool(false);
}

Value VMState::compare_ge(const Value& a, const Value& b) {
    if (a.isInt32() && b.isInt32()) {
        return Value::Bool(a.int32Val >= b.int32Val);
    }
    if (a.isFloat32() || b.isFloat32()) {
        float fa = a.isFloat32() ? a.float32Val : static_cast<float>(a.int32Val);
        float fb = b.isFloat32() ? b.float32Val : static_cast<float>(b.int32Val);
        return Value::Bool(fa >= fb);
    }
    return Value::Bool(false);
}

// ===== Logical Operations =====

Value VMState::logical_not(const Value& v) {
    return Value::Bool(!v.isTruthy());
}

Value VMState::logical_and(const Value& a, const Value& b) {
    return Value::Bool(a.isTruthy() && b.isTruthy());
}

Value VMState::logical_or(const Value& a, const Value& b) {
    return Value::Bool(a.isTruthy() || b.isTruthy());
}

// ===== Template Formatting =====

std::string VMState::formatTemplate(const std::string& template_str, const std::vector<Value>& args) {
    std::string result;
    result.reserve(template_str.size() + 100); // Reserve some extra space
    
    size_t pos = 0;
    while (pos < template_str.size()) {
        size_t placeholder_start = template_str.find("${", pos);
        
        if (placeholder_start == std::string::npos) {
            // No more placeholders, append the rest
            result.append(template_str, pos, template_str.size() - pos);
            break;
        }
        
        // Append text before placeholder
        result.append(template_str, pos, placeholder_start - pos);
        
        // Find end of placeholder
        size_t placeholder_end = template_str.find("}", placeholder_start + 2);
        if (placeholder_end == std::string::npos) {
            // Malformed placeholder, append as-is
            result.append(template_str, placeholder_start, template_str.size() - placeholder_start);
            break;
        }
        
        // Extract argument index
        std::string index_str = template_str.substr(placeholder_start + 2, placeholder_end - placeholder_start - 2);
        try {
            size_t arg_index = std::stoul(index_str);
            if (arg_index < args.size()) {
                // Replace with argument value
                result.append(args[arg_index].toString());
            } else {
                // Invalid index, keep placeholder
                result.append(template_str, placeholder_start, placeholder_end - placeholder_start + 1);
            }
        } catch (const std::exception&) {
            // Invalid number, keep placeholder
            result.append(template_str, placeholder_start, placeholder_end - placeholder_start + 1);
        }
        
        pos = placeholder_end + 1;
    }
    
    return result;
}

} // namespace vm
} // namespace dialos
