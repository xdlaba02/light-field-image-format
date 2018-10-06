#include "huffman_encoder.h"

#include <queue>
#include <iostream>
#include <bitset>

HuffmanEncoder::HuffmanEncoder(): m_root(nullptr), m_frequencies(), m_codewords() {}

HuffmanEncoder::~HuffmanEncoder() {
  delete m_root;
}

void HuffmanEncoder::incrementKey(uint8_t key) {
  m_frequencies[key]++;
}

void HuffmanEncoder::constructTable() {
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

  m_root = trees.top();
  trees.pop();

  generateCodes(m_root, HuffCode());
}

void HuffmanEncoder::generateCodes(const Node *node, const HuffCode& prefix) {
  if (const LeafNode* leaf = dynamic_cast<const LeafNode *>(node)) {
    m_codewords[leaf->key] = prefix;
  }
  else if (const InternalNode* internal = dynamic_cast<const InternalNode *>(node)) {
    HuffCode leftPrefix = prefix;
    leftPrefix.push_back(false);
    generateCodes(internal->left, leftPrefix);

    HuffCode rightPrefix = prefix;
    rightPrefix.push_back(true);
    generateCodes(internal->right, rightPrefix);
  }
}

void HuffmanEncoder::encode(const uint8_t key, std::vector<bool> &output) {
  output.insert(output.end(), m_codewords[key].begin(), m_codewords[key].end());;
}

void HuffmanEncoder::writeTable(std::ofstream &stream) {
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t leaves = m_root->countLeavesAtDepth(i);
    stream.write(reinterpret_cast<char *>(&leaves), sizeof(char));
  }
  m_root->writeLeavesInOrder(stream);
}
