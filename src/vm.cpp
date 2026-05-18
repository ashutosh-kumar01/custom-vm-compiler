#include "vm.hpp"
#include <iostream>
#include <stdexcept>

VM::VM(Chunk* chunk) : chunk(chunk) {
    if (!chunk->code.empty()) {
        ip = &chunk->code[0];
    } else {
        ip = nullptr;
    }
}

void VM::push(Value value) {
    stack.push_back(value);
}

Value VM::pop() {
    if (stack.empty()) throw std::runtime_error("Stack underflow");
    Value val = stack.back();
    stack.pop_back();
    return val;
}

uint8_t VM::readByte() {
    return *ip++;
}

Value VM::readConstant() {
    uint16_t index = (readByte() << 8) | readByte();
    return chunk->constants[index];
}

std::string VM::readString() {
    Value val = readConstant();
    if (val.isObj() && val.getObj()->type == Obj::Type::STRING) {
        return std::static_pointer_cast<ObjString>(val.getObj())->str;
    }
    throw std::runtime_error("Expected string constant");
}

void VM::run() {
    if (!ip) return;
    while (true) {
        uint8_t instruction = readByte();
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::OP_CONSTANT: {
                Value constant = readConstant();
                push(constant);
                break;
            }
            case OpCode::OP_ADD: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) {
                    push(Value(a.getInt() + b.getInt()));
                } else if (a.isFloat() && b.isFloat()) {
                    push(Value(a.getFloat() + b.getFloat()));
                } else if (a.isChar() && b.isInt()) {
                    push(Value(static_cast<char>(a.getChar() + b.getInt())));
                } else if (a.isInt() && b.isChar()) {
                    push(Value(static_cast<char>(a.getInt() + b.getChar())));
                } else if (a.isChar() && b.isChar()) {
                    push(Value(static_cast<char>(a.getChar() + b.getChar())));
                } else if (a.isObj() && b.isObj() && a.getObj()->type == Obj::Type::STRING && b.getObj()->type == Obj::Type::STRING) {
                    auto strA = std::static_pointer_cast<ObjString>(a.getObj());
                    auto strB = std::static_pointer_cast<ObjString>(b.getObj());
                    push(Value(std::make_shared<ObjString>(strA->str + strB->str)));
                } else {
                    std::cerr << "OP_ADD failed: a.index=" << a.data.index() << " b.index=" << b.data.index() << std::endl;
                    throw std::runtime_error("Invalid types for OP_ADD");
                }
                break;
            }
            case OpCode::OP_SUB: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() - b.getInt()));
                else if (a.isFloat() && b.isFloat()) push(Value(a.getFloat() - b.getFloat()));
                else if (a.isChar() && b.isInt()) push(Value(static_cast<char>(a.getChar() - b.getInt())));
                else if (a.isChar() && b.isChar()) push(Value(a.getChar() - b.getChar())); // difference
                else throw std::runtime_error("Invalid types for OP_SUB");
                break;
            }
            case OpCode::OP_MUL: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() * b.getInt()));
                else if (a.isFloat() && b.isFloat()) push(Value(a.getFloat() * b.getFloat()));
                else throw std::runtime_error("Invalid types for OP_MUL");
                break;
            }
            case OpCode::OP_DIV: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() / b.getInt()));
                else if (a.isFloat() && b.isFloat()) push(Value(a.getFloat() / b.getFloat()));
                else throw std::runtime_error("Invalid types for OP_DIV");
                break;
            }
            case OpCode::OP_MOD: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() % b.getInt()));
                else throw std::runtime_error("Invalid types for OP_MOD");
                break;
            }
            case OpCode::OP_LESS: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() < b.getInt()));
                else if (a.isFloat() && b.isFloat()) push(Value(a.getFloat() < b.getFloat()));
                else if (a.isChar() && b.isChar()) push(Value(a.getChar() < b.getChar()));
                else throw std::runtime_error("Invalid types for OP_LESS");
                break;
            }
            case OpCode::OP_GREATER: {
                Value b = pop();
                Value a = pop();
                if (a.isInt() && b.isInt()) push(Value(a.getInt() > b.getInt()));
                else if (a.isFloat() && b.isFloat()) push(Value(a.getFloat() > b.getFloat()));
                else if (a.isChar() && b.isChar()) push(Value(a.getChar() > b.getChar()));
                else throw std::runtime_error("Invalid types for OP_GREATER");
                break;
            }
            case OpCode::OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::OP_NOT: {
                Value a = pop();
                bool isFalse = (a.isBool() && !a.getBool()) || a.isNull() || (a.isInt() && a.getInt() == 0);
                push(Value(isFalse));
                break;
            }
            case OpCode::OP_AND: {
                Value b = pop();
                Value a = pop();
                bool aTrue = a.isBool() ? a.getBool() : (!a.isNull() && !(a.isInt() && a.getInt() == 0));
                bool bTrue = b.isBool() ? b.getBool() : (!b.isNull() && !(b.isInt() && b.getInt() == 0));
                push(Value(aTrue && bTrue));
                break;
            }
            case OpCode::OP_OR: {
                Value b = pop();
                Value a = pop();
                bool aTrue = a.isBool() ? a.getBool() : (!a.isNull() && !(a.isInt() && a.getInt() == 0));
                bool bTrue = b.isBool() ? b.getBool() : (!b.isNull() && !(b.isInt() && b.getInt() == 0));
                push(Value(aTrue || bTrue));
                break;
            }
            case OpCode::OP_SET_GLOBAL: {
                std::string name = readString();
                if (!envStack.empty() && envStack.back().count(name)) {
                    envStack.back()[name] = stack.back();
                } else {
                    globals[name] = stack.back(); // keep on stack for assignment expression
                }
                break;
            }
            case OpCode::OP_DECLARE_VAR: {
                std::string name = readString();
                if (!envStack.empty()) {
                    envStack.back()[name] = stack.back();
                } else {
                    globals[name] = stack.back();
                }
                break;
            }
            case OpCode::OP_GET_GLOBAL: {
                std::string name = readString();
                if (!envStack.empty() && envStack.back().count(name)) {
                    push(envStack.back()[name]);
                } else if (globals.find(name) != globals.end()) {
                    push(globals[name]);
                } else {
                    throw std::runtime_error("Undefined variable '" + name + "'");
                }
                break;
            }
            case OpCode::OP_POP: {
                pop();
                break;
            }
            case OpCode::OP_PRINT: {
                Value val = pop();
                val.print();
                break;
            }
            case OpCode::OP_ASK: {
                std::string input;
                if (std::getline(std::cin, input)) {
                    push(Value(std::make_shared<ObjString>(input)));
                } else {
                    push(Value(std::make_shared<ObjString>("")));
                }
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t offset = (readByte() << 8) | readByte();
                Value condition = stack.back(); // peek, don't pop
                bool isFalse = (condition.isBool() && !condition.getBool()) || condition.isNull();
                if (isFalse) {
                    ip += offset;
                }
                break;
            }            case OpCode::OP_JUMP: {
                uint16_t offset = (readByte() << 8) | readByte();
                ip += offset;
                break;
            }
            case OpCode::OP_LOOP: {
                uint16_t offset = (readByte() << 8) | readByte();
                ip -= offset;
                break;
            }
            case OpCode::OP_CALL: {
                uint8_t argCount = readByte();
                Value callee = stack[stack.size() - 1 - argCount];
                if (!callee.isObj() || callee.getObj()->type != Obj::Type::FUNCTION) {
                    throw std::runtime_error("Can only call functions.");
                }
                auto func = std::static_pointer_cast<ObjFunction>(callee.getObj());
                if (func->parameters.size() != argCount) {
                    throw std::runtime_error("Expected " + std::to_string(func->parameters.size()) + " arguments but got " + std::to_string(argCount) + ".");
                }
                
                std::unordered_map<std::string, Value> newEnv;
                for (int i = argCount - 1; i >= 0; --i) {
                    newEnv[func->parameters[i]] = pop();
                }
                pop(); // Pop the function object itself
                
                envStack.push_back(newEnv);
                callStack.push_back(ip - chunk->code.data());
                ip = chunk->code.data() + func->address;
                break;
            }
            case OpCode::OP_RETURN: {
                Value result = pop(); // The return value
                if (callStack.empty()) {
                    return; // Should not happen directly, but safe
                }
                ip = chunk->code.data() + callStack.back();
                callStack.pop_back();
                envStack.pop_back();
                push(result);
                break;
            }
            case OpCode::OP_ARRAY: {
                Value defaultVal = pop();
                Value sizeVal = pop();
                if (!sizeVal.isInt()) throw std::runtime_error("array size must be an integer");
                int size = sizeVal.getInt();
                std::vector<Value> elems(size, defaultVal);
                push(Value(std::make_shared<ObjArray>(elems)));
                break;
            }
            case OpCode::OP_INDEX_GET: {
                Value indexVal = pop();
                Value objVal = pop();
                if (!indexVal.isInt()) throw std::runtime_error("index must be an integer");
                int idx = indexVal.getInt();
                
                if (objVal.isObj() && objVal.getObj()->type == Obj::Type::ARRAY) {
                    auto arr = std::static_pointer_cast<ObjArray>(objVal.getObj());
                    if (idx < 0 || idx >= (int)arr->elements.size()) throw std::runtime_error("Array index out of bounds");
                    push(arr->elements[idx]);
                } else if (objVal.isObj() && objVal.getObj()->type == Obj::Type::STRING) {
                    auto str = std::static_pointer_cast<ObjString>(objVal.getObj());
                    if (idx < 0 || idx >= (int)str->str.length()) throw std::runtime_error("String index out of bounds");
                    push(Value(std::make_shared<ObjString>(std::string(1, str->str[idx]))));
                } else {
                    throw std::runtime_error("Cannot index non-array/string type");
                }
                break;
            }
            case OpCode::OP_INDEX_SET: {
                Value val = pop();
                Value indexVal = pop();
                Value objVal = pop();
                if (!indexVal.isInt()) throw std::runtime_error("index must be an integer");
                int idx = indexVal.getInt();
                
                if (objVal.isObj() && objVal.getObj()->type == Obj::Type::ARRAY) {
                    auto arr = std::static_pointer_cast<ObjArray>(objVal.getObj());
                    if (idx < 0 || idx >= (int)arr->elements.size()) throw std::runtime_error("Array index out of bounds");
                    arr->elements[idx] = val;
                    push(val); // Assignment expressions return the assigned value
                } else {
                    throw std::runtime_error("Cannot assign to index of non-array type");
                }
                break;
            }
            case OpCode::OP_LENGTH: {
                Value objVal = pop();
                if (objVal.isObj() && objVal.getObj()->type == Obj::Type::ARRAY) {
                    auto arr = std::static_pointer_cast<ObjArray>(objVal.getObj());
                    push(Value((int)arr->elements.size()));
                } else if (objVal.isObj() && objVal.getObj()->type == Obj::Type::STRING) {
                    auto str = std::static_pointer_cast<ObjString>(objVal.getObj());
                    push(Value((int)str->str.length()));
                } else {
                    throw std::runtime_error("length() expects array or string");
                }
                break;
            }
            case OpCode::OP_TO_INT: {
                Value val = pop();
                if (val.isInt()) {
                    push(Value(val.getInt()));
                } else if (val.isFloat()) {
                    push(Value(static_cast<int>(val.getFloat())));
                } else if (val.isBool()) {
                    push(Value(val.getBool() ? 1 : 0));
                } else if (val.isObj() && val.getObj()->type == Obj::Type::STRING) {
                    auto str = std::static_pointer_cast<ObjString>(val.getObj());
                    try {
                        int num = std::stoi(str->str);
                        push(Value(num));
                    } catch (...) {
                        int ascii_sum = 0;
                        for (char c : str->str) {
                            ascii_sum += static_cast<int>(c);
                        }
                        push(Value(ascii_sum));
                    }
                } else {
                    push(Value(0));
                }
                break;
            }
            case OpCode::OP_TO_CHAR: {
                Value val = pop();
                if (val.isInt()) {
                    push(Value(static_cast<char>(val.getInt())));
                } else if (val.isChar()) {
                    push(val);
                } else if (val.isObj() && val.getObj()->type == Obj::Type::STRING) {
                    auto str = std::static_pointer_cast<ObjString>(val.getObj());
                    if (str->str.length() > 0) push(Value(str->str[0]));
                    else push(Value('\0'));
                } else {
                    push(Value('\0'));
                }
                break;
            }
            case OpCode::OP_TO_STR: {
                Value val = pop();
                if (val.isObj() && val.getObj()->type == Obj::Type::STRING) {
                    push(val);
                } else if (val.isInt()) {
                    push(Value(std::make_shared<ObjString>(std::to_string(val.getInt()))));
                } else if (val.isFloat()) {
                    push(Value(std::make_shared<ObjString>(std::to_string(val.getFloat()))));
                } else if (val.isChar()) {
                    push(Value(std::make_shared<ObjString>(std::string(1, val.getChar()))));
                } else if (val.isBool()) {
                    push(Value(std::make_shared<ObjString>(val.getBool() ? "true" : "false")));
                } else {
                    push(Value(std::make_shared<ObjString>("null")));
                }
                break;
            }
            case OpCode::OP_TO_BOOL: {
                Value val = pop();
                bool isTrue = false;
                if (val.isBool()) isTrue = val.getBool();
                else if (val.isInt()) isTrue = val.getInt() != 0;
                else if (val.isFloat()) isTrue = val.getFloat() != 0.0f;
                else if (val.isChar()) isTrue = val.getChar() != '\0';
                else if (val.isObj() && val.getObj()->type == Obj::Type::STRING) {
                    auto str = std::static_pointer_cast<ObjString>(val.getObj());
                    isTrue = !str->str.empty();
                }
                push(Value(isTrue));
                break;
            }
            case OpCode::OP_HALT: {
                return;
            }
            default:
                std::cerr << "Unknown opcode: " << (int)instruction << std::endl;
                return;
        }
    }
}
