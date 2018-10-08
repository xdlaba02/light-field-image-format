#ifndef HUFFMAN_ENCODER_H
#define HUFFMAN_ENCODER_H

#include <vector>
#include <map>
#include <fstream>
#include <bitset>

using HuffCode = std::vector<bool>;

class Node {
public:
    const uint64_t frequency;

    virtual ~Node() {}

protected:
    Node(int f): frequency(f) {}
};

class InternalNode: public Node {
public:
    Node *const left;
    Node *const right;

    InternalNode(Node* l, Node* r): Node(l->frequency + r->frequency), left(l), right(r) {}
    ~InternalNode() {
        delete left;
        delete right;
    }
};

class LeafNode: public Node {
public:
    uint8_t key;

    LeafNode(uint64_t f, uint8_t k) : Node(f), key(k) {}
};

struct NodeCmp
{
    bool operator()(const Node* left, const Node* right) const { return left->frequency > right->frequency; }
};

class HuffmanEncoder {
public:
  HuffmanEncoder();
  ~HuffmanEncoder();

  void incrementKey(uint8_t key);
  void constructTable();
  void encode(const uint8_t key, std::vector<bool> &output);
  void writeTable(std::ofstream &stream);
  void print();

private:
  void treeToLevelOrder(const Node *node, const uint8_t depth);

  std::map<uint8_t, uint64_t> m_frequencies;
  std::array<std::vector<uint8_t>, 16> m_level_order_keys;
  std::map<uint8_t, HuffCode> m_codewords;
};

#endif
