#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "entity.hpp"

class Event {
public:
    Event(int timestamp) : timestamp(timestamp) {}

    virtual std::string to_string() const;
    virtual std::string get_type() const { throw std::bad_function_call(); }
    int get_timestamp() const { return timestamp; }

protected:
    int timestamp;
};

using EventList = std::vector<std::shared_ptr<Event>>;

class ResearchEvent : public Event {
public:
    ResearchEvent(int timestamp, std::string technology)
        : Event(timestamp), technology(technology) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResearchEvent, type, timestamp, technology);

    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_technology() const { return technology; }

    inline static std::string type = "research-event";

private:
    std::string technology;
};

class FactoryEvent : public Event {
public:
    FactoryEvent(int timestamp, FactoryIdMap::fid_t factory_id)
        : Event(timestamp), factory_id(factory_id) {}

    virtual std::string to_string() const override;
    FactoryIdMap::fid_t get_factory_id() const { return factory_id; }

protected:
    FactoryIdMap::fid_t factory_id;
};

class BuildEvent : public FactoryEvent {
public:
    BuildEvent(int timestamp, std::string type, std::string name,
               FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id),
          factory_type(type),
          factory_name(name) {}

    BuildEvent(int timestamp, const Factory &factory,
               FactoryIdMap::fid_t factory_id)
        : BuildEvent(timestamp, factory.get_name(), factory.to_string(),
                     factory_id) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BuildEvent, type, timestamp, factory_id,
                                   factory_type);

    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_factory_type() const { return factory_type; }
    std::string get_factory_name() const { return factory_name; }

    inline static std::string type = "build-factory-event";

private:
    std::string factory_type;
    std::string factory_name;
};

class DestroyEvent : public FactoryEvent {
public:
    DestroyEvent(int timestamp, FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DestroyEvent, type, timestamp, factory_id);

    std::string get_type() const override { return type; };

    inline static std::string type = "destroy-destroy-event";
};

class StartEvent : public FactoryEvent {
public:
    StartEvent(int timestamp, FactoryIdMap::fid_t factory_id,
               const Recipe &recipe)
        : StartEvent(timestamp, factory_id, recipe.get_name()) {}

    StartEvent(int timestamp, FactoryIdMap::fid_t factory_id,
               std::string recipe)
        : FactoryEvent(timestamp, factory_id), recipe(recipe) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StartEvent, type, timestamp, factory_id,
                                   recipe);

    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_recipe() const { return recipe; }

    inline static std::string type = "start-factory-event";

private:
    std::string recipe;
};

class StopEvent : public FactoryEvent {
public:
    StopEvent(int timestamp, FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StopEvent, type, timestamp, factory_id);

    std::string get_type() const override { return type; };

    inline static std::string type = "stop-factory-event";
};

class VictoryEvent : public Event {
public:
    VictoryEvent(int timestamp) : Event(timestamp) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VictoryEvent, type, timestamp);

    std::string get_type() const override { return type; };

    inline static std::string type = "victory-event";
};
