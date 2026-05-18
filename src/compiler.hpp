#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "ast.hpp"
#include "chunk.hpp"
#include <string>

class Compiler {
public:
    Compiler(Chunk* chunk);
    void compile(const std::vector<std::shared_ptr<Stmt>>& statements);

private:
    Chunk* chunk;
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitConstant(Value value);
    uint16_t makeConstant(Value value);
    uint16_t makeStringConstant(const std::string& str);
    
    void compileStmt(const std::shared_ptr<Stmt>& stmt);
    void compileExpr(const std::shared_ptr<Expr>& expr);
};

#endif
