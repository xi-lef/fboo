#include <sstream>

#include "entity.hpp"

#include "util.hpp"

std::string Item::to_string() const {
    std::ostringstream ss;
    ss << name << " (" << type << ")";
    return ss.str();
}

std::string Ingredient::to_string() const {
    std::ostringstream ss;
    ss << amount << "x " << name;
    return ss.str();
}

std::string Recipe::to_string() const {
    std::ostringstream ss;
    ss << "Recipe '" << name << "' (" << category << "; initially "
       << (enabled ? "enabled" : "disabled") << ") takes " << energy
       << " ticks; takes: " << ingredients << "; produces: " << products;
    return ss.str();
}

std::string Factory::to_string() const {
    std::ostringstream ss;
    ss << "Factory '" << name << "' can craft " << crafting_categories
       << " at speed " << crafting_speed;
    return ss.str();
}

std::string Technology::to_string() const {
    std::ostringstream ss;
    ss << "Technology '" << name << "'"
       << "; requires: " << prerequisites << "; consumes: " << ingredients
       << "; unlocks: " << unlocked_recipes;
    return ss.str();
}
