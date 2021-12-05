#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <variant>
#include <vector>

const size_t DEFAULT_CAPACITY = 128;

template <typename T>
class GenerationVector {
   public:
    class Index {
       private:
        friend GenerationVector;

        Index(size_t index_, size_t generation_)
            : INDEX{index_}, GENERATION{generation_} {};
        const size_t INDEX;
        const size_t GENERATION;
    };

    GenerationVector(size_t capacity = DEFAULT_CAPACITY)
        : generation{0}, next_free_entry{0} {
        items.resize(capacity);
        for (size_t i{0}; i < items.size(); i++) {
            items[i] = i + 1;
        };
        items.back() = std::numeric_limits<size_t>::max();
    };

    Index insert(const T& item) {
        size_t free_entry = next_free_entry;
        next_free_entry = std::get<size_t>(items[free_entry]);
        if (next_free_entry == std::numeric_limits<size_t>::max()) {
            size_t num_items = items.size();
            items.reserve(num_items * 2);
            for (size_t i{num_items}; i < items.size(); i++) {
                items[i] = i + 1;
            }
            items.back() = std::numeric_limits<size_t>::max();
            next_free_entry = num_items;
        }
        items[free_entry] = Entry{item, generation};
        num_items++;
        return Index{free_entry, generation};
    }

    void remove(const Index& index) {
        items[index.INDEX] = next_free_entry;
        next_free_entry = index.INDEX;
        num_items--, generation++;
    };

    T const& get(Index index) {
        Entry& entry{std::get<Entry>(items[index.INDEX])};
        if (entry.generation != index.GENERATION) {
            throw std::logic_error{"Invalid index generation"};
        }
        return entry.item;
    }

    T& get_mut(Index index) {
        Entry& entry{std::get<Entry>(items[index.INDEX])};
        if (entry.generation != index.GENERATION) {
            throw std::logic_error{"Invalid index generation"};
        }
        return entry.item;
    }

        size_t size() { return num_items; }
    size_t capacity() { return items.capacity(); }

   private:
    struct Entry {
        Entry(const T& item_, size_t generation_)
            : item{item_}, generation{generation_} {};
        T item;
        size_t generation;
    };
    std::vector<std::variant<size_t, Entry>> items;
    size_t next_free_entry;
    size_t generation;
    size_t num_items;
};