#pragma once

#include "json.h"
#include <vector>
#include <optional>
#include <string>

namespace json {

class Builder;
class BaseContext;
class DictItemContext;
class DictKeyContext;
class ArrayContext;

class Builder {
public:
    Builder();
    Node& Build();

    DictItemContext StartDict();
    ArrayContext StartArray();
    DictKeyContext Key(std::string key);

    Builder& Value(Node::Value value);
    Builder& EndDict();
    Builder& EndArray();

private:
    void CheckBuild();
    void AddNode(Node node);
    void AddNodeArray(Node node);
    bool AddNodeDict(Node node);
    Node root_;
    std::vector<Node*> nodes_stack_;
    std::optional<std::string> key_;
};

class BaseContext {
public:
    explicit BaseContext(Builder& builder) : builder_(builder) {}

    DictKeyContext Key(std::string key);
    Builder& Value(Node::Value value);
    DictItemContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();
    Builder& EndDict();
    Node& Build();

protected:
    Builder& builder_;
};

class DictItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;

    DictKeyContext Key(std::string key);
    Builder& EndDict();

    Builder& Value(Node::Value value) = delete;
    DictItemContext StartDict() = delete;
    ArrayContext StartArray() = delete;
    Builder& EndArray() = delete;
    Node& Build() = delete;
};

class DictKeyContext : public BaseContext {
public:
    using BaseContext::BaseContext;

    DictItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayContext StartArray();

    DictKeyContext Key(std::string key) = delete;
    Builder& EndArray() = delete;
    Builder& EndDict() = delete;
    Node& Build() = delete;
};

class ArrayContext : public BaseContext {
public:
    using BaseContext::BaseContext;

    ArrayContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayContext StartArray();
    Builder& EndArray();

    DictKeyContext& Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Node& Build() = delete;
};

} // namespace json
