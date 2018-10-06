#include "huffman.h"

#include <queue>
#include <iostream>

HuffmanTable::HuffmanTable(): m_frequencies(), m_codewords() {}

HuffmanTable::~HuffmanTable() {}

void HuffmanTable::incrementKey(uint8_t key) {
  m_frequencies[key]++;
}

void HuffmanTable::constructTable() {
  std::priority_queue<Node *, std::vector<Node *>, NodeCmp> trees;

  for (auto &tuple: m_frequencies) {
    trees.push(new LeafNode(tuple.second, tuple.first));
  }

  while (trees.size() > 1) {
    Node* left = trees.top();
    trees.pop();

    Node* right = trees.top();
    trees.pop();

    trees.push(new InternalNode(left, right));
  }

  Node *root = trees.top();
  trees.pop();

  generateCodes(root, HuffCode());

  delete root;

}
void HuffmanTable::generateCodes(const Node *node, const HuffCode& prefix) {
  if (const LeafNode* leaf = dynamic_cast<const LeafNode *>(node)) {
      m_codewords[leaf->key] = prefix;
  }
  else if (const InternalNode* internal = dynamic_cast<const InternalNode *>(node))
  {
      HuffCode leftPrefix = prefix;
      leftPrefix.push_back(false);
      generateCodes(internal->left, leftPrefix);

      HuffCode rightPrefix = prefix;
      rightPrefix.push_back(true);
      generateCodes(internal->right, rightPrefix);
  }
}
