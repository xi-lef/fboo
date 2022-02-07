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
    std::string type = "abstract-event";
    int timestamp;
};

class ResearchEvent : public Event {
public:
    ResearchEvent(int timestamp, std::string technology)
        : Event(timestamp), technology(technology) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResearchEvent, type, timestamp, technology);

private:
    std::string type = "research-event";
    std::string technology;
};

class BuildEvent : public Event {
public:
    BuildEvent(int timestamp, const Factory *factory)
        : Event(timestamp),
          factory_id(event::get_factory_id(factory)),
          factory_type(factory->get_name()),
          factory_name(factory->to_string()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BuildEvent, type, timestamp, factory_id, factory_type);

private:
    std::string type = "build-factory-event";
    int factory_id;
    std::string factory_type;
    std::string factory_name;
};

class DestroyEvent : public Event {
public:
    DestroyEvent(int timestamp, const Factory *factory)
        : Event(timestamp), factory_id(event::get_factory_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DestroyEvent, type, timestamp, factory_id);

private:
    std::string type = "destroy-destroy-event";
    int factory_id;
};

class StartEvent : public Event {
public:
    StartEvent(int timestamp, const Factory *factory, const Recipe &recipe)
        : Event(timestamp),
          factory_id(event::get_factory_id(factory)),
          recipe(recipe.get_name()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StartEvent, type, timestamp, factory_id, recipe);

private:
    std::string type = "start-factory-event";
    int factory_id;
    std::string recipe;
};

class StopEvent : public Event {
public:
    StopEvent(int timestamp, const Factory *factory)
        : Event(timestamp), factory_id(event::get_factory_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StopEvent, type, timestamp, factory_id);

private:
    std::string type = "stop-factory-event";
    int factory_id;
};

class VictoryEvent : public Event {
public:
    VictoryEvent(int timestamp) : Event(timestamp) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VictoryEvent, type, timestamp);

private:
    std::string type = "victory-event";
};
