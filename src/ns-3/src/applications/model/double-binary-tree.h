#ifndef DOUBLE_BINARY_TREE_H
#define DOUBLE_BINARY_TREE_H

#include<iostream>
#include <stdint.h>
#include <string>

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"


namespace ns3
{

class Adress;

class 
TreeNodeAddress{
public:
    uint16_t m_port_p0;
    uint16_t m_port_p1;
    uint16_t m_port_s0;
    uint16_t m_port_s1;
    Address m_parent_t0;
    Address m_parent_t1;
    Address m_child_left;
    Address m_child_right;
};

class 
DoubleBinaryTree{
public:
    DoubleBinaryTree();
    ~DoubleBinaryTree();
    void SetRoot(uint32_t node_id);
    void SetNodeNum(int n);
    void BuildTree();
    void BuildTree0(int n);
    //void BuildSubTree0(int start, int end, int height);
    int BuildSubTree0(int start, int end);
    void BuildTree1(int n);
    int BuildSubTree1(int start, int end);
    void InsertTree1(int n);
    void AddTree0(int a, int b, bool isleft);
    void AddTree1(int a, int b, bool isleft);
    int LeftShiftTree1(int root);
    void PrintTree0(int root);
    void PrintTree1(int root);

    int GetId(int node_id);
    int GetNodeId(int id);
    bool IsLeafT0(uint32_t node_id);
    bool IsLeafT1(uint32_t node_id);
    bool HasP0(uint32_t node_id);
    bool HasP1(uint32_t node_id);
    bool HasS0(uint32_t node_id);
    bool HasS1(uint32_t node_id);
    uint32_t GetNodeIdP0(uint32_t node_id);
    uint32_t GetNodeIdP1(uint32_t node_id);
    uint32_t GetNodeIdS0(uint32_t node_id);
    uint32_t GetNodeIdS1(uint32_t node_id);
    uint32_t GetNodeIdRoot0();
    uint32_t GetNodeIdRoot1();


    // void SetAddressP0(Address address, uint32_t node_id, uint16_t port);
    // void SetAddressP1(Address address, uint32_t node_id, uint16_t port);
    // void SetAddressS0(Address address, uint32_t node_id, uint16_t port);
    // void SetAddressS1(Address address, uint32_t node_id, uint16_t port);
    // Address GetAddressP0(uint32_t node_id);
    // Address GetAddressP1(uint32_t node_id);
    // Address GetAddressS0(uint32_t node_id);
    // Address GetAddressS1(uint32_t node_id);
    // uint16_t GetPortP0(uint32_t node_id);
    // uint16_t GetPortP1(uint32_t node_id);
    // uint16_t GetPortS0(uint32_t node_id);
    // uint16_t GetPortS1(uint32_t node_id);

    static const int m_node_max = 2000;
    int m_node_num;
    int m_root;
    int m_root_tree0;
    int m_root_tree1;
    int m_parent0[m_node_max];
    int m_parent1[m_node_max];
    int m_left_tree0[m_node_max];
    int m_right_tree0[m_node_max];
    int m_left_tree1[m_node_max];
    int m_right_tree1[m_node_max];

    TreeNodeAddress m_address_list[m_node_max];
};

} // namespace ns3

#endif /*DOUBLE_BINARY_TREE_H*/