#pragma once
#include "intrusive_tree.h"
#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  private:
    struct left_tag;
    struct right_tag;

    using left_map_t = intrusive_tree::bitree<Left, CompareLeft, left_tag>;
    using right_map_t = intrusive_tree::bitree<Right, CompareRight, right_tag>;
    using left_node_t = typename left_map_t::d_node;
    using right_node_t = typename right_map_t::d_node;

    template <typename Tag>
    using map_t = std::conditional_t<std::is_same_v<Tag, left_tag>, left_map_t,
                                     right_map_t>;
    template <typename Tag>
    using oposite_tag =
        std::conditional_t<std::is_same_v<Tag, left_tag>, right_tag, left_tag>;
    template <typename Tag>
    using value_t =
        std::conditional_t<std::is_same_v<Tag, left_tag>, Left, Right>;

    struct node_t : left_node_t, right_node_t {

        node_t(Left const &a, Right const &b)
            : left_node_t(a), right_node_t(b) {}

        node_t(Left const &a, Right &&b)
            : left_node_t(a), right_node_t(std::move(b)) {}

        node_t(Left &&a, Right const &b)
            : left_node_t(std::move(a)), right_node_t(b) {}

        node_t(Left &&a, Right &&b)
            : left_node_t(std::move(a)), right_node_t(std::move(b)) {}
    };

    left_map_t left_map;
    right_map_t right_map;
    size_t pair_count = 0;

    template <typename My_tag> struct bimap_iterator {
      private:
        using my_it_t = typename map_t<My_tag>::iterator;
        using other_tag = oposite_tag<My_tag>;
        using other_it_t = typename map_t<other_tag>::iterator;
        using oposite_node = typename map_t<other_tag>::d_node;

      public:
        bimap_iterator(my_it_t it) : bitree_it(it){};

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        value_t<My_tag> const &operator*() const { return *bitree_it; }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        bimap_iterator &operator++() {
            ++bitree_it;
            return *this;
        }

        bimap_iterator operator++(int) {
            auto it = *this;
            bitree_it++;
            return it;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        bimap_iterator &operator--() {
            --bitree_it;
            return *this;
        }

        bimap_iterator operator--(int) {
            auto it = *this;
            bitree_it--;
            return it;
        }

        bool operator==(bimap_iterator const &s) const {
            return bitree_it == s.bitree_it;
        }

        bool operator!=(bimap_iterator const &s) const {
            return bitree_it != s.bitree_it;
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        bimap_iterator<other_tag> flip() const {
            return other_it_t(static_cast<oposite_node const *>(
                static_cast<node_t const *>(bitree_it.get_data())));
        }

      private:
        my_it_t bitree_it;

        friend bimap;
    };

  public:
    using left_iterator = bimap_iterator<left_tag>;
    using right_iterator = bimap_iterator<right_tag>;

    // Создает bimap не содержащий ни одной пары.
    bimap(CompareLeft cmpLeft = CompareLeft(),
          CompareRight cmpRight = CompareRight())
        : left_map(std::move(cmpLeft)), right_map(std::move(cmpRight)){};

    // Конструкторы от других и присваивания
    bimap(bimap const &other)
        : left_map(other.left_map.get_compare()),
          right_map(other.right_map.get_compare()),
          pair_count(other.pair_count) {
        insert_from(other);
    }

    bimap(bimap &&other) noexcept
        : left_map(std::move(other.left_map)),
          right_map(std::move(other.right_map)), pair_count(other.pair_count) {
        other.pair_count = 0;
    }

    bimap &operator=(bimap const &other) {
        if (this != &other) {
            bimap temp(other);
            swap(*this, temp);
        }
        return *this;
    }

    bimap &operator=(bimap &&other) noexcept {
        if (this != &other) {
            left_map = std::move(other.left_map);
            right_map = std::move(other.right_map);
            pair_count = other.pair_count;
            other.pair_count = 0;
        }
        return *this;
    }

    // Деструктор. Вызывается при удалении объектов bimap.
    // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
    // (включая итераторы ссылающиеся на элементы следующие за последними).
    ~bimap() {
        auto it = begin_left();
        while (it != end_left()) {
            it = erase_left(it);
        }
    };

    void erase_all() { pair_count = 0; }

    // Вставка пары (left, right), возвращает итератор на left.
    // Если такой left или такой right уже присутствуют в bimap, вставка не
    // производится и возвращается end_left().
    template <typename L = Left, typename R = Right>
    left_iterator insert(L &&left, R &&right) {
        auto ret = left_map.end();
        if (left_map.find(left) == left_map.end() &&
            right_map.find(right) == right_map.end()) {

            auto *node =
                new node_t(std::forward<L>(left), std::forward<R>(right));
            right_map.insert(static_cast<right_node_t *>(node));
            ret = left_map.insert(static_cast<left_node_t *>(node));
            ++pair_count;
        }
        return ret;
    }

    // Удаляет элемент и соответствующий ему парный.
    // erase невалидного итератора неопределен.
    // erase(end_left()) и erase(end_right()) неопределены.
    // Пусть it ссылается на некоторый элемент e.
    // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к
    // e.
    left_iterator erase_left(left_iterator it) {
        left_iterator ret(left_map.erase(it.bitree_it));
        right_map.erase(it.flip().bitree_it);
        delete static_cast<node_t const *>(it.bitree_it.get_data());
        --pair_count;
        return ret;
    }
    // Аналогично erase, но по ключу, удаляет элемент если он присутствует,
    // иначе не делает ничего Возвращает была ли пара удалена
    bool erase_left(Left const &left) {
        auto it = find_left(left);
        if (it == end_left()) {
            return false;
        } else {
            erase_left(it);
            return true;
        }
    }

    right_iterator erase_right(right_iterator it) {
        right_iterator ret(right_map.erase(it.bitree_it));
        left_map.erase(it.flip().bitree_it);
        delete static_cast<node_t const *>(it.bitree_it.get_data());
        --pair_count;
        return ret;
    }

    bool erase_right(Right const &right) {
        auto it = find_right(right);
        if (it == end_right()) {
            return false;
        } else {
            erase_right(it);
            return true;
        }
    }

    // Возвращает итератор по элементу. Если не найден - соответствующий end()
    left_iterator find_left(Left const &left) const {
        return left_iterator{left_map.find(left)};
    }
    right_iterator find_right(Right const &right) const {
        return right_iterator{right_map.find(right)};
    }

    // Возвращает противоположный элемент по элементу
    // Если элемента не существует -- бросает std::out_of_range
    Right const &at_left(Left const &key) const {
        auto it = find_left(key);
        if (it == end_left()) {
            throw std::out_of_range("Left key not exist");
        } else {
            return *it.flip();
        }
    }
    Left const &at_right(Right const &key) const {
        auto it = find_right(key);
        if (it == end_right()) {
            throw std::out_of_range("Left key not exist");
        } else {
            return *it.flip();
        }
    }

    // Возвращает противоположный элемент по элементу
    // Если элемента не существует, добавляет его в bimap и на противоположную
    // сторону кладет дефолтный элемент, ссылку на который и возвращает
    // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
    // соответствующий ему элемент на запрашиваемый (смотри тесты)
    template <typename T = Right,
              std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
    Right const &at_left_or_default(Left const &key) {
        auto it = find_left(key);
        if (it == end_left()) {
            Right new_elem{};
            erase_right(new_elem);
            return *insert(key, std::move(new_elem)).flip();
        } else {
            return *it.flip();
        }
    }
    template <typename T = Left,
              std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
    Left const &at_right_or_default(Right const &key) {
        auto it = find_right(key);
        if (it == end_right()) {
            Left new_elem{};
            erase_left(new_elem);
            return *insert(std::move(new_elem), key);
        } else {
            return *it.flip();
        }
    }

    // lower и upper bound'ы по каждой стороне
    // Возвращают итераторы на соответствующие элементы
    // Смотри std::lower_bound, std::upper_bound.
    left_iterator lower_bound_left(const Left &left) const {
        return left_iterator{left_map.lower_bound(left)};
    }
    left_iterator upper_bound_left(const Left &left) const {
        return left_iterator{left_map.upper_bound(left)};
    }

    right_iterator lower_bound_right(const Right &right) const {
        return right_iterator{right_map.lower_bound(right)};
    }
    right_iterator upper_bound_right(const Right &right) const {
        return right_iterator{right_map.upper_bound(right)};
    }

    // Возващает итератор на минимальный по величине left.
    left_iterator begin_left() const { return left_iterator(left_map.begin()); }
    // Возващает итератор на следующий за последним по величине left.
    left_iterator end_left() const { return left_iterator(left_map.end()); }

    // Возващает итератор на минимальный по величине right.
    right_iterator begin_right() const {
        return right_iterator(right_map.begin());
    }
    // Возващает итератор на следующий за последним по величине right.
    right_iterator end_right() const { return right_iterator(right_map.end()); }

    // Проверка на пустоту
    bool empty() const {
        assert(left_map.empty() == right_map.empty());
        assert(left_map.empty() == (pair_count == 0));
        return left_map.empty();
    }

    // Возвращает размер бимапы (кол-во пар)
    std::size_t size() const { return pair_count; }

    // операторы сравнения
    friend bool operator!=(bimap const &a, bimap const &b) { return !(a == b); }
    friend bool operator==(bimap const &a, bimap const &b) {
        if (a.size() != b.size())
            return false;
        for (auto it_a = a.begin_left(), it_b = b.begin_left();
             it_a != a.end_left() && it_b != b.end_left(); ++it_a, ++it_b) {
            if (*it_a != *it_b || *it_a.flip() != *it_b.flip())
                return false;
        }
        return true;
    }

    // erase от ренжа, удаляет [first, last), возвращает итератор на последний
    // элемент за удаленной последовательностью
    left_iterator erase_left(left_iterator first, left_iterator last) {
        for (auto it = first; it != last;) {
            erase_left(it++);
        }
        return last;
    }
    right_iterator erase_right(right_iterator first, right_iterator last) {
        for (auto it = first; it != last;) {
            erase_right(it++);
        }
        return last;
    }

    friend void swap(bimap &a, bimap &b) {
        swap(a.left_map, b.left_map);
        swap(a.right_map, b.right_map);
        std::swap(a.pair_count, b.pair_count);
    }

  private:
    void insert_from(bimap const &other) {
        for (auto it = other.begin_left(); it != other.end_left(); it++) {
            insert(*it, *it.flip());
        }
    }
};
