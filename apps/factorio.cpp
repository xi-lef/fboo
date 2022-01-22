#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "fboo/entity.hpp"
#include "paths.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    json factory;
    std::ifstream(JSON_FACTORY) >> factory;
    std::vector<Factory> factories;
    for (const auto& [name, val] : factory.items()) {
        //std::cout << name << val << std::endl << std::endl;
        factories.emplace_back(name, val["crafting_speed"], val["crafting_categories"]);
    }

    //for (const auto& r : factories) { std::cout << r << std::endl; }

    json item;
    std::ifstream(JSON_ITEM) >> item;
    std::vector<Item> items;
    for (const auto& [name, val] : item.items()) {
        //std::cout << name << val << std::endl << std::endl;
        items.emplace_back(name, val["type"]);
    }

    //for (const auto& r : items) { std::cout << r << std::endl; }

    json recipe;
    std::ifstream(JSON_RECIPE) >> recipe;
    std::vector<Recipe> recipes;
    for (const auto& [name, val] : recipe.items()) {
        //std::cout << name << val << std::endl << std::endl;
        recipes.emplace_back(name, val["category"], val["energy"], val["enabled"],
                             val["ingredients"], val["products"]);
    }

    //for (const auto& r : recipes) { std::cout << r << std::endl; }

    json technology;
    std::ifstream(JSON_TECHNOLOGY) >> technology;
    std::vector<Technology> technologies;
    for (const auto& [name, val] : technology.items()) {
        //std::cout << name << val << std::endl << std::endl;

        std::vector<std::string> unlocked_recipes;
        for (const auto& e : val["effects"]) {
            if (e["type"] != "unlock-recipe") {
                std::cerr << "invalid field in " << name << ": " << e << std::endl;
                exit(EXIT_FAILURE);
            }
            unlocked_recipes.push_back(e["recipe"]);
        }

        technologies.emplace_back(name, val["prerequisites"], val["ingredients"],
                                  unlocked_recipes);
    }

    //for (const auto& r : technologies) { std::cout << r << std::endl; }

//------------------------------------------------------
//Writing to json-format
//------------------------------------------------------
/*
    std::stringstream ss;
    ss << R"({
        "number":23,
        "string":"Hello World!",
        "array":[1,2,3,4,5]
        })";

    json j2;
    ss >> j2;

    json jTest = {
        {"pi",3.141},
        {"happy",true},
        {"name","Malte"},
        {"nothing",nullptr},
        {"answer",{
                      {"everything",42}
        }}
    };
    std::cout << "Version 1: stringstream" << std::endl;
    std::cout << std::setw(2) <<j2 <<std::endl;
    std::cout << "Version 2: Writing directly to json" << std::endl;
    std::cout << std::setw(2) << jTest <<std::endl;
*/
//------------------------------------------------------
//Iterate over elements
//------------------------------------------------------
/*
    std::ifstream i2("../jsons/example_2.json");
    json j3;
    i2 >> j3;
    std::cout<<"Print out example_2.json"<<std::endl;
    std::cout<<std::setw(2) << j3 << std::endl;
    std::cout<<std::endl;

    //Version 1
    std::cout<<"Version 1: Iterator"<<std::endl;
    for(json::iterator it = j3.begin();it != j3.end();++it){
        std::cout<< *it <<std::endl;
    }
    std::cout<<std::endl;

    //Version 2
    std::cout<<"Version 2: For each Loop"<<std::endl;
    for(auto& element : j3){
        std::cout<< element << std::endl;
    }
    std::cout<<std::endl;

    //Version 3
    std::cout<<"Version 3: Iterator key/value"<<std::endl;
    for(json::iterator it = j3.begin();it != j3.end();++it){
        std::cout<<"Key: "<< it.key()<<" Value: "<<it.value() <<std::endl;
    }
    std::cout<<std::endl;

    //Version 4
    std::cout<<"Version 4: For each Loop key/value"<<std::endl;
    for(auto& element : j3.items()){
        std::cout<<"Key: "<< element.key() << " Value: "<< element.value() << std::endl;
    }
*/
//------------------------------------------------------
//Search for element in JSON
//------------------------------------------------------
/*
    std::ifstream i3("../jsons/example_2.json");
    json j4;
    i3 >> j4;
    std::cout<<"Print out example_2.json"<<std::endl;
    std::cout<<std::setw(2) << j4 << std::endl;
    std::cout<<std::endl;

    if(j4.find("water") != j4.end()){
        std::cout<<"Found Water!"<<std::endl;
    }
*/
//------------------------------------------------------
//Write JSON output
//------------------------------------------------------
/*
    std::ifstream i4("../jsons/example_2.json");
    json j5;
    i4 >> j5;
    std::cout<<"Print out example_2.json"<<std::endl;
    std::cout<<std::setw(2) << j5 << std::endl;
    std::cout<<std::endl;

    std::ofstream o("../jsons/new_json.json");
    o << std::setw(2) << j5 << std::endl;
*/

    return 0;
}
