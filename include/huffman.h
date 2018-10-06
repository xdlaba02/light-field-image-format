#ifndef HUFFMAN_H
#define HUFFMAN_H

//https://rosettacode.org/wiki/Huffman_coding#C.2B.2B

#include <vector>
#include <map>

using HuffCode = std::vector<bool>;

class HuffmanTable {
public:
  HuffmanTable();
  ~HuffmanTable();

  void incrementKey(uint8_t key);
  void constructTable();

private:
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
      const uint8_t key;

      LeafNode(uint64_t f, uint8_t k) : Node(f), key(k) {}
  };

  struct NodeCmp
  {
      bool operator()(const Node* left, const Node* right) const { return left->frequency > right->frequency; }
  };

  void generateCodes(const Node *node, const HuffCode& prefix);

  std::map<uint8_t, uint64_t> m_frequencies;
  std::map<uint8_t, HuffCode> m_codewords;
};

#endif
