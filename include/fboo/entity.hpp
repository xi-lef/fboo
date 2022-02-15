#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

class Entity {
public:
    Entity(std::string name) : name(name) {}

    // TODO fine? maybe bad for e.g. factory or item?
    virtual bool operator==(const Entity &o) const { return name == o.name; }

    virtual std::string to_string() const = 0;
    std::string get_name() const { return name; }

protected:
    std::string name;
};

class Item : public Entity {
public:
    Item(std::string name, std::string type = "") : Entity(name), type(type) {}

    std::string to_string() const override;
    std::string get_type() const { return type; }

private:
    std::string type;
};

using ItemMap = std::unordered_map<std::string, Item>;

class Ingredient : public Item {
public:
    Ingredient(std::string name = "", int amount = 0) : Item(name), amount(amount) {}

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
    Recipe(std::string name, std::string category, int energy, bool enabled, ItemList ingredients,
           ItemList products)
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
    int energy;  // Amount of ticks to execute the recipe.
    bool enabled;
    ItemList ingredients, products;
};

using RecipeMap = std::unordered_map<std::string, Recipe>;

class Factory : public Entity {
public:
    Factory(std::string name, double crafting_speed, std::vector<std::string> crafting_categories)
        : Entity(name), crafting_speed(crafting_speed), crafting_categories(crafting_categories) {}

    std::string to_string() const override;
    double get_crafting_speed() const { return crafting_speed; }
    std::vector<std::string> get_crafting_categories() const {
        return crafting_categories;
    }

private:
    double crafting_speed;
    std::vector<std::string> crafting_categories;
};

using FactoryMap = std::unordered_map<std::string, Factory>;

class Technology : public Entity {
public:
    Technology(std::string name, std::vector<std::string> prerequisites, ItemList ingredients,
               std::vector<std::string> unlocked_recipes)
        : Entity(name),
          prerequisites(prerequisites),
          ingredients(ingredients),
          unlocked_recipes(unlocked_recipes) {}

    std::string to_string() const override;
    std::vector<std::string> get_prerequisites() const { return prerequisites; }
    ItemList get_ingredients() const { return ingredients; }
    std::vector<std::string> get_unlocked_recipes() const {
        return unlocked_recipes;
    }

private:
    std::vector<std::string> prerequisites;
    ItemList ingredients;
    // The only effect in the json-file is "unlock-recipe", so we simplify this part.
    std::vector<std::string> unlocked_recipes;
};

using TechnologyMap = std::unordered_map<std::string, Technology>;
