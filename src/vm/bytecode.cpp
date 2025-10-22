#include "vm/bytecode.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace dialos {
namespace compiler {

// Serialize bytecode module to binary format
std::vector<uint8_t> BytecodeModule::serialize() const {
    std::vector<uint8_t> data;
    
    // Magic number "DSBC"
    data.push_back('D');
    data.push_back('S');
    data.push_back('B');
    data.push_back('C');
    
    // Format version
    data.push_back((metadata.version >> 8) & 0xFF);
    data.push_back(metadata.version & 0xFF);
    
    // Flags
    uint16_t flags = 0;
    if (hasDebugInfo()) {
        flags |= 0x0001; // Bit 0: has debug info
    }
    data.push_back(flags & 0xFF);
    data.push_back((flags >> 8) & 0xFF);
    
    // Helper to write uint32
    auto writeU32 = [&data](uint32_t value) {
        data.push_back(value & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 24) & 0xFF);
    };
    
    // Helper to write uint16
    auto writeU16 = [&data](uint16_t value) {
        data.push_back(value & 0xFF);
        data.push_back((value >> 8) & 0xFF);
    };
    
    // Helper to write string
    auto writeString = [&](const std::string& str) {
        writeU16(static_cast<uint16_t>(str.length()));
        data.insert(data.end(), str.begin(), str.end());
    };
    
    // Create a copy and update integrity before serializing
    BytecodeModule tempModule = *this;
    tempModule.updateIntegrity();
    
    // Metadata section
    writeU32(tempModule.metadata.heapSize);
    writeString(tempModule.metadata.appName);
    writeString(tempModule.metadata.appVersion);
    writeString(tempModule.metadata.author);
    writeU32(tempModule.metadata.timestamp);
    writeU32(tempModule.metadata.hashCode);
    writeU16(tempModule.metadata.checksum);
    
    // Constants section
    writeU32(static_cast<uint32_t>(constants.size()));
    for (const auto& str : constants) {
        writeString(str);
    }
    
    // Globals section
    writeU32(static_cast<uint32_t>(globals.size()));
    for (const auto& name : globals) {
        writeString(name);
    }
    
    // Functions section
    writeU32(static_cast<uint32_t>(functions.size()));
    for (size_t i = 0; i < functions.size(); i++) {
        writeString(functions[i]);
        writeU32(i < functionEntryPoints.size() ? functionEntryPoints[i] : 0);
        data.push_back(i < functionParamCounts.size() ? functionParamCounts[i] : 0);
    }
    
    // Main entry point
    writeU32(mainEntryPoint);
    
    // Code section
    writeU32(static_cast<uint32_t>(code.size()));
    data.insert(data.end(), code.begin(), code.end());
    
    // Debug section (optional)
    if (hasDebugInfo()) {
        writeU32(static_cast<uint32_t>(debugLines.size()));
        for (uint32_t line : debugLines) {
            writeU32(line);
        }
    }
    
    return data;
}

// Deserialize bytecode module from binary format
BytecodeModule BytecodeModule::deserialize(const std::vector<uint8_t>& data) {
    BytecodeModule module;
    size_t pos = 0;
    
    // Check magic number
    if (data.size() < 8 || data[0] != 'D' || data[1] != 'S' || 
        data[2] != 'B' || data[3] != 'C') {
        throw std::runtime_error("Invalid bytecode file format");
    }
    pos += 4;
    
    // Read format version
    module.metadata.version = (data[pos] << 8) | data[pos + 1];
    pos += 2;
    
    // Read flags
    uint16_t flags = data[pos] | (data[pos + 1] << 8);
    pos += 2;
    bool hasDebugInfo = (flags & 0x0001) != 0;
    
    // Helper to read uint32
    auto readU32 = [&data, &pos]() -> uint32_t {
        uint32_t value = data[pos] | (data[pos+1] << 8) | 
                        (data[pos+2] << 16) | (data[pos+3] << 24);
        pos += 4;
        return value;
    };
    
    // Helper to read uint16
    auto readU16 = [&data, &pos]() -> uint16_t {
        uint16_t value = data[pos] | (data[pos+1] << 8);
        pos += 2;
        return value;
    };
    
    // Helper to read string
    auto readString = [&]() -> std::string {
        uint16_t length = readU16();
        std::string str(data.begin() + pos, data.begin() + pos + length);
        pos += length;
        return str;
    };
    
    // Metadata section
    module.metadata.heapSize = readU32();
    module.metadata.appName = readString();
    module.metadata.appVersion = readString();
    module.metadata.author = readString();
    module.metadata.timestamp = readU32();
    module.metadata.hashCode = readU32();
    module.metadata.checksum = readU16();
    
    // Constants section
    uint32_t constantCount = readU32();
    module.constants.reserve(constantCount);
    for (uint32_t i = 0; i < constantCount; i++) {
        module.constants.push_back(readString());
    }
    
    // Globals section
    uint32_t globalCount = readU32();
    module.globals.reserve(globalCount);
    for (uint32_t i = 0; i < globalCount; i++) {
        module.globals.push_back(readString());
    }
    
    // Functions section
    uint32_t functionCount = readU32();
    module.functions.reserve(functionCount);
    module.functionEntryPoints.reserve(functionCount);
    module.functionParamCounts.reserve(functionCount);
    for (uint32_t i = 0; i < functionCount; i++) {
        module.functions.push_back(readString());
        module.functionEntryPoints.push_back(readU32());
        module.functionParamCounts.push_back(data[pos++]);
    }
    
    // Main entry point
    module.mainEntryPoint = readU32();
    
    // Code section
    uint32_t codeSize = readU32();
    module.code.assign(data.begin() + pos, data.begin() + pos + codeSize);
    pos += codeSize;
    
    // Debug section (optional)
    if (hasDebugInfo && pos < data.size()) {
        uint32_t debugSize = readU32();
        module.debugLines.reserve(debugSize);
        for (uint32_t i = 0; i < debugSize; i++) {
            module.debugLines.push_back(readU32());
        }
    }
    
    // Verify bytecode and metadata integrity
    if (!module.verifyIntegrity()) {
        throw std::runtime_error("Bytecode integrity check failed - file may be corrupted");
    }
    
    return module;
}
// Disassemble bytecode for debugging
std::string BytecodeModule::disassemble() const {
    std::stringstream ss;
    
    ss << "=== Bytecode Disassembly ===\n\n";
    
    // Metadata
    ss << "Metadata:\n";
    ss << "  App Name:    " << metadata.appName << "\n";
    ss << "  Version:     " << metadata.appVersion << "\n";
    ss << "  Author:      " << (metadata.author.empty() ? "(none)" : metadata.author) << "\n";
    ss << "  Heap Size:   " << metadata.heapSize << " bytes\n";
    ss << "  Format Ver:  " << metadata.version << "\n";
    ss << "  Hash Code:   0x" << std::hex << metadata.hashCode << std::dec << " (metadata)\n";
    ss << "  Checksum:    0x" << std::hex << metadata.checksum << std::dec << " (bytecode)\n";
    ss << "  Integrity:   " << (verifyIntegrity() ? "VALID" : "CORRUPTED") << "\n";
    ss << "\n";
    
    // Constants
    ss << "Constants (" << constants.size() << "):\n";
    for (size_t i = 0; i < constants.size(); i++) {
        ss << "  [" << i << "] \"" << constants[i] << "\"\n";
    }
    ss << "\n";
    
    // Globals
    ss << "Globals (" << globals.size() << "):\n";
    for (size_t i = 0; i < globals.size(); i++) {
        ss << "  [" << i << "] " << globals[i] << "\n";
    }
    ss << "\n";
    
    // Functions
    ss << "Functions (" << functions.size() << "):\n";
    for (size_t i = 0; i < functions.size(); i++) {
        ss << "  [" << i << "] " << functions[i];
        if (i < functionEntryPoints.size()) {
            ss << " @ PC:" << functionEntryPoints[i];
        }
        ss << "\n";
    }
    ss << "\n";
    
    ss << "Main Entry Point: PC:" << mainEntryPoint << "\n";
    ss << "\n";
    
    // Debug info status
    if (hasDebugInfo()) {
        ss << "Debug Info: Enabled (" << debugLines.size() << " entries)\n";
    } else {
        ss << "Debug Info: Disabled\n";
    }
    ss << "\n";
    
    // Code
    ss << "Code (" << code.size() << " bytes):\n";
    size_t pos = 0;
    while (pos < code.size()) {
        ss << std::setw(6) << std::setfill('0') << pos << "  ";
        
        // Show source line if debug info available
        if (hasDebugInfo() && pos < debugLines.size() && debugLines[pos] > 0) {
            ss << "[L" << std::setw(3) << debugLines[pos] << "] ";
        } else {
            ss << "      ";
        }
        
        Opcode op = static_cast<Opcode>(code[pos++]);
        
        switch (op) {
            case Opcode::NOP:
                ss << "NOP\n";
                break;
            case Opcode::POP:
                ss << "POP\n";
                break;
            case Opcode::DUP:
                ss << "DUP\n";
                break;
            case Opcode::SWAP:
                ss << "SWAP\n";
                break;
                
            case Opcode::PUSH_NULL:
                ss << "PUSH_NULL\n";
                break;
            case Opcode::PUSH_TRUE:
                ss << "PUSH_TRUE\n";
                break;
            case Opcode::PUSH_FALSE:
                ss << "PUSH_FALSE\n";
                break;
            case Opcode::PUSH_I8:
                if (pos < code.size()) {
                    int8_t value = static_cast<int8_t>(code[pos++]);
                    ss << "PUSH_I8 " << static_cast<int>(value) << "\n";
                }
                break;
            case Opcode::PUSH_I16:
                if (pos + 1 < code.size()) {
                    int16_t value = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "PUSH_I16 " << value << "\n";
                }
                break;
            case Opcode::PUSH_I32:
                if (pos + 3 < code.size()) {
                    int32_t value = code[pos] | (code[pos+1] << 8) | 
                                   (code[pos+2] << 16) | (code[pos+3] << 24);
                    pos += 4;
                    ss << "PUSH_I32 " << value << "\n";
                }
                break;
            case Opcode::PUSH_STR:
                if (pos + 1 < code.size()) {
                    uint16_t idx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "PUSH_STR [" << idx << "]";
                    if (idx < constants.size()) {
                        ss << " \"" << constants[idx] << "\"";
                    }
                    ss << "\n";
                }
                break;
                
            case Opcode::LOAD_LOCAL:
                if (pos < code.size()) {
                    ss << "LOAD_LOCAL " << static_cast<int>(code[pos++]) << "\n";
                }
                break;
            case Opcode::STORE_LOCAL:
                if (pos < code.size()) {
                    ss << "STORE_LOCAL " << static_cast<int>(code[pos++]) << "\n";
                }
                break;
                
            case Opcode::LOAD_GLOBAL:
                if (pos + 1 < code.size()) {
                    uint16_t idx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "LOAD_GLOBAL [" << idx << "]";
                    if (idx < globals.size()) {
                        ss << " " << globals[idx];
                    }
                    ss << "\n";
                }
                break;
            case Opcode::STORE_GLOBAL:
                if (pos + 1 < code.size()) {
                    uint16_t idx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "STORE_GLOBAL [" << idx << "]";
                    if (idx < globals.size()) {
                        ss << " " << globals[idx];
                    }
                    ss << "\n";
                }
                break;
                
            case Opcode::ADD:
                ss << "ADD\n";
                break;
            case Opcode::SUB:
                ss << "SUB\n";
                break;
            case Opcode::MUL:
                ss << "MUL\n";
                break;
            case Opcode::DIV:
                ss << "DIV\n";
                break;
            case Opcode::MOD:
                ss << "MOD\n";
                break;
            case Opcode::NEG:
                ss << "NEG\n";
                break;
            
            case Opcode::STR_CONCAT:
                ss << "STR_CONCAT\n";
                break;
                
            case Opcode::EQ:
                ss << "EQ\n";
                break;
            case Opcode::NE:
                ss << "NE\n";
                break;
            case Opcode::LT:
                ss << "LT\n";
                break;
            case Opcode::LE:
                ss << "LE\n";
                break;
            case Opcode::GT:
                ss << "GT\n";
                break;
            case Opcode::GE:
                ss << "GE\n";
                break;
                
            case Opcode::NOT:
                ss << "NOT\n";
                break;
            case Opcode::AND:
                ss << "AND\n";
                break;
            case Opcode::OR:
                ss << "OR\n";
                break;
                
            case Opcode::JUMP:
            case Opcode::JUMP_IF:
            case Opcode::JUMP_IF_NOT:
                if (pos + 3 < code.size()) {
                    int32_t offset = code[pos] | (code[pos+1] << 8) | 
                                    (code[pos+2] << 16) | (code[pos+3] << 24);
                    pos += 4;
                    const char* name = (op == Opcode::JUMP) ? "JUMP" :
                                      (op == Opcode::JUMP_IF) ? "JUMP_IF" : "JUMP_IF_NOT";
                    ss << name << " " << offset << " (to " << (pos + offset) << ")\n";
                }
                break;
                
            case Opcode::CALL:
                if (pos + 2 < code.size()) {
                    uint16_t funcIdx = code[pos] | (code[pos+1] << 8);
                    uint8_t argCount = code[pos+2];
                    pos += 3;
                    ss << "CALL [" << funcIdx << "]";
                    if (funcIdx < functions.size()) {
                        ss << " " << functions[funcIdx];
                    }
                    ss << " argc=" << static_cast<int>(argCount) << "\n";
                }
                break;
            case Opcode::CALL_NATIVE:
                if (pos + 2 < code.size()) {
                    uint16_t funcIdx = code[pos] | (code[pos+1] << 8);
                    uint8_t argCount = code[pos+2];
                    pos += 3;
                    ss << "CALL_NATIVE [" << funcIdx << "]";
                    if (funcIdx < functions.size()) {
                        ss << " " << functions[funcIdx];
                    }
                    ss << " argc=" << static_cast<int>(argCount) << "\n";
                }
                break;
            case Opcode::RETURN:
                ss << "RETURN\n";
                break;
                
            case Opcode::LOAD_FUNCTION:
                if (pos + 1 < code.size()) {
                    uint16_t funcIdx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "LOAD_FUNCTION [" << funcIdx << "]";
                    if (funcIdx < functions.size()) {
                        ss << " " << functions[funcIdx];
                    }
                    ss << "\n";
                }
                break;
                
            case Opcode::CALL_INDIRECT:
                if (pos < code.size()) {
                    uint8_t argCount = code[pos];
                    pos += 1;
                    ss << "CALL_INDIRECT argc=" << static_cast<int>(argCount) << "\n";
                }
                break;
            case Opcode::CALL_METHOD:
                if (pos + 2 < code.size()) {
                    uint8_t argCount = code[pos++];
                    uint16_t nameIdx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "CALL_METHOD argc=" << static_cast<int>(argCount) << " nameIdx=[" << nameIdx << "]";
                    if (nameIdx < constants.size()) ss << " " << constants[nameIdx];
                    ss << "\n";
                }
                break;
                
            case Opcode::GET_FIELD:
            case Opcode::SET_FIELD:
                if (pos + 1 < code.size()) {
                    uint16_t idx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    const char* name = (op == Opcode::GET_FIELD) ? "GET_FIELD" : "SET_FIELD";
                    ss << name << " [" << idx << "]";
                    if (idx < constants.size()) {
                        ss << " " << constants[idx];
                    }
                    ss << "\n";
                }
                break;
            case Opcode::GET_INDEX:
                ss << "GET_INDEX\n";
                break;
            case Opcode::SET_INDEX:
                ss << "SET_INDEX\n";
                break;
                
            case Opcode::NEW_OBJECT:
                if (pos + 1 < code.size()) {
                    uint16_t classIdx = code[pos] | (code[pos+1] << 8);
                    pos += 2;
                    ss << "NEW_OBJECT [" << classIdx << "]\n";
                }
                break;
            case Opcode::NEW_ARRAY:
                ss << "NEW_ARRAY\n";
                break;
                
            case Opcode::TRY:
                if (pos + 3 < code.size()) {
                    int32_t offset = 0;
                    memcpy(&offset, &code[pos], 4);
                    pos += 4;
                    ss << "TRY +" << offset << "\n";
                }
                break;
            case Opcode::END_TRY:
                ss << "END_TRY\n";
                break;
            case Opcode::THROW:
                ss << "THROW\n";
                break;
                
            case Opcode::PRINT:
                ss << "PRINT\n";
                break;
            case Opcode::HALT:
                ss << "HALT\n";
                break;
                
            default:
                ss << "UNKNOWN(" << std::hex << static_cast<int>(op) << std::dec << ")\n";
                break;
        }
    }
    
    return ss.str();
}

} // namespace compiler
} // namespace dialos
