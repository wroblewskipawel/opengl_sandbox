#pragma once

#include <any>
#include <iostream>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

class AnyMap {
   public:
    template <typename T>
    bool insert(T item_) {
        std::any item{std::make_shared<T>(std::move(item_))};
        auto [_, inserted] =
            map.insert({std::type_index(typeid(T)), std::move(item)});
        return inserted;
    };

    template <typename T>
    bool erase() {
        auto node = map.extract(std::type_index(typeid(T)));
        return node.empty();
    };

    template <typename T>
    T const& get() const {
        return *std::any_cast<const std::shared_ptr<T>&>(
            map.at(std::type_index(typeid(T))));
    };

    template <typename T>
    T& get_mut() {
        return *std::any_cast<std::shared_ptr<T>&>(
            map.at(std::type_index(typeid(T))));
    };

    template <typename T>
    const std::shared_ptr<const T> try_get() const {
        auto at = map.find(std::type_index(typeid(T)));
        if (at == map.end()) return nullptr;
        return std::any_cast<std::shared_ptr<T>>(at->second);
    };

    template <typename T>
    std::shared_ptr<T> try_get_mut() {
        auto at = map.find(std::type_index(typeid(T)));
        if (at == map.end()) return nullptr;
        return std::any_cast<std::shared_ptr<T>>(at->second);
    };

   private:
    std::unordered_map<std::type_index, std::any> map;
};