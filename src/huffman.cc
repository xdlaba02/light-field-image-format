#include "huffman.h"
#include "priority_list.h"

#include <bitset>

HuffmanTable::HuffmanTable(): m_key_counts(), m_codewords() {}

HuffmanTable::~HuffmanTable() {}

void HuffmanTable::addKey(RLTuple &tuple) {
  m_key_counts[key(tuple)]++;
}

void HuffmanTable::constructTable() {
  PriorityList<Node *> l;
  for (auto &tuple: m_key_counts) {
    Node *n = new Node(tuple.first);
    l.insert(n, tuple.second);
  }

  while (l.len() > 1) {
    Node *n1 = nullptr;
    Node *n2 = nullptr;

    uint64_t p1 = 0;
    uint64_t p2 = 0;

    l.top(n1, p1);
    l.pop();

    l.top(n2, p2);
    l.pop();

    Node *n3 = new Node();
    n3->left = n1;
    n3->right = n2;

    l.insert(n3, p1 + p2);
  }

  Node *root = nullptr;
  uint64_t root_priority;

  l.top(root, root_priority);
  l.pop();

  consumeTree(root, Codeword());
}

void HuffmanTable::printTable() {
  for (auto &tuple: m_codewords) {
    std::cout << "("<< static_cast<unsigned int>(tuple.first >> 4) << ", " << static_cast<unsigned int>(tuple.first & 0x0f) << ") ";
    tuple.second.print();
    std::cout << std::endl;
  }
}

uint8_t HuffmanTable::key(RLTuple &tuple) {
  return (tuple.zeroes << 4) | category(tuple.amplitude);
}

uint8_t HuffmanTable::category(int16_t value) {
  if (value < 0) {
    value = -value;
  }

  int cat = 0;
  while (value > 0) {
    value = value >> 1;
    cat++;
  }

  return cat;
}

void HuffmanTable::consumeTree(Node *node, Codeword codeword) {
  if(!node) {
    return;
  }

  if (!node->left && !node->right) {
    m_codewords[node->value] = codeword;
  }
  else {
    consumeTree(node->left, (codeword << 1) | 0);
    consumeTree(node->right, (codeword << 1) | 1);
  }

  delete node;
}
