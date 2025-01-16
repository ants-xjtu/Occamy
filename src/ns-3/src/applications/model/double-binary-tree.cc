#include "double-binary-tree.h"
#include <cmath>
#include <algorithm>

namespace ns3
{
DoubleBinaryTree::DoubleBinaryTree()
{
    for(int i = 0; i < m_node_max; i++)
    {
        m_left_tree0[i] = m_node_max;
        m_right_tree0[i] = m_node_max;
        m_left_tree1[i] = m_node_max;
        m_right_tree1[i] = m_node_max;
        m_parent0[i] = m_node_max;
        m_parent1[i] = m_node_max;
    }
    m_root_tree0 = m_node_max;
    m_root_tree1 = m_node_max;
}

DoubleBinaryTree::~DoubleBinaryTree()
{

}

void DoubleBinaryTree::SetRoot(uint32_t node_id)
{
    m_root = int(node_id);
}

void DoubleBinaryTree::SetNodeNum(int n)
{
    m_node_num = n;
}

void DoubleBinaryTree::BuildTree()
{
    //std::cout << "Dadwa" << std::endl;
    BuildTree0(m_node_num);
    BuildTree1(m_node_num + 1);
}

void DoubleBinaryTree::AddTree0(int a, int b, bool isleft)
{
    if(isleft)
    {
        m_left_tree0[a] = b;
    }
    else
    {
        m_right_tree0[a] = b;
    }
}

void DoubleBinaryTree::AddTree1(int a, int b, bool isleft)
{
    if(isleft)
    {
        m_left_tree1[a] = b;
    }
    else
    {
        m_right_tree1[a] = b;
    }
}

void DoubleBinaryTree::BuildTree0(int n)
{
    int h = int(ceil(log2(n + 2)));
    m_root_tree0 = (1 << (h - 1) ) - 1;
    if(n == m_root_tree0)
    {
        m_root_tree0 = n / 2;
    }
    int leftChild = BuildSubTree0(0, m_root_tree0 - 1);
    int rightChild = BuildSubTree0(m_root_tree0 + 1, n - 1);
    m_parent0[leftChild] = m_root_tree0;
    m_parent0[rightChild] = m_root_tree0;
    AddTree0(m_root_tree0, leftChild, true);
    AddTree0(m_root_tree0, rightChild, false);
}

void DoubleBinaryTree::BuildTree1(int n)
{
    int h = int(ceil(log2(n + 2)));
    m_root_tree1 = (1 << (h - 1) ) - 1;
    if(n == m_root_tree1)
    {
        m_root_tree1 = n / 2;
    }
    int leftChild = BuildSubTree1(0, m_root_tree1 - 1);
    int rightChild = BuildSubTree1(m_root_tree1 + 1, n - 1);
    AddTree1(m_root_tree1, leftChild, true);
    AddTree1(m_root_tree1, rightChild, false);

    m_root_tree1 = LeftShiftTree1(m_root_tree1);
}

int DoubleBinaryTree::BuildSubTree0(int start, int end)
{
    if(start > end) return m_node_max;
    int mid = (start + end + 1) / 2;
    int leftChild = BuildSubTree0(start, mid - 1);
    int rightChild = BuildSubTree0(mid + 1, end);
    m_parent0[leftChild] = mid;
    m_parent0[rightChild] = mid;
    AddTree0(mid, leftChild, true);
    AddTree0(mid, rightChild, false);
    return mid;
}

int DoubleBinaryTree::BuildSubTree1(int start, int end)
{
    if(start > end) return m_node_max;
    int mid = (start + end + 1) / 2;
    int leftChild = BuildSubTree1(start, mid - 1);
    int rightChild = BuildSubTree1(mid + 1, end);
    AddTree1(mid, leftChild, true);
    AddTree1(mid, rightChild, false);
    return mid;
}

void DoubleBinaryTree::InsertTree1(int n)
{
    int currentNode = m_root_tree1;
    while(m_right_tree1[currentNode] != m_node_max)
    {
        //std::cout << currentNode  << " " << m_right_tree1[currentNode] <<  " " <<m_node_max  << std::endl;
        currentNode = m_right_tree1[currentNode];
        //if(m_right_tree1[currentNode] == m_node_max) break;
    }
    AddTree1(currentNode, n, false);
}

int DoubleBinaryTree::LeftShiftTree1(int root)
{
    if(root == m_node_max || root == 0)
    {
        //std::cout<< "Root:" << root << " return:" << m_node_max << std::endl;
        return m_node_max;
    }
    if(m_left_tree1[root] != m_node_max)
    {
        
        //m_left_tree1[root - 1] = LeftShiftTree1(m_left_tree1[root]);
        //int leftChild = LeftShiftTree1(m_left_tree1[root]);
        int leftChild = m_left_tree1[root];
        m_left_tree1[root] = m_node_max;
        leftChild = LeftShiftTree1(leftChild);
        m_left_tree1[root - 1] = leftChild;
        if(leftChild != m_node_max)
        {
            m_parent1[leftChild] = root - 1;
        }
    }
    if(m_right_tree1[root] != m_node_max)
    {
        
        //m_right_tree1[root - 1] = LeftShiftTree1(m_right_tree1[root]);
        //std::cout<< "P:" << root - 1 << " RS:" << m_right_tree1[root] << std::endl;
        int rightChild = m_right_tree1[root];
        m_right_tree1[root] = m_node_max;
        rightChild = LeftShiftTree1(rightChild);
        m_right_tree1[root - 1] = rightChild;
        if(rightChild != m_node_max)
        {
            m_parent1[rightChild] = root - 1;
        }
    }
    //std::cout<< "Root:" << root << " return:" << root - 1 << std::endl;
    return root - 1;
}

void DoubleBinaryTree::PrintTree0(int node) {
    if (node == m_node_max) {
        return;
    }

    //std::cout << "Node: " << node << std::endl;

    if(m_left_tree0[node] == m_node_max && m_right_tree0[node] == m_node_max)
    {
        std::cout<< node << " ";
    }

    if (m_left_tree0[node] != m_node_max) {
        // std::cout << "P:" << node << " S:" << m_left_tree0[node];
        // if(m_parent0[m_left_tree0[node]] != node) std::cout << " ERRORP:" << m_parent0[m_left_tree0[node]];
        // std::cout << std::endl;
    }
    PrintTree0(m_left_tree0[node]);

    if (m_right_tree0[node] != m_node_max) {
        // std::cout << "P:" << node << " S:" << m_right_tree0[node];
        // if(m_parent0[m_right_tree0[node]] != node) std::cout << " ERRORP:" << m_parent0[m_right_tree0[node]];
        // std::cout << std::endl;
    }
    PrintTree0(m_right_tree0[node]);
}

void DoubleBinaryTree::PrintTree1(int node) {
    if (node == m_node_max) {
        return;
    }

    //std::cout << "Node: " << node << std::endl;
    if(m_left_tree1[node] == m_node_max && m_right_tree1[node] == m_node_max)
    {
        std::cout<< node << " ";
    }

    if (m_left_tree1[node] != m_node_max) {
        // std::cout << "P:" << node << " S:" << m_left_tree1[node];
        // if(m_parent1[m_left_tree1[node]] != node) std::cout << " ERROR_P:" << m_parent1[m_left_tree1[node]];
        // std::cout << std::endl;
    }
    PrintTree1(m_left_tree1[node]);

    if (m_right_tree1[node] != m_node_max) {
        // std::cout << "P:" << node << " S:" << m_right_tree1[node];
        // if(m_parent1[m_right_tree1[node]] != node) std::cout << " ERROR_P:" << m_parent1[m_right_tree1[node]];
        // std::cout << std::endl;
    }
    PrintTree1(m_right_tree1[node]);
}

int DoubleBinaryTree::GetId(int node_id)
{
    if(node_id == m_root) std::cout << "Error: Get root id" << std::endl;
    if(node_id < m_root) return node_id;
    else return node_id - 1;
}

int DoubleBinaryTree::GetNodeId(int id)
{
    if(id < m_root) return id;
    else return id + 1;
}

bool DoubleBinaryTree::HasP0(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return false;
    int id = GetId(nid);
    if(id == m_root_tree0) return true;
    else if(m_parent0[id] != m_node_max) return true;
    else
    {
        std::cout << "ERROR: NO P0" << std::endl;
        return false;
    }
}

bool DoubleBinaryTree::HasP1(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return false;
    int id = GetId(nid);
    if(id == m_root_tree1) return true;
    else if(m_parent1[id] != m_node_max) return true;
    else
    {
        std::cout << "ERROR: NO P1" << std::endl;
        return false;
    }
}

bool DoubleBinaryTree::HasS0(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return true;
    int id = GetId(nid);
    if(m_left_tree0[id] != m_node_max || m_left_tree1[id] != m_node_max) return true;
    else return false;
}

bool DoubleBinaryTree::HasS1(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return true;
    int id = GetId(nid);
    if(m_right_tree0[id] != m_node_max || m_right_tree1[id] != m_node_max) return true;
    else return false; 
}

uint32_t DoubleBinaryTree::GetNodeIdP0(uint32_t node_id)
{
    int nid = (int)node_id;
    int id = GetId(nid);
    if(id == m_root_tree0) return (uint32_t)m_root;
    return (uint32_t)GetNodeId(m_parent0[id]);
}

uint32_t DoubleBinaryTree::GetNodeIdP1(uint32_t node_id)
{
    int nid = (int)node_id;
    int id = GetId(nid);
    if(id == m_root_tree1) return (uint32_t)m_root;
    return (uint32_t)GetNodeId(m_parent1[id]);
}

uint32_t DoubleBinaryTree::GetNodeIdS0(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return uint32_t(GetNodeId(m_root_tree0));
    int id = GetId(nid);
    if(m_left_tree0[id] != m_node_max) return uint32_t(GetNodeId(m_left_tree0[id]));
    else if(m_left_tree1[id] != m_node_max) return uint32_t(GetNodeId(m_left_tree1[id]));
}

uint32_t DoubleBinaryTree::GetNodeIdS1(uint32_t node_id)
{
    int nid = (int)node_id;
    if(nid == m_root) return uint32_t(GetNodeId(m_root_tree1));
    int id = GetId(nid);
    if(m_right_tree0[id] != m_node_max) return uint32_t(GetNodeId(m_right_tree0[id]));
    else if(m_right_tree1[id] != m_node_max) return uint32_t(GetNodeId(m_right_tree1[id]));
}

bool DoubleBinaryTree::IsLeafT0(uint32_t node_id)
{
    int nid = (int)node_id;
    int id = GetId(nid);
    if(m_left_tree0[id] == m_node_max && m_right_tree0[id] == m_node_max) return true;
    else return false;
}

bool DoubleBinaryTree::IsLeafT1(uint32_t node_id)
{
    int nid = (int)node_id;
    int id = GetId(nid);
    if(m_left_tree1[id] == m_node_max && m_right_tree1[id] == m_node_max) return true;
    else return false;
}

uint32_t DoubleBinaryTree::GetNodeIdRoot0()
{
    return (uint32_t)GetNodeId(m_root_tree0);
}

uint32_t DoubleBinaryTree::GetNodeIdRoot1()
{
    return (uint32_t)GetNodeId(m_root_tree1);
}



// void DoubleBinaryTree::SetAddressP0(Address address, uint32_t node_id, uint16_t port)
// {
//     int id = GetId(int(node_id));
//     m_address_list[id].m_parent_t0 = address;
//     m_address_list[id].m_port_p0 = port; 
// }

// void DoubleBinaryTree::SetAddressP1(Address address, uint32_t node_id, uint16_t port)
// {
//     int id = GetId(int(node_id));
//     m_address_list[id].m_parent_t1 = address;
//     m_address_list[id].m_port_p1 = port;
// }

// void DoubleBinaryTree::SetAddressS0(Address address, uint32_t node_id, uint16_t port)
// {
//     int id = GetId(int(node_id));
//     m_address_list[id].m_child_left = address;
//     m_address_list[id].m_port_s0 = port;
// }

// void DoubleBinaryTree::SetAddressS1(Address address, uint32_t node_id, uint16_t port)
// {
//     int id = GetId(int(node_id));
//     m_address_list[id].m_child_right = address;
//     m_address_list[id].m_port_s1 = port;
// }

// uint16_t DoubleBinaryTree::GetPortP0(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_port_p0;
// }

// uint16_t DoubleBinaryTree::GetPortP1(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_port_p1;
// }

// uint16_t DoubleBinaryTree::GetPortS0(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_port_s0;
// }

// uint16_t DoubleBinaryTree::GetPortS1(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_port_s1;
// }

// Address DoubleBinaryTree::GetAddressP0(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_parent_t0;
// }

// Address DoubleBinaryTree::GetAddressP1(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_parent_t1;
// }

// Address DoubleBinaryTree::GetAddressS0(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_child_left;
// }

// Address DoubleBinaryTree::GetAddressS1(uint32_t node_id)
// {
//     int id = GetId(int(node_id));
//     return m_address_list[id].m_child_right;
// }



} // namespace ns3