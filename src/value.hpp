#ifndef VALUE_HPP
#define VALUE_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <variant>

struct Value;

struct Obj {
    enum class Type { STRING, ARRAY, FUNCTION };
    Type type;
    Obj(Type t) : type(t) {}
    virtual ~Obj() = default;
};

struct ObjString : public Obj {
    std::string str;
    ObjString(const std::string& s) : Obj(Type::STRING), str(s) {}
};

struct ObjArray : public Obj {
    std::vector<Value> elements;
    ObjArray(const std::vector<Value>& e) : Obj(Type::ARRAY), elements(e) {}
};

struct ObjFunction : public Obj {
    std::string name;
    std::vector<std::string> parameters;
    int address;
    ObjFunction(const std::string& n, const std::vector<std::string>& p, int addr) 
        : Obj(Type::FUNCTION), name(n), parameters(p), address(addr) {}
};

struct NullType {};

struct Value {
    std::variant<NullType, bool, int, float, char, std::shared_ptr<Obj>> data;

    Value() : data(NullType{}) {}
    Value(bool b) : data(b) {}
    Value(int i) : data(i) {}
    Value(float f) : data(f) {}
    Value(char c) : data(c) {}
    Value(std::shared_ptr<Obj> o) : data(o) {}

    bool isNull() const { return std::holds_alternative<NullType>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isInt() const { return std::holds_alternative<int>(data); }
    bool isFloat() const { return std::holds_alternative<float>(data); }
    bool isChar() const { return std::holds_alternative<char>(data); }
    bool isObj() const { return std::holds_alternative<std::shared_ptr<Obj>>(data); }

    bool getBool() const { return std::get<bool>(data); }
    int getInt() const { return std::get<int>(data); }
    float getFloat() const { return std::get<float>(data); }
    char getChar() const { return std::get<char>(data); }
    std::shared_ptr<Obj> getObj() const { return std::get<std::shared_ptr<Obj>>(data); }

    bool operator==(const Value& other) const {
        if (data.index() != other.data.index()) return false;
        if (isNull()) return true;
        if (isBool()) return getBool() == other.getBool();
        if (isInt()) return getInt() == other.getInt();
        if (isFloat()) return getFloat() == other.getFloat();
        if (isChar()) return getChar() == other.getChar();
        if (isObj()) {
            auto a = getObj();
            auto b = other.getObj();
            if (a->type != b->type) return false;
            if (a->type == Obj::Type::STRING) {
                return std::static_pointer_cast<ObjString>(a)->str == std::static_pointer_cast<ObjString>(b)->str;
            }
            return a == b; // fallback for arrays
        }
        return false;
    }

    void print() const {
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, NullType>) {
                std::cout << "null";
            } else if constexpr (std::is_same_v<T, bool>) {
                std::cout << (arg ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Obj>>) {
                if (arg->type == Obj::Type::STRING) {
                    std::cout << std::static_pointer_cast<ObjString>(arg)->str;
                } else if (arg->type == Obj::Type::ARRAY) {
                    std::cout << "[";
                    auto& elems = std::static_pointer_cast<ObjArray>(arg)->elements;
                    for (size_t i = 0; i < elems.size(); ++i) {
                        elems[i].print();
                        if (i != elems.size() - 1) std::cout << ", ";
                    }
                    std::cout << "]";
                } else if (arg->type == Obj::Type::FUNCTION) {
                    auto func = std::static_pointer_cast<ObjFunction>(arg);
                    std::cout << "<fn " << func->name << ">";
                }
            } else if constexpr (std::is_same_v<T, char>) {
                std::cout << "'" << arg << "'";
            } else {
                std::cout << arg;
            }
        }, data);
    }
};

#endif
