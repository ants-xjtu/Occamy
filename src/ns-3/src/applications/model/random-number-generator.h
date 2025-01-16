#ifndef RONDOM_NUMBER_GENERATOR
#define RONDOM_NUMBER_GENERATOR

#include <iostream>
#include <vector>
#include <algorithm> // include std::random_shuffle
#include <random>    // include std::default_random_engine

namespace ns3
{

class RandomNumGen
{
public:
    RandomNumGen(/* args */);
    ~RandomNumGen();
    void SetNodeNum(uint32_t num);
    std::vector<uint32_t>& GetRandomSequence(uint32_t node_id);
    uint32_t GetSequenceTurn(uint32_t node_id);
    void NodeFLowOver(uint32_t node_id);
    bool IsTurnOver(uint32_t node_id);
    bool IsOver();
    void GenerateRandomSequences();
    void Reset();
    
    uint32_t m_node_num;
    uint32_t m_seed;
    bool m_isover;
    uint32_t m_overnum;
    std::vector<std::vector<uint32_t>> m_random_sequences;
    std::vector<uint32_t> m_sequence_turn;
    std::vector<uint32_t> m_node_overnum;
    std::vector<bool> m_sequence_over;
};

}

#endif