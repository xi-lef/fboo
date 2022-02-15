#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "entity.hpp"

namespace event {

using fid_t = uintptr_t;

inline fid_t factory_to_id(const Factory *factory) {
    return static_cast<fid_t>(reinterpret_cast<uintptr_t>(factory));
}

inline Factory *id_to_factory(fid_t fid) {
    return reinterpret_cast<Factory *>(fid);
}

}  // namespace event

class Event {
public:
    Event(int timestamp) : timestamp(timestamp) {}

    virtual std::string get_type() const { return type; }
    int get_timestamp() const { return timestamp; }
    bool operator<(const Event &o) const { return timestamp < o.timestamp; }

protected:
    inline static std::string type = "abstract-event";
    int timestamp;
};

class ResearchEvent : public Event {
public:
    ResearchEvent(int timestamp, std::string technology)
        : Event(timestamp), technology(technology) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResearchEvent, type, timestamp, technology);

    std::string get_technology() const { return technology; }

    inline static std::string type = "research-event";

private:
    std::string technology;
};

class FactoryEvent : public Event {
public:
    FactoryEvent(int timestamp, event::fid_t factory_id)
        : Event(timestamp), factory_id(factory_id) {}

    event::fid_t get_factory_id() const { return factory_id; }

protected:
    event::fid_t factory_id;
};

class BuildEvent : public FactoryEvent {
public:
    BuildEvent(int timestamp, const Factory *factory)
        : FactoryEvent(timestamp, event::factory_to_id(factory)),
          factory_type(factory->get_name()),
          factory_name(factory->to_string()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BuildEvent, type, timestamp, factory_id, factory_type);

    inline static std::string type = "build-factory-event";

    std::string get_factory_type() const { return factory_type; }
    std::string get_factory_name() const { return factory_name; }

private:
    std::string factory_type;
    std::string factory_name;
};

class DestroyEvent : public FactoryEvent {
public:
    DestroyEvent(int timestamp, const Factory *factory)
        : FactoryEvent(timestamp, event::factory_to_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DestroyEvent, type, timestamp, factory_id);

    inline static std::string type = "destroy-destroy-event";
};

class StartEvent : public FactoryEvent {
public:
    StartEvent(int timestamp, const Factory *factory, const Recipe &recipe)
        : FactoryEvent(timestamp, event::factory_to_id(factory)),
          recipe(recipe.get_name()) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StartEvent, type, timestamp, factory_id, recipe);

    std::string get_recipe() const { return recipe; }

    inline static std::string type = "start-factory-event";

private:
    std::string recipe;
};

class StopEvent : public FactoryEvent {
public:
    StopEvent(int timestamp, const Factory *factory)
        : FactoryEvent(timestamp, event::factory_to_id(factory)) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StopEvent, type, timestamp, factory_id);

    inline static std::string type = "stop-factory-event";
};

class VictoryEvent : public Event {
public:
    VictoryEvent(int timestamp) : Event(timestamp) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VictoryEvent, type, timestamp);

    inline static std::string type = "victory-event";
};
