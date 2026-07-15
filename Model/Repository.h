#pragma once

#include <optional>
#include <vector>

namespace Model {

// Controllers depend on this interface, never on concrete repositories (DIP).
template <typename TEntity, typename TId>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::vector<TEntity> FindAll() const = 0;
    virtual std::optional<TEntity> FindById(const TId& id) const = 0;
    virtual void Add(const TEntity& entity) = 0;
    virtual void Update(const TEntity& entity) = 0;
    virtual void Remove(const TId& id) = 0;
    virtual void Save() = 0;
    virtual void Load() = 0;
};

}  // namespace Model
