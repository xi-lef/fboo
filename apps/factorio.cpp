#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "fboo/entity.hpp"
#include "paths.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    json factory;
    std::ifstream(JSON_FACTORY) >> factory;
    std::vector<Factory> factories;
    for (const auto& [name, val] : factory.items()) {
        //std::cout << name << val << std::endl << std::endl;
        factories.emplace_back(name, val["crafting_speed"], val["crafting_categories"]);
    }

    //for (const auto& r : factories) { std::cout << r << std::endl; }

    json item;
    std::ifstream(JSON_ITEM) >> item;
    std::vector<Item> items;
    for (const auto& [name, val] : item.items()) {
        //std::cout << name << val << std::endl << std::endl;
        items.emplace_back(name, val["type"]);
    }

    //for (const auto& r : items) { std::cout << r << std::endl; }

    json recipe;
    std::ifstream(JSON_RECIPE) >> recipe;
    std::vector<Recipe> recipes;
    for (const auto& [name, val] : recipe.items()) {
        //std::cout << name << val << std::endl << std::endl;
        recipes.emplace_back(name, val["category"], val["energy"], val["enabled"],
                             val["ingredients"], val["products"]);
    }

    //for (const auto& r : recipes) { std::cout << r << std::endl; }

    json technology;
    std::ifstream(JSON_TECHNOLOGY) >> technology;
    std::vector<Technology> technologies;
    for (const auto& [name, val] : technology.items()) {
        //std::cout << name << val << std::endl << std::endl;

        std::vector<std::string> unlocked_recipes;
        for (const auto& e : val["effects"]) {
            if (e["type"] != "unlock-recipe") {
                std::cerr << "invalid field in " << name << ": " << e << std::endl;
                exit(EXIT_FAILURE);
            }
            unlocked_recipes.push_back(e["recipe"]);
        }

        technologies.emplace_back(name, val["prerequisites"], val["ingredients"],
                                  unlocked_recipes);
    }

    //for (const auto& r : technologies) { std::cout << r << std::endl; }
}
