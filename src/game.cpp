#include "game.hpp"

#include <iostream>

#include "event.hpp"
#include "util.hpp"

namespace game {

using fid_t = FactoryIdMap::fid_t;

State::State(const RecipeMap &all_recipes) {
    for (const auto &[_, r] : all_recipes) {
        if (r.is_enabled()) {
            unlocked_recipes.insert(&r);
        }
    }
}

int State::has_item(const std::string &name) const {
    auto r = items.find(name);
    if (r == items.end()) {
        return 0;
    }
    return r->second;
}

bool State::has_items(const ItemCount &list) const {
    for (const auto &[name, amount] : list) {
        if (has_item(name) < amount) {
            return false;
        }
    }
    return true;
}

void State::add_item(const std::string &name, int amount) {
    //std::clog << "adding " << amount << "x " << name << std::endl;
    items[name] += amount;
    if (items[name] < 0) {
        throw std::invalid_argument("item amount must not be < 0");
    }
}

void State::add_items(const ItemCount &list) {
    for (const auto &[name, amount] : list) {
        add_item(name, amount);
    }
}

void State::remove_item(const std::string &name, int amount) {
    add_item(name, -amount);
}

void State::remove_items(const ItemCount &list) {
    for (const auto &[name, amount] : list) {
        remove_item(name, amount);
    }
}

bool State::is_unlocked(const Recipe &recipe) const {
    return unlocked_recipes.contains(&recipe);
}

bool State::is_unlocked(const Technology &technology) const {
    return unlocked_technologies.contains(&technology);
}

void State::unlock_technology(const Technology &technology,
                              const RecipeMap &recipe_map) {
    remove_items(technology.get_ingredients());
    unlocked_technologies.insert(&technology);
    for (const std::string &s : technology.get_unlocked_recipes()) {
        unlocked_recipes.insert(&recipe_map.at(s));
    }
}

void Simulation::cancel_recipe(fid_t fid) {
    auto search = active_factories.find(fid);
    if (search != active_factories.end()) {
        state.add_items(search->second.get_ingredients());
        active_factories.erase(search);
    }
    // In case the factory finished its recipe in the current tick.
    starved_factories.erase(fid);
}

void Simulation::build_factory(const BuildEvent *e, bool consume) {
    const Factory &f = all_factories.at(e->get_factory_type());
    if (consume) {
        state.remove_item(f.get_name());
    }
    //std::clog << "building " << f << std::endl;
    factory_id_map.insert(&f, e->get_factory_id());
}

long Simulation::simulate() {
    auto victory
        = std::ranges::find(events, VictoryEvent::type, &Event::get_type);
    if (victory == events.end()) {
        throw std::logic_error("no VictoryEvent found in EventList");
    }
    long victory_tick = victory->get()->get_timestamp();
    events.erase(victory);

    std::ranges::sort(events, {}, &Event::get_timestamp);

    // Initialization: execute all (Build)Events with the initial timestamp.
    //std::clog << "tick -1: initializing factories" << std::endl;
    while (!events.empty() && events.front()->get_timestamp() == tick) {
        build_factory(dynamic_cast<const BuildEvent *>(events.front().get()),
                      false);
        events.pop_front();
    }

    while (tick < victory_tick) {
        advance();

        //std::clog << "items after tick " << tick << ": " << state.get_items()
        //          << std::endl << std::endl;
    }

    //std::clog << "done in tick " << tick << ", items: " << state.get_items()
    //          << std::endl;
    return tick;
}

void Simulation::advance() {
    // Step 1: increment timestamp.
    if (++tick > (1ll << 40)) {
        throw std::logic_error("game duration exceeded 2^40, aborting");
    }

    // Step 2: gather, group and sort events for the current tick.
    std::vector<const Event *> cur_events;
    while (!events.empty() && events.front()->get_timestamp() == tick) {
        cur_events.push_back(events.front().get());
        events.pop_front();
    }
    //std::clog << "tick " << tick << ", cur_events: " << cur_events << std::endl;
    std::vector<const ResearchEvent *> research_events;
    std::vector<const FactoryEvent *> other_events;
    if (!cur_events.empty()) {
        auto mid = std::partition(cur_events.begin(), cur_events.end(),
                                  cast_event<ResearchEvent>);
        std::transform(cur_events.begin(), mid,
                       std::back_inserter(research_events),
                       cast_event<ResearchEvent>);
        // The VictoryEvent is removed in simulate, so casting the remaining
        // events to FactoryEvent is safe.
        std::transform(mid, cur_events.end(), std::back_inserter(other_events),
                       cast_event<FactoryEvent>);

        std::ranges::sort(research_events, {}, &ResearchEvent::get_technology);
        std::ranges::sort(other_events, {}, &FactoryEvent::get_factory_id);
    }

    // Step 3: work on (or finish) recipes.
    for (auto it = active_factories.begin(); it != active_factories.end(); ) {
        auto &[fid, r] = *it;
        //std::clog << "factory " << fid << ": ";
        if (r.tick() == 0) {
            //std::clog << "finished " << r << std::endl;
            state.add_items(r.get_products());
            starved_factories.insert({fid, r});  // Gather for step 10.
            it = active_factories.erase(it);
        } else {
            //std::clog << "working " << r << std::endl;
            ++it;
        }
    }

    // Step 4: execute research events.
    for (const ResearchEvent *e : research_events) {
        const Technology &technology = all_technologies.at(e->get_technology());
        for (const std::string &prerequisite : technology.get_prerequisites()) {
            if (!state.is_unlocked(all_technologies.at(prerequisite))) {
                throw std::logic_error("prerequisite not yet unlocked");
            }
        }

        //std::clog << "unlocking " << technology << std::endl;
        state.unlock_technology(technology, all_recipes);
    }

    // Step 5: execute stop factory events.
    for (const StopEvent *e : extract_subclass<StopEvent>(other_events)) {
        fid_t fid = e->get_factory_id();
        //std::clog << "factory " << fid << ": stopping" << std::endl;
        cancel_recipe(fid);
    }

    // Step 6: execute destroy factory events.
    for (const DestroyEvent *e : extract_subclass<DestroyEvent>(other_events)) {
        fid_t fid = e->get_factory_id();
        //std::clog << "factory " << fid << ": destroying" << std::endl;
        cancel_recipe(fid);
        state.add_item(factory_id_map.erase(fid)->get_name());
    }

    // Step 7: handle victory event. This is handled in simulate via the
    // while-condition. Executing the remaining steps in the victory-tick is
    // more or less irrelevant.

    // Step 8: handle build factory events.
    for (const BuildEvent *e : extract_subclass<BuildEvent>(other_events)) {
        build_factory(e);
    }

    // Step 9: execute start factory events.
    for (const StartEvent *e : extract_subclass<StartEvent>(other_events)) {
        fid_t fid = e->get_factory_id();
        cancel_recipe(fid);

        if (!state.is_unlocked(all_recipes.at(e->get_recipe()))) {
            throw std::logic_error("recipe not yet unlocked");
        }
        // std::clog << "factory " << fid << ": commencing " << r << std::endl;
        // Gather for step 10. Use insert_or_assign to potentially overwrite a
        // recipe that was inserted for fid in step 3.
        starved_factories.insert_or_assign(fid,
                                           all_recipes.at(e->get_recipe()));
    }

    // Step 10: handle starved factories by starting production if possible.
    for (auto it = starved_factories.begin(); it != starved_factories.end(); ) {
        auto [fid, r] = *it;  // Copy r, its energy is reset below.
        const ItemCount &ings = r.get_ingredients();
        if (state.has_items(ings)) {
            state.remove_items(ings);
            // r.energy is 0, so we need to set the energy before starting.
            r.set_energy(*factory_id_map[fid]);
            //std::clog << "factory " << fid << ": starting " << r << std::endl;
            active_factories.insert({fid, r});

            it = starved_factories.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace game
