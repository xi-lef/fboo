#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <set>
#include <utility>
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
    explicit Item(std::string name, std::string type = "")
        : Entity(name), type(type) {}

    std::string to_string() const override;
    std::string get_type() const { return type; }

private:
    std::string type;
};

using ItemMap = std::unordered_map<std::string, Item>;

class Ingredient : public Item {
public:
    explicit Ingredient(std::string name = "", int amount = 0)
        : Item(name), amount(amount) {}

    // Required for ItemList (below).
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ingredient, name, amount);

    std::string to_string() const override;
    int get_amount() const { return amount; }

private:
    int amount;
};

using ItemList = std::vector<Ingredient>;

class Recipe : public Entity {
public:
    Recipe(std::string name, std::string category, int energy, bool enabled,
           ItemList ingredients, ItemList products)
        : Entity(name),
          category(category),
          energy(energy),
          enabled(enabled),
          ingredients(ingredients),
          products(products) {}

    std::string to_string() const override;
    std::string get_category() const { return category; }
    bool is_enabled() const { return enabled; }
    int get_energy() const { return energy; }
    ItemList get_ingredients() const { return ingredients; }
    ItemList get_products() const { return products; }

    void set_energy(int e) { energy = e; }
    int tick() { return --energy; }

private:
    std::string category;
    // TODO required_energy and [cur,remaining]_energy?
    int energy;  // Amount of ticks to execute the recipe.
    bool enabled;
    ItemList ingredients, products;
};

using RecipeMap = std::unordered_map<std::string, Recipe>;

class Factory : public Entity {
public:
    Factory(std::string name, double crafting_speed,
            std::set<std::string> crafting_categories)
        : Entity(name),
          crafting_speed(crafting_speed),
          crafting_categories(crafting_categories) {}

    std::string to_string() const override;
    double get_crafting_speed() const { return crafting_speed; }
    std::set<std::string> get_crafting_categories() const {
        return crafting_categories;
    }

private:
    double crafting_speed;
    std::set<std::string> crafting_categories;
    // TODO Recipe *executing_recipe ?
};

using FactoryMap = std::unordered_map<std::string, Factory>;

class Technology : public Entity {
public:
    Technology(std::string name, std::vector<std::string> prerequisites,
               ItemList ingredients, std::vector<std::string> unlocked_recipes)
        : Entity(name),
          prerequisites(prerequisites),
          ingredients(ingredients),
          unlocked_recipes(unlocked_recipes) {}

    bool operator==(const Entity &o) const { return name == o.get_name(); }

    std::string to_string() const override;
    std::vector<std::string> get_prerequisites() const { return prerequisites; }
    ItemList get_ingredients() const { return ingredients; }
    std::vector<std::string> get_unlocked_recipes() const {
        return unlocked_recipes;
    }

private:
    std::vector<std::string> prerequisites;
    ItemList ingredients;
    // The only effect in the json-file is "unlock-recipe", so we simplify this
    // part.
    std::vector<std::string> unlocked_recipes;
};

using TechnologyMap = std::unordered_map<std::string, Technology>;

class FactoryIdMap {
public:
    using fid_t = int;

    FactoryIdMap() = default;

    fid_t insert(const Factory *f) {
        insert_checked(f, count);
        return count++;
    }
    fid_t insert(const Factory *f, fid_t fid) {
        insert_checked(f, fid);
        return fid;
    }
    const Factory *erase(fid_t fid) {
        const Factory *f = map.at(fid);
        map.erase(fid);
        return f;
    }

    const Factory *operator[](fid_t fid) const { return map.at(fid); }

private:
    void insert_checked(const Factory *f, fid_t fid) {
        if (!map.insert({fid, f}).second) {
            throw std::logic_error("factory id was used twice");
        }
    }

    fid_t count;
    std::unordered_map<fid_t, const Factory *> map;
};
