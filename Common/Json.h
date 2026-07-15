#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Common {

// Dependency-free JSON value tree: parses/serializes null, bool, number,
// string, array, and object values. Used as the persistence format for all
// Model entities.
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(double value);
    JsonValue(int value);
    JsonValue(std::string value);
    JsonValue(const char* value);
    JsonValue(std::vector<JsonValue> value);
    JsonValue(std::map<std::string, JsonValue> value);

    static JsonValue MakeArray();
    static JsonValue MakeObject();

    Type GetType() const { return type_; }
    bool IsNull() const { return type_ == Type::Null; }
    bool IsObject() const { return type_ == Type::Object; }
    bool IsArray() const { return type_ == Type::Array; }

    bool AsBool() const;
    double AsDouble() const;
    int AsInt() const;
    const std::string& AsString() const;
    const std::vector<JsonValue>& AsArray() const;
    std::vector<JsonValue>& AsArray();
    const std::map<std::string, JsonValue>& AsObject() const;

    bool Contains(const std::string& key) const;
    const JsonValue& At(const std::string& key) const;
    JsonValue& operator[](const std::string& key);

    // Returns std::nullopt when the key is missing or its value is null —
    // used for optional fields such as ProductionJob::startedAt.
    std::optional<std::string> GetOptionalString(const std::string& key) const;

    void PushBack(JsonValue value);

    std::string Dump() const;

    static JsonValue Parse(const std::string& text);
    static JsonValue ParseFile(const std::string& path);
    static void SaveFile(const std::string& path, const JsonValue& value);

private:
    Type type_ = Type::Null;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    std::vector<JsonValue> arrayValue_;
    std::map<std::string, JsonValue> objectValue_;

    void DumpTo(std::string& out) const;
};

}  // namespace Common
