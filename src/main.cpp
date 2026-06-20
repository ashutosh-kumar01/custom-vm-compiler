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
            case OpCode::OP_MOD: std::cout << "OP_MOD" << std::endl; offset++; break;
            case OpCode::OP_EQUAL: std::cout << "OP_EQUAL" << std::endl; offset++; break;
            case OpCode::OP_GREATER: std::cout << "OP_GREATER" << std::endl; offset++; break;
            case OpCode::OP_LESS: std::cout << "OP_LESS" << std::endl; offset++; break;
            case OpCode::OP_NOT: std::cout << "OP_NOT" << std::endl; offset++; break;
            case OpCode::OP_AND: std::cout << "OP_AND" << std::endl; offset++; break;
            case OpCode::OP_OR: std::cout << "OP_OR" << std::endl; offset++; break;
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
            case OpCode::OP_CALL: {
                uint8_t argCount = chunk.code[offset + 1];
                std::cout << "OP_CALL " << (int)argCount << std::endl;
                offset += 2;
                break;
            }
            case OpCode::OP_RETURN: std::cout << "OP_RETURN" << std::endl; offset++; break;
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

    if (argc < 3) {
        std::cerr << "Usage:\n"
                  << "  cvm compile <file.cvm> [output.cvmc] [--dump-ast] [--dump-bytecode]\n"
                  << "  cvm run <file.cvmc>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "compile") {
        std::string inputFile = argv[2];
        std::string outputFile = inputFile + "c"; // Default to .cvmc
        bool dumpAst = false;
        bool dumpBc = false;

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--dump-ast") dumpAst = true;
            else if (arg == "--dump-bytecode") dumpBc = true;
            else outputFile = arg;
        }

        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << inputFile << std::endl;
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

        chunk.saveToFile(outputFile);
        std::cout << "Successfully compiled to " << outputFile << std::endl;

    } else if (command == "run") {
        std::string inputFile = argv[2];
        Chunk chunk;
        try {
            chunk.loadFromFile(inputFile);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        VM vm(&chunk);
        vm.run();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
