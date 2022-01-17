#include <string>
#include <utility>
#include <vector>

class Entity {
public:
    Entity(std::string name) : name(name) {}

protected:
    std::string name;
};

class Item : Entity {};

using ItemList = std::vector<std::pair<Item, int>>;

class Recipe : Entity {
private:
    std::string category;
    int energy;  // Amount of ticks to execute the recipe.
    ItemList ingredients, products;
};

class Factory : Entity {
public:
    Factory(std::string name, double crafting_speed,
            std::vector<std::string> crafting_categories)
        : Entity(name), crafting_speed(crafting_speed),
          crafting_categories(crafting_categories) {}
private:
    double crafting_speed;
    std::vector<std::string> crafting_categories;
};

class Technology : Entity {
private:
    std::vector<Technology> prerequisites;
    ItemList ingredients;
    std::vector<std::pair<std::string, std::string>> effects;
};
