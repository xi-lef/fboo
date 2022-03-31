#include "order.hpp"

#include <iostream>

#include "game.hpp"
#include "util.hpp"

using fid_t = FactoryIdMap::fid_t;

bool Order::is_factory_available(const Recipe &r) {
    return craftable_categories.contains(r.get_category());
}

fid_t Order::add_factory(const Factory &f, fid_t fid) {
    //std::clog << "add_factory: " << order.back() << std::endl;
    for (const std::string &s : f.get_crafting_categories()) {
        craftable_categories.insert(s);
    }

    fid_map.insert(&f, fid);
    return fid;
}

FactoryIdMap::fid_t Order::add_factory(const Factory &f) {
    fid_t fid = add_factory(f, fid_map.get_next_fid());
    state.remove_item(f.get_name());

    // BuildEvents are handled before StartEvents, so we don't need to
    // increment tick here.
    order.push_back(std::make_shared<BuildEvent>(tick, f, fid));
    return fid;
}

void Order::add_technology(const Technology &t) {
    state.unlock_technology(t, all_recipes);
    order.push_back(std::make_shared<ResearchEvent>(tick, t.get_name()));
    //std::clog << "add_technology: " << order.back() << std::endl;
}

void Order::add_recipe(const Recipe &r, int amount) {
    auto f = std::ranges::find_if(fid_map, [&](const auto &v) {
        return v.second->get_crafting_categories().contains(r.get_category());
    });
    if (f == fid_map.end()) {
        throw std::logic_error("no factory exists for this recipe");
    }
    order.push_back(std::make_shared<StartEvent>(tick, f->first, r));
    //std::clog << "craft: " << order.back() << ", ";
    tick += f->second->calc_ticks(r) * amount;
    order.push_back(std::make_shared<StopEvent>(tick, f->first));
    //std::clog << order.back() << std::endl;

    // Update inventory.
    for (const auto &[iname, iamount] : r.get_ingredients()) {
        state.remove_item(iname, iamount * amount);
    }
    for (const auto &[iname, iamount] : r.get_products()) {
        state.add_item(iname, iamount * amount);
    }
}

namespace {
// How many times does r need to be executed to produce product_name
// product_amount many times?
int calc_execution_times(const Recipe &r, const std::string &product_name,
                          int product_amount) {
    int recipe_amount = r.get_products().at(product_name);
    return std::ceil(static_cast<double>(product_amount) / recipe_amount);
}

// How many ingredient_name need to be created in order to produce product_name
// product_amount many times using r?
int calc_ingredient_amount(const Recipe &r, const std::string &product_name,
                           int product_amount,
                           const std::string &ingredient_name) {
    int execution_amount
        = calc_execution_times(r, product_name, product_amount);
    int produced_amount = r.get_ingredients().at(ingredient_name);
    return execution_amount * produced_amount;
}
}  // namespace

bool Order::craft_recipe(const Recipe &r, const std::string &name, int amount,
                         std::set<std::string> visited, bool dry_run) {
    if (state.is_unlocked(r) || create_technology(r, visited, dry_run)) {
        if (is_factory_available(r)
            || create_factory(r.get_category(), visited, dry_run)) {
            // (Try to) create all ingredients.
            if (std::ranges::all_of(
                    r.get_ingredients(), [&](const Ingredient &i) {
                        return create_item(i.get_name(),
                                           calc_ingredient_amount(
                                               r, name, amount, i.get_name()),
                                           visited, dry_run);
                    })) {
                if (!dry_run) {
                    add_recipe(r, calc_execution_times(r, name, amount));
                }
                creatable_items[name] = &r;
                return true;
            }
        }
    }
    return false;
}

bool Order::create_item(const std::string &name, int amount,
                        std::set<std::string> visited, bool dry_run) {
    int have = state.has_item(name);
    //std::clog << "working on " << amount << " of " << name << " (" << have
    //          << " available)" << (dry_run ? " DRY" : "") << std::endl;

    if (creatable_items.contains(name)) {
        //std::clog << name << " is known to be creatable" << std::endl;
        if (!dry_run) {
            craft_recipe(*creatable_items[name], name, amount, visited, false);
        }
        return true;
    }
    // If this item is in the inventory, use the available ones.
    amount -= have;
    if (amount <= 0) {
        return true;
    }

    // Avoid dependency-cycles (i.e., an item depends on itself).
    if (visited.contains(name)) {
        return false;
    }
    visited.insert(name);

    auto options
        = all_recipes | std::views::values
          | std::views::filter([&](const Recipe &r) {
                return r.get_products().contains(name);
            })
          | std::views::transform([](const Recipe &r) { return &r; });
    std::vector<const Recipe *> better_options(options.begin(), options.end());
    std::ranges::sort(better_options, {}, [&](const Recipe *r) {
        // TODO opt: I don't think this makes sense, but it improves results...
        return is_factory_available(*r);
    });

    for (const Recipe *r : better_options) {
        // std::clog << "trying " << r << std::endl;
        if (craft_recipe(*r, name, amount, visited, true)) {
            if (!dry_run) {
                craft_recipe(*r, name, amount, visited, false);
            }
            return true;
        }
    }

    return false;
}

bool Order::create_factory(const std::string &category,
                           std::set<std::string> visited, bool dry_run) {
    //std::clog << "working on factory for " << category
    //          << (dry_run ? " DRY" : "") << std::endl;
    for (const auto &[fname, f] : all_factories) {
        if (f.get_crafting_categories().contains(category)
            && all_recipes.contains(fname)  // Skip player.
            && create_item(fname, 1, visited, dry_run)) {
            if (!dry_run) {
                add_factory(f);
            }
            return true;
        }
    }
    return false;
}

bool Order::create_technology(const Recipe &r, std::set<std::string> visited,
                              bool dry_run) {
    //std::clog << "working on technology for " << r << (dry_run ? " DRY" : "")
    //          << std::endl;
    auto tmp = std::ranges::find_if(all_technologies, [&](const auto &t) {
        return t.second.get_unlocked_recipes().contains(r.get_name());
    });
    if (tmp == all_technologies.end()) {
        throw std::logic_error("no technology found for this recipe");
    }

    const Technology &t = tmp->second;
    //std::clog << "trying " << t << std::endl;
    if (!create_technology(t, visited, true)) {
        return false;
    }
    //std::clog << t << " works for " << r << std::endl;
    if (!dry_run) {
        create_technology(t, visited, false);
    }
    return true;
}

bool Order::create_technology(const Technology &t,
                              std::set<std::string> visited, bool dry_run) {
    if (state.is_unlocked(t)) {
        return true;
    }

    // Check if all prerequisites can be (or already are) researched, and if all
    // ingredients can be created. Only if everything works, we actually create
    // the technology.
    auto descend_prerequisites = [&](bool dry_run) {
        return std::ranges::all_of(
            t.get_prerequisites(), [&](const std::string &s) {
                return create_technology(all_technologies.at(s), visited,
                                         dry_run);
            });
    };
    if (!descend_prerequisites(true)) {
        return false;
    }

    auto descend_ingredients = [&](bool dry_run) {
        return std::ranges::all_of(
            t.get_ingredients(), [&](const Ingredient &i) {
                return create_item(i.get_name(), i.get_amount(), visited,
                                   dry_run);
            });
    };
    if (!descend_ingredients(true)) {
        return false;
    }

    if (!dry_run) {
        descend_prerequisites(false);
        descend_ingredients(false);
        add_technology(t);
    }
    return true;
}

EventList Order::compute() {
    for (const auto &[name, amount] : goal_items) {
        create_item(name, amount);
    }

    // Victory is achieved in the same tick as the last event.
    order.push_back(
        std::make_shared<VictoryEvent>(order.back()->get_timestamp()));
    return order;
}
