#pragma once
#include <deque>
#include <queue>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "entity.hpp"
#include "event.hpp"

namespace game {

// TODO big meh
struct ActiveRecipe : Recipe {
    ActiveRecipe(const Recipe &r, event::fid_t fid) : Recipe(r), fid(fid) {}
    event::fid_t fid;
};

class State {
public:
    State(std::vector<Recipe> all_recipes);

    void add_item(const Ingredient &ingredient);
    void add_item(const std::string &name, int amount);
    void add_items(const ItemList &list);
    void remove_items(const ItemList &list);

    bool is_unlocked(const Technology &technology) const;
    void unlock_technology(const Technology &technology,
                           const RecipeMap &recipe_map);

    friend class Simulation;

private:
    std::unordered_map<std::string, int> items;
    std::vector<ActiveRecipe> active_recipes;  // TODO mpstubs like queue?

    std::vector<Recipe> available_recipes;
    std::vector<Factory> available_factories;
    std::vector<Technology> unlocked_technologies;
};

class Simulation {
public:
    Simulation(const ItemMap &all_items, const RecipeMap &all_recipes,
               const FactoryMap &all_factories,
               const TechnologyMap &all_technologies, std::deque<Event> events,
               std::vector<Ingredient> goals)
        : all_items(all_items),
          all_recipes(all_recipes),
          all_factories(all_factories),
          all_technologies(all_technologies),
          state(std::vector(std::views::values(all_recipes).begin(),
                            std::views::values(all_recipes).end())),
          events(events),
          goals(goals) {}

    long long simulate();

private:
    bool advance(std::vector<Event> cur_events);

    long long tick = -1;
    State state;
    std::vector<Ingredient> goals;
    std::deque<Event> events;  // TODO Event* ?

    const ItemMap all_items;
    const RecipeMap all_recipes;
    const FactoryMap all_factories;
    const TechnologyMap all_technologies;
};

}  // namespace game
