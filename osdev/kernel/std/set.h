#ifndef SET_H
#define SET_H

#include "memory.h"
#include "tree.h"
#include "vector.h"

namespace Kernel {
namespace std {

template <typename Node>
class SetIterator {
 public:
  using value_type = typename Node::KeyType;
  using pointer = typename Node::KeyType*;
  using const_pointer = const typename Node::KeyType*;
  using reference = typename Node::KeyType&;
  using const_reference = const typename Node::KeyType&;

  SetIterator(const SetIterator& itr) : stack_(itr.stack_) {}
  SetIterator(SetIterator&& itr) : stack_(std::move(itr.stack_)) {}
  SetIterator(const std::vector<Node*>& stack) : stack_(stack) {}

  SetIterator& operator=(const SetIterator& itr) {
    stack_ = itr.stack_;
    return *this;
  }

  SetIterator& operator++() {
    // Go up if it does not have any right element.
    // If it is coming from right, we have to continue.
    if (CurrentNode()->Right() == nullptr) {
      Node* current = CurrentNode();
      while (true) {
        stack_.pop_back();
        if (stack_.empty()) {
          break;
        }

        Node* parent = CurrentNode();
        if (parent->Left() == current) {
          break;
        }
      }
    } else {
      // Now find the next successor.
      Node* current = CurrentNode()->Right();
      while (current != nullptr) {
        stack_.push_back(current);
        current = current->Left();
      }
    }

    return *this;
  }

  bool operator==(const SetIterator& itr) const {
    if (stack_.size() != itr.stack_.size()) {
      return false;
    }

    for (size_t i = 0; i < stack_.size(); i++) {
      if (stack_.at(i) != itr.stack_.at(i)) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const SetIterator& itr) const { return !(operator==(itr)); }
  const_reference operator*() const { return CurrentNode()->GetKey(); }
  reference operator*() { return *CurrentNode()->MutableKey(); }
  const_pointer operator->() const { return &CurrentNode()->GetKey(); }
  pointer operator->() { return CurrentNode()->MutableKey(); }

  pair<Node*, Node*> CurrentAndParent() const {
    if (stack_.size() == 0) {
      return pair<Node*, Node*>(nullptr, nullptr);
    } else if (stack_.size() == 1) {
      return pair<Node*, Node*>(stack_.at(0), nullptr);
    } else {
      return pair<Node*, Node*>(stack_.at(stack_.size() - 1),
                                stack_.at(stack_.size() - 2));
    }
  }

 private:
  // Current stack. (Assuming that we visit nodes in DFS).
  std::vector<Node*> stack_;

  Node* CurrentNode() const { return stack_.back(); }
};

template <typename T, typename Allocator = std::allocator<BinaryTreeNode<T>>>
class set {
 public:
  using iterator = SetIterator<BinaryTreeNode<T>>;
  using const_iterator = const SetIterator<BinaryTreeNode<T>>;

  set() : num_elements_(0) {}

  iterator begin() const {
    auto* current = tree_.Root();

    std::vector<BinaryTreeNode<T>*> stack;
    while (current != nullptr) {
      stack.push_back(current);
      current = current->Left();
    }

    return SetIterator<BinaryTreeNode<T>>(std::move(stack));
  }

  iterator end() const { return iterator(std::vector<BinaryTreeNode<T>*>()); }

  size_t count(const T& t) const {
    auto current_and_parent = tree_.SearchNode(t);
    return current_and_parent.first == nullptr ? 0 : 1;
  }

  void insert(const T& t) {
    // No need to add if it already exists.
    if (count(t) == 1) {
      return;
    }

    BinaryTreeNode<T>* node =
        alloc_::allocate(allocator_, sizeof(BinaryTreeNode<T>));
    alloc_::construct(allocator_, node, t);

    ASSERT(tree_.InsertNode(node) == nullptr);
    num_elements_++;
  }

  // Reurns the number of erased elements.
  size_t erase(const T& t) {
    auto* node = tree_.DeleteNode(t);
    if (node == nullptr) {
      return 0;
    }

    // Now we have to destroy it.
    alloc_::destroy(allocator_, node);
    alloc_::deallocate(allocator_, node, sizeof(BinaryTreeNode<T>));

    num_elements_--;

    return 1;
  }

  void erase(iterator pos) {
    auto [current, parent] = pos.CurrentAndParent();
    auto* node = tree_.DeleteNode(current, parent);
    if (node == nullptr) {
      return;
    }

    // Now we have to destroy it.
    alloc_::destroy(allocator_, node);
    alloc_::deallocate(allocator_, node, sizeof(BinaryTreeNode<T>));

    num_elements_--;
  }

  size_t size() const { return num_elements_; }

  const_iterator find(const T& t) const { return iterator(tree_.FindPath(t)); }
  iterator find(const T& t) { return iterator(tree_.FindPath(t)); }

  void Print() { tree_.PrintTree(); }

  ~set() {
    // TODO This is very naive implementation :(
    while (num_elements_ > 0) {
      erase(begin());
    }
  }

 private:
  BinaryTree<BinaryTreeNode<T>> tree_;

  size_t num_elements_;
  Allocator allocator_;

  using alloc_ = std::allocator_traits<Allocator>;
};

}  // namespace std
}  // namespace Kernel

#endif
