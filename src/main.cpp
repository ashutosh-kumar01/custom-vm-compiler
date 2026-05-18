#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

void dumpBytecode(const Chunk& chunk) {
    std::cout << "--- Bytecode Chunk ---" << std::endl;
    for (size_t offset = 0; offset < chunk.code.size();) {
        std::cout << std::setfill('0') << std::setw(4) << offset << " ";
        uint8_t instruction = chunk.code[offset];
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::OP_CONSTANT: {
                uint16_t constant = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_CONSTANT " << (int)constant << " (";
                chunk.constants[constant].print();
                std::cout << ")" << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_ADD: std::cout << "OP_ADD" << std::endl; offset++; break;
            case OpCode::OP_SUB: std::cout << "OP_SUB" << std::endl; offset++; break;
            case OpCode::OP_MUL: std::cout << "OP_MUL" << std::endl; offset++; break;
            case OpCode::OP_DIV: std::cout << "OP_DIV" << std::endl; offset++; break;
            case OpCode::OP_EQUAL: std::cout << "OP_EQUAL" << std::endl; offset++; break;
            case OpCode::OP_GREATER: std::cout << "OP_GREATER" << std::endl; offset++; break;
            case OpCode::OP_LESS: std::cout << "OP_LESS" << std::endl; offset++; break;
            case OpCode::OP_PRINT: std::cout << "OP_PRINT" << std::endl; offset++; break;
            case OpCode::OP_ASK: std::cout << "OP_ASK" << std::endl; offset++; break;
            case OpCode::OP_SET_GLOBAL: {
                uint16_t constant = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_SET_GLOBAL " << (int)constant << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_DECLARE_VAR: {
                uint16_t constant = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_DECLARE_VAR " << (int)constant << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_GET_GLOBAL: {
                uint16_t constant = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_GET_GLOBAL " << (int)constant << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t jump = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_JUMP_IF_FALSE " << jump << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_JUMP: {
                uint16_t jump = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_JUMP " << jump << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_LOOP: {
                uint16_t jump = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
                std::cout << "OP_LOOP " << jump << std::endl;
                offset += 3;
                break;
            }
            case OpCode::OP_ARRAY: std::cout << "OP_ARRAY" << std::endl; offset++; break;
            case OpCode::OP_LENGTH: std::cout << "OP_LENGTH" << std::endl; offset++; break;
            case OpCode::OP_INDEX_GET: std::cout << "OP_INDEX_GET" << std::endl; offset++; break;
            case OpCode::OP_INDEX_SET: std::cout << "OP_INDEX_SET" << std::endl; offset++; break;
            case OpCode::OP_TO_INT: std::cout << "OP_TO_INT" << std::endl; offset++; break;
            case OpCode::OP_TO_CHAR: std::cout << "OP_TO_CHAR" << std::endl; offset++; break;
            case OpCode::OP_TO_STR: std::cout << "OP_TO_STR" << std::endl; offset++; break;
            case OpCode::OP_TO_BOOL: std::cout << "OP_TO_BOOL" << std::endl; offset++; break;
            case OpCode::OP_POP: std::cout << "OP_POP" << std::endl; offset++; break;
            case OpCode::OP_HALT: std::cout << "OP_HALT" << std::endl; offset++; break;
            default: std::cout << "Unknown opcode " << (int)instruction << std::endl; offset++; break;
        }
    }
    std::cout << "----------------------" << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Usage: cvm [file.cvm] [--dump-ast] [--dump-bytecode]" << std::endl;
        return 1;
    }

    bool dumpAst = false;
    bool dumpBc = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--dump-ast") dumpAst = true;
        if (std::string(argv[i]) == "--dump-bytecode") dumpBc = true;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();

    Parser parser(tokens);
    std::vector<std::shared_ptr<Stmt>> statements = parser.parse();

    if (dumpAst) {
        std::cout << "Program\n";
        for (size_t i = 0; i < statements.size(); ++i) {
            statements[i]->print("", i == statements.size() - 1);
        }
    }

    Chunk chunk;
    Compiler compiler(&chunk);
    compiler.compile(statements);

    if (dumpBc) {
        dumpBytecode(chunk);
    }

    if (!dumpAst && !dumpBc) {
        VM vm(&chunk);
        vm.run();
    }

    return 0;
}
