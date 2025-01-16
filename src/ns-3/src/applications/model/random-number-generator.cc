#include <iostream>
#include <vector>
#include <algorithm>
#include <random> 
#include "random-number-generator.h"

namespace ns3
{

RandomNumGen::RandomNumGen(/* args */)
{
    m_node_num = 0;
    m_isover = false;
    m_overnum = 0;
    m_seed = 0;
}

RandomNumGen::~RandomNumGen()
{
}

void RandomNumGen::SetNodeNum(uint32_t num) 
{
    m_node_num = num;
    m_sequence_turn.resize(num, 0);
    m_sequence_over.resize(num, false);
    m_node_overnum.resize(num, 0);
    GenerateRandomSequences();
}

std::vector<uint32_t>& RandomNumGen::GetRandomSequence(uint32_t node_id)
{
    return m_random_sequences[node_id];
}

uint32_t RandomNumGen::GetSequenceTurn(uint32_t node_id)
{
    const auto& sequence = GetRandomSequence(node_id);
    uint32_t turn = m_sequence_turn[node_id]++;
    // if(m_sequence_turn[node_id] >= m_node_num)
    // {
    //     m_sequence_over[node_id] = true;
    //     m_overnum ++;
    //     if(m_overnum >= m_node_num) m_isover = true;
    // }
    return sequence[turn];
}

void RandomNumGen::NodeFLowOver(uint32_t node_id)
{
    m_node_overnum[node_id] ++;
    if(m_node_overnum[node_id] >= m_node_num - 1)
    {
        m_sequence_over[node_id] = true;
        m_overnum++;
        if(m_overnum >= m_node_num)
        {
            m_isover = true;
        }
    }
}

bool RandomNumGen::IsTurnOver(uint32_t node_id)
{
    if(m_sequence_turn[node_id] >= m_node_num) return true;
    else return false;
    // return m_sequence_over[node_id];
}

bool RandomNumGen::IsOver()
{
    return m_isover;
}

void RandomNumGen::GenerateRandomSequences() 
{
    m_random_sequences.clear(); 

    for (uint32_t j = 0; j < m_node_num; ++j) {
        std::vector<uint32_t> sequence(m_node_num);
        for (uint32_t i = 0; i < m_node_num; ++i) {
            sequence[i] = i;
        }

        // std::random_device rd;
        // std::default_random_engine eng(rd());
        std::default_random_engine eng(m_seed++);
        std::shuffle(sequence.begin(), sequence.end(), eng);

        m_random_sequences.push_back(sequence);
    }
}

void RandomNumGen::Reset()
{
    m_node_num = 0;
    m_isover = false;
    m_overnum = 0;
    m_random_sequences.clear();
    m_sequence_turn.clear();
    m_node_overnum.clear();
    m_sequence_over.clear();
}

}
