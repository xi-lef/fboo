#pragma once
#include <deque>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "entity.hpp"
#include "event.hpp"

namespace game {

class State {
public:
    State(std::vector<Recipe> all_recipes);

    bool has_item(const Ingredient &ingredient) const;
    bool has_items(const ItemList &list) const;
    void add_item(const Ingredient &ingredient);
    void add_item(const std::string &name, int amount = 1);
    void add_items(const ItemList &list);
    void remove_item(const std::string &name, int amount = 1);
    void remove_items(const ItemList &list);

    bool is_unlocked(const Recipe &recipe) const;
    bool is_unlocked(const Technology &technology) const;
    void unlock_technology(const Technology &technology,
                           const RecipeMap &recipe_map);

    const Factory *build_factory(const Factory *factory, bool consume = true);
    void destroy_factory(const Factory *factory);

private:
    std::unordered_map<std::string, int> items;
    std::unordered_set<const Factory *> factories;

    // TODO pointers? technologies and recipes are kinda singletons
    std::unordered_set<const Recipe *> unlocked_recipes;
    std::unordered_set<const Technology *> unlocked_technologies;
};

class Simulation {
public:
    Simulation(const ItemMap &all_items, const RecipeMap &all_recipes,
               const FactoryMap &all_factories,
               const TechnologyMap &all_technologies, std::vector<Event> events,
               ItemList goals)
        : all_items(all_items),
          all_recipes(all_recipes),
          all_factories(all_factories),
          all_technologies(all_technologies),
          state(std::vector(std::views::values(all_recipes).begin(),
                            std::views::values(all_recipes).end())),
          events(events.begin(), events.end()),
          goals(goals) {}

    long long simulate();

private:
    // Does nothing if "fid" is not a known factory.
    bool cancel_recipe(FactoryIdMap::fid_t fid);
    void build_factory(const BuildEvent &e, bool consume = true);

    bool advance(std::vector<Event> cur_events);

    long long tick = -1;
    State state;
    ItemList goals; // TODO map?
    std::deque<Event> events;  // TODO Event* ?
    // TODO mv these two to state?
    std::unordered_map<FactoryIdMap::fid_t, Recipe> active_factories;
    std::map<FactoryIdMap::fid_t, Recipe> starved_factories;
    FactoryIdMap factory_id_map;

    const ItemMap all_items;
    const RecipeMap all_recipes;
    const FactoryMap all_factories;
    const TechnologyMap all_technologies;
};

}  // namespace game
