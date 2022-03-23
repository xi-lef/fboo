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
          // factories(initial_factories),
          state(all_recipes) {
        state.add_items(initial_items);
        for (const Factory &f : initial_factories) {
            add_factory(f, true);
        }
    }

    EventList compute();

private:
    FactoryIdMap::fid_t add_factory(const Factory &f, bool init = false);
    bool is_craftable(const Recipe &r) const;
    void craft(const Recipe &r, int amount = 1);
    bool create_item(const std::string &name, int amount);

    const RecipeMap &all_recipes;
    const FactoryMap &all_factories;
    const TechnologyMap &all_technologies;
    const ItemList &goal_items;

    long long tick;
    //std::vector<Factory> factories;
    std::unordered_set<std::string> craftable_categories;
    std::unordered_set<std::string> craftable_items;
    game::State state;
    FactoryIdMap fid_map;
    EventList order;
};
