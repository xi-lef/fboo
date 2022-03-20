#include "order.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "game.hpp"
#include "util.hpp"

EventList order::compute(const ItemMap &all_items, const RecipeMap &all_recipes,
                         const FactoryMap &all_factories,
                         const TechnologyMap &all_technologies,
                         std::vector<Factory> initial_factories,
                         const ItemList &goal_items) {
    std::unordered_set<std::string> available_categories;
    for (const Factory &f : initial_factories) {
        for (const std::string &c : f.get_crafting_categories()) {
            available_categories.insert(c);
        }
    }

    //while (std::ranges::any_of(needed_categories, [&](const std::string &s) {
    //    return !available_categories.contains(s);
    //})) {
    //    std::clog << "bla" << std::endl;
    //}

    EventList order;
    FactoryIdMap fid_map;
    for (const Factory &f : initial_factories) {
        fid_map.insert(&f); // TODO only works for player
    }

    std::unordered_map<std::string, int> needed_items;
    std::deque<Ingredient> stack(goal_items.begin(), goal_items.end());
    while (!stack.empty()) {
        Ingredient i = stack.front();
        stack.pop_front();
        needed_items[i.get_name()] += i.get_amount();
        for (const Ingredient &x :
             all_recipes.at(i.get_name()).get_ingredients()) {
            stack.push_back(x);
        }
    }
    // TODO set<string> available_items, fill with items that have no prereqs,
    // then iteratively as we get more items, to build factories for needed
    // categories

    std::clog << "needed_items: " << needed_items << std::endl;

    long long tick = 0;
    for (const auto &[name, amount] : needed_items) {
        const Recipe &r = all_recipes.at(name);
        std::string needed_category = r.get_category();
        auto f = std::ranges::find_if(fid_map, [&](const auto &v) {
            return v.second->get_crafting_categories().contains(needed_category);
        });
        if (f == fid_map.end()) {
            throw std::logic_error("idk");
        }
        std::clog << f->second << std::endl;
        order.push_back(std::make_shared<StartEvent>(tick, f->first, r));
        tick += f->second->calc_ticks(r) * amount;
        order.push_back(std::make_shared<StopEvent>(tick, f->first));
    }
    //game::State state(all_recipes);

    return order;
}
