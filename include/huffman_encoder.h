#ifndef HUFFMAN_ENCODER_H
#define HUFFMAN_ENCODER_H

//https://rosettacode.org/wiki/Huffman_coding#C.2B.2B

#include <vector>
#include <map>
#include <fstream>

using HuffCode = std::vector<bool>;

class HuffmanEncoder {
public:
  HuffmanEncoder();
  ~HuffmanEncoder();

  void incrementKey(uint8_t key);
  void constructTable();

  void encode(const uint8_t key, std::vector<bool> &output);

  void writeTable(std::ofstream &stream);

private:
  class Node {
  public:
      const uint64_t frequency;

      virtual ~Node() {}

      virtual uint8_t countLeavesAtDepth(const int8_t depth) = 0;
      virtual void writeLeavesInOrder(std::ofstream &stream) = 0;

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

      uint8_t countLeavesAtDepth(const int8_t depth) {
        if (depth == 0) {
          return 0;
        }

        return left->countLeavesAtDepth(depth - 1) + right->countLeavesAtDepth(depth - 1);
      }

      void writeLeavesInOrder(std::ofstream &stream) {
        left->writeLeavesInOrder(stream);
        right->writeLeavesInOrder(stream);
      }
  };

  class LeafNode: public Node {
  public:
      uint8_t key;

      LeafNode(uint64_t f, uint8_t k) : Node(f), key(k) {}

      uint8_t countLeavesAtDepth(const int8_t depth) {
        return depth == 0 ? 1 : 0;
      }

      void writeLeavesInOrder(std::ofstream &stream) {
        stream.write(reinterpret_cast<char *>(&key), sizeof(key));
      }
  };

  struct NodeCmp
  {
      bool operator()(const Node* left, const Node* right) const { return left->frequency > right->frequency; }
  };

  void generateCodes(const Node *node, const HuffCode& prefix);

  Node *m_root;
  std::map<uint8_t, uint64_t> m_frequencies;
  std::map<uint8_t, HuffCode> m_codewords;
};

#endif
