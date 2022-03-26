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
    //std::clog << "craftable categories: " << craftable_categories << std::endl;
    state.build_factory(&f, !init);

    if (!init) {
        // BuildEvents are handled before StartEvents, so we don't need to
        // increment tick here.
        order.push_back(std::make_shared<BuildEvent>(tick, f, fid));
        std::clog << "add_factory: " << order.back() << std::endl;
    }
    return fid;
}

bool Order::is_craftable(const Recipe &r) {
    if (craftable_recipes.contains(&r)) {
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
        craftable_recipes.insert(&r);
    }
    return ret;
}

void Order::craft(const Recipe &r, int amount) {
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
    auto tmp2 = std::ranges::find(r.get_ingredients(), ingredient_name,
                                  &Ingredient::get_name);
    return execution_amount * tmp2->get_amount();
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
    }
    // If this item is in the inventory, use the available ones.
    amount -= have;
    if (amount <= 0) {
        return true;
    }

    if (visited.contains(name)) {
        // cycle (?)
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

    // We need to craft each ingredient separately, going to the cases below for
    // some (or all) ingredients in the recursive call.
    const Recipe *good = nullptr;
    for (const Recipe &r : options) {
        std::clog << "trying " << r << std::endl;
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
        if (is_craftable(r)) {
            // Do nothing.
        // TODO check if state.is_unlocked(r), if not, see if technology can be
        // unlocked, opt: only create factory if technology is possible
        } else if (std::clog << name << std::endl,
                   create_factory(r.get_category(), visited, true)) {
            if (!dry_run) {
                create_factory(r.get_category(), visited, false);
            }
        } else {
            // No suitable factory can be created.
            std::clog << "no suitable factory can be created for " << r << std::endl;
            continue;
        }
        std::clog << "Recipe " << r.get_name() << " works for " << name
                  << std::endl;

        if (!dry_run) {
            descend(false);
        }
        good = &r;
        break;
    }

    if (good) {
        if (!dry_run) {
            if (!is_craftable(*good)) {
                throw std::logic_error("huh");
            }
            std::clog << "actually crafting " << good << std::endl;
            if (!state.has_items(good->get_ingredients())) {
                std::clog << "items: " << state.get_items() << std::endl;
                throw std::logic_error("double you tee eff");
            }
            if (!state.is_unlocked(*good)) {
                throw std::logic_error("implement technologies plz");
            }
            //std::clog << "using " << *good << std::endl;
            craft(*good, calc_execution_amount(*good, name, amount));
        }
        creatable_items.insert(name);
        return true;
    }

    return false;
}

bool Order::create_factory(const std::string &category,
                           std::set<std::string> visited, bool dry_run) {
    std::clog << "working on factory for " << category
              << (dry_run ? " DRY" : "") << std::endl;
    for (const auto &[fname, f] : all_factories) {
        if (!f.get_crafting_categories().contains(category)) {
            continue;
        }
        std::clog << "trying " << f << std::endl;
        if (!all_recipes.contains(fname)) {
            continue;  // Skip player.
        }

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

EventList Order::compute() {
    for (const auto &[name, amount] : goal_items) {
        create_item(name, amount);
    }
    return order;
}
