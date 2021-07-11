#pragma once

#include <functional>
#include <memory>
#include <new>
#include <utility>
#include <type_traits>
#include <numeric>
#include <vector>

namespace fefu {

	enum class State {
		DELETED = 0,
		NOT_DELETE = 1,
		TAIL = 2
	};

	template <typename T, typename K>
	class Node {
	public:
		State state;

		T value;
		K key;

		std::size_t ref_count = 0;
		int height = 0;

		Node* left = nullptr;
		Node* right = nullptr;
		Node* parent = nullptr;

		Node(State _state) : state(_state), value(), key() {}

		Node(T _value, K _key, State _state) : Node(_state), value(_value), key(_key) {}

		Node(T _value, K _key): value(_value), key(_key), state(State::NOT_DELETE) {}

		Node(T _value, K _key, Node* _parent) : Node(_value, _key) {
			this->parent = _parent;
		}
		

		void inc_ref_count() {
			if (this) {
				++ref_count;
			}
		}

		void dec_ref_count() {
			if (this) {
				--ref_count;
			}
		}

		void remove() {
			if (this) {

				std::vector<Node*> deleted_stack;

				deleted_stack.push_back(this);

				while (!deleted_stack.empty()) {

					Node* node = deleted_stack[deleted_stack.size() - 1];
					deleted_stack.pop_back();
					if (node && node->state == State::DELETED && node->ref_count == 0) {

						node->left->dec_ref_count();
						deleted_stack.push_back(node->left);

						node->right->dec_ref_count();
						deleted_stack.push_back(node->right);

						node->parent->dec_ref_count();
						deleted_stack.push_back(node->parent);

						delete node;

					}

				}
			}
		}
	};

	template <typename T, typename K>
	class AVLIterator {
	public:

		template <typename T, typename K,
			typename Compare = std::less<T>>
		friend class AVLTree;

		AVLIterator() noexcept {}

		AVLIterator(const AVLIterator& other) noexcept {
			value = other.value;
			value->inc_ref_count();
		}

		~AVLIterator() {
			if (value) {
				value->dec_ref_count();
				if (value->ref_count == 0) {
					value->remove();

				}

			}
		}

		AVLIterator(Node<T, K>* value) noexcept : value(value) {
			value->inc_ref_count();
		}

		AVLIterator& operator=(AVLIterator& other) {
			*this = AVLIterator(other);
			return *this;

		}

		AVLIterator& operator=(AVLIterator&& other) {
			value->dec_ref_count();
			value->remove();
			value = other.value;
			value->inc_ref_count();
			return *this;

		}

		T& operator*() const {
			return this->value->value;
		}

		T* operator->() const {
			return &this->value->value;
		}

		AVLIterator &operator++() {

			if (value->state != State::TAIL) {

				Node<T, K>* prev_value = value;
				value->dec_ref_count();

				if (value->right) {
					value = value->right;

					while (value->left && prev_value->key < value->left->key) {
						value = value->left;
					}

				} else if (value->parent) {

					if (value->parent->left == value) {

						value = value->parent;

					} else if (value->parent->right == value) {

						while (value->parent != nullptr && value->parent->right == value) {
							value = value->parent;
						}
						value = value->parent;

					} else {

						value = value->parent;

					}
				}
				value->inc_ref_count();

				if (prev_value->ref_count == 0 && prev_value->state == State::DELETED) {
					prev_value->remove();
				}
			}
			return *this;
		}

		// postfix ++
		AVLIterator operator++(int) {

			AVLIterator temp = *this;
			++*this;
			return temp;
		}

		bool operator==(const AVLIterator& other) {

			return other.value == this->value;
		}

		bool operator!=(const AVLIterator& other) {

			return other.value != this->value;
		}

	private:
		Node<T, K>* value = nullptr;

	};

	template <typename T, typename K,
		typename Compare = std::less<T>>
	class AVLTree {
	public:
		using size_type = std::size_t;
		using height_type = std::size_t;
		using bf_type = int;
		using value_compare = Compare;
		using const_reference = const T&;
		using iterator = AVLIterator<T, K>;

		AVLTree(std::initializer_list<std::pair<T, K>> list) : AVLTree() {
			for (auto& it : list) {
				insert(it);
			}
		}

		AVLTree() {
			root = new Node<T, K>(State::TAIL);
		}

		~AVLTree() {

			iterator current = this->begin();
			iterator end = this->end();

			std::vector<Node<T, K>*> stack_deleted;

			while (current != end) {
				stack_deleted.push_back(current.value);
				++current;
			}

			while (!stack_deleted.empty()) {
				delete stack_deleted[stack_deleted.size() - 1];
				stack_deleted[stack_deleted.size() - 1] = nullptr;
				stack_deleted.pop_back();	
			}

			delete current.value;
			current.value = nullptr;
		}

		bool empty() {
			return set_size == 0;
		}

		size_type size() {
			return set_size;
		}

		iterator begin() {
			Node<T, K>* current = root;

			while (current->left) 
				current = current->left;

			return iterator(current);
		}

		iterator end() {
			Node<T, K>* current = root;

			while (current->right) 
				current = current->right;
			
			return iterator(current);
		}

		void insert(T value, K key) {

			Node<T, K> *parent_node = find_node(key);

			if (parent_node->key != key || parent_node->state == State::TAIL) {

				Node<T, K>* new_node = new Node<T, K>(value, key, parent_node);

				if (_compare(new_node->key, parent_node->key)) {

					parent_node->left = new_node;

				} else {

					parent_node->right = new_node;

				}

				if (parent_node->state == State::TAIL && parent_node->right) {

					swap(parent_node, new_node);
					new_node = parent_node;

				}

				++set_size;
				balance_insert(new_node);
			}
		}

		void insert(std::pair<T, K> pair) {
			insert(pair.first, pair.second);
		}

		iterator find(K key) {
			auto it = iterator(find_node(key));
			if (it.value->key != key) {
				return end();
			}
			return it;
		}

		void erase(K key) {
			if (!empty()) {

				Node<T, K>* node = find_node(key);

				if (node->key == key && node->state == State::NOT_DELETE) {

					--set_size;
					Node<T, K>* lower_node = node;
					
					if (node->left) {
						lower_node = get_lower_right_child(node->left);
					} else if (node->right) {
						lower_node = get_lower_left_child(node->right);
					}

					root = (node == root) ? lower_node : root;

					if (lower_node->parent && lower_node->parent != node) {
						if (lower_node->left) {
							change_parent_child(lower_node, lower_node->left, lower_node->parent);
							lower_node->left->parent = lower_node->parent;
						}
						else if (lower_node->right) {
							change_parent_child(lower_node, lower_node->right, lower_node->parent);
							lower_node->right->parent = lower_node->parent;
						}
						else {
							change_parent_child(lower_node, nullptr, lower_node->parent);
						}
					}

					auto balanced_node = (lower_node->parent == node) ? lower_node : lower_node->parent;
					replace_node(lower_node, node);

					while (balanced_node) {
						balance_delete(balanced_node);
						balanced_node = balanced_node->parent;
					}

					delete_node(node);
				}
			}
		}

	private:
		Node<T, K> *root = nullptr;
		size_type set_size = 0;
		value_compare _compare;

		void delete_node(Node<T, K> *node) {
			node->state = State::DELETED;
			if (node->ref_count == 0 ) {
				delete node;
				node = nullptr;
			} else {
				node->parent->inc_ref_count();
				node->left->inc_ref_count();
				node->right->inc_ref_count();
			}
		}

		void change_parent_child(Node<T, K> *old_child, Node<T, K> *new_child, Node<T, K> *parent) {
			if (parent && parent != new_child) {
				if (parent->left == old_child) {
					parent->left = new_child;
				}
				else {
					parent->right = new_child;		
				}
				parent->height = get_height(parent);
			}
		}

		void swap_child(Node<T, K> *lhs, Node<T, K> *rhs) {
			if (lhs->left && lhs->left != rhs) {
				lhs->left->parent = rhs;
			}
			if (lhs->right && lhs->right != rhs) {
				lhs->right->parent = rhs;
			}
			if (rhs->left && rhs->left != lhs) {
				rhs->left->parent = lhs;
			}
			if (rhs->right && rhs->right != lhs) {
				rhs->right->parent = lhs;
			}

			auto *temp = rhs->left;
			rhs->left = (lhs->left == rhs) ? lhs : lhs->left;
			lhs->left = (temp == lhs) ? rhs : temp;

			temp = rhs->right;
			rhs->right = (lhs->right == rhs) ? lhs : lhs->right;
			lhs->right = (temp == lhs) ? rhs : temp;
		}

		void swap_parent(Node<T, K> *lhs, Node<T, K> *rhs) {
			auto *temp = rhs->parent;
			rhs->parent = (lhs->parent == rhs) ? lhs : lhs->parent;
			lhs->parent = (temp == lhs) ? rhs : temp;
		}

		void swap_height(Node<T, K>* lhs, Node<T, K>* rhs) {
			auto height = lhs->height;
			lhs->height = rhs->height;
			rhs->height = height;
		}

		void swap(Node<T, K> *lhs, Node<T, K> *rhs) {
			if (lhs != rhs) {
				change_parent_child(lhs, rhs, lhs->parent);
				change_parent_child(rhs, lhs, rhs->parent);
				swap_child(lhs, rhs);
				swap_parent(lhs, rhs);
				swap_height(lhs, rhs);

				if (!rhs->parent) {
					root = rhs;
				}
				else if (!lhs->parent) {
					root = lhs;
				}
			}
		}

		void replace_node(Node<T, K>* lhs, Node<T, K>* rhs) {
			if (lhs != rhs) {
				if (rhs->left && rhs->left != lhs) {
					rhs->left->parent = lhs;
				}
				if (rhs->right && rhs->right != lhs) {
					rhs->right->parent = lhs;
				}

				if (rhs->left == lhs) {
					lhs->left = (lhs->left) ? lhs->left : lhs->right;
				}
				else {
					lhs->left = rhs->left;
				}

				if (rhs->right == lhs) {
					lhs->right = (lhs->left) ? lhs->left : lhs->right;
				}
				else {
					lhs->right = rhs->right;
				}
				lhs->height = get_height(lhs);

				change_parent_child(rhs, lhs, rhs->parent);
				lhs->parent = rhs->parent;
			}
		}

		Node<T, K> *get_lower_right_child(Node<T, K> *node) {
			while (node->right)
				node = node->right;
			return node;
		}

		Node<T, K> *get_lower_left_child(Node<T, K> *node) {
			while (node->left)
				node = node->left;
			return node;
		}

		height_type get_height(Node<T, K> *node) {
			if (node->left && node->right) {
				if (node->left->height < node->right->height) {
					return node->right->height + 1;
				} else {
					return  node->left->height + 1;
				}
			} else if (node->left && node->right == nullptr) {
				return node->left->height + 1;
			} else if (node->left == nullptr && node->right) {
				return node->right->height + 1;
			}
			return 0;
		}

		bf_type get_bf(Node<T, K>* node) {
			if (node == nullptr) {
				return 0;
			}
			if (node->left && node->right) {
				return node->left->height - node->right->height;
			} else if (node->left && node->right == nullptr) {
				return node->height;
			} else if (node->left == nullptr && node->right) {
				return -node->height;
			}
			return 0;
		}

		void left_leftRotate(Node<T, K> *node) {
			Node<T, K> *child = node->left;

			if (node->parent != nullptr) {
				change_parent_child(node, child, node->parent);
			}

			child->parent = node->parent;
			node->parent = child;

			node->left = child->right;
			child->right = node;
			if (node->left) {
				node->left->parent = node;
			}
			
			node->height = get_height(node);
			child->height = get_height(child);

			if (child->parent == nullptr) {
				root = child;
			}
		}

		void right_rightRotate(Node<T, K> *node) {
			Node<T, K>* child = node->right;

			if (node->parent) {
				change_parent_child(node, child, node->parent);
			}

			child->parent = node->parent;
			node->parent = child;

			node->right = child->left;
			child->left = node;
			if (node->right) {
				node->right->parent = node;
			}

			node->height = get_height(node);
			child->height = get_height(child);

			if (child->parent == nullptr) {
				root = child;
			}
		}

		void left_rightRotate(Node<T, K> *node) {
			right_rightRotate(node->left);
			left_leftRotate(node);
		}

		void right_leftRotate(Node<T, K> *node) {
			left_leftRotate(node->right);
			right_rightRotate(node);
		}

		void balance_delete(Node<T, K> *node) {
			node->height = get_height(node);
			bf_type node_bf = get_bf(node);
			bf_type lnode_bf = get_bf(node->left);
			bf_type rnode_bf = get_bf(node->right);

			if (node_bf == 2 && lnode_bf == 1) {
				left_leftRotate(node); 
			} else if (node_bf == 2 && lnode_bf == -1) {
				left_rightRotate(node);
			} else if (node_bf == 2 && lnode_bf == 0) {
				left_leftRotate(node);
			} else if (node_bf == -2 && rnode_bf == -1) {
				right_rightRotate(node);
			} else if (node_bf == -2 && rnode_bf == 1) {
				right_leftRotate(node);
			} else if (node_bf == -2 && rnode_bf == 0) { 
				right_rightRotate(node);
			}
		}

		void balance_insert(Node<T, K>* node) { 
			while (node != nullptr) {
				node->height = get_height(node);

				bf_type node_bf = get_bf(node);
				bf_type lnode_bf = get_bf(node->left);
				bf_type rnode_bf = get_bf(node->right);

				if (node_bf == 2 && lnode_bf == 1) {
					left_leftRotate(node);
				} else if (node_bf == -2 && rnode_bf == -1) {
					right_rightRotate(node);
				} else if (node_bf == -2 && rnode_bf == 1) {
					right_leftRotate(node);
				} else if (node_bf == 2 && lnode_bf == -1) {
					left_rightRotate(node);
				}
				node = node->parent;
			}
		}
		
		Node<T, K>* find_node(K key) {
			Node<T, K> *current = root;
			while (current && current->key != key) {
				if (_compare(key, current->key)) {
					if (!current->left) {
						return current;
					}
					current = current->left;
				} else {
					if (!current->right) {
						return current;
					}
					current = current->right;
				}
			}
			return current;
		}
	};
}