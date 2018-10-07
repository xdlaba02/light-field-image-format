#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <vector>

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

#endif
