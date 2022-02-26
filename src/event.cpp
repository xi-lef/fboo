#include "event.hpp"

#include "util.hpp"

std::string Event::to_string() const {
    std::ostringstream ss;
    ss << timestamp << ": " << get_type();
    return ss.str();
}

std::string ResearchEvent::to_string() const {
    std::ostringstream ss;
    ss << Event::to_string() << " for " << technology;
    return ss.str();
}

std::string FactoryEvent::to_string() const {
    std::ostringstream ss;
    ss << Event::to_string() << " for factory " << factory_id;
    return ss.str();
}

std::string BuildEvent::to_string() const {
    std::ostringstream ss;
    ss << FactoryEvent::to_string() << " (" << factory_type << ")";
    return ss.str();
}

std::string StartEvent::to_string() const {
    std::ostringstream ss;
    ss << FactoryEvent::to_string() << " (building " << recipe << ")";
    return ss.str();
}
