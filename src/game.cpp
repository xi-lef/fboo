#include "game.hpp"

#include <iostream>

#include "event.hpp"
#include "util.hpp"

namespace game {

using event::fid_t;

State::State(std::vector<Recipe> all_recipes) {
    for (const auto &r : all_recipes) {
        if (r.is_enabled()) {
            available_recipes.push_back(r);
        }
    }
}

void State::add_item(const Ingredient &ingredient) {
    add_item(ingredient.get_name(), ingredient.get_amount());
}

void State::add_item(const std::string &name, int amount) {
    items[name] += amount;
    if (items[name] < 0) {
        throw std::invalid_argument("item amount must not be < 0");
    }
}

void State::add_items(const ItemList &list) {
    for (const auto &ingredient : list) {
        add_item(ingredient);
    }
}

void State::remove_items(const ItemList &list) {
    for (const auto &ingredient : list) {
        add_item(ingredient.get_name(), -ingredient.get_amount());
    }
}

bool State::is_unlocked(const Technology &technology) const {
    return std::find(unlocked_technologies.begin(), unlocked_technologies.end(),
                     technology)
           != unlocked_technologies.end();
}

void State::unlock_technology(const Technology &technology,
                              const RecipeMap &recipe_map) {
    remove_items(technology.get_ingredients());
    unlocked_technologies.push_back(technology);
    for (const std::string &s : technology.get_unlocked_recipes()) {
        available_recipes.push_back(recipe_map.at(s));
    }
}

long long Simulation::simulate() {
    std::sort(events.begin(), events.end());

    while (!goals.empty()) {
        std::vector<Event> cur_events;
        while (events.front().get_timestamp() == tick) {
            cur_events.push_back(events.front());
            events.pop_front();
        }
        advance(cur_events);
    }

    return 0;
}

bool Simulation::advance(std::vector<Event> cur_events) {
    // Step 1: increment timestamp.
    if (++tick > (1ll << 40)) {
        std::cerr << "game duration exceeded 2^40, aborting" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Step 2: group and sort events.
    std::vector<ResearchEvent> research_events;
    std::vector<FactoryEvent> other_events;
    if (!cur_events.empty()) {
        auto mid = std::partition(
            cur_events.begin(), cur_events.end(),
            [](const Event &e) { return e.get_type() == ResearchEvent::type; });
        std::transform(
            cur_events.begin(), mid, std::back_inserter(research_events),
            [](Event &e) { return dynamic_cast<ResearchEvent &>(e); });
        // TODO victory_event? maybe just not include it in events at all? just
        // generate it at the end of simulate
        std::transform(
            mid, cur_events.end(), std::back_inserter(other_events),
            [](Event &e) { return dynamic_cast<FactoryEvent &>(e); });

        std::ranges::sort(research_events, {}, &ResearchEvent::get_technology);
        std::ranges::sort(other_events, {}, &FactoryEvent::get_factory_id);
    }

    // Step 3: finish recipes.
    size_t removed = 0;
    for (ActiveRecipe &r : state.active_recipes) {
        if (r.tick() == 0) {
            state.add_items(r.get_products());
            ++removed;
        }
    }
    for (size_t i = 0; i < removed; ++i) {
        std::ranges::pop_heap(state.active_recipes, {},
                              &ActiveRecipe::get_energy);
        state.active_recipes.pop_back();
    }
    // TODO verify correctness (because of heap)

    // Step 4: execute research events.
    for (const ResearchEvent &e : research_events) {
        const Technology &technology = all_technologies.at(e.get_technology());
        for (const std::string &prerequisite : technology.get_prerequisites()) {
            if (!state.is_unlocked(all_technologies.at(prerequisite))) {
                return false;  // TODO exception?
            }
        }

        state.unlock_technology(technology, all_recipes);
    }

    // Step 5: execute stop factory events.
    for (const StopEvent &e : extract_subclass<StopEvent>(other_events)) {
    }

    // Step 9: execute start factory events.
    for (const StartEvent &e : extract_subclass<StartEvent>(other_events)) {
        Recipe r = all_recipes.at(e.get_recipe());
        fid_t fid = e.get_factory_id();
        double crafting_speed = event::id_to_factory(fid)->get_crafting_speed();
        r.set_energy(std::ceil(r.get_energy() / crafting_speed));

        state.active_recipes.push_back(ActiveRecipe(r, fid));
        std::ranges::push_heap(state.active_recipes, {},
                               &ActiveRecipe::get_energy);
    }

    return true;
}

}  // namespace game
