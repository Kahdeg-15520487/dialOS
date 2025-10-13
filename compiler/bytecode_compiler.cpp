#include "bytecode_compiler.h"
#include <sstream>
#include <cmath>

namespace dialos {
namespace compiler {

BytecodeModule BytecodeCompiler::compile(const Program& program) {
    // Reset state
    module_ = BytecodeModule();
    errors_.clear();
    locals_.clear();
    localCount_ = 0;
    jumpPatches_.clear();
    labels_.clear();
    
    // Compile all statements
    for (const auto& stmt : program.statements) {
        compileStatement(*stmt);
    }
    
    // Add halt at end
    module_.emit(Instruction(Opcode::HALT));
    
    // Patch jumps
    patchJumps();
    
    return module_;
}

void BytecodeCompiler::compileStatement(const Statement& stmt) {
    if (auto* varDecl = dynamic_cast<const VariableDeclaration*>(&stmt)) {
        compileVariableDecl(*varDecl);
    } else if (auto* assign = dynamic_cast<const Assignment*>(&stmt)) {
        compileAssignment(*assign);
    } else if (auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(&stmt)) {
        compileFunctionDecl(*funcDecl);
    } else if (auto* classDecl = dynamic_cast<const ClassDeclaration*>(&stmt)) {
        compileClassDecl(*classDecl);
    } else if (auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        compileIfStatement(*ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        compileWhileStatement(*whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        compileForStatement(*forStmt);
    } else if (auto* retStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        compileReturnStatement(*retStmt);
    } else if (auto* block = dynamic_cast<const Block*>(&stmt)) {
        compileBlock(*block);
    } else if (auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        compileExpressionStatement(*exprStmt);
    }
}

void BytecodeCompiler::compileVariableDecl(const VariableDeclaration& decl) {
    // Compile initializer
    if (decl.initializer) {
        compileExpression(*decl.initializer);
    } else {
        module_.emit(Instruction(Opcode::PUSH_NULL));
    }
    
    // Store to global variable
    uint16_t globalIdx = module_.addGlobal(decl.name);
    Instruction instr(Opcode::STORE_GLOBAL);
    instr.addOperandU16(globalIdx);
    module_.emit(instr);
}

void BytecodeCompiler::compileAssignment(const Assignment& assign) {
    // Compile value
    compileExpression(*assign.value);
    
    // Check if target is identifier
    if (auto* id = dynamic_cast<const Identifier*>(assign.target.get())) {
        // Check if it's a local variable
        auto it = locals_.find(id->name);
        if (it != locals_.end()) {
            Instruction instr(Opcode::STORE_LOCAL);
            instr.addOperandU8(it->second);
            module_.emit(instr);
        } else {
            // Global variable
            uint16_t globalIdx = module_.addGlobal(id->name);
            Instruction instr(Opcode::STORE_GLOBAL);
            instr.addOperandU16(globalIdx);
            module_.emit(instr);
        }
    } else if (auto* member = dynamic_cast<const MemberAccess*>(assign.target.get())) {
        // Member assignment: object.field = value
        compileExpression(*member->object);  // Push object
        // Value is already on stack
        uint16_t fieldIdx = module_.addConstant(member->property);
        Instruction instr(Opcode::SET_FIELD);
        instr.addOperandU16(fieldIdx);
        module_.emit(instr);
    } else if (auto* arrayAccess = dynamic_cast<const ArrayAccess*>(assign.target.get())) {
        // Array assignment: array[index] = value
        compileExpression(*arrayAccess->array);   // Push array
        compileExpression(*arrayAccess->index);   // Push index
        // Value is already on stack
        module_.emit(Instruction(Opcode::SET_INDEX));
    }
}

void BytecodeCompiler::compileFunctionDecl(const FunctionDeclaration& func) {
    // Add function to function table
    uint16_t funcIdx = module_.addFunction(func.name);
    
    // TODO: Proper function compilation with separate code blocks
    // For now, skip function bodies (will be handled in Phase 2)
    error("Function declarations not yet fully implemented in bytecode compiler");
}

void BytecodeCompiler::compileClassDecl(const ClassDeclaration& cls) {
    // TODO: Class compilation
    error("Class declarations not yet implemented in bytecode compiler");
}

void BytecodeCompiler::compileIfStatement(const IfStatement& ifStmt) {
    // Compile condition
    compileExpression(*ifStmt.condition);
    
    // Jump to else/end if false
    std::string elseLabel = "else_" + std::to_string(module_.getCurrentPosition());
    std::string endLabel = "end_" + std::to_string(module_.getCurrentPosition());
    
    emitJump(Opcode::JUMP_IF_NOT, elseLabel);
    
    // Compile consequence
    compileStatement(*ifStmt.consequence);
    
    if (ifStmt.alternative) {
        // Jump over else block
        emitJump(Opcode::JUMP, endLabel);
        
        // Else block
        placeLabel(elseLabel);
        compileStatement(*ifStmt.alternative);
        
        placeLabel(endLabel);
    } else {
        placeLabel(elseLabel);
    }
}

void BytecodeCompiler::compileWhileStatement(const WhileStatement& whileStmt) {
    std::string startLabel = "while_start_" + std::to_string(module_.getCurrentPosition());
    std::string endLabel = "while_end_" + std::to_string(module_.getCurrentPosition());
    
    // Loop start
    placeLabel(startLabel);
    
    // Compile condition
    compileExpression(*whileStmt.condition);
    
    // Jump to end if false
    emitJump(Opcode::JUMP_IF_NOT, endLabel);
    
    // Compile body
    compileStatement(*whileStmt.body);
    
    // Jump back to start
    emitJump(Opcode::JUMP, startLabel);
    
    // End
    placeLabel(endLabel);
}

void BytecodeCompiler::compileForStatement(const ForStatement& forStmt) {
    // TODO: For loop compilation
    error("For loops not yet implemented in bytecode compiler");
}

void BytecodeCompiler::compileReturnStatement(const ReturnStatement& ret) {
    if (ret.value) {
        compileExpression(*ret.value);
    } else {
        module_.emit(Instruction(Opcode::PUSH_NULL));
    }
    module_.emit(Instruction(Opcode::RETURN));
}

void BytecodeCompiler::compileBlock(const Block& block) {
    for (const auto& stmt : block.statements) {
        compileStatement(*stmt);
    }
}

void BytecodeCompiler::compileExpressionStatement(const ExpressionStatement& stmt) {
    compileExpression(*stmt.expression);
    module_.emit(Instruction(Opcode::POP));  // Discard result
}

void BytecodeCompiler::compileExpression(const Expression& expr) {
    if (auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        compileBinaryExpression(*binary);
    } else if (auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        compileUnaryExpression(*unary);
    } else if (auto* ternary = dynamic_cast<const TernaryExpression*>(&expr)) {
        compileTernaryExpression(*ternary);
    } else if (auto* call = dynamic_cast<const CallExpression*>(&expr)) {
        compileCallExpression(*call);
    } else if (auto* member = dynamic_cast<const MemberAccess*>(&expr)) {
        compileMemberAccess(*member);
    } else if (auto* arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        compileArrayAccess(*arrayAccess);
    } else if (auto* ctor = dynamic_cast<const ConstructorCall*>(&expr)) {
        compileConstructorCall(*ctor);
    } else if (auto* id = dynamic_cast<const Identifier*>(&expr)) {
        compileIdentifier(*id);
    } else if (auto* num = dynamic_cast<const NumberLiteral*>(&expr)) {
        compileNumberLiteral(*num);
    } else if (auto* str = dynamic_cast<const StringLiteral*>(&expr)) {
        compileStringLiteral(*str);
    } else if (auto* boolean = dynamic_cast<const BooleanLiteral*>(&expr)) {
        compileBooleanLiteral(*boolean);
    } else if (auto* null = dynamic_cast<const NullLiteral*>(&expr)) {
        compileNullLiteral(*null);
    } else if (auto* array = dynamic_cast<const ArrayLiteral*>(&expr)) {
        compileArrayLiteral(*array);
    } else if (auto* tmpl = dynamic_cast<const TemplateLiteral*>(&expr)) {
        compileTemplateLiteral(*tmpl);
    }
}

void BytecodeCompiler::compileBinaryExpression(const BinaryExpression& expr) {
    // Compile operands
    compileExpression(*expr.left);
    compileExpression(*expr.right);
    
    // Emit operator
    switch (expr.op) {
        case BinaryExpression::Operator::ADD:
            module_.emit(Instruction(Opcode::ADD));
            break;
        case BinaryExpression::Operator::SUB:
            module_.emit(Instruction(Opcode::SUB));
            break;
        case BinaryExpression::Operator::MUL:
            module_.emit(Instruction(Opcode::MUL));
            break;
        case BinaryExpression::Operator::DIV:
            module_.emit(Instruction(Opcode::DIV));
            break;
        case BinaryExpression::Operator::MOD:
            module_.emit(Instruction(Opcode::MOD));
            break;
        case BinaryExpression::Operator::EQ:
            module_.emit(Instruction(Opcode::EQ));
            break;
        case BinaryExpression::Operator::NE:
            module_.emit(Instruction(Opcode::NE));
            break;
        case BinaryExpression::Operator::LT:
            module_.emit(Instruction(Opcode::LT));
            break;
        case BinaryExpression::Operator::LE:
            module_.emit(Instruction(Opcode::LE));
            break;
        case BinaryExpression::Operator::GT:
            module_.emit(Instruction(Opcode::GT));
            break;
        case BinaryExpression::Operator::GE:
            module_.emit(Instruction(Opcode::GE));
            break;
        case BinaryExpression::Operator::AND:
            module_.emit(Instruction(Opcode::AND));
            break;
        case BinaryExpression::Operator::OR:
            module_.emit(Instruction(Opcode::OR));
            break;
    }
}

void BytecodeCompiler::compileUnaryExpression(const UnaryExpression& expr) {
    compileExpression(*expr.operand);
    
    switch (expr.op) {
        case UnaryExpression::Operator::NOT:
            module_.emit(Instruction(Opcode::NOT));
            break;
        case UnaryExpression::Operator::NEG:
            module_.emit(Instruction(Opcode::NEG));
            break;
        case UnaryExpression::Operator::PLUS:
            // No-op for unary plus
            break;
    }
}

void BytecodeCompiler::compileTernaryExpression(const TernaryExpression& expr) {
    // Compile condition
    compileExpression(*expr.condition);
    
    std::string elseLabel = "ternary_else_" + std::to_string(module_.getCurrentPosition());
    std::string endLabel = "ternary_end_" + std::to_string(module_.getCurrentPosition());
    
    // Jump to else if false
    emitJump(Opcode::JUMP_IF_NOT, elseLabel);
    
    // True branch
    compileExpression(*expr.consequence);
    emitJump(Opcode::JUMP, endLabel);
    
    // False branch
    placeLabel(elseLabel);
    compileExpression(*expr.alternative);
    
    placeLabel(endLabel);
}

void BytecodeCompiler::compileCallExpression(const CallExpression& expr) {
    // Push arguments
    for (const auto& arg : expr.arguments) {
        compileExpression(*arg);
    }
    
    // Get function name
    std::string funcName;
    if (auto* id = dynamic_cast<const Identifier*>(expr.callee.get())) {
        funcName = id->name;
    } else if (auto* member = dynamic_cast<const MemberAccess*>(expr.callee.get())) {
        // Method call: compile object first
        compileExpression(*member->object);
        funcName = member->property;
    }
    
    uint16_t funcIdx = module_.addFunction(funcName);
    Instruction instr(Opcode::CALL);
    instr.addOperandU16(funcIdx);
    instr.addOperandU8(static_cast<uint8_t>(expr.arguments.size()));
    module_.emit(instr);
}

void BytecodeCompiler::compileMemberAccess(const MemberAccess& expr) {
    // Push object
    compileExpression(*expr.object);
    
    // Get field
    uint16_t fieldIdx = module_.addConstant(expr.property);
    Instruction instr(Opcode::GET_FIELD);
    instr.addOperandU16(fieldIdx);
    module_.emit(instr);
}

void BytecodeCompiler::compileArrayAccess(const ArrayAccess& expr) {
    // Push array and index
    compileExpression(*expr.array);
    compileExpression(*expr.index);
    
    module_.emit(Instruction(Opcode::GET_INDEX));
}

void BytecodeCompiler::compileConstructorCall(const ConstructorCall& expr) {
    // Push arguments
    for (const auto& arg : expr.arguments) {
        compileExpression(*arg);
    }
    
    // Create object
    uint16_t classIdx = module_.addConstant(expr.typeName);
    Instruction instr(Opcode::NEW_OBJECT);
    instr.addOperandU16(classIdx);
    module_.emit(instr);
}

void BytecodeCompiler::compileIdentifier(const Identifier& expr) {
    // Check if it's a local variable
    auto it = locals_.find(expr.name);
    if (it != locals_.end()) {
        Instruction instr(Opcode::LOAD_LOCAL);
        instr.addOperandU8(it->second);
        module_.emit(instr);
    } else {
        // Global variable
        uint16_t globalIdx = module_.addGlobal(expr.name);
        Instruction instr(Opcode::LOAD_GLOBAL);
        instr.addOperandU16(globalIdx);
        module_.emit(instr);
    }
}

void BytecodeCompiler::compileNumberLiteral(const NumberLiteral& expr) {
    // Parse number
    if (expr.isFloat) {
        float value = std::stof(expr.value);
        Instruction instr(Opcode::PUSH_F32);
        instr.addOperand<float>(value);
        module_.emit(instr);
    } else if (expr.isHex) {
        int32_t value = std::stoi(expr.value, nullptr, 16);
        Instruction instr(Opcode::PUSH_I32);
        instr.addOperand<int32_t>(value);
        module_.emit(instr);
    } else {
        int32_t value = std::stoi(expr.value);
        
        // Choose smallest representation
        if (value >= -128 && value <= 127) {
            Instruction instr(Opcode::PUSH_I8);
            instr.addOperandU8(static_cast<uint8_t>(value));
            module_.emit(instr);
        } else if (value >= -32768 && value <= 32767) {
            Instruction instr(Opcode::PUSH_I16);
            instr.addOperandU16(static_cast<uint16_t>(value));
            module_.emit(instr);
        } else {
            Instruction instr(Opcode::PUSH_I32);
            instr.addOperand<int32_t>(value);
            module_.emit(instr);
        }
    }
}

void BytecodeCompiler::compileStringLiteral(const StringLiteral& expr) {
    uint16_t strIdx = module_.addConstant(expr.value);
    Instruction instr(Opcode::PUSH_STR);
    instr.addOperandU16(strIdx);
    module_.emit(instr);
}

void BytecodeCompiler::compileBooleanLiteral(const BooleanLiteral& expr) {
    module_.emit(Instruction(expr.value ? Opcode::PUSH_TRUE : Opcode::PUSH_FALSE));
}

void BytecodeCompiler::compileNullLiteral(const NullLiteral& expr) {
    module_.emit(Instruction(Opcode::PUSH_NULL));
}

void BytecodeCompiler::compileArrayLiteral(const ArrayLiteral& expr) {
    // Push all elements
    for (const auto& elem : expr.elements) {
        compileExpression(*elem);
    }
    
    // Push array size
    Instruction sizeInstr(Opcode::PUSH_I32);
    sizeInstr.addOperand<int32_t>(static_cast<int32_t>(expr.elements.size()));
    module_.emit(sizeInstr);
    
    // Create array
    module_.emit(Instruction(Opcode::NEW_ARRAY));
}

void BytecodeCompiler::compileTemplateLiteral(const TemplateLiteral& expr) {
    // TODO: Template literal compilation
    // For now, just push empty string
    uint16_t strIdx = module_.addConstant("");
    Instruction instr(Opcode::PUSH_STR);
    instr.addOperandU16(strIdx);
    module_.emit(instr);
}

void BytecodeCompiler::error(const std::string& message) {
    errors_.push_back(message);
}

uint8_t BytecodeCompiler::allocateLocal(const std::string& name) {
    uint8_t index = localCount_++;
    locals_[name] = index;
    return index;
}

void BytecodeCompiler::emitJump(Opcode jumpOp, const std::string& label) {
    Instruction instr(jumpOp);
    size_t patchPos = module_.getCurrentPosition() + 1;  // Position after opcode
    instr.addOperandU32(0);  // Placeholder offset
    module_.emit(instr);
    
    jumpPatches_.push_back({patchPos, label});
}

void BytecodeCompiler::placeLabel(const std::string& label) {
    labels_[label] = module_.getCurrentPosition();
}

void BytecodeCompiler::patchJumps() {
    for (const auto& patch : jumpPatches_) {
        auto it = labels_.find(patch.label);
        if (it != labels_.end()) {
            int32_t offset = static_cast<int32_t>(it->second) - static_cast<int32_t>(patch.position + 4);
            module_.patchJump(patch.position, offset);
        } else {
            error("Undefined label: " + patch.label);
        }
    }
}

} // namespace compiler
} // namespace dialos
