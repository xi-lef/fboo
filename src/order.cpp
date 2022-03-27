#include "order.hpp"

#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "game.hpp"
#include "util.hpp"

using fid_t = FactoryIdMap::fid_t;

fid_t Order::add_factory(const Factory &f, bool init) {
    fid_t fid = fid_map.insert(&f);  // TODO only works for player (for initial_factories)
    for (const std::string &s : f.get_crafting_categories()) {
        craftable_categories.insert(s);
    }
    state.build_factory(&f, !init);

    if (!init) {
        // BuildEvents are handled before StartEvents, so we don't need to
        // increment tick here.
        order.push_back(std::make_shared<BuildEvent>(tick, f, fid));
        std::clog << "add_factory: " << order.back() << std::endl;
    }
    return fid;
}

void Order::add_technology(const Technology &t) {
    state.unlock_technology(t, all_recipes);
    order.push_back(std::make_shared<ResearchEvent>(tick, t.get_name()));
    std::clog << "add_technology: " << order.back() << std::endl;
}

bool Order::is_craftable(const Recipe &r) {
    return craftable_categories.contains(r.get_category());
}

void Order::craft(const Recipe &r, int amount) {
    auto f = std::ranges::find_if(fid_map, [&](const auto &v) {
        return v.second->get_crafting_categories().contains(r.get_category());
    });
    if (f == fid_map.end()) {
        throw std::logic_error("no factory exists for this recipe");
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

static int calc_execution_amount(const Recipe &r,
                                 const std::string &product_name,
                                 int product_amount) {
    auto tmp = std::ranges::find(r.get_products(), product_name,
                                 &Ingredient::get_name);
    return std::ceil(1. * product_amount / tmp->get_amount());
}

static int calc_ingredient_amount(const Recipe &r,
                                  const std::string &product_name,
                                  int product_amount,
                                  const std::string &ingredient_name) {
    int execution_amount
        = calc_execution_amount(r, product_name, product_amount);
    auto tmp = std::ranges::find(r.get_ingredients(), ingredient_name,
                                 &Ingredient::get_name);
    return execution_amount * tmp->get_amount();
}

bool Order::create_item(const std::string &name, int amount,
                        std::set<std::string> visited, bool dry_run) {
    int have = state.has_item(name);
    std::clog << "working on " << amount << " of " << name << " (" << have
              << " available)" << (dry_run ? " DRY" : "") << std::endl;

    if (creatable_items.contains(name)) {
        std::clog << name << " is known to be creatable" << std::endl;
        if (dry_run) {
            return true;
        }
        const Recipe &r = *creatable_items[name];
        if (!state.is_unlocked(r)) {
            create_technology(r, visited, false);
        }
        if (!is_craftable(r)) {
            create_factory(r.get_category(), visited, false);
        }
        for (const Ingredient &i : r.get_ingredients()) {
            create_item(i.get_name(),
                        calc_ingredient_amount(r, name, amount, i.get_name()),
                        visited, false);
        }
        craft(r, calc_execution_amount(r, name, amount));
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
                auto p = r.get_products();
                return std::ranges::find(p, name, &Ingredient::get_name)
                       != p.end();
            });
    // TODO opt?
    //std::ranges::sort(options, {}, [&](const Recipe &r) { return is_craftable(r); });

    // We need to craft each ingredient separately, going to the cases below for
    // some (or all) ingredients in the recursive call.
    const Recipe *good = nullptr;
    for (const Recipe &r : options) {
        std::clog << "trying " << r << std::endl;

        // Check if recipe is unlocked. If not, the technology needs to be
        // researched first.
        if (!state.is_unlocked(r)) {
            std::clog << "recipe is locked, trying research" << std::endl;
            if (!create_technology(r, visited, true)) {
                std::clog << "technology didn't worked for " << r << std::endl;
                continue;
            }
        }

        auto descend = [&](bool dry_run) {
            return std::ranges::all_of(
                r.get_ingredients(), [&](const Ingredient &i) {
                    return create_item(
                        i.get_name(),
                        calc_ingredient_amount(r, name, amount, i.get_name()),
                        visited, dry_run);
                });
        };
        if (!descend(true)) {
            std::clog << r << " doesn't work for " << name << std::endl;
            continue;
        }
        if (!is_craftable(r)
            && !create_factory(r.get_category(), visited, true)) {
            // No suitable factory can be created.
            std::clog << "no suitable factory can be created for " << r << std::endl;
            continue;
        }
        std::clog << r << " works for " << name << std::endl;

        if (!dry_run) {
            if (!state.is_unlocked(r)) {
                create_technology(r, visited, false);
            }
            if (!is_craftable(r)) {
                create_factory(r.get_category(), visited, false);
            }
            descend(false);
        }
        good = &r;
        break;
    }

    if (good) {
        if (!dry_run) {
            std::clog << "actually crafting " << good << std::endl;
            craft(*good, calc_execution_amount(*good, name, amount));
        }
        creatable_items[name] = good;
        return true;
    }

    return false;
}

// TODO try without visited? shouldnt work tho (lol)
bool Order::create_factory(const std::string &category,
                           std::set<std::string> visited, bool dry_run) {
    std::clog << "working on factory for " << category
              << (dry_run ? " DRY" : "") << std::endl;
    for (const auto &[fname, f] : all_factories) {
        if (!f.get_crafting_categories().contains(category)) {
            continue;
        }
        if (!all_recipes.contains(fname)) {
            continue;  // Skip player.
        }

        std::clog << "trying " << f << std::endl;
        if (!create_item(fname, 1, visited, dry_run)) {
            continue;
        }
        std::clog << f << " works for " << category << std::endl;
        if (!dry_run) {
            add_factory(f);
        }
        return true;
    }
    return false;
}

bool Order::create_technology(const Recipe &r, std::set<std::string> visited,
                              bool dry_run) {
    std::clog << "working on technology for " << r << (dry_run ? " DRY" : "")
              << std::endl;
    auto tmp = std::ranges::find_if(all_technologies, [&](const auto &t) {
        return t.second.get_unlocked_recipes().contains(r.get_name());
    });
    if (tmp == all_technologies.end()) {
        throw std::logic_error("no technology found for this recipe");
    }

    const Technology &t = tmp->second;
    std::clog << "trying " << t << std::endl;
    if (!create_technology(t, visited, true)) {
        return false;
    }
    std::clog << t << " works for " << r << std::endl;
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
    return order;
}
