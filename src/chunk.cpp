#include "chunk.hpp"
#include <fstream>
#include <stdexcept>

void Chunk::write(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

int Chunk::addConstant(Value value) {
    constants.push_back(value);
    return constants.size() - 1;
}

template<typename T>
void writeBin(std::ostream& os, const T& val) {
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template<typename T>
T readBin(std::istream& is) {
    T val;
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

void writeStringBin(std::ostream& os, const std::string& str) {
    uint32_t len = str.length();
    writeBin(os, len);
    os.write(str.data(), len);
}

std::string readStringBin(std::istream& is) {
    uint32_t len = readBin<uint32_t>(is);
    std::string str(len, '\0');
    is.read(&str[0], len);
    return str;
}

void Chunk::saveToFile(const std::string& path) const {
    std::ofstream os(path, std::ios::binary);
    if (!os) throw std::runtime_error("Cannot open file for writing: " + path);
    
    // Magic bytes to identify CVM bytecode files
    os.write("CVM1", 4);
    
    // Serialize Constants Pool
    uint32_t constSize = constants.size();
    writeBin(os, constSize);
    for (const auto& val : constants) {
        if (val.isNull()) {
            writeBin<uint8_t>(os, 0);
        } else if (val.isBool()) {
            writeBin<uint8_t>(os, 1);
            writeBin<bool>(os, val.getBool());
        } else if (val.isInt()) {
            writeBin<uint8_t>(os, 2);
            writeBin<int>(os, val.getInt());
        } else if (val.isFloat()) {
            writeBin<uint8_t>(os, 3);
            writeBin<float>(os, val.getFloat());
        } else if (val.isChar()) {
            writeBin<uint8_t>(os, 4);
            writeBin<char>(os, val.getChar());
        } else if (val.isObj()) {
            auto obj = val.getObj();
            if (obj->type == Obj::Type::STRING) {
                writeBin<uint8_t>(os, 5);
                writeStringBin(os, std::static_pointer_cast<ObjString>(obj)->str);
            } else if (obj->type == Obj::Type::FUNCTION) {
                writeBin<uint8_t>(os, 6);
                auto func = std::static_pointer_cast<ObjFunction>(obj);
                writeStringBin(os, func->name);
                writeBin<uint32_t>(os, func->parameters.size());
                for (const auto& p : func->parameters) {
                    writeStringBin(os, p);
                }
                writeBin<int>(os, func->address);
            } else {
                throw std::runtime_error("Cannot serialize array object in constant pool.");
            }
        }
    }
    
    // Serialize Bytecode
    uint32_t codeSize = code.size();
    writeBin(os, codeSize);
    os.write(reinterpret_cast<const char*>(code.data()), codeSize);
}

void Chunk::loadFromFile(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) throw std::runtime_error("Cannot open file for reading: " + path);
    
    char magic[4];
    is.read(magic, 4);
    if (std::string(magic, 4) != "CVM1") {
        throw std::runtime_error("Invalid CVM binary file. Magic header missing.");
    }
    
    // Deserialize Constants Pool
    uint32_t constSize = readBin<uint32_t>(is);
    constants.clear();
    for (uint32_t i = 0; i < constSize; ++i) {
        uint8_t tag = readBin<uint8_t>(is);
        switch (tag) {
            case 0: constants.push_back(Value()); break;
            case 1: constants.push_back(Value(readBin<bool>(is))); break;
            case 2: constants.push_back(Value(readBin<int>(is))); break;
            case 3: constants.push_back(Value(readBin<float>(is))); break;
            case 4: constants.push_back(Value(readBin<char>(is))); break;
            case 5: constants.push_back(Value(std::make_shared<ObjString>(readStringBin(is)))); break;
            case 6: {
                std::string name = readStringBin(is);
                uint32_t pCount = readBin<uint32_t>(is);
                std::vector<std::string> params;
                for (uint32_t p = 0; p < pCount; ++p) {
                    params.push_back(readStringBin(is));
                }
                int addr = readBin<int>(is);
                constants.push_back(Value(std::make_shared<ObjFunction>(name, params, addr)));
                break;
            }
            default: throw std::runtime_error("Corrupted CVM binary file. Unknown type tag.");
        }
    }
    
    // Deserialize Bytecode
    uint32_t codeSize = readBin<uint32_t>(is);
    code.resize(codeSize);
    is.read(reinterpret_cast<char*>(code.data()), codeSize);
}
