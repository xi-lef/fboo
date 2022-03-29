#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "entity.hpp"

class Event {
public:
    Event(long timestamp) : timestamp(timestamp) {}
    virtual ~Event() = default;

    // TODO as_json is bad, duplicate code, but idk how to fix it
    virtual nlohmann::json as_json() const { throw std::bad_function_call(); }
    virtual std::string to_string() const;
    virtual std::string get_type() const { throw std::bad_function_call(); }
    long get_timestamp() const { return timestamp; }

protected:
    long timestamp;
};

using EventList = std::vector<std::shared_ptr<Event>>;
void to_json(nlohmann::json &j, const EventList &l);

class ResearchEvent : public Event {
public:
    inline static std::string type = "research-event";

    ResearchEvent(long timestamp, std::string technology)
        : Event(timestamp), technology(technology) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResearchEvent, type, timestamp, technology);

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_technology() const { return technology; }

private:
    std::string technology;
};

class FactoryEvent : public Event {
public:
    FactoryEvent(long timestamp, FactoryIdMap::fid_t factory_id)
        : Event(timestamp), factory_id(factory_id) {}

    virtual std::string to_string() const override;
    FactoryIdMap::fid_t get_factory_id() const { return factory_id; }

protected:
    FactoryIdMap::fid_t factory_id;
};

class BuildEvent : public FactoryEvent {
public:
    inline static std::string type = "build-factory-event";
    static constexpr int initial = -1;

    BuildEvent(long timestamp, std::string type, std::string name,
               FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id),
          factory_type(type),
          factory_name(name) {}

    BuildEvent(long timestamp, const Factory &factory,
               FactoryIdMap::fid_t factory_id)
        : BuildEvent(timestamp, factory.get_name(), factory.to_string(),
                     factory_id) {}

    friend void to_json(nlohmann::json &j, const BuildEvent &e) {
        // Replace '_' with '-' for the necessary fields.
        j = nlohmann::json{{"type", e.type},
                           {"timestamp", e.timestamp},
                           {"factory-id", e.factory_id},
                           {"factory-type", e.factory_type},
                           {"factory-name", e.factory_name}};
    }

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_factory_type() const { return factory_type; }
    std::string get_factory_name() const { return factory_name; }

private:
    std::string factory_type;
    std::string factory_name;
};

class DestroyEvent : public FactoryEvent {
public:
    inline static std::string type = "destroy-destroy-event";

    DestroyEvent(long timestamp, FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id) {}

    friend void to_json(nlohmann::json &j, const DestroyEvent &e) {
        // Replace '_' with '-' for the necessary fields.
        j = nlohmann::json{{"type", e.type},
                           {"timestamp", e.timestamp},
                           {"factory-id", e.factory_id}};
    }

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string get_type() const override { return type; };
};

class StartEvent : public FactoryEvent {
public:
    inline static std::string type = "start-factory-event";

    StartEvent(long timestamp, FactoryIdMap::fid_t factory_id,
               const Recipe &recipe)
        : StartEvent(timestamp, factory_id, recipe.get_name()) {}

    StartEvent(long timestamp, FactoryIdMap::fid_t factory_id,
               std::string recipe)
        : FactoryEvent(timestamp, factory_id), recipe(recipe) {}

    friend void to_json(nlohmann::json &j, const StartEvent &e) {
        // Replace '_' with '-' for the necessary fields.
        j = nlohmann::json{{"type", e.type},
                           {"timestamp", e.timestamp},
                           {"factory-id", e.factory_id},
                           {"recipe", e.recipe}};
    }

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string to_string() const override;
    std::string get_type() const override { return type; };
    std::string get_recipe() const { return recipe; }

private:
    std::string recipe;
};

class StopEvent : public FactoryEvent {
public:
    inline static std::string type = "stop-factory-event";

    StopEvent(long timestamp, FactoryIdMap::fid_t factory_id)
        : FactoryEvent(timestamp, factory_id) {}

    friend void to_json(nlohmann::json &j, const StopEvent &e) {
        // Replace '_' with '-' for the necessary fields.
        j = nlohmann::json{{"type", e.type},
                           {"timestamp", e.timestamp},
                           {"factory-id", e.factory_id}};
    }

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string get_type() const override { return type; };
};

class VictoryEvent : public Event {
public:
    inline static std::string type = "victory-event";

    VictoryEvent(long timestamp) : Event(timestamp) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VictoryEvent, type, timestamp);

    nlohmann::json as_json() const override {
        nlohmann::json j;
        to_json(j, *this);
        return j;
    }
    std::string get_type() const override { return type; };
};
