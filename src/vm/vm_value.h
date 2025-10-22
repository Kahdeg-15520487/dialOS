/**
 * dialScript VM Value System
 * 
 * Tagged union value type for VM operations
 */

#ifndef DIALOS_VM_VALUE_H
#define DIALOS_VM_VALUE_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dialos {
namespace vm {

// Forward declarations
struct Object;
struct Array;
struct Function;

// Value type enumeration
enum class ValueType : uint8_t {
    NULL_VAL,
    BOOL,
    INT32,
    FLOAT32,
    STRING,
    OBJECT,
    ARRAY,
    FUNCTION,    // Function reference
    NATIVE_FN
};

// Function reference - just index + param count
struct Function {
    uint16_t functionIndex;  // Index into BytecodeModule.functions
    uint8_t paramCount;      // Number of parameters (for validation)
    
    Function(uint16_t idx, uint8_t params) 
        : functionIndex(idx), paramCount(params) {}
};

// Value struct - tagged union
struct Value {
    ValueType type;
    
    union {
        bool boolVal;
        int32_t int32Val;
        float float32Val;
        std::string* stringVal;
        Object* objVal;
        Array* arrayVal;
        Function* functionVal;
        void* nativeFn;
    };
    
    // Constructors
    Value() : type(ValueType::NULL_VAL) { int32Val = 0; }
    
    static Value Null() {
        Value v;
        v.type = ValueType::NULL_VAL;
        return v;
    }
    
    static Value Bool(bool b) {
        Value v;
        v.type = ValueType::BOOL;
        v.boolVal = b;
        return v;
    }
    
    static Value Int32(int32_t i) {
        Value v;
        v.type = ValueType::INT32;
        v.int32Val = i;
        return v;
    }
    
    static Value Float32(float f) {
        Value v;
        v.type = ValueType::FLOAT32;
        v.float32Val = f;
        return v;
    }
    
    static Value String(const std::string& s);
    static Value StringFromPool(std::string* s) {
        Value v;
        v.type = ValueType::STRING;
        v.stringVal = s;
        return v;
    }
    static Value Object(Object* obj);
    static Value Array(Array* arr);
    static Value Function(Function* fn);
    
    // Type checking
    bool isNull() const { return type == ValueType::NULL_VAL; }
    bool isBool() const { return type == ValueType::BOOL; }
    bool isInt32() const { return type == ValueType::INT32; }
    bool isFloat32() const { return type == ValueType::FLOAT32; }
    bool isString() const { return type == ValueType::STRING; }
    bool isObject() const { return type == ValueType::OBJECT; }
    bool isArray() const { return type == ValueType::ARRAY; }
    bool isFunction() const { return type == ValueType::FUNCTION; }
    
    // Value getters
    struct Function* asFunction() const { return functionVal; }
    
    // Truthiness (for conditionals)
    bool isTruthy() const {
        switch (type) {
            case ValueType::NULL_VAL: return false;
            case ValueType::BOOL: return boolVal;
            case ValueType::INT32: return int32Val != 0;
            case ValueType::FLOAT32: return float32Val != 0.0f;
            case ValueType::STRING: return stringVal && !stringVal->empty();
            case ValueType::OBJECT: return objVal != nullptr;
            case ValueType::ARRAY: return arrayVal != nullptr;
            case ValueType::FUNCTION: return functionVal != nullptr;
            default: return false;
        }
    }
    
    // Conversion to string (for printing/concatenation)
    std::string toString() const;
    
    // Comparison
    bool equals(const Value& other) const;
};

// Object type (key-value map)
struct Object {
    std::map<std::string, Value> fields;
    std::string className;
    
    Object() : className("Object") {}
    explicit Object(const std::string& name) : className(name) {}
};

// Array type (dynamic array)
struct Array {
    std::vector<Value> elements;
    
    Array() {}
    explicit Array(size_t size) : elements(size) {}
};

// Memory pool for heap-allocated values with reference counting
class ValuePool {
public:
    ValuePool(size_t heapSize) : heapSize_(heapSize), allocated_(0) {}
    
    ~ValuePool() {
        // Clean up all allocated memory
        for (auto* str : strings_) delete str;
        for (auto* obj : objects_) delete obj;
        for (auto* arr : arrays_) delete arr;
        for (auto* fn : functions_) delete fn;
    }
    
    std::string* allocateString(const std::string& str) {
        // String interning: check if we already have this string
        for (size_t i = 0; i < strings_.size(); ++i) {
            if (*strings_[i] == str) {
                return strings_[i]; // Reuse existing string
            }
        }
        
        // Not found, allocate new string
        size_t size = str.length() + sizeof(std::string);
        if (allocated_ + size > heapSize_) {
            // Allocation will fail - don't try GC here to avoid recursion
            return nullptr;
        }
        
        auto* s = new std::string(str);
        strings_.push_back(s);
        stringMarked_.push_back(false); // Not marked initially
        allocated_ += size;
        return s;
    }
    
    Object* allocateObject(const std::string& className = "Object") {
        size_t size = sizeof(Object);
        if (allocated_ + size > heapSize_) {
            return nullptr; // Don't trigger GC to avoid recursion
        }
        
        auto* obj = new Object(className);
        objects_.push_back(obj);
        objectMarked_.push_back(false); // Not marked initially
        allocated_ += size;
        return obj;
    }
    
    Array* allocateArray(size_t size = 0) {
        size_t allocSize = sizeof(Array) + size * sizeof(Value);
        if (allocated_ + allocSize > heapSize_) {
            return nullptr; // Don't trigger GC to avoid recursion
        }
        
        auto* arr = new Array(size);
        arrays_.push_back(arr);
        arrayMarked_.push_back(false); // Not marked initially
        allocated_ += allocSize;
        return arr;
    }
    
    Function* allocateFunction(uint16_t funcIndex, uint8_t paramCount) {
        size_t size = sizeof(Function);
        if (allocated_ + size > heapSize_) {
            return nullptr; // Don't trigger GC to avoid recursion
        }
        
        auto* fn = new Function(funcIndex, paramCount);
        functions_.push_back(fn);
        functionMarked_.push_back(false); // Not marked initially
        allocated_ += size;
        return fn;
    }
    
    size_t getAllocated() const { return allocated_; }
    size_t getAvailable() const { return heapSize_ - allocated_; }
    size_t getHeapSize() const { return heapSize_; }
    
    void garbageCollect() {
        // Mark phase - reset all marks
        for (size_t i = 0; i < strings_.size(); ++i) {
            stringMarked_[i] = false;
        }
        for (size_t i = 0; i < objects_.size(); ++i) {
            objectMarked_[i] = false;
        }
        for (size_t i = 0; i < arrays_.size(); ++i) {
            arrayMarked_[i] = false;
        }
        for (size_t i = 0; i < functions_.size(); ++i) {
            functionMarked_[i] = false;
        }
        
        // Sweep phase - remove unmarked objects
        sweepStrings();
        sweepObjects();
        sweepArrays();
        sweepFunctions();
    }
    
    // Mark a string as reachable
    void markString(std::string* str) {
        if (!str) return;
        for (size_t i = 0; i < strings_.size(); ++i) {
            if (strings_[i] == str) {
                stringMarked_[i] = true;
                break;
            }
        }
    }
    
    // Mark an object as reachable
    void markObject(Object* obj) {
        if (!obj) return;
        for (size_t i = 0; i < objects_.size(); ++i) {
            if (objects_[i] == obj) {
                objectMarked_[i] = true;
                // Mark object's field values recursively
                for (const auto& field : obj->fields) {
                    markValue(field.second);
                }
                break;
            }
        }
    }
    
    // Mark an array as reachable  
    void markArray(Array* arr) {
        if (!arr) return;
        for (size_t i = 0; i < arrays_.size(); ++i) {
            if (arrays_[i] == arr) {
                arrayMarked_[i] = true;
                // Mark array elements recursively
                for (const auto& element : arr->elements) {
                    markValue(element);
                }
                break;
            }
        }
    }
    
    // Mark a function as reachable
    void markFunction(Function* fn) {
        if (!fn) return;
        for (size_t i = 0; i < functions_.size(); ++i) {
            if (functions_[i] == fn) {
                functionMarked_[i] = true;
                break;
            }
        }
    }
    
    // Mark a value and its reachable references
    void markValue(const Value& value) {
        switch (value.type) {
            case ValueType::STRING:
                markString(value.stringVal);
                break;
            case ValueType::OBJECT:
                markObject(value.objVal);
                break;
            case ValueType::ARRAY:
                markArray(value.arrayVal);
                break;
            case ValueType::FUNCTION:
                markFunction(value.functionVal);
                break;
            default:
                // Primitive types don't need marking
                break;
        }
    }
    
private:
    size_t heapSize_;
    size_t allocated_;
    std::vector<std::string*> strings_;
    std::vector<bool> stringMarked_;      // Mark bits for strings
    std::vector<Object*> objects_;
    std::vector<bool> objectMarked_;      // Mark bits for objects
    std::vector<Array*> arrays_;
    std::vector<bool> arrayMarked_;       // Mark bits for arrays
    std::vector<Function*> functions_;
    std::vector<bool> functionMarked_;    // Mark bits for functions
    
    // Sweep phase methods
    void sweepStrings() {
        for (size_t i = strings_.size(); i > 0; i--) {
            size_t idx = i - 1;
            if (!stringMarked_[idx]) {
                size_t size = strings_[idx]->length() + sizeof(std::string);
                allocated_ -= size;
                delete strings_[idx];
                strings_.erase(strings_.begin() + idx);
                stringMarked_.erase(stringMarked_.begin() + idx);
            }
        }
    }
    
    void sweepObjects() {
        for (size_t i = objects_.size(); i > 0; i--) {
            size_t idx = i - 1;
            if (!objectMarked_[idx]) {
                size_t size = sizeof(Object);
                allocated_ -= size;
                delete objects_[idx];
                objects_.erase(objects_.begin() + idx);
                objectMarked_.erase(objectMarked_.begin() + idx);
            }
        }
    }
    
    void sweepArrays() {
        for (size_t i = arrays_.size(); i > 0; i--) {
            size_t idx = i - 1;
            if (!arrayMarked_[idx]) {
                size_t size = sizeof(Array) + arrays_[idx]->elements.size() * sizeof(Value);
                allocated_ -= size;
                delete arrays_[idx];
                arrays_.erase(arrays_.begin() + idx);
                arrayMarked_.erase(arrayMarked_.begin() + idx);
            }
        }
    }
    
    void sweepFunctions() {
        for (size_t i = functions_.size(); i > 0; i--) {
            size_t idx = i - 1;
            if (!functionMarked_[idx]) {
                size_t size = sizeof(Function);
                allocated_ -= size;
                delete functions_[idx];
                functions_.erase(functions_.begin() + idx);
                functionMarked_.erase(functionMarked_.begin() + idx);
            }
        }
    }
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_VM_VALUE_H
