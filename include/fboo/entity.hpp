#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

class Entity {
public:
    Entity(std::string name) : name(name) {}
    virtual ~Entity() = default;

    virtual std::string to_string() const = 0;
    std::string get_name() const { return name; }

protected:
    std::string name;
};

class Item : public Entity {
public:
    explicit Item(std::string name) : Item(name, {}) {}
    Item(std::string name, std::string type) : Entity(name), type(type) {}

    std::string to_string() const override;
    std::string get_type() const { return type; }

private:
    std::string type;
};

using ItemMap = std::unordered_map<std::string, Item>;

class Ingredient : public Item {
public:
    Ingredient() : Ingredient({}, 0) {}
    Ingredient(std::string name, int amount) : Item(name), amount(amount) {}

    Ingredient(const std::pair<const std::string, int> &p)
        : Ingredient(p.first, p.second) {}

    // Required for ItemList (below).
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ingredient, name, amount);

    std::string to_string() const override;
    int get_amount() const { return amount; }

    template <size_t I>
    const auto &get() & {
        if constexpr (I == 0) return name;
        else if constexpr (I == 1) return amount;
    }
    template <size_t I>
    const auto &get() const & {
        if constexpr (I == 0) return name;
        else if constexpr (I == 1) return amount;
    }
    template <size_t I>
    const auto &get() && {
        if constexpr (I == 0) return std::move(name);
        else if constexpr (I == 1) return std::move(amount);
    }

private:
    int amount;
};

namespace std {
template <>
struct tuple_size<Ingredient> : integral_constant<size_t, 2> {};
template <size_t I>
struct tuple_element<I, Ingredient> : tuple_element<I, tuple<string, int>> {};
}  // namespace std

using ItemList = std::vector<Ingredient>;
using ItemCount = std::unordered_map<std::string, int>;

class Recipe : public Entity {
public:
    Recipe(std::string name, std::string category, int required_energy,
           bool enabled, ItemList ingredients, ItemList products)
        : Entity(name),
          category(category),
          required_energy(required_energy),
          enabled(enabled) {
        for (const auto &[name, amount] : ingredients) {
            this->ingredients[name] = amount;
        }
        for (const auto &[name, amount] : products) {
            this->products[name] = amount;
        }
    }

    std::string to_string() const override;
    std::string get_category() const { return category; }
    bool is_enabled() const { return enabled; }
    int get_required_energy() const { return required_energy; }
    const ItemCount &get_ingredients() const { return ingredients; }
    const ItemCount &get_products() const { return products; }

    void set_energy(const class Factory &f);
    int tick() { return --remaining_energy; }

private:
    std::string category;
    int required_energy;  // Amount of ticks to execute the recipe.
    int remaining_energy = 0;
    bool enabled;
    ItemCount ingredients, products;
};

using RecipeMap = std::unordered_map<std::string, Recipe>;

class Factory : public Entity {
public:
    Factory(std::string name, double crafting_speed,
            std::unordered_set<std::string> crafting_categories)
        : Entity(name),
          crafting_speed(crafting_speed),
          crafting_categories(crafting_categories) {}

    std::string to_string() const override;
    double get_crafting_speed() const { return crafting_speed; }
    int calc_ticks(const Recipe &r) const {
        return std::ceil(r.get_required_energy() / get_crafting_speed());
    }
    const std::unordered_set<std::string> &get_crafting_categories() const {
        return crafting_categories;
    }

private:
    double crafting_speed;
    std::unordered_set<std::string> crafting_categories;
};

using FactoryMap = std::unordered_map<std::string, Factory>;

class Technology : public Entity {
public:
    Technology(std::string name, std::vector<std::string> prerequisites,
               ItemList ingredients, std::vector<std::string> unlocked_recipes)
        : Entity(name),
          prerequisites(prerequisites.begin(), prerequisites.end()),
          unlocked_recipes(unlocked_recipes.begin(), unlocked_recipes.end()) {
        for (auto const &[name, amount] : ingredients) {
            this->ingredients[name] = amount;
        }
    }

    bool operator==(const Entity &o) const { return name == o.get_name(); }

    std::string to_string() const override;
    const std::unordered_set<std::string> &get_prerequisites() const {
        return prerequisites;
    }
    const ItemCount &get_ingredients() const { return ingredients; }
    const std::unordered_set<std::string> &get_unlocked_recipes() const {
        return unlocked_recipes;
    }

private:
    std::unordered_set<std::string> prerequisites;
    ItemCount ingredients;
    // The only effect in the json-file is "unlock-recipe", so we simplify this
    // part.
    std::unordered_set<std::string> unlocked_recipes;
};

using TechnologyMap = std::unordered_map<std::string, Technology>;

class FactoryIdMap {
public:
    using fid_t = int;

    fid_t get_next_fid() const { return count; }

    fid_t insert(const Factory *f) {
        insert_checked(f, count);
        return count++;
    }
    // This version also increments the internal count by one.
    fid_t insert(const Factory *f, fid_t fid) {
        insert_checked(f, fid);
        ++count;
        return fid;
    }
    const Factory *erase(fid_t fid) {
        const Factory *f = map.at(fid);
        map.erase(fid);
        return f;
    }

    const Factory *operator[](fid_t fid) const { return map.at(fid); }

    auto begin() { return map.begin(); }
    auto begin() const { return map.begin(); }
    auto end() { return map.end(); }
    auto end() const { return map.end(); }

private:
    void insert_checked(const Factory *f, fid_t fid) {
        if (!map.insert({fid, f}).second) {
            throw std::logic_error("factory id was used twice");
        }
    }

    fid_t count = 0;
    std::unordered_map<fid_t, const Factory *> map;
};
