#include "json.h"
#include <iomanip>
#include <sstream>

using namespace std::literals;
namespace json {

using Number = std::variant<int, double>;
std::string LoadString(std::istream& input);
Number LoadNumber(std::istream& input);
Node LoadNode(std::istream& input);
Node LoadArray(std::istream& input);
Node LoadDict(std::istream& input);
Node LoadStringImpl(std::istream& input);
Node LoadNumberImpl(std::istream& input);


Node LoadArray(std::istream& input) {
    Array result;

    char c;
    input >> c;
    if (c == ']') {
        return Node(Array{});
    }
    input.putback(c);

    while (true) {
        result.push_back(LoadNode(input));

        input >> c;
        if (c == ']') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or ']' in array");
        }

        if (input.peek() == ']') {
            throw ParsingError("Trailing comma in array");
        }
    }

    return Node(std::move(result));
}

Node LoadStringImpl(std::istream& input) {
    std::string line = LoadString(input);
    return Node(std::move(line));
}

Node LoadNumberImpl(std::istream& input) {
    Number number = LoadNumber(input);
    if (std::holds_alternative<int>(number)) {
        return Node(std::get<int>(number));
    } else {
        return Node(std::get<double>(number));
    }
}

Node LoadDict(std::istream& input) {
    Dict result;

    char c;
    input >> c;
    if (c == '}') {
        return Node(Dict{});
    }
    input.putback(c);

    while (true) {

        input >> c;
        if (c != '"') {
            throw ParsingError("Expected '\"' for dictionary key");
        }
        std::string key = LoadString(input);

        input >> c;
        if (c != ':') {
            throw ParsingError("Expected ':' after dictionary key");
        }

        result.insert({std::move(key), LoadNode(input)});

        input >> c;
        if (c == '}') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or '}' in dictionary");
        }

        if (input.peek() == '}') {
            throw ParsingError("Trailing comma in dictionary");
        }
    }

    return Node(std::move(result));
}

Node LoadBoolOrNull(std::istream& input) {
    std::string value;
    char c;

    while (input.get(c)) {
        if (std::isspace(c) || c == ',' || c == ']' || c == '}') {
            input.putback(c);
            break;
        }
        value += c;
    }

    if (value == "true") {
        return Node(true);
    }
    else if (value == "false") {
        return Node(false);
    }
    else if (value == "null") {
        return Node(nullptr);
    }

    throw ParsingError("Invalid value: " + value);
}
Node LoadNode(std::istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("Unexpected end of input");
    }

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadStringImpl(input);
    } else if (c == 't' || c == 'f' || c == 'n') {
        input.putback(c);
        return LoadBoolOrNull(input);
    } else {
        input.putback(c);
        return LoadNumberImpl(input);
    }
}


// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};


// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintValue(const bool& value, const PrintContext& ctx) {
    ctx.out << (value ? "true" : "false");
}

void PrintValue(const std::string& value, const PrintContext& ctx) {
    ctx.out << "\"";
    for (char c : value) {
        switch (c) {
        case '"':  ctx.out << "\\\""; break;
        case '\\': ctx.out << "\\\\"; break;
        case '\n': ctx.out << "\\n"; break;
        case '\t': ctx.out << "\\t"; break;
        case '\r': ctx.out << "\\r"; break;
        default:   ctx.out << c; break;
        }
    }
    ctx.out << "\"";
}


void PrintNode(const Node& node, const PrintContext& ctx);

void PrintValue(const Array& value, const PrintContext& ctx) {
    if (value.empty()) {
        ctx.out << "[]";
        return;
    }

    bool first = true;
    ctx.out << std::endl;
    ctx.PrintIndent();

    ctx.out << "[";
    ctx.out << std::endl;
    PrintContext inner_ctx = ctx.Indented();

    for (const auto& node : value) {
        if (!first) {
            ctx.out << ",";
            ctx.out << std::endl;
        }
        first = false;

        inner_ctx.PrintIndent();
        PrintNode(node, inner_ctx);
    }

    ctx.out << std::endl;
    ctx.PrintIndent();
    ctx.out << "]";
}

void PrintValue(const Dict& value, const PrintContext& ctx) {
    if (value.empty()) {
        ctx.out << "{}";
        return;
    }

    bool first = true;
    ctx.out << "{";
    ctx.out << std::endl;
    PrintContext inner_ctx = ctx.Indented();

    for (const auto& [key, node] : value) {
        if (!first) {
            ctx.out << ",";
            ctx.out << std::endl;
        }
        first = false;

        inner_ctx.PrintIndent();
        PrintValue(key, inner_ctx);
        ctx.out << ": ";
        PrintNode(node, inner_ctx);
    }

    ctx.out << std::endl;
    ctx.PrintIndent();
    ctx.out << "}";
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}


void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value){ PrintValue(value, ctx); },
        node.GetValue());
}

void PrintNode(const Node& node, std::ostream& out) {
    PrintContext ctx{out};
    std::visit(
        [&ctx](const auto& value){ PrintValue(value, ctx); },
        node.GetValue());
}

bool Node::IsInt() const{
    return std::holds_alternative<int>(value_);
}
bool Node::IsDouble() const{
    return (std::holds_alternative<int>(value_) || std::holds_alternative<double>(value_));
}
bool Node::IsPureDouble() const{
    return std::holds_alternative<double>(value_);
}
bool Node::IsBool() const{
    return std::holds_alternative<bool>(value_);
}
bool Node::IsString() const{
    return std::holds_alternative<std::string>(value_);
}
bool Node::IsNull() const{
    return std::holds_alternative<nullptr_t>(value_);
}
bool Node::IsArray() const{
    return std::holds_alternative<Array>(value_);
}
bool Node::IsMap() const{
    return std::holds_alternative<Dict>(value_);
}


const Array& Node::AsArray() const {
    return IsArray() ? std::get<Array>(value_) : throw std::logic_error("Wrong type");
}

const Dict& Node::AsMap() const {
    return IsMap() ? std::get<Dict>(value_) : throw std::logic_error("Wrong type");
}

int Node::AsInt() const {
    return IsInt() ? std::get<int>(value_) : throw std::logic_error("Wrong type");
}

const std::string& Node::AsString() const {
    return IsString() ? std::get<std::string>(value_) : throw std::logic_error("Wrong type");
}


double Node::AsDouble() const {
    if (IsInt()) {
        return static_cast<double>(std::get<int>(value_));
    } else if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    } else {
        throw std::logic_error("Wrong type");
    }
}

bool Node::AsBool() const {
    return IsBool() ? std::get<bool>(value_) : throw std::logic_error("Wrong type");
}



Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(std::istream& input) {
    return Document(LoadNode(input));
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}
bool operator==(const json::Node& lhs, const json::Node& rhs) {
    return std::visit([&](const auto& lhs_value) {
        return std::visit([&](const auto& rhs_value) {
            using T1 = std::decay_t<decltype(lhs_value)>;
            using T2 = std::decay_t<decltype(rhs_value)>;

            if constexpr (std::is_same_v<T1, T2>) {
                return lhs_value == rhs_value;
            } else {
                return false;
            }
        }, rhs.GetValue());
    }, lhs.GetValue());
}


bool operator!=(const json::Node& lhs, const json::Node& rhs) {
    return !(lhs == rhs);
}

bool operator==(const json::Document& lhs, const json::Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}

bool operator!=(const json::Document& lhs, const json::Document& rhs) {
    return lhs.GetRoot() != rhs.GetRoot();
}

}  // namespace json



