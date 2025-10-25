#pragma once

#include <iostream>
#include <type_traits>

template <typename U, typename... Types>
struct ContainsType {
    static constexpr bool value = (std::is_same_v<U, Types> || ...);
};

template <typename... Types>
class UniqueTypeList;

template <typename U, typename... Types>
class UniqueTypeList<U, Types...> {
   public:
    template <typename T>
    constexpr const T& get() const {
        return getInner<T>();
    }

    template <typename T>
    constexpr T& get() {
        return getInner<T>();
    }

   private:
    template <typename T>
    constexpr T& getInner() {
        if constexpr (std::is_same_v<U, T>) {
            return head;
        } else {
            return tail.template get<T>();
        }
    }

    U head;
    UniqueTypeList<Types...> tail;
};

template <typename U>
class UniqueTypeList<U> {
   public:
    template <typename T>
    constexpr const T& get() const {
        return getInner<T>();
    }

    template <typename T>
    constexpr T& get() {
        return getInner<T>();
    }

   private:
    template <typename T>
    constexpr T& getInner() {
        constexpr bool isSame = std::is_same_v<U, T>;
        if constexpr (isSame) {
            return head;
        } else {
            static_assert(isSame, "Type not found in UniqueTypeList");
        }
    }

    U head;
};

template <typename... Views>
class ListView;

template <typename V, typename... Views>
class ListView<V, Views...> {
   public:
    template <typename List>
    constexpr ListView(List& list)
        : head(list.template get<V>()), tail(ListView<Views...>{list}) {}

    template <typename T>
    constexpr const T& get() const {
        return getInner<T>();
    }

    template <typename T>
    constexpr T& get() {
        return getInner<T>();
    }

   private:
    template <typename T>
    constexpr T& getInner() {
        if constexpr (std::is_same_v<T, V>) {
            return head;
        } else {
            return tail.template get<T>();
        }
    }

    V& head;
    ListView<Views...> tail;
};

template <typename V>
class ListView<V> {
   public:
    template <typename List>
    constexpr ListView(List& list) : head(list.template get<V>()) {}

    template <typename T>
    constexpr const T& get() const {
        return getInner<T>();
    }

    template <typename T>
    constexpr T& get() {
        return getInner<T>();
    }

   private:
    template <typename T>
    constexpr T& getInner() {
        constexpr bool isSame = std::is_same_v<T, V>;
        if constexpr (isSame) {
            return head;
        } else {
            static_assert(isSame, "Type not found in UniqueTypeList");
        }
    }

    V& head;
};

template <typename U, typename... T>
constexpr auto make_uniqueTypeListBuilder();

template <typename... Types>
class UniqueTypeListBuilder {
   public:
    using TypeList = UniqueTypeList<Types...>;

    constexpr TypeList build() const { return TypeList{}; }

   private:
    template <typename U, typename... T>
    friend constexpr auto make_uniqueTypeListBuilder();

    template <typename U>
    constexpr auto pushType() const {
        if constexpr (ContainsType<U, Types...>::value) {
            return *this;
        } else {
            return UniqueTypeListBuilder<U, Types...>{};
        }
    }
};

template <typename U, typename... Types>
constexpr auto make_uniqueTypeListBuilder() {
    if constexpr (sizeof...(Types) == 0) {
        return UniqueTypeListBuilder<U>{};
    } else {
        UniqueTypeListBuilder typeList{make_uniqueTypeListBuilder<Types...>()};
        return typeList.template pushType<U>();
    }
}

template <typename... Types>
using UniqueTypeListType =
    decltype(make_uniqueTypeListBuilder<Types...>().build());

template <typename... Types1, typename... Types2>
constexpr auto operator+(UniqueTypeList<Types1...>, UniqueTypeList<Types2...>) {
    return make_uniqueTypeListBuilder<Types1..., Types2...>().build();
}

template <typename... Stages>
class Pipeline;

template <typename... Resources>
class PipelineStage {
   public:
    using ResourceList = UniqueTypeListType<Resources...>;
    using ResourceView = ListView<Resources...>;

   private:
    template <typename... Stages>
    friend class Pipeline;

    template <typename... Types>
    void executeOuter(UniqueTypeList<Types...>& resources) {
        execute(ResourceView{resources});
    }

   protected:
    virtual void execute(ResourceView&& resources) = 0;
};

template <typename... Stages>
class Pipeline {
   public:
    using StageList = UniqueTypeListType<Stages...>;
    using ResourceStorage = decltype((typename Stages::ResourceList{} + ...));

    void execute() { (..., m_stages.template get<Stages>().executeOuter(m_resources)); }

   private:
    StageList m_stages;
    ResourceStorage m_resources;
};

class TestPipelineStage1 : public PipelineStage<int, double> {
   protected:
    void execute(ResourceView&& resources) override {
        std::cout << "Executing stage 1 with int and double" << std::endl;
        resources.get<int>() = 42;
        resources.get<double>() = 3.14;
    }
};

class TestPipelineStage2 : public PipelineStage<int, float> {
   protected:
    void execute(ResourceView&& resources) override {
        std::cout << "Executing stage 2 with int and float" << std::endl;
        resources.get<int>() += 1;
        resources.get<float>() = 2.71f;
    }
};

class TestPipelineStage3 : public PipelineStage<double, char> {
   protected:
    void execute(ResourceView&& resources) override {
        std::cout << "Executing stage 3 with double and char" << std::endl;
        resources.get<double>() *= 2.0;
        resources.get<char>() = 'A';
    }
};

class TestPipelineStage4 : public PipelineStage<int, double, float, char> {
   protected:
    void execute(ResourceView&& resources) override {
        std::cout << "Executing stage 4" << std::endl;
        std::cout << "Final values: int = " << resources.get<int>()
                  << ", double = " << resources.get<double>()
                  << ", float = " << resources.get<float>()
                  << ", char = " << resources.get<char>() << std::endl;
    }
};

using TestPipeline = Pipeline<TestPipelineStage1, TestPipelineStage2,
                              TestPipelineStage3, TestPipelineStage4>;
