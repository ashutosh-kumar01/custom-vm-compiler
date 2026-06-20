#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "value.hpp"
#include <vector>
#include <cstdint>

enum class OpCode : uint8_t {
    OP_CONSTANT,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_NEGATE, OP_PRINT, OP_ASK, OP_HALT, OP_POP,
    OP_SET_GLOBAL, OP_GET_GLOBAL, OP_DECLARE_VAR,
    OP_EQUAL, OP_GREATER, OP_LESS, OP_NOT, OP_AND, OP_OR,
    OP_JUMP_IF_FALSE, OP_JUMP, OP_LOOP,
    OP_CALL, OP_RETURN,
    OP_ARRAY, OP_LENGTH, OP_INDEX_GET, OP_INDEX_SET, OP_TO_INT,
    OP_TO_CHAR, OP_TO_STR, OP_TO_BOOL
};

class Chunk {
public:
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

    void write(uint8_t byte, int line);
    int addConstant(Value value);
    
    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);
};

#endif
