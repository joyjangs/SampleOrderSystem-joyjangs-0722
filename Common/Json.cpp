#include "Common/Json.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Common {

namespace {

std::string FormatNumber(double value) {
    if (std::abs(value) < 1e15 && value == static_cast<double>(static_cast<long long>(value))) {
        return std::to_string(static_cast<long long>(value));
    }
    std::ostringstream out;
    out << std::setprecision(15) << value;
    return out.str();
}

void AppendUtf8(std::string& out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

void DumpString(const std::string& text, std::string& out) {
    out += '"';
    for (char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream escape;
                    escape << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(static_cast<unsigned char>(c));
                    out += escape.str();
                } else {
                    out += c;
                }
        }
    }
    out += '"';
}

// Recursive-descent parser over a fixed input string.
class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue ParseValue() {
        SkipWhitespace();
        if (pos_ >= text_.size()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }
        char c = text_[pos_];
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '"') return JsonValue(ParseString());
        if (c == 't' || c == 'f') return ParseBool();
        if (c == 'n') return ParseNull();
        return ParseNumber();
    }

    void ExpectEnd() {
        SkipWhitespace();
        if (pos_ != text_.size()) {
            throw std::runtime_error("Trailing characters after JSON value");
        }
    }

private:
    const std::string& text_;
    size_t pos_ = 0;

    void SkipWhitespace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    char Peek() const { return text_[pos_]; }

    void Expect(char c) {
        if (pos_ >= text_.size() || text_[pos_] != c) {
            throw std::runtime_error(std::string("Expected '") + c + "' in JSON input");
        }
        ++pos_;
    }

    JsonValue ParseObject() {
        Expect('{');
        std::map<std::string, JsonValue> obj;
        SkipWhitespace();
        if (pos_ < text_.size() && Peek() == '}') {
            ++pos_;
            return JsonValue(obj);
        }
        while (true) {
            SkipWhitespace();
            std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            JsonValue value = ParseValue();
            obj.emplace(std::move(key), std::move(value));
            SkipWhitespace();
            if (pos_ < text_.size() && Peek() == ',') {
                ++pos_;
                continue;
            }
            break;
        }
        SkipWhitespace();
        Expect('}');
        return JsonValue(obj);
    }

    JsonValue ParseArray() {
        Expect('[');
        std::vector<JsonValue> arr;
        SkipWhitespace();
        if (pos_ < text_.size() && Peek() == ']') {
            ++pos_;
            return JsonValue(arr);
        }
        while (true) {
            arr.push_back(ParseValue());
            SkipWhitespace();
            if (pos_ < text_.size() && Peek() == ',') {
                ++pos_;
                continue;
            }
            break;
        }
        SkipWhitespace();
        Expect(']');
        return JsonValue(arr);
    }

    std::string ParseString() {
        Expect('"');
        std::string result;
        while (true) {
            if (pos_ >= text_.size()) {
                throw std::runtime_error("Unterminated string in JSON input");
            }
            char c = text_[pos_++];
            if (c == '"') break;
            if (c != '\\') {
                result += c;
                continue;
            }
            if (pos_ >= text_.size()) {
                throw std::runtime_error("Unterminated escape in JSON input");
            }
            char escaped = text_[pos_++];
            switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) {
                        throw std::runtime_error("Invalid unicode escape in JSON input");
                    }
                    unsigned int codepoint =
                        static_cast<unsigned int>(std::stoul(text_.substr(pos_, 4), nullptr, 16));
                    pos_ += 4;
                    AppendUtf8(result, codepoint);
                    break;
                }
                default:
                    throw std::runtime_error("Invalid escape sequence in JSON input");
            }
        }
        return result;
    }

    JsonValue ParseBool() {
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            return JsonValue(true);
        }
        if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            return JsonValue(false);
        }
        throw std::runtime_error("Invalid literal in JSON input");
    }

    JsonValue ParseNull() {
        if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            return JsonValue(nullptr);
        }
        throw std::runtime_error("Invalid literal in JSON input");
    }

    JsonValue ParseNumber() {
        size_t start = pos_;
        if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) {
            ++pos_;
        }
        while (pos_ < text_.size() &&
               (std::isdigit(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '.' ||
                text_[pos_] == 'e' || text_[pos_] == 'E' || text_[pos_] == '+' || text_[pos_] == '-')) {
            ++pos_;
        }
        std::string numberText = text_.substr(start, pos_ - start);
        if (numberText.empty()) {
            throw std::runtime_error("Invalid number in JSON input");
        }
        return JsonValue(std::stod(numberText));
    }
};

}  // namespace

JsonValue::JsonValue() : type_(Type::Null) {}
JsonValue::JsonValue(std::nullptr_t) : type_(Type::Null) {}
JsonValue::JsonValue(bool value) : type_(Type::Bool), boolValue_(value) {}
JsonValue::JsonValue(double value) : type_(Type::Number), numberValue_(value) {}
JsonValue::JsonValue(int value) : type_(Type::Number), numberValue_(static_cast<double>(value)) {}
JsonValue::JsonValue(std::string value) : type_(Type::String), stringValue_(std::move(value)) {}
JsonValue::JsonValue(const char* value) : type_(Type::String), stringValue_(value) {}
JsonValue::JsonValue(std::vector<JsonValue> value) : type_(Type::Array), arrayValue_(std::move(value)) {}
JsonValue::JsonValue(std::map<std::string, JsonValue> value)
    : type_(Type::Object), objectValue_(std::move(value)) {}

JsonValue JsonValue::MakeArray() { return JsonValue(std::vector<JsonValue>{}); }
JsonValue JsonValue::MakeObject() { return JsonValue(std::map<std::string, JsonValue>{}); }

bool JsonValue::AsBool() const {
    if (type_ != Type::Bool) throw std::runtime_error("JsonValue is not a bool");
    return boolValue_;
}

double JsonValue::AsDouble() const {
    if (type_ != Type::Number) throw std::runtime_error("JsonValue is not a number");
    return numberValue_;
}

int JsonValue::AsInt() const { return static_cast<int>(AsDouble()); }

const std::string& JsonValue::AsString() const {
    if (type_ != Type::String) throw std::runtime_error("JsonValue is not a string");
    return stringValue_;
}

const std::vector<JsonValue>& JsonValue::AsArray() const {
    if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
    return arrayValue_;
}

std::vector<JsonValue>& JsonValue::AsArray() {
    if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
    return arrayValue_;
}

const std::map<std::string, JsonValue>& JsonValue::AsObject() const {
    if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
    return objectValue_;
}

bool JsonValue::Contains(const std::string& key) const {
    return type_ == Type::Object && objectValue_.find(key) != objectValue_.end();
}

const JsonValue& JsonValue::At(const std::string& key) const {
    if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
    return objectValue_.at(key);
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (type_ == Type::Null) {
        type_ = Type::Object;
    }
    if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
    return objectValue_[key];
}

std::optional<std::string> JsonValue::GetOptionalString(const std::string& key) const {
    if (!Contains(key) || At(key).IsNull()) {
        return std::nullopt;
    }
    return At(key).AsString();
}

void JsonValue::PushBack(JsonValue value) {
    if (type_ == Type::Null) {
        type_ = Type::Array;
    }
    if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
    arrayValue_.push_back(std::move(value));
}

void JsonValue::DumpTo(std::string& out) const {
    switch (type_) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += boolValue_ ? "true" : "false";
            break;
        case Type::Number:
            out += FormatNumber(numberValue_);
            break;
        case Type::String:
            DumpString(stringValue_, out);
            break;
        case Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& item : arrayValue_) {
                if (!first) out += ',';
                first = false;
                item.DumpTo(out);
            }
            out += ']';
            break;
        }
        case Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& [key, value] : objectValue_) {
                if (!first) out += ',';
                first = false;
                DumpString(key, out);
                out += ':';
                value.DumpTo(out);
            }
            out += '}';
            break;
        }
    }
}

std::string JsonValue::Dump() const {
    std::string out;
    DumpTo(out);
    return out;
}

JsonValue JsonValue::Parse(const std::string& text) {
    Parser parser(text);
    JsonValue value = parser.ParseValue();
    parser.ExpectEnd();
    return value;
}

JsonValue JsonValue::ParseFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open JSON file for reading: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return Parse(buffer.str());
}

void JsonValue::SaveFile(const std::string& path, const JsonValue& value) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Failed to open JSON file for writing: " + path);
    }
    file << value.Dump();
}

}  // namespace Common
