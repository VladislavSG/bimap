//
// Created by VladSG on 22.12.2020.
//

#pragma once
#include <cassert>
#include <random>
#include <utility>

namespace intrusive_tree {
struct default_tag;

std::mt19937 rand_gen(std::random_device{}());

template <typename T, typename Compare = std::less<T>,
          typename Tag = default_tag>
struct bitree : private Compare {
    struct d_node;

    struct p_node {
        p_node *left = nullptr;
        p_node *right = nullptr;
        p_node *parent = nullptr;

        d_node &get_data() {
            assert(parent);
            return static_cast<d_node &>(*this);
        }

        d_node const &get_data() const {
            assert(parent);
            return static_cast<d_node const &>(*this);
        }

        [[nodiscard]] bool isLeft() const {
            assert(parent);
            return parent->left == this;
        }

        void reset() {
            left = nullptr;
            right = nullptr;
            parent = nullptr;
        }
    };

    struct d_node : p_node {
        T key;

        explicit d_node(T key) noexcept
            : key(std::move(key)), priority(rand_gen()) {}

        uint_fast32_t get_pr() { return priority; }

      private:
        const uint_fast32_t priority;
    };

    struct iterator {
        iterator(p_node const *n) : it_node(n) {}
        iterator(d_node *n) : it_node(n) {}

        T const &operator*() const { return it_node->get_data().key; }

        iterator &operator++() {
            if (it_node->right) {
                it_node = it_node->right;
                while (it_node->left)
                    it_node = it_node->left;
            } else {
                while (!it_node->isLeft())
                    it_node = it_node->parent;
                it_node = it_node->parent;
            }
            return *this;
        }

        iterator operator++(int) {
            auto it = *this;
            operator++();
            return it;
        }

        iterator &operator--() {
            if (it_node->left) {
                it_node = it_node->left;
                while (it_node->right)
                    it_node = it_node->right;
            } else {
                while (it_node->isLeft())
                    it_node = it_node->parent;
                it_node = it_node->parent;
            }
            return *this;
        }

        iterator operator--(int) {
            auto it = *this;
            operator--();
            return it;
        }

        bool operator==(iterator const &s) const {
            return it_node == s.it_node;
        }

        bool operator!=(iterator const &s) const {
            return it_node != s.it_node;
        }

        d_node const *get_data() const { return &(it_node->get_data()); }

      private:
        p_node const *it_node;
    };

    explicit bitree(Compare cmp = Compare()) noexcept
        : Compare(std::move(cmp)){};

    bitree(bitree &&other) noexcept
        : Compare(std::move(other)), fake_node(other.fake_node) {
        other.fake_node.reset();
        update_parent(fake_node.left, &fake_node);
    }

    bitree &operator=(bitree &&other) noexcept {
        if (this != &other) {
            static_cast<Compare &>(*this) = static_cast<Compare &&>(other);
            fake_node = other.fake_node;
            other.fake_node.reset();
            update_parent(fake_node.left, &fake_node);
        }
        return *this;
    }

    iterator find(T const &x) const {
        p_node *n = fake_node.left;
        while (n) {
            if (cmp(x, n->get_data().key)) {
                n = n->left;
            } else if (cmp(n->get_data().key, x)) {
                n = n->right;
            } else {
                return iterator(n);
            }
        }
        return end();
    }

    iterator erase(iterator it) {
        p_node *element = const_cast<d_node *>(it.get_data());
        p_node *elem_parent = element->parent;
        ++it;
        if (element->isLeft()) {
            merge(elem_parent->left, element->left, element->right);
            update_parent(elem_parent->left, elem_parent);
        } else {
            merge(elem_parent->right, element->left, element->right);
            update_parent(elem_parent->right, elem_parent);
        }
        return it;
    }

    iterator insert(d_node *x) {
        auto ret = insert(fake_node.left, x);
        update_parent(fake_node.left, &fake_node);
        return ret;
    }

    iterator lower_bound(T const &x) const {
        p_node const *n = fake_node.left;
        p_node const *buf = &fake_node;
        while (n) {
            if (!cmp(n->get_data().key, x)) {
                buf = n;
                n = n->left;
            } else {
                n = n->right;
            }
        }
        return buf;
    }

    iterator upper_bound(T const &x) const {
        p_node const *n = fake_node.left;
        p_node const *buf = &fake_node;
        while (n) {
            if (cmp(x, n->get_data().key)) {
                buf = n;
                n = n->left;
            } else {
                n = n->right;
            }
        }
        return buf;
    }

    iterator begin() const {
        p_node const *cur = &fake_node;
        while (cur->left)
            cur = cur->left;
        return cur;
    }

    iterator end() const { return iterator(&fake_node); }

    [[nodiscard]] bool empty() const { return fake_node.left == nullptr; }

    Compare const &get_compare() const {
        return static_cast<Compare const &>(*this);
    }

    friend void swap(bitree &a, bitree &b) {
        std::swap(a.fake_node, b.fake_node);
        update_parent(a.fake_node.left, &a.fake_node);
        update_parent(b.fake_node.left, &b.fake_node);
    }

  private:
    p_node fake_node;

    bool cmp(T const &a, T const &b) const { return get_compare()(a, b); }

    static void update_parent(p_node *node, p_node *parent) {
        if (node)
            node->parent = parent;
    }

    void split(p_node *t, T const &key, p_node *&l, p_node *&r) {
        if (!t) {
            l = r = nullptr;
        } else if (cmp(key, t->get_data().key)) {
            split(t->left, key, l, t->left);
            update_parent(t->left, t);
            r = t;
        } else {
            split(t->right, key, t->right, r);
            update_parent(t->right, t);
            l = t;
        }
    }

    iterator insert(p_node *&t, p_node *it) {
        if (!t) {
            t = it;
        } else {
            it->parent = t->parent;
            if (it->get_data().get_pr() > t->get_data().get_pr()) {
                split(t, it->get_data().key, it->left, it->right);
                update_parent(it->left, it);
                update_parent(it->right, it);
                t = it;
            } else {
                if (cmp(it->get_data().key, t->get_data().key)) {
                    auto ret = insert(t->left, it);
                    update_parent(t->left, t);
                    return ret;
                } else {
                    auto ret = insert(t->right, it);
                    update_parent(t->right, t);
                    return ret;
                }
            }
        }
        return it;
    }

    void merge(p_node *&t, p_node *l, p_node *r) {
        if (!l || !r) {
            t = l ? l : r;
        } else if (l->get_data().get_pr() > r->get_data().get_pr()) {
            merge(l->right, l->right, r);
            update_parent(l->right, l);
            t = l;
        } else {
            merge(r->left, l, r->left);
            update_parent(r->left, r);
            t = r;
        }
    }
};
} // namespace intrusive_tree
