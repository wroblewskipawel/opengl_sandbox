#pragma once

#include <iostream>
#include <optional>
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

template <typename... Types>
struct TypeCell;

template <>
class TypeCell<> {
   public:
    template <typename U>
    const U& get() const {
        static_assert(false, "Type not found in TypeCell");
    }

    template <typename U>
    U& get() {
        static_assert(false, "Type not found in TypeCell");
    }

    template <typename Func>
    void forEach(Func&& func) {}

   private:
    template <typename... T>
    friend class StaticTypeMap;

    template <typename... T>
    friend class TypeCell;

    template <typename U>
    std::optional<U>& store() {
        static_assert(false, "Type not found in TypeCell");
    }
};

template <typename T, typename... Types>
class TypeCell<T, Types...> {
   public:
    template <typename U>
    const U& get() const {
        return getInner<U>();
    }

    template <typename U>
    U& get() {
        return getInner<U>();
    }

    template <typename Func>
    void forEach(Func&& func) {
        if (data.has_value()) {
            func(data.value());
        }
        next.forEach(std::forward<Func>(func));
    };

   private:
    template <typename...>
    friend class StaticTypeMap;

    template <typename...>
    friend class TypeCell;

    template <typename U>
    U& getInner() {
        if constexpr (std::is_same<T, U>::value) {
            if (data.has_value()) {
                return data.value();
            } else {
                throw std::runtime_error("Value not initialized!");
            }
        } else {
            return next.template get<U>();
        }
    };

    template <typename U>
    std::optional<U>& store() {
        if constexpr (std::is_same<T, U>::value) {
            return data;
        } else {
            return next.template store<U>();
        }
    };

    std::optional<T> data;
    TypeCell<Types...> next;
};

template <typename... Types>
class StaticTypeMap {
   public:
    template <typename T, typename... Args>
    StaticTypeMap& insert(Args&&... args) {
        auto& cell = cells.template store<T>();
        cell = T(std::forward<Args>(args)...);
        return *this;
    }

    template <typename Func>
    void forEach(Func&& func) {
        cells.forEach(std::forward<Func>(func));
    }

    template <typename T>
    T& at() {
        return cells.template get<T>();
    }

    template <typename T>
    const T& at() const {
        return cells.template get<T>();
    }

   private:
    TypeCell<Types...> cells;
};
