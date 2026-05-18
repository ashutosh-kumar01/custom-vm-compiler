#include "compiler.hpp"
#include <iostream>

Compiler::Compiler(Chunk* chunk) : chunk(chunk) {}

void Compiler::compile(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        compileStmt(stmt);
    }
    emitByte(static_cast<uint8_t>(OpCode::OP_HALT));
}

void Compiler::emitByte(uint8_t byte) {
    chunk->write(byte, 1); // hardcoded line 1 for now
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

uint16_t Compiler::makeConstant(Value value) {
    int constant = chunk->addConstant(value);
    if (constant > 65535) {
        std::cerr << "Too many constants in one chunk." << std::endl;
        return 0;
    }
    return static_cast<uint16_t>(constant);
}

void Compiler::emitConstant(Value value) {
    uint16_t constant = makeConstant(value);
    emitByte(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    emitByte((constant >> 8) & 0xff);
    emitByte(constant & 0xff);
}

uint16_t Compiler::makeStringConstant(const std::string& str) {
    return makeConstant(Value(std::make_shared<ObjString>(str)));
}

int Compiler::emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return chunk->code.size() - 2;
}

void Compiler::patchJump(int offset) {
    int jump = chunk->code.size() - offset - 2;
    if (jump > 65535) {
        std::cerr << "Too much code to jump over." << std::endl;
    }
    chunk->code[offset] = (jump >> 8) & 0xff;
    chunk->code[offset + 1] = jump & 0xff;
}

void Compiler::compileStmt(const std::shared_ptr<Stmt>& stmt) {
    if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        compileExpr(exprStmt->expression);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    } else if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (const auto& s : blockStmt->statements) {
            compileStmt(s);
        }
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        compileExpr(ifStmt->condition);
        int jumpIfFalse = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // Pop condition
        
        compileStmt(ifStmt->thenBranch);
        
        int jumpOverElse = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP));
        patchJump(jumpIfFalse);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // Pop condition if false
        
        if (ifStmt->elseBranch) {
            compileStmt(ifStmt->elseBranch);
        }
        patchJump(jumpOverElse);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        int loopStart = chunk->code.size();
        compileExpr(whileStmt->condition);
        int exitJump = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE));
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // Pop condition
        
        compileStmt(whileStmt->body);
        
        // emit loop back
        emitByte(static_cast<uint8_t>(OpCode::OP_LOOP));
        int offset = chunk->code.size() - loopStart + 2;
        emitByte((offset >> 8) & 0xff);
        emitByte(offset & 0xff);
        
        patchJump(exitJump);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // Pop condition
    } else if (auto funcStmt = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
        int jumpOver = emitJump(static_cast<uint8_t>(OpCode::OP_JUMP));
        int address = chunk->code.size();
        
        for (const auto& s : funcStmt->body->statements) {
            compileStmt(s);
        }
        // Emit default return
        emitConstant(Value());
        emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
        
        patchJump(jumpOver);
        
        std::vector<std::string> params;
        for (auto& p : funcStmt->params) params.push_back(p.lexeme);
        
        Value funcVal(std::make_shared<ObjFunction>(funcStmt->name.lexeme, params, address));
        emitConstant(funcVal);
        
        uint16_t nameConst = makeStringConstant(funcStmt->name.lexeme);
        emitByte(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL));
        emitByte((nameConst >> 8) & 0xff);
        emitByte(nameConst & 0xff);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP)); // Pop funcVal off stack
    } else if (auto retStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        if (retStmt->value) {
            compileExpr(retStmt->value);
        } else {
            emitConstant(Value()); // null
        }
        emitByte(static_cast<uint8_t>(OpCode::OP_RETURN));
    } else if (auto varDeclStmt = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        if (varDeclStmt->initializer) {
            compileExpr(varDeclStmt->initializer);
        } else {
            emitConstant(Value()); // null
        }
        uint16_t nameConst = makeStringConstant(varDeclStmt->name.lexeme);
        emitByte(static_cast<uint8_t>(OpCode::OP_DECLARE_VAR));
        emitByte((nameConst >> 8) & 0xff);
        emitByte(nameConst & 0xff);
        emitByte(static_cast<uint8_t>(OpCode::OP_POP));
    }
}

void Compiler::compileExpr(const std::shared_ptr<Expr>& expr) {
    if (auto litExpr = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        if (litExpr->value.type == TokenType::NUMBER) {
            emitConstant(Value(std::stoi(litExpr->value.lexeme)));
        } else if (litExpr->value.type == TokenType::FLOAT_NUMBER) {
            emitConstant(Value(std::stof(litExpr->value.lexeme)));
        } else if (litExpr->value.type == TokenType::STRING) {
            emitConstant(Value(std::make_shared<ObjString>(litExpr->value.lexeme)));
        } else if (litExpr->value.type == TokenType::TRUE_LIT) {
            emitConstant(Value(true));
        } else if (litExpr->value.type == TokenType::FALSE_LIT) {
            emitConstant(Value(false));
        } else if (litExpr->value.type == TokenType::CHAR_LITERAL) {
            emitConstant(Value(litExpr->value.lexeme[0]));
        }
    } else if (auto logExpr = std::dynamic_pointer_cast<LogicalExpr>(expr)) {
        compileExpr(logExpr->left);
        compileExpr(logExpr->right);
        if (logExpr->op.type == TokenType::AND) {
            emitByte(static_cast<uint8_t>(OpCode::OP_AND));
        } else if (logExpr->op.type == TokenType::OR) {
            emitByte(static_cast<uint8_t>(OpCode::OP_OR));
        }
    } else if (auto binExpr = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        compileExpr(binExpr->left);
        compileExpr(binExpr->right);
        switch (binExpr->op.type) {
            case TokenType::PLUS: emitByte(static_cast<uint8_t>(OpCode::OP_ADD)); break;
            case TokenType::MINUS: emitByte(static_cast<uint8_t>(OpCode::OP_SUB)); break;
            case TokenType::STAR: emitByte(static_cast<uint8_t>(OpCode::OP_MUL)); break;
            case TokenType::SLASH: emitByte(static_cast<uint8_t>(OpCode::OP_DIV)); break;
            case TokenType::MOD: emitByte(static_cast<uint8_t>(OpCode::OP_MOD)); break;
            case TokenType::EQUAL_EQUAL: emitByte(static_cast<uint8_t>(OpCode::OP_EQUAL)); break;
            case TokenType::BANG_EQUAL: emitBytes(static_cast<uint8_t>(OpCode::OP_EQUAL), static_cast<uint8_t>(OpCode::OP_NOT)); break;
            case TokenType::GREATER: emitByte(static_cast<uint8_t>(OpCode::OP_GREATER)); break;
            case TokenType::GREATER_EQUAL: emitBytes(static_cast<uint8_t>(OpCode::OP_LESS), static_cast<uint8_t>(OpCode::OP_NOT)); break;
            case TokenType::LESS: emitByte(static_cast<uint8_t>(OpCode::OP_LESS)); break;
            case TokenType::LESS_EQUAL: emitBytes(static_cast<uint8_t>(OpCode::OP_GREATER), static_cast<uint8_t>(OpCode::OP_NOT)); break;
            default: break;
        }
    } else if (auto assignExpr = std::dynamic_pointer_cast<AssignExpr>(expr)) {
        compileExpr(assignExpr->value);
        uint16_t nameConst = makeStringConstant(assignExpr->name.lexeme);
        emitByte(static_cast<uint8_t>(OpCode::OP_SET_GLOBAL));
        emitByte((nameConst >> 8) & 0xff);
        emitByte(nameConst & 0xff);
    } else if (auto indexAssignExpr = std::dynamic_pointer_cast<IndexAssignExpr>(expr)) {
        compileExpr(indexAssignExpr->object);
        compileExpr(indexAssignExpr->index);
        compileExpr(indexAssignExpr->value);
        emitByte(static_cast<uint8_t>(OpCode::OP_INDEX_SET));
    } else if (auto varExpr = std::dynamic_pointer_cast<VarExpr>(expr)) {
        uint16_t nameConst = makeStringConstant(varExpr->name.lexeme);
        emitByte(static_cast<uint8_t>(OpCode::OP_GET_GLOBAL));
        emitByte((nameConst >> 8) & 0xff);
        emitByte(nameConst & 0xff);
    } else if (auto subscriptExpr = std::dynamic_pointer_cast<SubscriptExpr>(expr)) {
        compileExpr(subscriptExpr->object);
        compileExpr(subscriptExpr->index);
        emitByte(static_cast<uint8_t>(OpCode::OP_INDEX_GET));
    } else if (auto callExpr = std::dynamic_pointer_cast<CallExpr>(expr)) {
        // Special case for built-in 'show'
        if (auto calleeVar = std::dynamic_pointer_cast<VarExpr>(callExpr->callee)) {
            if (calleeVar->name.lexeme == "show") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_PRINT));
                emitConstant(Value()); // show returns null
                return;
            } else if (calleeVar->name.lexeme == "ask") {
                emitByte(static_cast<uint8_t>(OpCode::OP_ASK));
                return;
            } else if (calleeVar->name.lexeme == "array") {
                compileExpr(callExpr->arguments[0]); // size
                if (callExpr->arguments.size() > 1) {
                    compileExpr(callExpr->arguments[1]); // default value
                } else {
                    emitConstant(Value(0)); // default to 0
                }
                emitByte(static_cast<uint8_t>(OpCode::OP_ARRAY));
                return;
            } else if (calleeVar->name.lexeme == "length") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_LENGTH));
                return;
            } else if (calleeVar->name.lexeme == "to_int") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_TO_INT));
                return;
            } else if (calleeVar->name.lexeme == "to_char") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_TO_CHAR));
                return;
            } else if (calleeVar->name.lexeme == "to_str") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_TO_STR));
                return;
            } else if (calleeVar->name.lexeme == "to_bool") {
                compileExpr(callExpr->arguments[0]);
                emitByte(static_cast<uint8_t>(OpCode::OP_TO_BOOL));
                return;
            }
        }
        
        compileExpr(callExpr->callee);
        for (auto& arg : callExpr->arguments) {
            compileExpr(arg);
        }
        emitByte(static_cast<uint8_t>(OpCode::OP_CALL));
        emitByte(static_cast<uint8_t>(callExpr->arguments.size()));
    }
}
