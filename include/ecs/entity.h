#pragma once

#include <cstddef>
#include <optional>
#include <tuple>

template <typename C, typename Tuple>
struct tuple_type_index;

template <typename C>
struct tuple_type_index<C, std::tuple<>> {
    static constexpr size_t index = 0;
};

template <typename C, typename... Members>
struct tuple_type_index<C, std::tuple<C, Members...>> {
    static constexpr size_t index = 0;
};

template <typename C, typename F, typename... Members>
struct tuple_type_index<C, std::tuple<F, Members...>> {
    static constexpr size_t index =
        1 + tuple_type_index<C, std::tuple<Members...>>::index;
};

template <typename T, typename... Members>
class Entity {
   public:
    template <typename A>
    void set(const A& component) {
        get<A>() = component;
    }

    template <typename A>
    void remove(const A& component) {
        get<A>() = std::nullopt;
    }

    template <typename A, typename... Args>
    std::optional<std::tuple<A&, Args&...>> match() {
        if (!match_component<A, Args...>(get<A>(), get<Args>()...)) {
            return std::nullopt;
        }
        return std::tuple<A&, Args&...>(get<A>().value(),
                                        get<Args>().value()...);
    }

   private:
    template <typename C>
    std::optional<C>& get() {
        return std::get<tuple_type_index<C, std::tuple<T, Members...>>::index>(
            data);
    };

    template <typename A, typename... Args>
    bool match_component(std::optional<A>& arg, std::optional<Args>&... args) {
        if (arg.has_value()) {
            return match_component<Args...>(args...);
        }
        return false;
    }

    template <typename A>
    bool match_component(std::optional<A>& arg) {
        return arg.has_value();
    }
    std::tuple<std::optional<T>, std::optional<Members>...> data;
};
