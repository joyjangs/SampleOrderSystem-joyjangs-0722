#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Common/Json.h"
#include "Model/Repository.h"

namespace Model {

// Shared JSON-file-backed CRUD implementation for entities that expose
// GetId()/ToJson()/static FromJson(). Every CRUD mutation flushes to disk
// immediately, so an interrupted process never loses committed state.
template <typename TEntity>
class JsonFileRepository : public IRepository<TEntity, std::string> {
public:
    explicit JsonFileRepository(std::string filePath) : filePath_(std::move(filePath)) {}

    std::vector<TEntity> FindAll() const override { return entities_; }

    std::optional<TEntity> FindById(const std::string& id) const override {
        auto it = std::find_if(entities_.begin(), entities_.end(),
                                [&id](const TEntity& entity) { return entity.GetId() == id; });
        if (it == entities_.end()) return std::nullopt;
        return *it;
    }

    void Add(const TEntity& entity) override {
        entities_.push_back(entity);
        Save();
    }

    void Update(const TEntity& entity) override {
        auto it = std::find_if(entities_.begin(), entities_.end(), [&entity](const TEntity& existing) {
            return existing.GetId() == entity.GetId();
        });
        if (it != entities_.end()) {
            *it = entity;
            Save();
        }
    }

    void Remove(const std::string& id) override {
        entities_.erase(std::remove_if(entities_.begin(), entities_.end(),
                                        [&id](const TEntity& entity) { return entity.GetId() == id; }),
                         entities_.end());
        Save();
    }

    void Save() override {
        std::filesystem::path path(filePath_);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        Common::JsonValue array = Common::JsonValue::MakeArray();
        for (const auto& entity : entities_) {
            array.PushBack(entity.ToJson());
        }
        Common::JsonValue::SaveFile(filePath_, array);
    }

    void Load() override {
        entities_.clear();
        if (!std::filesystem::exists(filePath_)) {
            return;
        }
        Common::JsonValue array = Common::JsonValue::ParseFile(filePath_);
        for (const auto& item : array.AsArray()) {
            entities_.push_back(TEntity::FromJson(item));
        }
    }

protected:
    std::string filePath_;
    std::vector<TEntity> entities_;
};

}  // namespace Model
