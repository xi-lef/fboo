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
    State(const RecipeMap &all_recipes);

    auto get_items() const { return items; }  // For debugging.
    int has_item(const std::string &name) const;
    bool has_items(const ItemCount &list) const;
    void add_item(const std::string &name, int amount = 1);
    void add_items(const ItemCount &list);
    void remove_item(const std::string &name, int amount = 1);
    void remove_items(const ItemCount &list);

    bool is_unlocked(const Recipe &recipe) const;
    bool is_unlocked(const Technology &technology) const;
    void unlock_technology(const Technology &technology,
                           const RecipeMap &recipe_map);

    const Factory *build_factory(const Factory *factory, bool consume = true);
    void destroy_factory(const Factory *factory);

private:
    std::unordered_map<std::string, int> items;
    std::unordered_set<const Factory *> factories; // TODO kinda useless

    // TODO pointers? technologies and recipes are kinda singletons
    std::unordered_set<const Recipe *> unlocked_recipes;
    std::unordered_set<const Technology *> unlocked_technologies;
};

// TODO namespace?
class Simulation {
public:
    Simulation(const ItemMap &all_items, const RecipeMap &all_recipes,
               const FactoryMap &all_factories,
               const TechnologyMap &all_technologies, EventList events,
               ItemList goal_items, ItemList initial_items)
        : state(all_recipes),
          events(events.begin(), events.end()),
          all_items(all_items),
          all_recipes(all_recipes),
          all_factories(all_factories),
          all_technologies(all_technologies) {
        for (const auto &[name, amount] : goal_items) {
            this->goal_items[name] = amount;
        }
        for (const auto &[name, amount] : initial_items) {
            state.add_item(name, amount);
        }
    }

    long simulate();

private:
    // Does nothing if "fid" is not a known factory.
    void cancel_recipe(FactoryIdMap::fid_t fid);
    void build_factory(const BuildEvent *e, bool consume = true);

    void advance();

    long tick = -1;
    State state;
    std::unordered_map<std::string, int> goal_items; // TODO rm
    std::deque<std::shared_ptr<Event>> events;
    // TODO mv these two to state?
    std::unordered_map<FactoryIdMap::fid_t, Recipe> active_factories;
    std::map<FactoryIdMap::fid_t, Recipe> starved_factories;
    FactoryIdMap factory_id_map;

    const ItemMap &all_items; // TODO unused
    const RecipeMap &all_recipes;
    const FactoryMap &all_factories;
    const TechnologyMap &all_technologies;
};

}  // namespace game
