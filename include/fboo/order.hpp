#pragma once

#include <unordered_set>

#include "entity.hpp"
#include "event.hpp"
#include "game.hpp"

class Order {
public:
    Order(const RecipeMap &all_recipes, const FactoryMap &all_factories,
          const TechnologyMap &all_technologies,
          const std::vector<Factory> &initial_factories,
          const ItemList &initial_items, const ItemList &goal_items)
        : all_recipes(all_recipes),
          all_factories(all_factories),
          all_technologies(all_technologies),
          goal_items(goal_items),
          tick(0),
          state(all_recipes) {
        for (const auto &[name, amount] : initial_items) {
            state.add_item(name, amount);
        }
        for (const Factory &f : initial_factories) {
            add_factory(f, true);
        }
    }

    EventList compute();

private:
    bool is_factory_available(const Recipe &r);

    FactoryIdMap::fid_t add_factory(const Factory &f, bool init = false,
                                    FactoryIdMap::fid_t fid = 0);
    void add_technology(const Technology &t);
    void add_recipe(const Recipe &r, int amount = 1);

    bool craft_recipe(const Recipe &r, const std::string &name, int amount,
                      std::set<std::string> visited = {}, bool dry_run = false);
    bool create_item(const std::string &name, int amount,
                     std::set<std::string> visited = {}, bool dry_run = false);
    bool create_factory(const std::string &category,
                        std::set<std::string> visited = {},
                        bool dry_run = false);
    bool create_technology(const Recipe &r, std::set<std::string> visited = {},
                           bool dry_run = false);
    bool create_technology(const Technology &t,
                           std::set<std::string> visited = {},
                           bool dry_run = false);

    const RecipeMap &all_recipes;
    const FactoryMap &all_factories;
    const TechnologyMap &all_technologies;
    const ItemList &goal_items;

    long tick;
    std::unordered_set<std::string> craftable_categories;
    std::unordered_set<std::string> craftable_items;
    game::State state;
    FactoryIdMap fid_map;
    EventList order;

    // Memoization
    std::unordered_map<std::string, const Recipe *> creatable_items;
};
