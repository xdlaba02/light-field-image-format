#include "huffman_encoder.h"

#include <queue>
#include <bitset>
#include <iostream>

HuffmanEncoder::HuffmanEncoder():
  m_frequencies(),
  m_level_order_keys(),
  m_codewords() {}

HuffmanEncoder::~HuffmanEncoder() {}

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

  Node *root = trees.top();
  trees.pop();

  treeToLevelOrder(root, 0);

  delete root;

  uint16_t codeword = 0;

  for (uint8_t i = 0; i < 16; i++) {
    for (auto &key: m_level_order_keys[i]) {
      std::bitset<16> bits(codeword);
      for (uint8_t k = 0; k < i; k++) {
        m_codewords[key].push_back(bits[15 - k]);
      }
      codeword = ((codeword >> (16 - i)) + 1) << (16 - i);
    }
  }
}

void HuffmanEncoder::treeToLevelOrder(const Node *node, const uint8_t depth) {
  if (const LeafNode* leaf = dynamic_cast<const LeafNode *>(node)) {
    m_level_order_keys[depth].push_back(leaf->key);
  }
  else if (const InternalNode* internal = dynamic_cast<const InternalNode *>(node)) {
    treeToLevelOrder(internal->left, depth + 1);
    treeToLevelOrder(internal->right, depth + 1);
  }
}

void HuffmanEncoder::encode(const uint8_t key, std::vector<bool> &output) {
  output.insert(output.end(), m_codewords[key].begin(), m_codewords[key].end());;
}

void HuffmanEncoder::writeTable(std::ofstream &stream) {
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t leaves = m_level_order_keys[i].size();
    stream.write(reinterpret_cast<char *>(&leaves), sizeof(char));
  }
  for (uint8_t i = 0; i < 16; i++) {
    for (uint8_t key: m_level_order_keys[i]) {
      stream.write(reinterpret_cast<char *>(&key), sizeof(char));
    }
  }
}

void HuffmanEncoder::print() {
  for (auto &pair: m_codewords) {
    std::cout << std::bitset<8>(pair.first) << ": ";
    for (auto &&bit: pair.second) {
      std::cout << bit;
    }
    std::cout << std::endl;
  }
}
