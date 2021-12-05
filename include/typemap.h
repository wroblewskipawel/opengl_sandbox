#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <variant>

template <typename T, typename... Types>
static constexpr bool contains_type() {
    return std::disjunction_v<std::is_same<T, Types>...>;
};

template <typename... Types>
class TypeMap {
   public:
    template <typename T>
    bool insert_resource() {
        resource_map.emplace(std::type_index(typeid(T)), T());
        return true;
    }

    TypeMap() { auto dummy = {insert_resource<Types>()...}; }

    template <typename T>
    T& at() {
        static_assert(contains_type<T, Types...>(), "Type T not in Resources");
        return std::get<T>(resource_map.at(std::type_index(typeid(T))));
    }

    template <typename T>
    const T& at() const {
        static_assert(contains_type<T, Types...>(), "Type T not in Resources");
        return std::get<T>(resource_map.at(std::type_index(typeid(T))));
    }

   private:
    template <typename... Views>
    friend class ResourceView;

    std::unordered_map<std::type_index, std::variant<Types...>> resource_map;
};
