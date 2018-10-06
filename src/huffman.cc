#include "huffman.h"
#include "priority_list.h"

#include <bitset>

HuffmanTable::HuffmanTable(): m_key_counts(), m_codewords() {}

HuffmanTable::~HuffmanTable() {}

void HuffmanTable::addKey(const RLTuple &tuple) {
  m_frequencies[key(tuple)]++;
}

void HuffmanTable::constructTable() {
  std::priority_queue<INode *, std::vector<INode *>, NodeCmp> trees;

  for (auto &tuple: m_frequencies) {
    trees.push(new LeafNode(tuple.second, tuple.first));
  }

  while (trees.size() > 1)
  {
      INode* right = trees.top();
      trees.pop();

      INode* left = trees.top();
      trees.pop();

      trees.push(new InternalNode(childR, childL));
  }

  INode *root = trees.top();
  trees.pop();

  generateCodes(root, HuffCoef());

  delete root;
}

void HuffmanTable::printTable() {
  for (auto &tuple: m_codewords) {
    std::cout << "("<< static_cast<unsigned int>(tuple.first >> 4) << ", " << static_cast<unsigned int>(tuple.first & 0x0f) << ") ";
    tuple.second.print();
    std::cout << std::endl;
  }
}

void HuffmanTable::generateCodes(const Node *node, const HuffCode& prefix) {
  if (const LeafNode* leaf = dynamic_cast<const LeafNode *>(node)) {
      m_codewords[leag->key] = prefix;
  }
  else if (const InternalNode* internal = dynamic_cast<const InternalNode *>(node))
  {
      HuffCode leftPrefix = prefix;
      leftPrefix.push_back(false);
      GenerateCodes(internal->left, leftPrefix);

      HuffCode rightPrefix = prefix;
      rightPrefix.push_back(true);
      GenerateCodes(internal->right, rightPrefix);
  }
}
