#ifndef SET_H
#define SET_H

#include "memory.h"
#include "tree.h"
#include "vector.h"

namespace Kernel {
namespace std {

template <typename Node>
class SetIterator {
  using value_type = typename Node::KeyType;
  using pointer = typename Node::KeyType*;
  using reference = typename Node::KeyType&;

  SetIterator(const std::vector<Node*>& stack) : stack_(stack) {}

  SetIterator& operator++() {
    // Go up if it does not have any child.
    // But if it is coming from right, we have to continue.
    if (CurrentNode()->NumChild() == 0) {
      Node* current = CurrentNode();
      while (stack_.empty()) {
        stack_.pop_back();
        Node* parent = CurrentNode();

        if (parent->Left() == current) {
          break;
        }
      }
    } else if (CurrentNode()->Right() != nullptr) {
      // Now find the next successor.
      Node* current = CurrentNode()->Right();
      while (current != nullptr) {
        stack_.push_back(current);
        current = current->Left();
      }
    }
  }

  reference operator*() { return CurrentNode()->GetKey(); }

 private:
  // Current stack. (Assuming that we visit nodes in DFS).
  std::vector<Node*> stack_;

  Node* CurrentNode() const { return stack_.back(); }
};

template <typename T, typename Allocator = std::allocator<BinaryTreeNode<T>>>
class set {
 public:
  set() : num_elements_(0) {}

  size_t count(const T& t) {
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

  void erase(const T& t) {
    auto* node = tree_.DeleteNode(t);
    if (node == nullptr) {
      return;
    }

    // Now we have to destroy it.
    alloc_::destroy(allocator_, node);
    alloc_::deallocate(allocator_, node, sizeof(BinaryTreeNode<T>));

    num_elements_--;
  }

  void Print() { tree_.PrintTree(); }

  ~set() {}

 private:
  BinaryTree<BinaryTreeNode<T>> tree_;

  size_t num_elements_;
  Allocator allocator_;

  using alloc_ = std::allocator_traits<Allocator>;
};

}  // namespace std
}  // namespace Kernel

#endif
