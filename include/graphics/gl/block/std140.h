#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

namespace gl {

namespace std140 {

template <typename Type>
struct ArrayElement {
    alignas(16) Type item;
};

template <typename Type>
struct AlignedArray {
    using BaseType = typename std::remove_all_extents<Type>::type;
    std::array<ArrayElement<BaseType>, std::extent<Type, 0>::value> data{};
    BaseType& operator[](size_t index) { return data[index].item; }
};

template <typename Type>
struct std140Type {
    static constexpr auto type_fn() {
        if constexpr (std::is_array<Type>::value)
            return AlignedArray<Type>{};
        else
            return Type{};
    }
    using type = decltype(type_fn());
};

template <typename Type>
constexpr size_t alignment() {
    if (std::is_array<Type>::value) {
        return 16;
    } else if (std::is_aggregate<Type>::value) {
        if (sizeof(Type) / 4 < 3) {
            return 8;
        } else {
            return 16;
        }
    } else if (std::is_arithmetic<Type>::value) {
        if (sizeof(Type) / 4 < 2)
            return 4;
        else
            return 8;
    } else {
        return 16;
    }
}

template <typename... Types>
struct Block {};

template <typename First, typename Second, typename... Rest>
struct Block<First, Second, Rest...> {
    alignas(alignment<First>()) typename std140Type<First>::type value;
    Block<Second, Rest...> next;
};

template <typename Type>
struct Block<Type> {
    alignas(alignment<Type>()) typename std140Type<Type>::type value;
};

template <size_t index, typename Type>
struct GetHelper {};

template <typename Type, typename... Rest>
struct GetHelper<0, Block<Type, Rest...>> {
    static auto& get(Block<Type, Rest...>& block) { return block.value; }
};

template <size_t index, typename Type, typename... Rest>
struct GetHelper<index, Block<Type, Rest...>> {
    static auto& get(Block<Type, Rest...>& block) {
        return GetHelper<index - 1, Block<Rest...>>::get(block.next);
    }
};

template <size_t index, typename Type, typename... Rest>
auto& get(Block<Type, Rest...>& block) {
    return GetHelper<index, Block<Type, Rest...>>::get(block);
}

}  // namespace std140

}  // namespace gl