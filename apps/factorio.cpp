#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "fboo/entity.hpp"
#include "fboo/event.hpp"
#include "fboo/game.hpp"
#include "fboo/order.hpp"
#include "fboo/util.hpp"
#include "paths.h"

using json = nlohmann::json;

namespace {

// Read the json-files for each type of entity and construct the appropriate
// C++-objects for them. Return a tuple containing the unordered maps of all
// items, recipes, factories, and technologies (in that order).
auto init_entities() {
    json item;
    std::ifstream(JSON_ITEM) >> item;
    ItemMap items;
    for (const auto &[name, val] : item.items()) {
        items.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                      std::forward_as_tuple(name, val["type"]));
    }

    //for (const auto& [k, v] : items) { std::clog << v << std::endl; }

    json recipe;
    std::ifstream(JSON_RECIPE) >> recipe;
    RecipeMap recipes;
    for (const auto &[name, val] : recipe.items()) {
        recipes.emplace(
            std::piecewise_construct, std::forward_as_tuple(name),
            std::forward_as_tuple(name, val["category"], val["energy"],
                                  val["enabled"], val["ingredients"],
                                  val["products"]));
    }

    //for (const auto& [k, v] : recipes) { std::clog << v << std::endl; }

    json factory;
    std::ifstream(JSON_FACTORY) >> factory;
    FactoryMap factories;
    for (const auto &[name, val] : factory.items()) {
        factories.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                          std::forward_as_tuple(name, val["crafting_speed"],
                                                val["crafting_categories"]));
    }

    //for (const auto& [k, v] : factories) { std::clog << v << std::endl; }

    json technology;
    std::ifstream(JSON_TECHNOLOGY) >> technology;
    TechnologyMap technologies;
    for (const auto &[name, val] : technology.items()) {
        std::vector<std::string> unlocked_recipes;
        for (const auto &e : val["effects"]) {
            if (e["type"] != "unlock-recipe") {
                std::cerr << "invalid field in " << name << ": " << e
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            unlocked_recipes.push_back(e["recipe"]);
        }

        technologies.emplace(
            std::piecewise_construct, std::forward_as_tuple(name),
            std::forward_as_tuple(name, val["prerequisites"],
                                  val["ingredients"], unlocked_recipes));
    }

    //for (const auto& [k, v] : technologies) { std::clog << v << std::endl; }

    return std::tuple(items, recipes, factories, technologies);
}

[[maybe_unused]] void test_challenge1() {
    json target;
    std::ifstream(JSON_CHALLENGE1) >> target;

    auto initial_items = target["initial-items"].get<ItemList>();
    auto goal_items = target["goal-items"].get<ItemList>();

    EventList events;
    for (const auto &[_, v] : target["initial-factories"].items()) {
        events.push_back(std::make_shared<BuildEvent>(
            -1, v["factory-type"], v["factory-name"], v["factory-id"]));
    }

    events.push_back(std::make_shared<StartEvent>(0, 0, "coal"));
    events.push_back(std::make_shared<StopEvent>(60, 0));
    std::clog << events << std::endl;

    const auto [items, recipes, factories, technologies] = init_entities();
    game::Simulation sim(items, recipes, factories, technologies, events,
                         goal_items, initial_items);
    long long tick = sim.simulate();
    if (tick != 60) {
        std::cerr << "challenge 1 failed, got tick " << tick << std::endl;
        exit(EXIT_FAILURE);
    }
}

[[maybe_unused]] void test_challenge2() {
    json target;
    std::ifstream(JSON_CHALLENGE2) >> target;

    auto initial_items = target["initial-items"].get<ItemList>();
    auto goal_items = target["goal-items"].get<ItemList>();

    EventList events;
    for (const auto &[_, v] : target["initial-factories"].items()) {
        events.push_back(std::make_shared<BuildEvent>(
            -1, v["factory-type"], v["factory-name"], v["factory-id"]));
    }

    events.push_back(std::make_shared<StartEvent>(0, 0, "coal"));
    events.push_back(std::make_shared<BuildEvent>(60, "burner-mining-drill",
                                                  "coal-mine", 1));
    events.push_back(std::make_shared<StartEvent>(60, 1, "coal-burner"));
    events.push_back(std::make_shared<StartEvent>(60, 0, "iron-ore"));
    events.push_back(std::make_shared<BuildEvent>(120, "stone-furnace",
                                                  "iron-smelter", 2));
    events.push_back(std::make_shared<StartEvent>(120, 2, "iron-plate-burner"));
    std::clog << events << std::endl;

    const auto [items, recipes, factories, technologies] = init_entities();
    game::Simulation sim(items, recipes, factories, technologies, events,
                         goal_items, initial_items);
    long long tick = sim.simulate();
    // solution-2.json has the VictoryEvent at tick 6600, but it is already
    // possible as early as tick 6132.
    if (tick != 6132) {
        std::cerr << "challenge 2 failed, got tick " << tick << std::endl;
        exit(EXIT_FAILURE);
    }
}

}  // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " target.json" << std::endl;
        return EXIT_FAILURE;
    }

    std::clog.setstate(std::ios_base::failbit);
    test_challenge1();
    test_challenge2();
    std::clog.clear();

    const auto [items, recipes, factories, technologies] = init_entities();

    json target;
    std::ifstream(argv[1]) >> target;
    auto initial_items = target["initial-items"].get<ItemList>();
    auto goal_items = target["goal-items"].get<ItemList>();

    EventList events;
    std::vector<Factory> initial_factories;
    for (const auto &[_, v] : target["initial-factories"].items()) {
        initial_factories.push_back(factories.at(v["factory-type"]));
        events.push_back(std::make_shared<BuildEvent>(
            -1, v["factory-type"], v["factory-name"], v["factory-id"]));
    }

    EventList solution_events
        = order::compute(items, recipes, factories, technologies,
                         initial_factories, initial_items, goal_items);
    std::ranges::copy(solution_events, std::back_inserter(events));

    game::Simulation sim(items, recipes, factories, technologies, events,
                         goal_items, initial_items);
    long long tick = sim.simulate();
    solution_events.push_back(std::make_shared<VictoryEvent>(tick));

    std::cout << json(solution_events) << std::endl;
}
