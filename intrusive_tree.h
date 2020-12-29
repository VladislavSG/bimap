//
// Created by VladSG on 22.12.2020.
//

#pragma once
#include <utility>
#include <optional>
#include <cassert>
#include <random>
#include "helper_functions.h"

namespace intrusive_tree {
struct default_tag;



template <typename T, typename Compare = std::less<T>, typename Tag = default_tag>
struct bitree {
    struct d_node;

  private:
    struct p_node {
        p_node *left = nullptr;
        p_node *right = nullptr;
        p_node *parent = nullptr;

        d_node& get_data() {
            assert(parent);
            return static_cast<d_node&>(*this);
        }

        bool isLeft() {
            assert(parent);
            return parent->left == this;
        }
    };

  public:
    struct d_node : private p_node {
        T key;

        explicit d_node(T key) noexcept
            : key(std::move(key)),
              priority(rand_gen())
        {}

      private:
        const uint_fast32_t priority;
    };

    struct iterator {
        iterator(p_node* n) : it_node(n) {}

        d_node* operator*() const {
            return &(it_node->get_data());
        }

        iterator& operator++() {
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

        iterator& operator--() {
            return *this;
        }

        iterator operator--(int) {
            auto it = *this;
            operator--();
            return it;
        }

        bool operator==(iterator const& s) const {
            return it_node == s.it_node;
        }

        bool operator!=(iterator const& s) const {
            return it_node != s.it_node;
        }

      private:
        p_node* it_node;
    };

    bitree(bitree&& other)  noexcept
            : fake_node(other.fake_node) {
        other.root = new p_node();
    }

    bitree& operator=(bitree&& other) noexcept {
        fake_node = other.fake_node;
        other.root = new p_node();
    }


    iterator find(T const& x) const {
        p_node* n = fake_node->left;
        while (!n) {
            if (comp(x, n->get_data().key)) {
                n = n->left;
            } else if (comp(n->get_data().key, x)) {
                n = n->right;
            } else {
                return n;
            }
        }
        return end();
    }


    iterator lower_bound(T const& x) const {
        p_node* n = fake_node->left;
        while (!n) {
            if (comp(x, n->get_data().key)) {
                n = n->left;
            } else if (comp(n->get_data().key, x)) {
                n = n->right;
            } else {
                return n;
            }
        }
        return end();
    }

    iterator upper_bound(T const& x) const {
        p_node* n = fake_node->left;
        while (!n) {
            if (comp(x, n->get_data().key)) {
                n = n->left;
            } else if (comp(n->get_data().key, x)) {
                n = n->right;
            } else {
                return n;
            }
        }
        return end();
    }

    iterator begin() const {
        p_node* cur = fake_node;
        while (cur->left)
            cur = cur->left;
        return cur;
    }

    iterator end() const {
        return fake_node;
    }

    bool empty() const {
        return fake_node->left == nullptr;
    }

  private:
    p_node *fake_node;
    static std::mt19937 rand_gen;
    Compare comp;

    void split (p_node* t, T key, p_node* & l, p_node* & r) {
        if (!t) {
            l = r = nullptr;
        } else if (comp(key, t->get_data().key)) {
            split(t->left, key, l, t->left);
            if (!t->left)
                t->left->parent = t;
            r = t;
        } else {
            split(t->right, key, t->right, r);
            if (!t->right)
                t->right->parent = t;
            l = t;
        }
    }

    void insert (p_node* & t, p_node* it) {
        if (!t) {
            t = it;
        } else {
            it->parent = t->parent;
            if (it->get_data().priority > t->get_data().priority) {
                split(t, it->key, it->left, it->right);
                if (!it->left)
                    it->left->parent = it;
                if (!it->right)
                    it->right->parent = it;
                t = it;
            } else {
                insert(it->key < t->key ? t->left : t->right, it);
            }
        }
    }

    void merge (p_node* & t, p_node* l, p_node* r) {
        if (!l || !r) {
            t = l ? l : r;
        } else if (l->get_data().priority > r->get_data().priority) {
            merge(l->right, l->right, r);
            if (!l->right)
                l->right->parent = l;
            t = l;
        } else {
            merge(r->left, l, r->left);
            if (!r->left)
                r->left->parent = r;
            t = r;
        }
    }

    void erase (p_node* & t, T key) {
        if (!comp(t->get_data().key, key) && !comp(key, t->get_data().key)) {
            if (!t->left)
                t->left->parent = t->parent;
            if (!t->right)
                t->right->parent = t->parent;
            merge(t, t->left, t->right);
        } else {
            erase(comp(key, t->key) ? t->left : t->right, key);
        }
    }
};
}
