/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "multiple-queue.h"
#include "drop-tail-queue.h"
#include "ns3/simulator.h"
#include <iostream>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MultipleQueue");

NS_OBJECT_TEMPLATE_CLASS_DEFINE (MultipleQueue,Packet);
NS_OBJECT_TEMPLATE_CLASS_DEFINE (MultipleQueue,QueueDiscItem);


/**
 * Implementation of the templates declared above.
 */

template <typename Item>
TypeId
MultipleQueue<Item>::GetTypeId (void)
{
    static TypeId tid = TypeId (GetTemplateClassName<MultipleQueue<Item>>())
                                .SetParent<Queue<Item> > ()
                        .SetGroupName ("Network")
                        .template AddConstructor<MultipleQueue<Item>> ()
                        .AddAttribute ("MaxSize",
                                    "The max queue size",
                                    QueueSizeValue (QueueSize ("100p")),
                                    MakeQueueSizeAccessor (&QueueBase::SetMaxSize,
                                                            &QueueBase::GetMaxSize),
                                    MakeQueueSizeChecker ())
                        .AddAttribute ("UseqCnt",
                                    "the subqueue numbers that use for test",
                                    UintegerValue(8),
                                    MakeUintegerAccessor(&MultipleQueue::m_useqCnt),
                                    MakeUintegerChecker<uint32_t>())
                        .AddAttribute ("DequeueType",
                                    "the type of packet dequeue",
                                    EnumValue (MultipleQueue::DequeueType (RR)),
                                    MakeEnumAccessor(&MultipleQueue::m_dequeueType),
                                    MakeEnumChecker(MultipleQueue::RR,
                                                    "RR",
                                                    MultipleQueue::DRR,
                                                    "DRR",
                                                    MultipleQueue::WRR,
                                                    "WRR",
                                                    MultipleQueue::PQ,
                                                    "PQ",
                                                    MultipleQueue::PQDRR,
                                                    "PQDRR"));                   
    return tid;
}

template <typename Item>
MultipleQueue<Item>::MultipleQueue () :
  Queue<Item> (),
  NS_LOG_TEMPLATE_DEFINE ("MultipleQueue")
{
    for (uint32_t i = 0; i < qCnt; i++){
        m_queues.push_back (CreateObject<DropTailQueue<Item> >());
        m_queues[i]->SetAttribute("MaxSize",QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS,100000000)));
        //100000000p if a packetsize is 64B, the subqueue size is about 6GB
        //in this way ,the subqueue won't be fulled.
        m_drr_num[i] = 0;
    }
    NS_LOG_FUNCTION (this);

    m_deindex = m_useqCnt - 1;
    m_enindex = 0;
}

template <typename Item>
MultipleQueue<Item>::~MultipleQueue ()
{
    NS_LOG_FUNCTION (this);
}

template <typename Item>
bool
MultipleQueue<Item>::Enqueue (Ptr<Item> item)
{
    NS_LOG_FUNCTION (this << item);

    bool flag = this->Enqueue(item,0);

    return flag;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::Dequeue (void)
{
    NS_LOG_FUNCTION (this);
    Ptr<Item> item = NULL;
    if(m_dequeueType == MultipleQueue::RR){
        item = this->DequeueRR();
    }else if(m_dequeueType == MultipleQueue::PQ){
        item = this->DequeuePQ();
    }else if(m_dequeueType == MultipleQueue::DRR){
        item = this->DequeueDRR();
    }else if(m_dequeueType == MultipleQueue::PQDRR){
        item = this->DequeuePQDRR();
    }else{
        item = NULL;   //there can add WRR or DRR
    }
    
    NS_LOG_LOGIC ("Popped " << item);

    return item;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::Remove (void)
{
    NS_LOG_FUNCTION (this);

    Ptr<Item> item = m_queues[0]->Remove();
    NS_LOG_LOGIC ("Removed " << item);

    return item;
}

template <typename Item>
Ptr<const Item>
MultipleQueue<Item>::Peek (void) const
{
    NS_LOG_FUNCTION (this);

    return m_queues[0]->Peek();
}

template <typename Item>
bool
MultipleQueue<Item>::Enqueue (Ptr<Item> item,uint32_t index)
{
    NS_LOG_FUNCTION (this << item);

    if(index<0||index>=qCnt||index>=m_useqCnt){
        return false;
    }

    bool flag = m_queues[index]->Enqueue(item);

    return flag;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::Dequeue (uint32_t index)
{ 
    NS_LOG_FUNCTION (this);
    if(index<0||index>=qCnt||index>=m_useqCnt){
        return 0;
    }
    Ptr<Item> item = m_queues[index]->Dequeue();
    
    return item;
}

template <typename Item>
bool
MultipleQueue<Item>::EnqueueTos (Ptr<Item> item,uint32_t index)
{
    NS_LOG_FUNCTION (this << item);
    bool flag = this->Enqueue(item,index);

    return flag;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::DequeueTos (uint32_t index)
{ 
    NS_LOG_FUNCTION (this);

    Ptr<Item> item = this->Dequeue(index);
    
    return item;
}

template <typename Item>
bool
MultipleQueue<Item>::EnqueueRR (Ptr<Item> item)
{
    NS_LOG_FUNCTION (this << item);

    m_enindex = (m_enindex)%(this->GetUseqCnt());
    bool flag = this->Enqueue(item,m_enindex);
    m_enindex = (m_enindex + 1)%(this->GetUseqCnt());

    return flag;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::DequeueRR (void)
{
    Ptr<Item> item = 0;
    for(uint32_t i=1;i<=this->GetUseqCnt();i++){
        uint32_t tempindex = (m_deindex+i)%(this->GetUseqCnt());
        item = this->Dequeue(tempindex);
        if(item){
            m_deindex = tempindex;
        break;
        }
    }
    return item;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::DequeuePQ (void)
{
    Ptr<Item> item = 0;
    for(uint32_t i=0;i<this->GetUseqCnt();i++){
        uint32_t tempindex = i;
        item = this->Dequeue(tempindex);
        if(item){
            m_deindex = tempindex;
            break;
        }
    }
    return item;
}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::DequeueDRR (void)
{   
    
    Ptr<Item> item = 0;
    for(uint32_t i=0;i<=this->GetUseqCnt();i++){
        uint32_t tempindex = (m_deindex+i)%(this->GetUseqCnt());
        if(i!=0){
            m_drr_num[tempindex] += m_weight;
        }
        if(m_drr_num[tempindex] <= 0){
            continue;
        }
        item = this->Dequeue(tempindex);
        if(item){
            m_deindex = tempindex;
            m_drr_num[m_deindex] -= item->GetSize();
            break;
        }else{
            m_drr_num[tempindex] = 0;
        }
    }

    return item;

}

template <typename Item>
Ptr<Item>
MultipleQueue<Item>::DequeuePQDRR (void)
{
    Ptr<Item> item = 0;
    //queue 0 dequeue
    item = this->Dequeue(0);
    if(item){
        m_deindex = 0;
        return item;
    }

    //queue 1 dequeue
    item = this->Dequeue(1);
    if(item){
        m_deindex = 1;
        return item;
    }

    //queue 2 dequeue
    // item = this->Dequeue(2);
    // if(item){
    //     m_deindex = 2;
    //     return item;
    // }

    //queue 3-useCout use DRR
    for(uint32_t i=0;i<=this->GetUseqCnt();i++){
        uint32_t tempindex = (drr_index+i)%(this->GetUseqCnt());
        if(tempindex < 2){
            continue;
        }
        if(i!=0){
            m_drr_num[tempindex] += m_weight;
        }
        if(m_drr_num[tempindex] <= 0){
            continue;
        }
        item = this->Dequeue(tempindex);
        if(item){
            m_deindex = tempindex;
            drr_index = tempindex;
            m_drr_num[m_deindex] -= item->GetSize();
            break;
        }else{
            m_drr_num[tempindex] = 0;
        }
    }
    return item;
}

template <typename Item>
void
MultipleQueue<Item>::SetEnIndex(uint32_t enindex)
{
    m_enindex = enindex;
}

template <typename Item>
uint32_t
MultipleQueue<Item>::GetEnIndex()
{
    return m_enindex;
}

template <typename Item>
void
MultipleQueue<Item>::SetDeIndex(uint32_t deindex)
{
    m_deindex = deindex;
}

template <typename Item>
uint32_t
MultipleQueue<Item>::GetDeIndex()
{
    return m_deindex;
}

template <typename Item>
void
MultipleQueue<Item>::SetUseqCnt(uint32_t useqCnt)
{
    m_useqCnt = useqCnt;
}

template <typename Item>
uint32_t
MultipleQueue<Item>::GetUseqCnt()
{
    return m_useqCnt;
}

template <typename Item>
void
MultipleQueue<Item>::SetDequeueType(DequeueType dtype)
{
    m_dequeueType = dtype;
}

template <typename Item> 
typename MultipleQueue<Item>::DequeueType
MultipleQueue<Item>::GetDequeueType()
{
    return m_dequeueType;
}

// The following explicit template instantiation declarations prevent all the
// translation units including this header file to implicitly instantiate the
// DropTailQueue<Packet> class and the DropTailQueue<QueueDiscItem> class. The
// unique instances of these classes are explicitly created through the macros
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,Packet) and
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,QueueDiscItem), which are included
// in drop-tail-queue.cc
extern template class MultipleQueue<Packet>;
extern template class MultipleQueue<QueueDiscItem>;

} // namespace ns3
