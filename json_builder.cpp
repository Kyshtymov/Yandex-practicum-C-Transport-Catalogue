#include "json_builder.h"


namespace json{

DictKeyContext DictItemContext::Key(std::string key) {
    return builder_.Key(move(key));
}

Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

DictItemContext DictKeyContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext DictKeyContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext DictKeyContext::StartArray() {
    return builder_.StartArray();
}

ArrayContext ArrayContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayContext(builder_);
}

DictItemContext ArrayContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext ArrayContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayContext::EndArray() {
    return builder_.EndArray();
}

Node& BaseContext::Build() {
    return builder_.Build();
}


Builder::Builder() = default;

void Builder::CheckBuild(){
    if (root_.IsNull() || !nodes_stack_.empty())
        throw std::logic_error("Error in the Build request");
}

void Builder::AddNode(Node node){
    if (!root_.IsNull())
        throw std::logic_error("Error in the Build request");
    root_ = std::move(node);
    nodes_stack_.push_back(&root_);
}

void Builder::AddNodeArray(Node node){
    Array& node_array = const_cast<Array&>(nodes_stack_.back()->AsArray());
    node_array.push_back(std::move(node));
    nodes_stack_.push_back(&node_array.back());
}

bool Builder::AddNodeDict(Node node){
    Dict& node_dict = const_cast<Dict&>(nodes_stack_.back()->AsMap());
    if (key_){
        node_dict[key_.value()] = std::move(node);
        nodes_stack_.push_back(&node_dict[key_.value()]);
        return true;
    }
    return false;
}

json::Node& Builder::Build(){
    CheckBuild();
    return root_;
}

Builder& Builder::Value(Node::Value value){
    Node node(std::move(value));
    if (nodes_stack_.empty()){
        AddNode(node);
        nodes_stack_.pop_back();
        return *this;
    }

    if (nodes_stack_.back()->IsArray()){
        AddNodeArray(node);
        nodes_stack_.pop_back();
        return *this;
    }

    if (nodes_stack_.back()->IsMap()){
        if (AddNodeDict(node)){
            key_.reset();
            nodes_stack_.pop_back();
            return *this;
        }
    }

    throw std::logic_error("Error in the Value request");
}

DictItemContext Builder::StartDict(){
    if (nodes_stack_.empty()){
        AddNode(Node(Dict()));
        return DictItemContext(*this);
    }

    if (nodes_stack_.back()->IsArray()){
        AddNodeArray(Node(Dict()));
        return DictItemContext(*this);
    }

    if (nodes_stack_.back()->IsMap()){
        if (AddNodeDict(Node(Dict()))){
            key_.reset();
            return DictItemContext(*this);
        }
    }

    throw std::logic_error("Error in the StartDict request");
}

DictKeyContext Builder::Key(std::string key){

    if (key_ || !nodes_stack_.back()->IsMap())
        throw std::logic_error("Error in the Key request");

    key_ = std::move(key);
    return DictKeyContext(*this);
}

Builder& Builder::EndDict(){

    if (!nodes_stack_.back()->IsMap())
        throw std::logic_error("Wrong end container");

    nodes_stack_.pop_back();
    return *this;
}

ArrayContext Builder::StartArray(){
    if (nodes_stack_.empty()){
        AddNode(Node(Array()));
        return ArrayContext(*this);
    }

    if (nodes_stack_.back()->IsArray()){
        AddNodeArray(Array());
        return ArrayContext(*this);
    }

    if (nodes_stack_.back()->IsMap()){
        if (AddNodeDict(Node(Array()))){
            key_.reset();
            return ArrayContext(*this);
        }
    }

    throw std::logic_error("Error in the StartArray request");
}

Builder& Builder::EndArray(){

    if (!nodes_stack_.back()->IsArray())
        throw std::logic_error("Wrong end container");

    nodes_stack_.pop_back();
    return *this;
}

}
