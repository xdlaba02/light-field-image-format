#ifndef PRIORITY_LIST_H
#define PRIORITY_LIST_H

#include <cstdint>

template <typename T>
class PriorityList {
public:
  PriorityList(): m_len(0), head(nullptr) {}
  ~PriorityList() {
    Node *node = head;
    while (node) {
      Node *next = node->next;
      delete node;
      node = next;
    }
  }

  void insert(T data, uint64_t priority) {
    m_len++;
    Node *newnode = new Node(data, priority);
    if (!head) {
      head = newnode;
      return;
    }

    Node *node = head;
    while (node) {
      if (node->priority > priority) {
        if (node->prev) {
          newnode->prev = node->prev;
          newnode->next = node;
          node->prev->next = newnode;
          node->prev = newnode;
          return;
        }
        else {
          newnode->next = node;
          head = newnode;
          node->prev = newnode;
          return;
        }
      }
      else if (!node->next) {
        newnode->prev = node;
        node->next = newnode;
        return;
      }

      node = node->next;
    }
  }

  bool top(T &data, uint64_t &priority) {
    if (!head) {
      return false;
    }
    else {
      data = head->data;
      priority = head->priority;
      return true;
    }
  }

  void pop() {
    if (!head) {
      return;
    }

    Node *h = head;
    head = head->next;
    if (head) {
      head->prev = nullptr;
    }
    delete h;

    m_len--;
  }

  uint64_t len() { return m_len; }

private:
  class Node {
  public:
    Node(T d, uint64_t p): data(d), priority(p), prev(nullptr), next(nullptr) {}
    T data;
    uint64_t priority;
    Node *prev;
    Node *next;
  };

  uint64_t m_len;
  Node *head;
};

#endif
