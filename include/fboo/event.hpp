#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "entity.hpp"

namespace event {
inline int get_factory_id(const Factory *factory) {
    return static_cast<int>(reinterpret_cast<uintptr_t>(factory));
}
}  // namespace event

class Event {
public:
    Event(int timestamp) : timestamp(timestamp) {}

protected:
    int timestamp;
};

class ResearchEvent : public Event {
public:
    ResearchEvent(int timestamp, std::string technology)
        : Event(timestamp), technology(technology) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResearchEvent, timestamp, technology);

private:
    std::string technology;
};

class BuildEvent : public Event {
public:
    BuildEvent(int timestamp, const Factory *factory)
        : Event(timestamp),
          factory_id(event::get_factory_id(factory)),
          factory_type(factory->get_name()),
          factory_name(factory->to_string()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BuildEvent, timestamp, factory_id, factory_type);

private:
    int factory_id;
    std::string factory_type;
    std::string factory_name;
};

class DestroyEvent : public Event {
public:
    DestroyEvent(int timestamp, const Factory *factory)
        : Event(timestamp), factory_id(event::get_factory_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DestroyEvent, timestamp, factory_id);

private:
    int factory_id;
};

class StartEvent : public Event {
public:
    StartEvent(int timestamp, const Factory *factory, const Recipe &recipe)
        : Event(timestamp),
          factory_id(event::get_factory_id(factory)),
          recipe(recipe.get_name()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StartEvent, timestamp, factory_id, recipe);

private:
    int factory_id;
    std::string recipe;
};

class StopEvent : public Event {
public:
    StopEvent(int timestamp, const Factory *factory)
        : Event(timestamp), factory_id(event::get_factory_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StopEvent, timestamp, factory_id);

private:
    int factory_id;
};

class VictoryEvent : public Event {
public:
    VictoryEvent(int timestamp) : Event(timestamp) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VictoryEvent, timestamp);
};
