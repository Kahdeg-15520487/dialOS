/**
 * dialScript VM Value System Implementation
 */

#include "../../include/vm_value.h"
#include <sstream>
#include <cmath>

namespace dialos {
namespace vm {

Value Value::String(const std::string& s) {
    Value v;
    v.type = ValueType::STRING;
    v.stringVal = new std::string(s);
    return v;
}

Value Value::Object(vm::Object* obj) {
    Value v;
    v.type = ValueType::OBJECT;
    v.objVal = obj;
    return v;
}

Value Value::Array(vm::Array* arr) {
    Value v;
    v.type = ValueType::ARRAY;
    v.arrayVal = arr;
    return v;
}

std::string Value::toString() const {
    std::stringstream ss;
    
    switch (type) {
        case ValueType::NULL_VAL:
            return "null";
            
        case ValueType::BOOL:
            return boolVal ? "true" : "false";
            
        case ValueType::INT32:
            ss << int32Val;
            return ss.str();
            
        case ValueType::FLOAT32:
            ss << float32Val;
            return ss.str();
            
        case ValueType::STRING:
            return stringVal ? *stringVal : "";
            
        case ValueType::OBJECT:
            if (objVal) {
                ss << "[Object " << objVal->className << "]";
            } else {
                ss << "[Object null]";
            }
            return ss.str();
            
        case ValueType::ARRAY:
            if (arrayVal) {
                ss << "[Array length=" << arrayVal->elements.size() << "]";
            } else {
                ss << "[Array null]";
            }
            return ss.str();
            
        case ValueType::NATIVE_FN:
            return "[NativeFunction]";
            
        default:
            return "[Unknown]";
    }
}

bool Value::equals(const Value& other) const {
    if (type != other.type) {
        return false;
    }
    
    switch (type) {
        case ValueType::NULL_VAL:
            return true;
            
        case ValueType::BOOL:
            return boolVal == other.boolVal;
            
        case ValueType::INT32:
            return int32Val == other.int32Val;
            
        case ValueType::FLOAT32:
            return std::fabs(float32Val - other.float32Val) < 1e-6f;
            
        case ValueType::STRING:
            if (!stringVal || !other.stringVal) {
                return stringVal == other.stringVal;
            }
            return *stringVal == *other.stringVal;
            
        case ValueType::OBJECT:
            return objVal == other.objVal;
            
        case ValueType::ARRAY:
            return arrayVal == other.arrayVal;
            
        case ValueType::NATIVE_FN:
            return nativeFn == other.nativeFn;
            
        default:
            return false;
    }
}

} // namespace vm
} // namespace dialos
