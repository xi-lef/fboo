#pragma once

#include <unordered_set>

#include "entity.hpp"
#include "event.hpp"

namespace order {

EventList compute(const ItemMap &all_items, const RecipeMap &all_recipes,
                  const FactoryMap &all_factories,
                  const TechnologyMap &all_technologies,
                  const std::vector<Factory> &initial_factories,
                  const ItemList &initial_items,
                  const ItemList &goal_items);

}  // namespace order
