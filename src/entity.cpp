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
    ss << "Recipe \"" << name << "\" (" << category << ", initially "
       << (enabled ? "enabled" : "disabled") << ") costs " << energy
       << std::endl
       << "takes: " << ingredients << std::endl
       << "produces: " << products << std::endl;
    return ss.str();
}

std::string Factory::to_string() const {
    std::ostringstream ss;
    ss << "Factory \"" << name << "\" can craft " << crafting_categories
       << " at speed " << crafting_speed << std::endl;
    return ss.str();
}

std::string Technology::to_string() const {
    std::ostringstream ss;
    ss << "Technology \"" << name << "\"" << std::endl
       << "requires: " << prerequisites << std::endl
       << "consumes: " << ingredients << std::endl
       << "unlocks: " << unlocked_recipes << std::endl;
    return ss.str();
}
