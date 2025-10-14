/**
 * dialScript VM Core Implementation
 * 
 * Stack-based virtual machine with cooperative multitasking
 */

#include "vm_core.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <cmath>

namespace dialos {
namespace vm {

VMState::VMState(const compiler::BytecodeModule& module, ValuePool& pool, PlatformInterface& platform)
    : module_(module), pool_(pool), platform_(platform), pc_(module.mainEntryPoint), running_(false) {
    
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
            globals_["os"] = Value::Object(osObj);
        }
    }
}

void VMState::reset() {
    pc_ = module_.mainEntryPoint;
    running_ = true;
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
}Value VMState::pop() {
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
    
    return Value::String(module_.constants[index]);
}

Value VMState::loadGlobal(uint16_t index) {
    if (index >= module_.globals.size()) {
        setError("Invalid global index");
        return Value::Null();
    }
    
    const std::string& name = module_.globals[index];
    auto it = globals_.find(name);
    if (it != globals_.end()) {
        return it->second;
    }
    
    return Value::Null();
}

void VMState::storeGlobal(uint16_t index, const Value& value) {
    if (index >= module_.globals.size()) {
        setError("Invalid global index");
        return;
    }
    
    const std::string& name = module_.globals[index];
    globals_[name] = value;
}

VMResult VMState::execute(uint32_t maxInstructions) {
    if (!running_) {
        return VMResult::ERROR;
    }
    
    uint32_t executed = 0;
    
    while (running_ && executed < maxInstructions && pc_ < code_.size()) {
        VMResult result = executeInstruction();
        
        if (result != VMResult::OK) {
            return result;
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
                setError("Out of memory in string concatenation");
                return VMResult::OUT_OF_MEMORY;
            }
            push(Value::String(result));
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
            
            // Stack layout: [..., arg1, arg2, ..., argN, receiver]
            // Receiver is on top, arguments below
            
            const std::string& funcName = module_.functions[funcIndex];
            
            // Dispatch to native functions based on name
            // os.console.log(message)
            if (funcName == "log") {
                if (argCount >= 1) {
                    Value receiver = pop();  // Pop console object
                    Value arg = pop();       // Pop message argument
                    std::string message = arg.toString();
                    platform_.console_log(message);
                    
                    // Pop any remaining arguments
                    for (uint8_t i = 1; i < argCount; i++) {
                        pop();
                    }
                    
                    push(Value::Null());  // Return value
                } else {
                    setError("log() requires at least 1 argument");
                    return VMResult::ERROR;
                }
            }
            // os.display.clear(color)
            else if (funcName == "clear") {
                if (argCount >= 1) {
                    Value receiver = pop();  // Pop display object
                    Value colorVal = pop();  // Pop color argument
                    uint32_t color = colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0;
                    platform_.display_clear(color);
                    
                    // Pop any remaining arguments
                    for (uint8_t i = 1; i < argCount; i++) {
                        pop();
                    }
                    
                    push(Value::Null());
                } else {
                    setError("clear() requires at least 1 argument");
                    return VMResult::ERROR;
                }
            }
            // os.display.drawText(x, y, text, color, size)
            else if (funcName == "drawText") {
                if (argCount >= 5) {
                    Value receiver = pop();  // Pop display object
                    Value sizeVal = pop();
                    Value colorVal = pop();
                    Value textVal = pop();
                    Value yVal = pop();
                    Value xVal = pop();
                    
                    int x = xVal.isInt32() ? xVal.int32Val : 0;
                    int y = yVal.isInt32() ? yVal.int32Val : 0;
                    std::string text = textVal.toString();
                    uint32_t color = colorVal.isInt32() ? static_cast<uint32_t>(colorVal.int32Val) : 0xFFFFFF;
                    int size = sizeVal.isInt32() ? sizeVal.int32Val : 1;
                    
                    platform_.display_drawText(x, y, text, color, size);
                    
                    // Pop any remaining arguments
                    for (uint8_t i = 5; i < argCount; i++) {
                        pop();
                    }
                    
                    push(Value::Null());
                } else {
                    setError("drawText() requires at least 5 arguments");
                    return VMResult::ERROR;
                }
            }
            // os.encoder.getButton()
            else if (funcName == "getButton") {
                Value receiver = pop();  // Pop encoder object
                
                // Pop any arguments (shouldn't be any)
                for (uint8_t i = 0; i < argCount; i++) {
                    pop();
                }
                
                bool pressed = platform_.encoder_getButton();
                push(Value::Bool(pressed));
            }
            // os.encoder.getDelta()
            else if (funcName == "getDelta") {
                Value receiver = pop();  // Pop encoder object
                
                // Pop any arguments (shouldn't be any)
                for (uint8_t i = 0; i < argCount; i++) {
                    pop();
                }
                
                int delta = platform_.encoder_getDelta();
                push(Value::Int32(delta));
            }
            // os.system.getTime()
            else if (funcName == "getTime") {
                Value receiver = pop();  // Pop system object
                
                // Pop any arguments (shouldn't be any)
                for (uint8_t i = 0; i < argCount; i++) {
                    pop();
                }
                
                uint32_t time = platform_.system_getTime();
                push(Value::Int32(static_cast<int32_t>(time)));
            }
            // os.system.sleep(ms)
            else if (funcName == "sleep") {
                if (argCount >= 1) {
                    Value receiver = pop();  // Pop system object
                    Value msVal = pop();     // Pop milliseconds argument
                    uint32_t ms = msVal.isInt32() ? static_cast<uint32_t>(msVal.int32Val) : 0;
                    platform_.system_sleep(ms);
                    
                    // Pop any remaining arguments
                    for (uint8_t i = 1; i < argCount; i++) {
                        pop();
                    }
                    
                    push(Value::Null());
                } else {
                    setError("sleep() requires at least 1 argument");
                    return VMResult::ERROR;
                }
            }
            else {
                // Unknown native function - pop arguments and return null
                pop();  // receiver
                for (uint8_t i = 0; i < argCount; i++) {
                    pop();
                }
                push(Value::Null());
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
            
            // Push return value
            push(returnValue);
            break;
        }
        
        // ===== Object/Array Operations =====
        case compiler::Opcode::GET_FIELD: {
            uint16_t fieldIndex = readU16();
            Value obj = pop();
            
            if (!obj.isObject() || !obj.objVal) {
                setError("GET_FIELD on non-object");
                return VMResult::ERROR;
            }
            
            if (fieldIndex >= module_.constants.size()) {
                setError("Invalid field name index");
                return VMResult::ERROR;
            }
            
            const std::string& fieldName = module_.constants[fieldIndex];
            auto it = obj.objVal->fields.find(fieldName);
            if (it != obj.objVal->fields.end()) {
                push(it->second);
            } else {
                push(Value::Null());
            }
            break;
        }
        
        case compiler::Opcode::SET_FIELD: {
            uint16_t fieldIndex = readU16();
            Value value = pop();
            Value obj = pop();
            
            if (!obj.isObject() || !obj.objVal) {
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
            
            // Push object onto stack (it will be 'this' for constructor)
            push(Value::Object(obj));
            
            // If constructor exists, call it
            // Note: Arguments are already on stack before NEW_OBJECT
            // Stack layout: [arg1, arg2, ..., argN, object(this)]
            // Constructor will use these arguments
            
            // For now, object is on stack ready for use
            // TODO: Implement actual constructor calling mechanism
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
            platform_.program_output(v.toString());
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
        return Value::String(result);
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

} // namespace vm
} // namespace dialos
