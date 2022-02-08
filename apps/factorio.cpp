#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "fboo/entity.hpp"
#include "fboo/util.hpp"
#include "paths.h"

using json = nlohmann::json;

namespace {
// Read the json-files for each type of entity and construct the appropriate C++-objects for
// them. Return a tuple containing the unordered maps of all items, recipes, factories, and
// technologies (in that order).
auto init_entities() {
    json item;
    std::ifstream(JSON_ITEM) >> item;
    std::unordered_map<std::string, Item> items;
    for (const auto &[name, val] : item.items()) {
        items.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                      std::forward_as_tuple(name, val["type"]));
    }

    //for (const auto& [k, v] : items) { std::cout << v << std::endl; }

    json recipe;
    std::ifstream(JSON_RECIPE) >> recipe;
    std::unordered_map<std::string, Recipe> recipes;
    for (const auto &[name, val] : recipe.items()) {
        recipes.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                        std::forward_as_tuple(name, val["category"], val["energy"], val["enabled"],
                                              val["ingredients"], val["products"]));
    }

    //for (const auto& [k, v] : recipes) { std::cout << v << std::endl; }

    json factory;
    std::ifstream(JSON_FACTORY) >> factory;
    std::unordered_map<std::string, Factory> factories;
    for (const auto &[name, val] : factory.items()) {
        factories.emplace(
            std::piecewise_construct, std::forward_as_tuple(name),
            std::forward_as_tuple(name, val["crafting_speed"], val["crafting_categories"]));
    }

    //for (const auto& [k, v] : factories) { std::cout << v << std::endl; }

    json technology;
    std::ifstream(JSON_TECHNOLOGY) >> technology;
    std::unordered_map<std::string, Technology> technologies;
    for (const auto &[name, val] : technology.items()) {
        std::vector<std::string> unlocked_recipes;
        for (const auto &e : val["effects"]) {
            if (e["type"] != "unlock-recipe") {
                std::cerr << "invalid field in " << name << ": " << e << std::endl;
                exit(EXIT_FAILURE);
            }
            unlocked_recipes.push_back(e["recipe"]);
        }

        technologies.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                             std::forward_as_tuple(name, val["prerequisites"], val["ingredients"],
                                                   unlocked_recipes));
    }

    //for (const auto& [k, v] : technologies) { std::cout << v << std::endl; }

    return std::tuple(items, recipes, factories, technologies);
}
}  // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " target.json" << std::endl;
        return EXIT_FAILURE;
    }
    auto target = std::ifstream(argv[1]);

    auto [items, recipes, factories, technologies] = init_entities();
}
