#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

class Entity {
public:
    Entity(std::string name) : name(name) {}

    std::string get_name() const { return name; }

    virtual std::string to_string() const = 0;

protected:
    std::string name;
};

class Item : public Entity {
public:
    Item(std::string name, std::string type = "") : Entity(name), type(type) {}

    std::string to_string() const override;

private:
    std::string type;
};

class Ingredient : public Item {
public:
    Ingredient(std::string name = "", int amount = 0) : Item(name), amount(amount) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ingredient, name, amount);

    std::string to_string() const override;

private:
    int amount;
};

using ItemList = std::vector<Ingredient>;

class Recipe : public Entity {
public:
    Recipe(std::string name, std::string category, int energy, bool enabled,
           ItemList ingredients, ItemList products)
        : Entity(name), category(category), energy(energy), enabled(enabled),
          ingredients(ingredients), products(products) {}

    std::string to_string() const override;

private:
    std::string category;
    int energy;  // Amount of ticks to execute the recipe.
    bool enabled;
    ItemList ingredients, products;
};

class Factory : public Entity {
public:
    Factory(std::string name, double crafting_speed,
            std::vector<std::string> crafting_categories)
        : Entity(name), crafting_speed(crafting_speed),
          crafting_categories(crafting_categories) {}

    std::string to_string() const override;

private:
    double crafting_speed;
    std::vector<std::string> crafting_categories;
};

class Technology : public Entity {
public:
    Technology(std::string name, std::vector<std::string> prerequisites,
               ItemList ingredients, std::vector<std::string> unlocked_recipes)
        : Entity(name), prerequisites(prerequisites), ingredients(ingredients),
          unlocked_recipes(unlocked_recipes) {}

    std::string to_string() const override;

private:
    // TODO union vector<string> and vector<technology>, everywhere else aswell
    std::vector<std::string> prerequisites;
    ItemList ingredients;
    //std::vector<std::pair<std::string, std::string>> effects;
    // The only effect in the json-file is "unlock-recipe", so we simplify this part.
    std::vector<std::string> unlocked_recipes;
};
