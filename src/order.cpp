#include "order.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "game.hpp"
#include "util.hpp"

using fid_t = FactoryIdMap::fid_t;

//FactoryMap unbuilt_factories;

fid_t Order::add_factory(const Factory &f, bool init) {
    fid_t fid = fid_map.insert(&f);  // TODO only works for player (for initial_factories)
    for (const std::string &s : f.get_crafting_categories()) {
        craftable_categories.insert(s);
    }
    //unbuilt_factories.erase(f.get_name());
    state.build_factory(&f, !init);

    if (!init) {
        // BuildEvents are handled before StartEvents, so we don't need to
        // increment tick here.
        std::clog << tick << f << fid << std::endl;
        order.push_back(std::make_shared<BuildEvent>(tick, f, fid)); // TODO in here?
        std::clog << "add_factory: " << order.back() << std::endl;
    }
    return fid;
}

bool Order::is_craftable(const Recipe &r) const {
    // If all ingredients are craftable and we have a factory that can
    // produce this item, this recipe is also craftable.
    return std::ranges::all_of(
               r.get_ingredients(),
               [&](const Ingredient &i) {
                   return craftable_items.contains(i.get_name());
               })
           && craftable_categories.contains(r.get_category());
}

void Order::craft(const Recipe &r, int amount) {
    std::clog << "crafting " << amount << " of " << r << std::endl;
    auto f = std::ranges::find_if(fid_map, [&](const auto &v) {
        return v.second->get_crafting_categories().contains(r.get_category());
    });
    if (f == fid_map.end()) {
        throw std::logic_error("no factory exists for this category");
    }
    order.push_back(std::make_shared<StartEvent>(tick, f->first, r));
    std::clog << "craft: " << order.back() << ", ";
    tick += f->second->calc_ticks(r) * amount; // TODO /produced_amount
    order.push_back(std::make_shared<StopEvent>(tick, f->first));
    std::clog << order.back() << std::endl;
    for (const Ingredient &i : r.get_products()) {
        state.add_item(i.get_name(), i.get_amount() * amount);
    }
}

EventList Order::compute() {
    //unbuilt_factories = all_factories;
    //std::clog << "craftable categories: " << craftable_categories << std::endl;

    // Determine all needed items and their respective amounts.
    std::unordered_map<std::string, int> needed_items;
    std::unordered_map<std::string, const Recipe *> uncraftable_items;
    std::deque<Ingredient> work(goal_items.begin(), goal_items.end());
    while (!work.empty()) {
        const Ingredient ingredient = work.front();
        work.pop_front();
        const auto &[name, amount] = ingredient;
        std::clog << "working on " << name << amount << std::endl;
        needed_items[name] += amount;

        auto options = all_recipes | std::views::filter([name = name](const auto &t) {
            auto p = t.second.get_products();
            return std::ranges::find(p, name, &Ingredient::get_name) != p.end();
        });
        auto craftable_options
            = options | std::views::filter([&](const auto &t) {
                  // return is_craftable(t.second);
                  return craftable_categories.contains(t.second.get_category());
              });
        //for (const auto &x : craftable_options) { std::clog << x.second << std::endl; }
        // TODO this results in endless loop if there's only one item in work
        //if (std::ranges::empty(craftable_options)) {
        //    work.push_back(ingredient);
        //    continue;
        //}

        // TODO use options here, descend recursive function
        const Recipe &r = all_recipes.at(name == "iron-plate" ? "iron-plate-burner" : name);
        if (is_craftable(r)) {
            craftable_items.insert(name);
            craft(r, amount);
        } else {
            uncraftable_items.insert({name, &r});
        }

        for (const Ingredient &x : r.get_ingredients()) {
            std::clog << "needs " << amount << " of " << x << std::endl;
            work.push_back(Ingredient(x.get_name(), x.get_amount() * amount));
        }
    }

    std::clog << "all needed items: " << needed_items << std::endl;
    std::clog << "craftable items: " << craftable_items << std::endl;
    std::clog << "uncraftable items: " << uncraftable_items << std::endl;

    // Create all uncraftable items (possibly by creating factories).
    auto keys = std::views::keys(uncraftable_items);
    std::deque<std::string> uncraftable(keys.begin(), keys.end());
    while (!uncraftable.empty()) {
        std::string name = uncraftable.front();
        uncraftable.pop_front();
        std::clog << "working on " << name << std::endl;
        //const Recipe &r = all_recipes.at(name);
        const Recipe &r = *uncraftable_items.at(name);
        if (is_craftable(r)) {
            std::clog << 0 << std::endl;
            craftable_items.insert(name);
            craft(r, needed_items[r.get_name()]);
        } else {
            std::clog << 1 << std::endl;
            // See if we can build a factory that can produce this item.
            bool ok = false;
            for (const auto &[fname, f] : all_factories) {
                if (f.get_crafting_categories().contains(r.get_category())) {
                    std::clog << fname << f << std::endl;
                    if (!all_recipes.contains(fname)) {
                        continue;  // skip player
                    }
                    const Recipe &fr = all_recipes.at(fname); // TODO search for a recipe that produces fname like above?
                    if (state.has_item(fname)) {
                        // nothing
                    } else if (is_craftable(fr)) {
                        craft(fr);
                    } else {
                        continue;
                    }
                    std::clog << "building " << f << " for " << name << std::endl;
                    add_factory(f);
                    // TODO push_front and continue outer
                    craftable_items.insert(name);
                    craft(r, needed_items.at(name));
                    ok = true;
                    break;
                }
            }

            // If we can't, just try again later and hope that it works...
            // TODO opt: alternate postponing and factory-building, try every
            // item again after building each factory
            if (!ok) {
                uncraftable.push_back(name);
            }
        }
    }

    return order;
}
