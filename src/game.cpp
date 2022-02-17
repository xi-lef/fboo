#include "game.hpp"

#include <iostream>

#include "event.hpp"
#include "util.hpp"

namespace game {

using namespace event;

State::State(std::vector<Recipe> all_recipes) {
    for (const auto &r : all_recipes) {
        if (r.is_enabled()) {
            available_recipes.push_back(r);
        }
    }
}

bool State::has_item(const Ingredient &ingredient) const {
    return items.at(ingredient.get_name()) >= ingredient.get_amount();
}

bool State::has_items(const ItemList &list) const {
    for (const auto &i : list) {
        if (!has_item(i)) {
            return false;
        }
    }
    return true;
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

void State::remove_item(const std::string &name, int amount) {
    add_item(name, -amount);
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

void State::add_factory(fid_t fid) {
    factories.insert(fid);
}

void State::remove_factory(fid_t fid) {
    factories.erase(fid);
}

bool Simulation::cancel_recipe(fid_t fid) {
    auto search = active_factories.find(fid);
    if (search == active_factories.end()) {
        return false;
    }
    state.add_items(search->second.get_ingredients());
    active_factories.erase(search);
    return true;
}

long long Simulation::simulate() {
    std::ranges::sort(events, {}, &Event::get_timestamp);

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
    std::vector<fid_t> finished_factories;
    for (auto &[fid, r] : active_factories) {
        if (r.tick() == 0) {
            state.add_items(r.get_products());
            finished_factories.push_back(fid);
            starved_factories.insert({fid, r});  // Gather for step 10.
        }
    }
    for (fid_t fid : finished_factories) {
        active_factories.erase(fid);
    }

    // Step 4: execute research events.
    for (const auto &e : research_events) {
        const Technology &technology = all_technologies.at(e.get_technology());
        for (const std::string &prerequisite : technology.get_prerequisites()) {
            if (!state.is_unlocked(all_technologies.at(prerequisite))) {
                throw std::logic_error("prerequisite not yet unlocked");
            }
        }

        state.unlock_technology(technology, all_recipes);
    }

    // Step 5: execute stop factory events.
    for (const auto &e : extract_subclass<StopEvent>(other_events)) {
        fid_t fid = e.get_factory_id();
        cancel_recipe(fid);
    }

    // Step 6: execute destroy factory events.
    for (const auto &e : extract_subclass<DestroyEvent>(other_events)) {
        fid_t fid = e.get_factory_id();
        cancel_recipe(fid);
        state.remove_factory(fid);
        state.add_item(id_to_factory(fid)->get_name());
    }

    // Step 7: handle victory event TODO

    // Step 8: Handle build factory events
    for (const auto &e : extract_subclass<BuildEvent>(other_events)) {
        state.remove_item(e.get_factory_type());
        // TODO ID? mhm
    }

    // Step 9: execute start factory events.
    for (const auto &e : extract_subclass<StartEvent>(other_events)) {
        fid_t fid = e.get_factory_id();
        cancel_recipe(fid);

        Recipe r = all_recipes.at(e.get_recipe());
        double crafting_speed = id_to_factory(fid)->get_crafting_speed();
        r.set_energy(std::ceil(r.get_energy() / crafting_speed));

        active_factories.insert({fid, r});
        starved_factories.insert({fid, r});  // Gather for step 10.
    }

    // Step 10: handle starved factories.
    std::vector<fid_t> satisfied_factories;
    for (auto [fid, r] : starved_factories) {
        const ItemList &ings = r.get_ingredients();
        if (state.has_items(ings)) {
            state.remove_items(ings);
            double crafting_speed = id_to_factory(fid)->get_crafting_speed();
            // r.energy is 0, so we need to get the actual required energy from
            // all_recipes.
            r.set_energy(std::ceil(all_recipes.at(r.get_name()).get_energy()
                                   / crafting_speed));
            active_factories.insert({fid, r});
            satisfied_factories.push_back(fid);
        }
    }
    for (fid_t fid : satisfied_factories) {
        starved_factories.erase(fid);
    }

    return true;
}

}  // namespace game
