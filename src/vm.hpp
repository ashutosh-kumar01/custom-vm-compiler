#ifndef VM_HPP
#define VM_HPP

#include "chunk.hpp"
#include <unordered_map>

class VM {
public:
    VM(Chunk* chunk);
    void run();

private:
    Chunk* chunk;
    uint8_t* ip;
    std::vector<Value> stack;
    std::unordered_map<std::string, Value> globals;
    std::vector<std::string> globalNames; // Simple trick for indexing globals if needed
    std::vector<std::unordered_map<std::string, Value>> envStack;
    std::vector<int> callStack;

    void push(Value value);
    Value pop();
    uint8_t readByte();
    Value readConstant();
    std::string readString();
};

#endif
