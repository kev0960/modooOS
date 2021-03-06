#ifndef TREE_H
#define TREE_H

#include "printf.h"
#include "utility.h"
#include "vector.h"

namespace Kernel {
namespace std {

template <typename Key>
class BinaryTreeNode {
 public:
  using KeyType = Key;

  BinaryTreeNode(const KeyType& key)
      : key_(key), left_(nullptr), right_(nullptr) {}
  BinaryTreeNode(KeyType&& key)
      : key_(std::move(key)), left_(nullptr), right_(nullptr) {}

  const KeyType& GetKey() const { return key_; }

  // Changing the key can be dangerous.
  // DONT USE WHEN YOU DON'T KNOW WHAT YOU ARE DOING
  KeyType* MutableKey() { return &key_; }

  BinaryTreeNode* Left() const { return left_; }
  BinaryTreeNode* Right() const { return right_; }

  void SetLeft(BinaryTreeNode* left) {
    ASSERT(left_ == nullptr);
    left_ = left;
  }

  void SetRight(BinaryTreeNode* right) {
    ASSERT(right_ == nullptr);
    right_ = right;
  }

  int NumChild() const {
    int num = 0;
    if (left_ != nullptr) {
      num++;
    }
    if (right_ != nullptr) {
      num++;
    }
    return num;
  }

  BinaryTreeNode* GetOnlyChild() {
    if (left_ != nullptr) {
      return left_;
    }
    return right_;
  }

  BinaryTreeNode* AddChild(BinaryTreeNode* node) {
    if (node->GetKey() > key_) {
      BinaryTreeNode* prev = right_;
      right_ = node;
      return prev;
    } else {
      BinaryTreeNode* prev = left_;
      left_ = node;
      return prev;
    }
  }

  void RemoveChild(BinaryTreeNode* node) {
    if (left_ == node) {
      left_ = nullptr;
    } else if (right_ == node) {
      right_ = nullptr;
    }
  }

  void SetChild(BinaryTreeNode* node) {
    if (node->GetKey() > key_) {
      right_ = node;
    } else {
      left_ = node;
    }
  }

  void SwitchChild(BinaryTreeNode* from, BinaryTreeNode* to) {
    if (left_ == from) {
      left_ = to;
    } else if (right_ == from) {
      right_ = to;
    }
  }

 private:
  KeyType key_;

  BinaryTreeNode* left_;
  BinaryTreeNode* right_;
};

template <typename Node>
class BinaryTree {
 public:
  BinaryTree() : root_(nullptr) {}

  void PrintTree() {
    if (root_ == nullptr) {
      return;
    }
    PrintTree(root_);
  }

  // Returns the previous node that occupied the new node's position.
  Node* InsertNode(Node* node) {
    if (root_ == nullptr) {
      root_ = node;
      return nullptr;
    }

    auto [current, parent] = SearchNode(node->GetKey());
    if (parent != nullptr) {
      // This will either add new child or replace existing child.
      Node* prev_node = parent->AddChild(node);
      return prev_node;
    } else {
      Node* prev_root = root_;
      root_ = node;
      return prev_root;
    }
  }

  Node* DeleteNode(Node* current, Node* parent) {
    if (root_ == nullptr) {
      return nullptr;
    }
    if (current == nullptr) {
      return nullptr;
    }

    if (current->NumChild() == 0) {
      if (parent != nullptr) {
        parent->RemoveChild(current);
      } else {
        root_ = nullptr;
      }
      return current;
    } else if (current->NumChild() == 1) {
      Node* child = current->GetOnlyChild();
      current->RemoveChild(child);

      if (parent != nullptr) {
        parent->RemoveChild(current);
        parent->SetChild(child);
      } else {
        root_ = child;
      }
    } else if (current->NumChild() == 2) {
      Node* successor = FindLeftModeNodeFrom(current->Right());
      // Delete that node from the tree.
      DeleteNode(successor->GetKey());
      successor->SetRight(current->Right());
      successor->SetLeft(current->Left());

      if (parent != nullptr) {
        parent->SwitchChild(current, successor);
      } else {
        root_ = successor;
      }
    }

    return current;
  }

  Node* DeleteNode(const typename Node::KeyType& key) {
    if (root_ == nullptr) {
      return nullptr;
    }

    auto [current, parent] = SearchNode(key);
    return DeleteNode(current, parent);
  }

  // Returns found node and its parent.
  //   - For root node, the parent will be nullptr.
  //   - For non-existing node, current will be nullptr.
  pair<Node*, Node*> SearchNode(const typename Node::KeyType& key) const {
    if (root_ == nullptr) {
      return std::pair<Node*, Node*>(nullptr, nullptr);
    }

    Node* current = root_;
    Node* parent = nullptr;
    while (current != nullptr) {
      if (current->GetKey() < key) {
        parent = current;
        current = current->Right();
      } else if (current->GetKey() > key) {
        parent = current;
        current = current->Left();
      } else {
        break;
      }
    }

    return make_pair(current, parent);
  }

  vector<Node*> FindPath(const typename Node::KeyType& key) const {
    vector<Node*> path;

    Node* current = root_;
    while (current != nullptr) {
      path.push_back(current);
      if (current->GetKey() < key) {
        current = current->Right();
      } else if (current->GetKey() > key) {
        current = current->Left();
      } else {
        // Found the key!
        return path;
      }
    }

    // Return an empty vector if the path is not found.
    return vector<Node*>{};
  }

  Node* FindLeftModeNodeFrom(Node* node) const {
    if (node == nullptr) {
      return nullptr;
    }
    Node* current = node;
    while (current->Left() != nullptr) {
      current = current->Left();
    }
    return current;
  }

  Node* Root() const { return root_; }

 private:
  void PrintTree(Node* node) const {
    Node* left = node->Left();
    Node* right = node->Right();

    kprintf("Node [%d] --> ", node->GetKey());
    if (left) {
      kprintf("Left : [%d] ", left->GetKey());
    } else {
      kprintf("Left : [None] ");
    }
    if (right) {
      kprintf("Right : [%d] \n", right->GetKey());
    } else {
      kprintf("Right: [None] \n");
    }

    if (left) {
      PrintTree(left);
    }

    if (right) {
      PrintTree(right);
    }
  }

  Node* root_;
};

}  // namespace std
}  // namespace Kernel
#endif
