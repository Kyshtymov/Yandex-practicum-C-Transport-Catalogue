#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {
class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;
    Node() = default;

    Node(Value value) : value_(value) {}
    Node(int value) : value_(value) {}
    Node(double value) : value_(value) {}
    Node(bool value) : value_(value) {}
    Node(std::string value) : value_(std::move(value)) {}
    Node(std::nullptr_t) : value_(nullptr) {}
    Node(Array array) : value_(std::move(array)) {}
    Node(Dict dict) : value_(std::move(dict)) {}

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsArray() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsMap() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;
    int AsInt() const;
    double AsDouble() const;
    bool AsBool() const;
    const std::string& AsString() const;
    const Value& GetValue() const { return value_; }
private:
    Value value_;
};


class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);


bool operator==(const json::Document& lhs, const json::Document& rhs);
bool operator!=(const json::Document& lhs, const json::Document& rhs);


}  // namespace json

