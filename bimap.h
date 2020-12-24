#pragma once
#include <type_traits>
#include "intrusive_map.h"
#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
struct bimap {
    using left_t  = Left;
    using right_t = Right;

  private:
    struct left_tag;
    struct right_tag;

    using left_node_t = simple_node<left_t, left_tag>;
    using right_node_t = simple_node<right_t, right_tag>;

    struct composite_node :
        left_node_t,
        right_node_t {

        composite_node(Left const& a, Right const& b)
            : left_t::value(a),
              right_t::value(b) {}

        composite_node(Left const& a, Right&& b)
            : left_t::value(a),
              right_t::value(std::move(b)) {}

        composite_node(Left&& a, Right const& b)
            : left_t::value(std::move(a)),
              right_t::value(b) {}

        composite_node(Left&& a, Right&& b)
            : left_t::value(std::move(a)),
              right_t::value(std::move(b)) {}
    };

    using node_t = composite_node;

    intrusive_map<left_t, left_tag> left_map;
    intrusive_map<right_t, right_tag> right_map;

  public:
    struct left_iterator;
    struct right_iterator {
        // Элемент на который сейчас ссылается итератор.
        // Разыменование итератора end_left() неопределено.
        // Разыменование невалидного итератора неопределено.
        right_t const& operator*() const {
            return *intrusive_it;
        }

        // Переход к следующему по величине left'у.
        // Инкремент итератора end_left() неопределен.
        // Инкремент невалидного итератора неопределен.
        right_iterator& operator++() {
            intrusive_it++;
            return *this;
        }

        right_iterator operator++(int) {
            auto it = *this;
            intrusive_it++;
            return it;
        }

        // Переход к предыдущему по величине left'у.
        // Декремент итератора begin_left() неопределен.
        // Декремент невалидного итератора неопределен.
        right_iterator& operator--() {
            intrusive_it--;
            return *this;
        }
        right_iterator operator--(int) {
            auto it = *this;
            intrusive_it--;
            return it;
        }

        // left_iterator ссылается на левый элемент некоторой пары.
        // Эта функция возвращает итератор на правый элемент той же пары.
        // end_left().flip() возращает end_right().
        // end_right().flip() возвращает end_left().
        // flip() невалидного итератора неопределен.
        left_iterator flip() const {
            return left_iterator{intrusive_map_iterator<left_t, left_tag>
                {&static_cast<left_node_t>(static_cast<node_t>(*intrusive_it))}};
        }

      private:
        intrusive_map_iterator<right_t, right_tag> intrusive_it;
    };
    struct left_iterator {
        left_t const& operator*() const {
            return *intrusive_it;
        }

        left_iterator& operator++() {
            intrusive_it++;
            return *this;
        }
        left_iterator operator++(int) {
            auto it = *this;
            intrusive_it++;
            return it;
        }

        left_iterator& operator--() {
            intrusive_it--;
            return *this;
        }
        left_iterator operator--(int) {
            auto it = *this;
            intrusive_it--;
            return it;
        }

        right_iterator flip() const {
            return left_iterator{intrusive_map_iterator<right_t, right_tag>
                {&static_cast<right_node_t>(static_cast<node_t>(*intrusive_it))}};
        }

      private:
        intrusive_map_iterator<left_t, left_tag> intrusive_it;
    };

    // Создает bimap не содержащий ни одной пары.
    bimap() noexcept = default;

    // Конструкторы от других и присваивания
    bimap(bimap const& other) = delete;
    bimap(bimap&& other) noexcept
        : left_map(std::move(other.left_map)),
          right_map(std::move(other.right_map)) {}

    bimap& operator=(bimap const& other) = delete;
    bimap& operator=(bimap&& other) noexcept {
        left_map = std::move(other.left_map);
        right_map = std::move(other.right_map);
    }

    // Деструктор. Вызывается при удалении объектов bimap.
    // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
    // (включая итераторы ссылающиеся на элементы следующие за последними).
    ~bimap();

    // Вставка пары (left, right), возвращает итератор на left.
    // Если такой left или такой right уже присутствуют в bimap, вставка не
    // производится и возвращается end_left().
    left_iterator insert(left_t const& left, right_t const& right) {
        auto* node = new node_t(left, right);
        right_map.insert(static_cast<right_node_t>(*node));
        return left_map.insert(static_cast<left_node_t>(*node));
    }

    left_iterator insert(left_t const& left, right_t&& right) {
        auto* node = new node_t(left, std::move(right));
        right_map.insert(static_cast<right_node_t>(*node));
        return left_map.insert(static_cast<left_node_t>(*node));
    }

    left_iterator insert(left_t&& left, right_t const& right) {
        auto* node = new node_t(std::move(left), right);
        right_map.insert(static_cast<right_node_t>(*node));
        return left_map.insert(static_cast<left_node_t>(*node));
    }

    left_iterator insert(left_t&& left, right_t&& right) {
        auto* node = new node_t(std::move(left), std::move(right));
        right_map.insert(static_cast<right_node_t>(*node));
        return left_map.insert(static_cast<left_node_t>(*node));
    }

    // Удаляет элемент и соответствующий ему парный.
    // erase невалидного итератора неопределен.
    // erase(end_left()) и erase(end_right()) неопределены.
    // Пусть it ссылается на некоторый элемент e.
    // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
    left_iterator erase_left(left_iterator it) {
        left_map.erase(it.intrusive_it);
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
        right_map.erase(it.in)
    }
    bool erase_right(right_t const& right) {
        auto it = find_right(right);
        if (it == end_left()) {
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
        return left_iterator{left_map.begin()};
    }
    // Возващает итератор на следующий за последним по величине left.
    left_iterator end_left() const {
        return left_iterator{left_map.end()};
    }

    // Возващает итератор на минимальный по величине right.
    right_iterator begin_right() const {
        return right_iterator{right_map.begin()};
    }
    // Возващает итератор на следующий за последним по величине right.
    right_iterator end_right() const {
        return right_iterator{right_map.end()};
    }

    // Проверка на пустоту
    bool empty() const {
        return left_map.empty();
    }

    // Возвращает размер бимапы (кол-во пар)
    std::size_t size() const {
        return left_map.size();
    }

    // зачем? кому это нужно? TODO
    // операторы сравнения
    friend bool operator==(bimap const& a, bimap const& b);
    friend bool operator!=(bimap const& a, bimap const& b);
    // операторы >, < ???????? (лексикографическое сравнение)

    // ==== как бонус если делать нормально ====
    // erase от ренжа
    left_iterator erase_left(left_iterator first, left_iterator last) {
        return left_iterator{left_map.erase(first.intrusive_it, last.intrusive_it)};
    }
    right_iterator erase_right(right_iterator first, right_iterator last) {
        return right_iterator{right_map.erase(first.intrusive_it, last.intrusive_it)};
    }
};
