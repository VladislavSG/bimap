#pragma once
#include "intrusive_tree.h"
#include <stdexcept>
#include <type_traits>
#include "helper_functions.h"

template <typename Left, typename Right,
    typename CompareLeft = std::less<Left>,
    typename CompareRight = std::less<Right>>
struct bimap {
    using left_t  = Left;
    using right_t = Right;

  private:
    uint32_t pair_count = 0;

    struct left_tag;
    struct right_tag;

    using left_map_t = intrusive_tree::bitree<left_t, CompareLeft, left_tag>;
    using right_map_t = intrusive_tree::bitree<right_t, CompareRight, right_tag>;
    using left_node_t = typename left_map_t::d_node;
    using right_node_t = typename right_map_t::d_node;
    using left_bitree_it_t = typename left_map_t::iterator;
    using right_bitree_it_t = typename right_map_t::iterator;

    struct node_t :
        left_node_t,
        right_node_t {

        node_t(left_t const& a, right_t const& b)
            : left_node_t(a),
              right_node_t(b) {}

        node_t(left_t const& a, right_t&& b)
            : left_node_t(a),
              right_node_t(std::move(b)) {}

        node_t(left_t&& a, right_t const& b)
            : left_node_t(std::move(a)),
              right_node_t(b) {}

        node_t(left_t&& a, right_t&& b)
            : left_node_t(std::move(a)),
              right_node_t(std::move(b)) {}
    };


    left_map_t left_map;
    right_map_t right_map;

  public:
    struct left_iterator;
    struct right_iterator {
        right_iterator(right_bitree_it_t it) : bitree_it(it) {};

        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        right_t const& operator*() const {
            return (*bitree_it)->key;
        }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        right_iterator& operator++() {
            ++bitree_it;
            return *this;
        }

        right_iterator operator++(int) {
            auto it = *this;
            bitree_it++;
            return it;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        right_iterator& operator--() {
            --bitree_it;
            return *this;
        }

        right_iterator operator--(int) {
            auto it = *this;
            bitree_it--;
            return it;
        }

        bool operator==(right_iterator const& s) const {
            return bitree_it == s.bitree_it;
        }

        bool operator!=(right_iterator const& s) const {
            return bitree_it != s.bitree_it;
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        left_iterator flip() const {
            return left_iterator(left_bitree_it_t(
                static_cast<left_node_t const*>(static_cast<node_t const*>(*bitree_it))));
        }

      private:
        right_bitree_it_t bitree_it;

        friend bimap;
    };
    struct left_iterator {
        left_iterator(left_bitree_it_t it) : bitree_it(it) {};

        left_t const& operator*() const {
            return (*bitree_it)->key;
        }

        left_iterator& operator++() {
            ++bitree_it;
            return *this;
        }
        left_iterator operator++(int) {
            auto it = *this;
            bitree_it++;
            return it;
        }

        left_iterator& operator--() {
            --bitree_it;
            return *this;
        }

        left_iterator operator--(int) {
            auto it = *this;
            bitree_it--;
            return it;
        }

        bool operator==(left_iterator const& s) const {
            return bitree_it == s.bitree_it;
        }

        bool operator!=(left_iterator const& s) const {
            return bitree_it != s.bitree_it;
        }

        right_iterator flip() const {
            return right_iterator(right_bitree_it_t(
                static_cast<right_node_t const*>(static_cast<node_t const*>(*bitree_it))));
        }

      private:
        left_bitree_it_t bitree_it;

        friend bimap;
    };

    // Создает bimap не содержащий ни одной пары.
    bimap() noexcept = default;

    // Конструкторы от других и присваивания
    bimap(bimap const& other) :
            pair_count(other.pair_count) {
        insert_from(other);
    }

    bimap(bimap&& other) noexcept
        : left_map(std::move(other.left_map)),
          right_map(std::move(other.right_map)),
          pair_count(other.pair_count) {
        other.pair_count = 0;
    }

    bimap& operator=(bimap const& other) {
        if (this != &other) {
            erase_all();
            insert_from(other);
            pair_count = other.pair_count;
        }
        return *this;
    }

    bimap& operator=(bimap&& other) noexcept {
        left_map = std::move(other.left_map);
        right_map = std::move(other.right_map);
        pair_count = other.pair_count;
        other.pair_count = 0;
    }

    // Деструктор. Вызывается при удалении объектов bimap.
    // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
    // (включая итераторы ссылающиеся на элементы следующие за последними).
    ~bimap() {
        erase_all();
    };

    void erase_all() {
        for (auto it = begin_left(); it != end_left();) {
            erase_left(it++);
        }
    }

    // TODO use universal reference
    // Вставка пары (left, right), возвращает итератор на left.
    // Если такой left или такой right уже присутствуют в bimap, вставка не
    // производится и возвращается end_left().
    left_iterator insert(left_t const& left, right_t const& right) {
        auto* node = new node_t(left, right);
        right_map.insert(static_cast<right_node_t*>(node));
        auto ret = left_map.insert(static_cast<left_node_t*>(node));
        if (ret != left_map.end())
            ++pair_count;
        return ret;
    }

    left_iterator insert(left_t const& left, right_t&& right) {
        auto* node = new node_t(left, std::move(right));
        right_map.insert(static_cast<right_node_t*>(node));
        auto ret = left_map.insert(static_cast<left_node_t*>(node));
        if (ret != left_map.end())
            ++pair_count;
        return ret;
    }

    left_iterator insert(left_t&& left, right_t const& right) {
        auto* node = new node_t(std::move(left), right);
        right_map.insert(static_cast<right_node_t*>(node));
        auto ret = left_map.insert(static_cast<left_node_t*>(node));
        if (ret != left_map.end())
            ++pair_count;
        return ret;
    }

    left_iterator insert(left_t&& left, right_t&& right) {
        auto* node = new node_t(std::move(left), std::move(right));
        right_map.insert(static_cast<right_node_t*>(node));
        auto ret = left_map.insert(static_cast<left_node_t*>(node));
        if (ret != left_map.end())
            ++pair_count;
        return ret;
    }

    // Удаляет элемент и соответствующий ему парный.
    // erase невалидного итератора неопределен.
    // erase(end_left()) и erase(end_right()) неопределены.
    // Пусть it ссылается на некоторый элемент e.
    // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
    left_iterator erase_left(left_iterator it) {
        left_iterator ret(left_map.erase(*it));
        right_map.erase(*it.flip());
        delete static_cast<node_t const*>(*it.bitree_it);
        --pair_count;
        return ret;
    }
    // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе не делает ничего
    // Возвращает была ли пара удалена
    bool erase_left(left_t const& left) {
        auto it = find_left(left);
        if (it == end_left()) {
            return false;
        } else {
            erase_left(it);
            return true;
        }
    }

    right_iterator erase_right(right_iterator it) {
        right_iterator ret{right_map.erase(*it)};
        left_map.erase(*it.flip());
        delete static_cast<node_t const*>(*it.bitree_it);
        --pair_count;
        return ret;
    }

    bool erase_right(right_t const& right) {
        auto it = find_right(right);
        if (it == end_right()) {
            return false;
        } else {
            erase_right(it);
            return true;
        }
    }

    // Возвращает итератор по элементу. Если не найден - соответствующий end()
    left_iterator  find_left (left_t  const& left)  const {
        return left_iterator{left_map.find(left)};
    }
    right_iterator find_right(right_t const& right) const {
        return right_iterator{right_map.find(right)};
    }

    // Возвращает противоположный элемент по элементу
    // Если элемента не существует -- бросает std::out_of_range
    right_t const& at_left(left_t const& key) const {
        auto it = find_left(key);
        if (it == end_left()) {
            throw std::out_of_range("left_t key not exist");
        } else {
            return *it.flip();
        }
    }
    left_t const& at_right(right_t const& key) const {
        auto it = find_right(key);
        if (it == end_right()) {
            throw std::out_of_range("left_t key not exist");
        } else {
            return *it.flip();
        }
    }

    left_t at_left_or_default(left_t const& key) const {
        auto it = find_left(key);
        if (it == end_left()) {
            return Left();
        } else {
            return *it.flip();
        }
    }
    right_t at_right_or_default(right_t const& key) const {
        auto it = find_right(key);
        if (it == end_right()) {
            return Right();
        } else {
            return *it.flip();
        }
    }

    // lower и upper bound'ы по каждой стороне
    // Возвращают итераторы на соответствующие элементы
    // Смотри std::lower_bound, std::upper_bound.
    left_iterator lower_bound_left(const left_t& left) const {
        return left_iterator{left_map.lower_bound(left)};
    }
    left_iterator upper_bound_left(const left_t& left) const {
        return left_iterator{left_map.upper_bound(left)};
    }

    right_iterator lower_bound_right(const right_t& right) const {
        return right_iterator{right_map.lower_bound(right)};
    }
    right_iterator upper_bound_right(const right_t& right) const {
        return right_iterator{right_map.upper_bound(right)};
    }

    // Возващает итератор на минимальный по величине left.
    left_iterator begin_left() const {
        return left_iterator(left_map.begin());
    }
    // Возващает итератор на следующий за последним по величине left.
    left_iterator end_left() const {
        return left_iterator(left_map.end());
    }

    // Возващает итератор на минимальный по величине right.
    right_iterator begin_right() const {
        return right_iterator(right_map.begin());
    }
    // Возващает итератор на следующий за последним по величине right.
    right_iterator end_right() const {
        return right_iterator(right_map.end());
    }

    // Проверка на пустоту
    bool empty() const {
        assert(left_map.empty() == right_map.empty());
        assert(left_map.empty() == (pair_count == 0));
        return left_map.empty();
    }

    // Возвращает размер бимапы (кол-во пар)
    std::size_t size() const {
        return pair_count;
    }

    // операторы сравнения
    friend bool operator!=(bimap const& a, bimap const& b) {
        for (auto it = a.begin_left(); it != a.end_left(); ++it) {
            auto it2 = b.find_left(*it);
            if (it2 == b.end_left() || !equiv<Right, CompareRight>(*(it.flip()), *(it2.flip())))
                return true;
        }
        return false;
    }
    friend bool operator==(bimap const& a, bimap const& b) {
        return !(a != b);
    }
    // операторы >, < ???????? (лексикографическое сравнение)

    // ==== как бонус если делать нормально ====
    // erase от ренжа
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

  private:
    void insert_from(bimap const& other) {
        for (auto it = other.begin_left(); it != other.end_left(); it++) {
            insert(*it, *it.flip());
        }
    }
};
