#include "order.hpp"

#include <iostream>
#include <ranges>
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
    //std::clog << "craftable categories: " << craftable_categories << std::endl;
    //unbuilt_factories.erase(f.get_name());
    state.build_factory(&f, !init);

    if (!init) {
        // BuildEvents are handled before StartEvents, so we don't need to
        // increment tick here. TODO
        std::clog << tick << f << fid << std::endl;
        order.push_back(std::make_shared<BuildEvent>(tick++, f, fid)); // TODO in here?
        std::clog << "add_factory: " << order.back() << std::endl;
    }
    return fid;
}

bool Order::is_craftable(const Recipe &r) const {
    static std::unordered_set<const Recipe *> craftable; // TODO member
    if (craftable.contains(&r)) {
        return true;
    }
    // If all ingredients are craftable and we have a factory that can
    // produce this item, this recipe is also craftable.
    bool ret
        = craftable_categories.contains(r.get_category())
          && std::ranges::all_of(r.get_ingredients(), [&](const Ingredient &i) {
                 // return craftable_items.contains(i.get_name());
                 return is_craftable(all_recipes.at(i.get_name()));
             });
    if (ret) {
        // Assume nothing ever becomes uncraftable again.
        craftable.insert(&r);
    }
    return ret;
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
    tick += f->second->calc_ticks(r) * amount;
    order.push_back(std::make_shared<StopEvent>(tick, f->first));
    std::clog << order.back() << std::endl;

    // Update inventory.
    for (const Ingredient &i : r.get_ingredients()) {
        state.remove_item(i.get_name(), i.get_amount() * amount);
    }
    for (const Ingredient &i : r.get_products()) {
        state.add_item(i.get_name(), i.get_amount() * amount);
    }
}

bool Order::create_item(const std::string &name, int amount,
                        std::set<std::string> visited, bool dry_run) {
    int have = state.has_item(name);
    std::clog << "working on " << amount << " of " << name << " (" << have << " available)" << (dry_run ? " DRY" : "") << std::endl;
    // If the item is in the inventory, use the available ones.
    amount -= have;
    if (amount <= 0) {
        return true;
    }

    if (visited.contains(name)) {
        // cycle (?)
        return false;
    }
    visited.insert(name);

    auto options = all_recipes | std::views::filter([&](const auto &t) {
                       auto p = t.second.get_products();
                       return std::ranges::find(p, name, &Ingredient::get_name)
                              != p.end();
                   }) | std::views::values;
    // Easy case.
    //for (const Recipe &r : options) {
    //    if (is_craftable(r)) {
    //        std::clog << "directly using " << r << std::endl;
    //        craft(r, amount); // TODO opt: amount / produced_amount
    //        //craftable_items.insert(name);
    //        return true;
    //    }
    //}

    // We need to craft each ingredient separately, going to the cases below for
    // some (or all) ingredients.
    //std::clog << "creating ingredients" << std::endl;
    const Recipe *good = nullptr;
    for (const Recipe &r : options) {
        std::clog << "trying " << r << std::endl;
        auto bla = [&](bool dry_run) {
            return std::ranges::all_of(
                r.get_ingredients(), [&](const Ingredient &i) {
                    return create_item(i.get_name(), i.get_amount() * amount,
                                       visited, dry_run);
                });
        };
        if (bla(true)) {
            if (is_craftable(r)) {
                // pass
            } else if (std::clog << name << std::endl,
                       create_factory(r.get_category(), visited, true)) {
                if (!dry_run) {
                    create_factory(r.get_category(), visited, false);
                }
            } else {
                std::clog << "doesn't work" << std::endl;
                continue;
            }
            std::clog << "Recipe " << r.get_name() << " works for " << name << std::endl;
            if (!dry_run) {
                bla(false);
            }
            //for (const auto &[iname, iamount] : r.get_ingredients()) {
            //    std::clog << "bla " << dry_run << std::endl;
            //    if (!create_item(iname, iamount * amount, dry_run)) {
            //        throw std::logic_error("idk");
            //    }
            //}
            good = &r;
            break;
        } else {
            std::clog << "doesn't work" << std::endl;
        }
    }
    // Try again. TODO ??
    //for (const Recipe &r : options) {
    if (good) {
        if (!dry_run) {
            if (!is_craftable(*good)) {
                throw std::logic_error("huh");
            }
            std::clog << "actually creating " << good << std::endl;
            if (!state.has_items(good->get_ingredients())) {
                std::clog << "items: " << state.get_items() << std::endl;
                throw std::logic_error("double you tee eff");
            }
            //std::clog << "using " << *good << std::endl;
            craft(*good, amount); // TODO opt: amount / produced_amount
        }
        //craftable_items.insert(name);
        return true;
    }

    return false;
#if 0
    throw std::logic_error("could happen");
    std::clog << "no recipe worked; trying to create factory for " << name << std::endl;
    for (const Recipe &r : options) {
        if (create_factory(r.get_category(), dry_run)) {
            if (!is_craftable(r)) {
                throw std::logic_error("can't ever happen lmao");
            }
            craft(r, amount);
        }
    }
#endif

    return false;
}

bool Order::create_factory(const std::string &category,
                           std::set<std::string> visited,
                           bool dry_run) {
    std::clog << "working on factory for " << category
              << (dry_run ? " DRY" : "") << std::endl;
    for (const auto &[fname, f] : all_factories) {
        if (!f.get_crafting_categories().contains(category)) {
            continue;
        }
        std::clog << "trying " << f << std::endl;
        if (!all_recipes.contains(fname)) {
            continue;  // skip player
        }

        //const Recipe &fr = all_recipes.at(fname); // TODO search for a recipe that produces fname like above?
        //if (state.has_item(fname)) {
            // Do nothing.
        //} else if (is_craftable(fr)) {
        //    craft(fr);
        //} else {
        if (!create_item(fname, 1, visited, dry_run)) {
            continue;
        }
            //continue;
        //}
        std::clog << f << " works for " << category << std::endl;
        if (!dry_run) {
            std::clog << "building " << f << std::endl;
            add_factory(f);
        }
        // TODO push_front and continue outer
        //craftable_items.insert(name);
        //craft(r, needed_items.at(name));
        return true;
    }
    return false;
}

EventList Order::compute() {
    //unbuilt_factories = all_factories;

    // Determine all needed items and their respective amounts.
    std::unordered_map<std::string, int> needed_items;
    std::unordered_map<std::string, const Recipe *> uncraftable_items;
    std::deque<Ingredient> work(goal_items.begin(), goal_items.end());
    while (!work.empty()) {
        const Ingredient ingredient = work.front();
        work.pop_front();
        const auto &[name, amount] = ingredient;
        //std::clog << "working on " << amount << " of " << name << std::endl;
        create_item(name, amount);
#if 0
        needed_items[name] += amount;

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
#endif
    }
    return order;

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
