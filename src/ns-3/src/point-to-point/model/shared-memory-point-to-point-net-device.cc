/*
 * Copyright (c) 2007, 2008 University of Washington
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

#include "shared-memory-point-to-point-net-device.h"

#include "shared-memory-point-to-point-channel.h"
#include "ppp-header.h"

#include "ns3/error-model.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "ns3/multiple-queue.h"
#include <iostream>
#include <fstream>
#include <cmath>

#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#include "ns3/flow-id-tag.h"
#include "ns3/custom-priority-tag.h"
#include "ns3/unsched-tag.h"
#include "ns3/feedback-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SharedMemoryPointToPointNetDevice");

NS_OBJECT_ENSURE_REGISTERED(SharedMemoryPointToPointNetDevice);
std::unordered_map <uint32_t,uint32_t> SharedMemoryPointToPointNetDevice::queueIdMap;
TypeId
SharedMemoryPointToPointNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SharedMemoryPointToPointNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("PointToPoint")
            .AddConstructor<SharedMemoryPointToPointNetDevice>()
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(DEFAULT_MTU),
                          MakeUintegerAccessor(&SharedMemoryPointToPointNetDevice::SetMtu,
                                               &SharedMemoryPointToPointNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("Address",
                          "The MAC address of this device.",
                          Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
                          MakeMac48AddressAccessor(&SharedMemoryPointToPointNetDevice::m_address),
                          MakeMac48AddressChecker())
            .AddAttribute("DataRate",
                          "The default data rate for point to point links",
                          DataRateValue(DataRate("32768b/s")),
                          MakeDataRateAccessor(&SharedMemoryPointToPointNetDevice::m_bps),
                          MakeDataRateChecker())
            .AddAttribute("ReceiveErrorModel",
                          "The receiver error model used to simulate packet loss",
                          PointerValue(),
                          MakePointerAccessor(&SharedMemoryPointToPointNetDevice::m_receiveErrorModel),
                          MakePointerChecker<ErrorModel>())
            .AddAttribute("InterframeGap",
                          "The time to wait between packet (frame) transmissions",
                          TimeValue(Seconds(0.0)),
                          MakeTimeAccessor(&SharedMemoryPointToPointNetDevice::m_tInterframeGap),
                          MakeTimeChecker())
            .AddAttribute ("UpdataInterval",
                   "NANOSECONDS update interval for dequeue rate and N in ActiveBufferManagement(ns)",
                   UintegerValue (1054*8*10*1000),
                   MakeUintegerAccessor(&SharedMemoryPointToPointNetDevice::updateInterval),
                   MakeUintegerChecker<uint64_t> ())
            .AddAttribute ("Sat","saturation detection",
                            UintegerValue (20*1400),
                            MakeUintegerAccessor (&SharedMemoryPointToPointNetDevice::sat),
                            MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("StaticBuffer","static buffer",
                                    UintegerValue (0),
                                    MakeUintegerAccessor (&SharedMemoryPointToPointNetDevice::staticBuffer),
                                    MakeUintegerChecker<uint64_t> ())
            .AddAttribute ("DropType","drop type",
                                    EnumValue (SharedMemoryPointToPointNetDevice::DropType (NONE)),
                                    MakeEnumAccessor(&SharedMemoryPointToPointNetDevice::m_dropType),
                                    MakeEnumChecker(SharedMemoryPointToPointNetDevice::NONE,
                                                    "NONE",
                                                    SharedMemoryPointToPointNetDevice::RR,
                                                    "RR",
                                                    SharedMemoryPointToPointNetDevice::TOKEN,
                                                    "TOKEN",
                                                    SharedMemoryPointToPointNetDevice::LONGTOKEN,
                                                    "LONGTOKEN"))
            .AddAttribute ("PCType","PC schedule type",
                                    EnumValue (SharedMemoryPointToPointNetDevice::PCType (PCNONE)),
                                    MakeEnumAccessor(&SharedMemoryPointToPointNetDevice::m_pcType),
                                    MakeEnumChecker(SharedMemoryPointToPointNetDevice::PCNONE,
                                                    "NONE",
                                                    SharedMemoryPointToPointNetDevice::PCPQ,
                                                    "PQ"))

            //
            // Transmit queueing discipline for the device which includes its own set
            // of trace hooks.
            //
            .AddAttribute("TxQueue",
                          "A queue to use as the transmit queue in the device.",
                          PointerValue(),
                          MakePointerAccessor(&SharedMemoryPointToPointNetDevice::m_queue),
                          MakePointerChecker<Queue<Packet>>())

            //
            // Trace sources at the "top" of the net device, where packets transition
            // to/from higher layers.
            //
            .AddTraceSource("MacTx",
                            "Trace source indicating a packet has arrived "
                            "for transmission by this device",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_macTxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxDrop",
                            "Trace source indicating a packet has been dropped "
                            "by the device before transmission",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_macTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacPromiscRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a promiscuous trace,",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_macPromiscRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a non-promiscuous trace,",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_macRxTrace),
                            "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop",
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&SharedMemoryPointToPointNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
            //
            // Trace sources at the "bottom" of the net device, where packets transition
            // to/from the channel.
            //
            .AddTraceSource("PhyTxBegin",
                            "Trace source indicating a packet has begun "
                            "transmitting over the channel",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_phyTxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxEnd",
                            "Trace source indicating a packet has been "
                            "completely transmitted over the channel",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxDrop",
                            "Trace source indicating a packet has been "
                            "dropped by the device during transmission",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_phyTxDropTrace),
                            "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&SharedMemoryPointToPointNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
            .AddTraceSource("PhyRxEnd",
                            "Trace source indicating a packet has been "
                            "completely received by the device",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_phyRxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxDrop",
                            "Trace source indicating a packet has been "
                            "dropped by the device during reception",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_phyRxDropTrace),
                            "ns3::Packet::TracedCallback")

            //
            // Trace sources designed to simulate a packet sniffer facility (tcpdump).
            // Note that there is really no difference between promiscuous and
            // non-promiscuous traces in a point-to-point link.
            //
            .AddTraceSource("Sniffer",
                            "Trace source simulating a non-promiscuous packet sniffer "
                            "attached to the device",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_snifferTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PromiscSniffer",
                            "Trace source simulating a promiscuous packet sniffer "
                            "attached to the device",
                            MakeTraceSourceAccessor(&SharedMemoryPointToPointNetDevice::m_promiscSnifferTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

SharedMemoryPointToPointNetDevice::SharedMemoryPointToPointNetDevice()
    : m_txMachineState(READY),
      m_channel(nullptr),
      m_linkUp(false),
      m_currentPkt(nullptr)
{
    NS_LOG_FUNCTION(this);
}

SharedMemoryPointToPointNetDevice::~SharedMemoryPointToPointNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
SharedMemoryPointToPointNetDevice::AddHeader(Ptr<Packet> p, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << p << protocolNumber);
    PppHeader ppp;
    ppp.SetProtocol(EtherToPpp(protocolNumber));
    p->AddHeader(ppp);
}

bool
SharedMemoryPointToPointNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param)
{
    NS_LOG_FUNCTION(this << p << param);
    PppHeader ppp;
    p->RemoveHeader(ppp);
    param = PppToEther(ppp.GetProtocol());
    return true;
}

void
SharedMemoryPointToPointNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_channel = nullptr;
    m_receiveErrorModel = nullptr;
    m_currentPkt = nullptr;
    m_queue = nullptr;
    NetDevice::DoDispose();
}

void
SharedMemoryPointToPointNetDevice::SetDataRate(DataRate bps)
{
    NS_LOG_FUNCTION(this);
    m_bps = bps;
}

void
SharedMemoryPointToPointNetDevice::SetInterframeGap(Time t)
{
    NS_LOG_FUNCTION(this << t.As(Time::S));
    m_tInterframeGap = t;
}

bool
SharedMemoryPointToPointNetDevice::TransmitStartSwitch (Ptr<Packet> p, uint32_t dequeue_index)
{
    NS_LOG_FUNCTION (this << p);
    NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");
    if(m_throughputFlag == true && throughputFirstFlag == true)
    {
        throughputFirstFlag = false;
        Update(throughputUpdateInterval);
    }

    //
    // This function is called to start the process of transmitting a packet.
    // We need to tell the channel that we've started wiggling the wire and
    // schedule an event that will be executed when the transmission is complete.
    //
    FeedbackTag Int;
    bool found;
    found = p->PeekPacketTag(Int);
    if (found)
    {   
        Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
        Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[dequeue_index];
        Int.setTelemetryQlenDeq(Int.getHopCount(), subqueue->GetNBytes()); // queue length at dequeue
        Int.setTelemetryTsDeq(Int.getHopCount(), Simulator::Now().GetNanoSeconds()); // timestamp at dequeue
        Int.setTelemetryBw(Int.getHopCount(), m_bps.GetBitRate());
        Int.setTelemetryTxBytes(Int.getHopCount(), txBytesInt);
        Int.incrementHopCount(); // Incrementing hop count at Dequeue. Don't do this at enqueue.
        p->ReplacePacketTag(Int);
    }

    NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
    m_txMachineState = BUSY;
    m_currentPkt = p;
    m_phyTxBeginTrace (m_currentPkt);

    Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
    Time txCompleteTime = txTime + m_tInterframeGap;
    
    //msg
    Ptr<Switch> m_switch = GetMMUSwitch();
    m_switch->send_size += p->GetSize();

    NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.As (Time::S));
    Simulator::Schedule (txCompleteTime, &SharedMemoryPointToPointNetDevice::TransmitCompleteSwitch, this);

    //ABM---------
    ChangeDeqPriorityBytes(p, dequeue_index);
    txBytesInt += p->GetSize();
    throughputBytes += p->GetSize();
    //ABM---------end


    bool result = m_channel->TransmitStart (p, this, txTime);
    if (result == false)
        {
        m_phyTxDropTrace (p);
        }
    return result;
}

bool
SharedMemoryPointToPointNetDevice::TransmitStartNode (Ptr<Packet> p)
{
    NS_LOG_FUNCTION (this << p);
    NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

    //
    // This function is called to start the process of transmitting a packet.
    // We need to tell the channel that we've started wiggling the wire and
    // schedule an event that will be executed when the transmission is complete.
    //
    NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
    m_txMachineState = BUSY;
    m_currentPkt = p;
    m_phyTxBeginTrace (m_currentPkt);

    Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
    Time txCompleteTime = txTime + m_tInterframeGap;

    NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.As (Time::S));
    Simulator::Schedule (txCompleteTime, &SharedMemoryPointToPointNetDevice::TransmitCompleteNode, this);

    bool result = m_channel->TransmitStart (p, this, txTime);
    if (result == false)
        {
        m_phyTxDropTrace (p);
        }
    return result;
}

void
SharedMemoryPointToPointNetDevice::TransmitCompleteSwitch (void)
{
    NS_LOG_FUNCTION (this);
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    //
    // This function is called to when we're all done transmitting a packet.
    // We try and pull another packet off of the transmit queue.  If the queue
    // is empty, we are done, otherwise we need to start transmitting the
    // next packet.
    //
    NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
    m_txMachineState = READY;

    NS_ASSERT_MSG (m_currentPkt, "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

    m_phyTxEndTrace (m_currentPkt);
    m_currentPkt = 0;

    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >(); 
    Ptr<Packet> packet;
    uint32_t dequeue_index;
    Ptr<Queue<Packet> >subqueue_de;

    if(m_switch->GetDequeueMethod() == Switch::NORMAL_DEQUEUE){
        //check-drop
        DropCheck();
        packet = mulqueue->Dequeue();
        if(packet == NULL){
        return;
        }
            dequeue_index = mulqueue->GetDeIndex();
            subqueue_de = mulqueue->m_queues[dequeue_index];
        if(usedStaticBuffer[dequeue_index] > subqueue_de->GetNBytes()){
            usedStaticBuffer[dequeue_index] -= packet->GetSize();   //the packet comes from static memory
        }else{
            
            m_switch->DeleteUsed(packet->GetSize(), dequeue_index); //the packet comes from shared memory
            TokenDelete(packet->GetSize());
        }
        
        WriteDetail("-",packet,dequeue_index);
        WriteQueueMsg(dequeue_index);
        
    }else{
        return;
    }
    

    if (!packet)
        {
        NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
        return;
        }

    //
    // Got another packet off of the queue, so start the transmit process again.
    //
    // std::cout<<"------------------------"<<std::endl;
    uint64_t currentSize = subqueue_de->GetNBytes() - usedStaticBuffer[dequeue_index];
    double satLevel =0;
    if(currentSize >= 0.9*GetQueueThreshold(dequeue_index) ){
        satLevel = 1;
    }
    uint32_t portId = this->GetIfIndex();
    m_switch->SetSaturated(portId, dequeue_index, satLevel);
    
    m_snifferTrace (packet);
    m_promiscSnifferTrace (packet);
    TransmitStartSwitch (packet, dequeue_index);
}

void
SharedMemoryPointToPointNetDevice::TransmitCompleteNode (void)
{
    NS_LOG_FUNCTION (this);

    //
    // This function is called to when we're all done transmitting a packet.
    // We try and pull another packet off of the transmit queue.  If the queue
    // is empty, we are done, otherwise we need to start transmitting the
    // next packet.
    //
    NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
    m_txMachineState = READY;

    NS_ASSERT_MSG (m_currentPkt, "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

    m_phyTxEndTrace (m_currentPkt);
    m_currentPkt = 0;

    Ptr<Packet> p;
    
    // p = m_queue->GetObject<MultipleQueue<Packet> >()-> Dequeue(0);
    if(m_pcType == PCType::PCNONE){
        p = m_queue->GetObject<MultipleQueue<Packet> >()-> Dequeue(0);
    }else if(m_pcType == PCType::PCPQ){
        p = m_queue->GetObject<MultipleQueue<Packet> >()-> DequeuePQ();
    }
    

    if (!p)
    {
        NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
        return;
    }
    WriteDetail("-", p);
    //
    // Got another packet off of the queue, so start the transmit process again.
    //
    m_snifferTrace (p);
    m_promiscSnifferTrace (p);
    TransmitStartNode (p);
}

bool
SharedMemoryPointToPointNetDevice::Attach(Ptr<SharedMemoryPointToPointChannel> ch)
{
    NS_LOG_FUNCTION(this << &ch);

    m_channel = ch;

    m_channel->Attach(this);

    //
    // This device is up whenever it is attached to a channel.  A better plan
    // would be to have the link come up when both devices are attached, but this
    // is not done for now.
    //
    NotifyLinkUp();
    return true;
}

void
SharedMemoryPointToPointNetDevice::SetQueue(Ptr<Queue<Packet>> q)
{
    NS_LOG_FUNCTION(this << q);
    m_queue = q;
}

void
SharedMemoryPointToPointNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em)
{
    NS_LOG_FUNCTION(this << em);
    m_receiveErrorModel = em;
}

void
SharedMemoryPointToPointNetDevice::Receive(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    uint16_t protocol = 0;

    if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
    {
        //
        // If we have an error model and it indicates that it is time to lose a
        // corrupted packet, don't forward this packet up, let it go.
        //
        m_phyRxDropTrace(packet);
    }
    else
    {
        //
        // Hit the trace hooks.  All of these hooks are in the same place in this
        // device because it is so simple, but this is not usually the case in
        // more complicated devices.
        //
        m_snifferTrace(packet);
        m_promiscSnifferTrace(packet);
        m_phyRxEndTrace(packet);

        //
        // Trace sinks will expect complete packets, not packets without some of the
        // headers.
        //
        Ptr<Packet> originalPacket = packet->Copy();

        //
        // Strip off the point-to-point protocol header and forward this packet
        // up the protocol stack.  Since this is a simple point-to-point link,
        // there is no difference in what the promisc callback sees and what the
        // normal receive callback sees.
        //
        ProcessHeader(packet, protocol);
        WriteDetail("r",packet);
        if (!m_promiscCallback.IsNull())
        {
            m_macPromiscRxTrace(originalPacket);
            m_promiscCallback(this,
                              packet,
                              protocol,
                              GetRemote(),
                              GetAddress(),
                              NetDevice::PACKET_HOST);
        }

        m_macRxTrace(originalPacket);
        m_rxCallback(this, packet, protocol, GetRemote());
    }
}

Ptr<Queue<Packet>>
SharedMemoryPointToPointNetDevice::GetQueue() const
{
    NS_LOG_FUNCTION(this);
    return m_queue;
}

void
SharedMemoryPointToPointNetDevice::NotifyLinkUp()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = true;
    m_linkChangeCallbacks();
}

void
SharedMemoryPointToPointNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this);
    m_ifIndex = index;
}

uint32_t
SharedMemoryPointToPointNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
SharedMemoryPointToPointNetDevice::GetChannel() const
{
    return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
SharedMemoryPointToPointNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = Mac48Address::ConvertFrom(address);
}

Address
SharedMemoryPointToPointNetDevice::GetAddress() const
{
    return m_address;
}

bool
SharedMemoryPointToPointNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_linkUp;
}

void
SharedMemoryPointToPointNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this);
    m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
SharedMemoryPointToPointNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
SharedMemoryPointToPointNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool
SharedMemoryPointToPointNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
SharedMemoryPointToPointNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address("01:00:5e:00:00:00");
}

Address
SharedMemoryPointToPointNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address("33:33:00:00:00:00");
}

bool
SharedMemoryPointToPointNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

bool
SharedMemoryPointToPointNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}


bool
SharedMemoryPointToPointNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
    bool flag = false;
    if(m_node->GetNodeType() == 1){
        flag = SendSwitch(packet,dest,protocolNumber);
    }else{
        flag = SendNode(packet,dest,protocolNumber);
    } 
    return flag;
}

bool
SharedMemoryPointToPointNetDevice::SendSwitch (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{   
    NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
    NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
    NS_LOG_LOGIC ("UID is " << packet->GetUid ());

    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    //get the tos, ip, port

    Ipv4Header ipv4Header;
    Ptr<Packet> copyPacket = packet->Copy();
    copyPacket->RemoveHeader(ipv4Header);

    bool found;
    uint32_t unsched = 0;
    UnSchedTag tag;
    found = copyPacket->PeekPacketTag (tag);
    if(found){
        unsched=tag.GetValue();
    }

    Ipv4Address source_address = ipv4Header.GetSource ();
    Ipv4Address dest_address = ipv4Header.GetDestination ();
    uint8_t protocol = ipv4Header.GetProtocol ();
    uint32_t tos = (uint32_t)ipv4Header.GetTos();
    uint32_t ecn = (uint32_t)ipv4Header.GetEcn();
    uint32_t dscp = (uint32_t)ipv4Header.GetDscp();
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;

    uint32_t enqueue_index = 100;
    if(protocol == UdpL4Protocol::PROT_NUMBER){
        UdpHeader udpHeader;
        copyPacket->RemoveHeader (udpHeader);
        srcPort = udpHeader.GetSourcePort ();
        dstPort = udpHeader.GetDestinationPort ();
        
        // enqueue_index = 0;
        enqueue_index = dscp;
        
    }else if(protocol == TcpL4Protocol::PROT_NUMBER){
        TcpHeader tcpHeader;
        copyPacket->RemoveHeader (tcpHeader);
        srcPort = tcpHeader.GetSourcePort ();
        dstPort = tcpHeader.GetDestinationPort ();
        
        enqueue_index = GetQueueId(source_address.Get(), dest_address.Get(), uint32_t(dstPort));
        if(enqueue_index == 100)
        {   //may be it is not a positive flow, it is ack
            enqueue_index = 0;
        }
    
    }
    if(0){
        std::cout<<"--------------------"<<std::endl;
        std::cout<<"pkt_size: "<<packet->GetSize()<<std::endl;
        std::cout<<source_address<<" : "<<srcPort<<" --> "<<dest_address<<" : "<<dstPort<<std::endl;
        std::cout<<"protocol: "<<int(protocol)<<std::endl;
        std::cout<<"tos: "<<tos<<std::endl;
        std::cout<<"ecn: "<<ecn<<std::endl;
        std::cout<<"dscp: "<<dscp<<std::endl;
        std::cout<<std::endl;
        std::cout<<"--------------------"<<std::endl;
    }
    bool is_query = false;
    if(enqueue_index == 1 && dstPort >= 20010 && dstPort <= 65530){
        is_query = true;
    }
    
    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    // uint32_t enqueue_index = dscp;
    Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[enqueue_index];

    
    //ECN process--------------------------------------------------------------------------------------------------------
    std::string ecnFlag = "false";
    if(int(subqueue->GetNBytes() + packet->GetSize() - usedStaticBuffer[enqueue_index]) > int(m_switch->GetEcnThreshold())){
        Ipv4Header tempheader;
        packet->RemoveHeader(tempheader);

        if(tempheader.GetEcn()==Ipv4Header::EcnType::ECN_ECT1||tempheader.GetEcn()==Ipv4Header::EcnType::ECN_ECT0)
        {
            tempheader.SetEcn(Ipv4Header::EcnType::ECN_CE);
            ecnFlag = "true";
        }
        // if(enqueue_index<=1){
        //     if(tempheader.GetEcn()==Ipv4Header::EcnType::ECN_ECT1||tempheader.GetEcn()==Ipv4Header::EcnType::ECN_ECT0)
        //     {
        //         tempheader.SetEcn(Ipv4Header::EcnType::ECN_CE);
        //         ecnFlag = "true";
        //     }
        // }
        
        packet->AddHeader(tempheader);
    }

    //
    // If IsLinkUp() is false it means there is no channel to send any packet 
    // over so we just hit the drop trace on the packet and return an error.
    //
    if (IsLinkUp () == false)
        {
        m_macTxDropTrace (packet);
        return false;
        }

    //
    // Stick a point to point protocol header on the packet in preparation for
    // shoving it out the door.
    //
    AddHeader (packet, protocolNumber);
    m_macTxTrace (packet);

    //
    // We should enqueue and dequeue the packet to hit the tracing hooks.
    //
    
    //all enqueueMethod has to obey this rule, need enough space to store it
    bool enqueue_flag = false;
    // first need to use static buffer
    bool usedStaticBufferFlag = false;
    if(usedStaticBuffer[enqueue_index] + packet->GetSize() <= staticBuffer){
        usedStaticBufferFlag = true;
        enqueue_flag = mulqueue->EnqueueTos(packet,enqueue_index);
    }else{
        if(AcceptPacket(packet, enqueue_index, unsched)==true){
        enqueue_flag = mulqueue->EnqueueTos(packet,enqueue_index);
        }
    }
    
    // if(enqueue_flag == false && is_query == true){
    //     WriteQueryLoss();
    // }
    if(enqueue_flag == false){
        WriteQueryLoss();
    }

    if(enqueue_flag == false){
        
    }

    if(enqueue_flag){
        
        if(usedStaticBufferFlag==true){
            usedStaticBuffer[enqueue_index] += packet->GetSize();
        }else{
            m_switch->AddUsed(packet->GetSize(), enqueue_index);
        }    
        WriteDetail("+",packet, enqueue_index);
        //show in terminal and write into file
        WriteQueueMsg(enqueue_index, ecnFlag);
        
        // if(m_node->GetId() == 10)
        // {
        //     ShowMMU();
        // }

        if(m_txMachineState == READY){
        uint32_t dequeue_index = 0 ;
        if(m_switch->GetDequeueMethod() == Switch::NORMAL_DEQUEUE){
            //check-drop

            DropCheck();

            packet = mulqueue->Dequeue();
            if(packet==NULL){
                return false;
            }
            dequeue_index = mulqueue->GetDeIndex();        
            Ptr<Queue<Packet> >subqueue_de = mulqueue->m_queues[dequeue_index];

            if(usedStaticBuffer[dequeue_index] > subqueue_de->GetNBytes()){
                usedStaticBuffer[dequeue_index] -= packet->GetSize();   //the packet comes from static memory
            }else{
                
                m_switch->DeleteUsed(packet->GetSize(), dequeue_index); //the packet comes from shared memory
                TokenDelete(packet->GetSize());
            }

            WriteDetail("-",packet, dequeue_index);

            WriteQueueMsg(dequeue_index);
            // if(m_node->GetId() == 10)
            // {
            //     ShowMMU();
            // }

        }else{
            return false;
        }
        m_snifferTrace (packet);
        m_promiscSnifferTrace (packet);
        bool ret = TransmitStartSwitch (packet, dequeue_index);
        return ret;
        }
        return true;
    }
    WriteDetail("ed",packet, enqueue_index);
    m_macTxDropTrace (packet);
    return false;
}

bool
SharedMemoryPointToPointNetDevice::AcceptPacket(Ptr<Packet> packet, uint32_t index, uint32_t unsched){
    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    uint32_t enqueue_index = index;
    Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[enqueue_index];
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    if(m_switch->GetEnqueueMethod() == Switch::NORMAL_ENQUEUE){
        //do nothing, the packet can enqueue if the subqueue have space to store it
        if(int(m_switch->GetUsedBuffer() + packet->GetSize()  ) > m_switch->GetMaxBuffer()){         //we need delete the sum of all static buffer used
            return false;
        }
    }else if(m_switch->GetEnqueueMethod() == Switch::DT){

        if(int(m_switch->GetUsedBuffer() + packet->GetSize()  ) > m_switch->GetMaxBuffer()){          //we need delete static buffer from the total queue length
            return false;
        }

        //queue len need less than threshold 
        if (int(subqueue->GetNBytes() + packet->GetSize() - usedStaticBuffer[index]) > GetQueueThreshold(enqueue_index)){
            return false;
        }


    }else if(m_switch->GetEnqueueMethod() == Switch::MULTILAYER_DT){

        Ptr<Switch> global_switch = m_node->m_switch;
        if(!global_switch->first_flag){ //only need to call one
            // std::cout<<"call "<<std::endl;
            TokenAdd();
        } 



        if(int(m_switch->GetUsedBuffer() + packet->GetSize()  ) > m_switch->GetMaxBuffer()){
            return false;
        }

        //if queue len is 0, packet can enqueue
        if (int(subqueue->GetNBytes() - usedStaticBuffer[index]) != 0){

            //queue len need less than threshold
            if (int(subqueue->GetNBytes() + packet->GetSize() - usedStaticBuffer[index]) > GetQueueThreshold(enqueue_index)){
                return false;
            }
            //priority len need less than the priority threshold, switch have the priority space, so we don't need to deleting ______delete finally
            // if( uint64_t(m_switch->GetPrioritySize(enqueue_index) + packet->GetSize()) > m_switch->GetPriorityThreshold(enqueue_index)){
            //     std::cout<<"drop: "<<m_switch->GetPrioritySize(enqueue_index)<<" "<<m_switch->GetPriorityThreshold(enqueue_index)<<std::endl;
            //     return false;
            // }
        }

        
    }else if(m_switch->GetEnqueueMethod() == Switch::PUSHOUT){

        if(int(m_switch->GetUsedBuffer() + packet->GetSize()  ) > m_switch->GetMaxBuffer()){              
            uint32_t need_space = packet->GetSize();
            uint32_t push_space = 0;
            while(push_space < need_space){
                Ptr<Packet> pushout_packet = PushOutPacket(enqueue_index);
                if(pushout_packet == NULL){
                    return false;
                }
                WriteDetail("p", pushout_packet, 0); 
                push_space += pushout_packet->GetSize();
            }
             
        }
 
    }else if(m_switch->GetEnqueueMethod() == Switch::ABM){
        if(firstTimeUpdate){
            firstTimeUpdate=false;
            InvokeUpdates(updateInterval);
        }
        uint64_t currentSize = subqueue->GetNBytes() - usedStaticBuffer[index];
        double satLevel = 0;
        if(currentSize >= 0.9*GetQueueThreshold(enqueue_index) ){
            satLevel = 1;
        }
        uint32_t portId = this->GetIfIndex();
        m_switch->SetSaturated(portId, enqueue_index, satLevel);

        if(int(m_switch->GetUsedBuffer() + packet->GetSize()  ) > m_switch->GetMaxBuffer()){
            return false;
        }

        //queue len need less than threshold 
        int threshold = GetQueueThreshold(enqueue_index);
        if (unsched){
        uint32_t priority = m_switch->qToP[enqueue_index];
            // threshold = threshold * alphaUnsched / m_switch->qAlpha[priority];
            threshold = threshold;    //don't use
        } 
        if (int(subqueue->GetNBytes() + packet->GetSize() - usedStaticBuffer[index]) > threshold){
        return false;
        }

    }

    return true;
}  

bool
SharedMemoryPointToPointNetDevice::SendNode (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
    NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
    NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
    NS_LOG_LOGIC ("UID is " << packet->GetUid ());
    //
    // If IsLinkUp() is false it means there is no channel to send any packet 
    // over so we just hit the drop trace on the packet and return an error.
    //

    // Ipv4Header tempheader;
    // packet->RemoveHeader(tempheader);
    // if(tempheader.GetEcn()==Ipv4Header::EcnType::ECN_NotECT){
    //     if(tempheader.GetDscp() == 7){  //this is because we set tos, will delete ecn, so we need to reset 
    //         tempheader.SetEcn(Ipv4Header::EcnType::ECN_ECT1);
    //     }
    // }
    // packet->AddHeader(tempheader);

    //get the tos, ip, port

    Ipv4Header ipv4Header;
    Ptr<Packet> copyPacket = packet->Copy();
    copyPacket->RemoveHeader(ipv4Header);

    Ipv4Address source_address = ipv4Header.GetSource ();
    Ipv4Address dest_address = ipv4Header.GetDestination ();
    uint8_t protocol = ipv4Header.GetProtocol ();
    uint32_t tos = (uint32_t)ipv4Header.GetTos();
    uint32_t ecn = (uint32_t)ipv4Header.GetEcn();
    uint32_t dscp = (uint32_t)ipv4Header.GetDscp();
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;

    uint32_t enqueue_index = 100;
    if(protocol == UdpL4Protocol::PROT_NUMBER){
        UdpHeader udpHeader;
        copyPacket->RemoveHeader (udpHeader);
        srcPort = udpHeader.GetSourcePort ();
        dstPort = udpHeader.GetDestinationPort ();
        
        enqueue_index = 0;
        
    }else if(protocol == TcpL4Protocol::PROT_NUMBER){
        TcpHeader tcpHeader;
        copyPacket->RemoveHeader (tcpHeader);
        srcPort = tcpHeader.GetSourcePort ();
        dstPort = tcpHeader.GetDestinationPort ();
        
        enqueue_index = GetQueueId(source_address.Get(), dest_address.Get(), uint32_t(dstPort));
        if(enqueue_index == 100)
        {   //may be it is not a positive flow, it is ack
            enqueue_index = 0;
        }
    
    }

    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    // uint32_t enqueue_index = dscp;
    Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[enqueue_index];


    if (IsLinkUp () == false)
        {
            m_macTxDropTrace (packet);
            return false;
        }

    //
    // Stick a point to point protocol header on the packet in preparation for
    // shoving it out the door.
    //
    
    AddHeader (packet, protocolNumber);

    m_macTxTrace (packet);

    //
    // We should enqueue and dequeue the packet to hit the tracing hooks.
    //
    bool enqueue_flag = false;
    if(m_pcType == PCType::PCNONE){
        enqueue_flag = m_queue->Enqueue (packet);
    }else if(m_pcType == PCType::PCPQ){
        enqueue_flag = mulqueue->EnqueueTos(packet,enqueue_index);
    }

    if (m_queue->Enqueue (packet))
        { 
            WriteDetail("+", packet);
            //
            // If the channel is ready for transition we send the packet right now
            // 
            if (m_txMachineState == READY)
            {   
                // packet = m_queue->GetObject<MultipleQueue<Packet> >()-> Dequeue(0);

                if(m_pcType == PCType::PCNONE){
                    packet = m_queue->GetObject<MultipleQueue<Packet> >()-> Dequeue(0);
                }else if(m_pcType == PCType::PCPQ){
                    packet = m_queue->GetObject<MultipleQueue<Packet> >()-> DequeuePQ();
                }
                WriteDetail("-",packet);
                m_snifferTrace (packet);
                m_promiscSnifferTrace (packet);
                bool ret = TransmitStartNode (packet);
                return ret;
            }
            return true;
        }

    // Enqueue may fail (overflow)
    WriteDetail("ed", packet);
    m_macTxDropTrace (packet);
    return false;
}





bool
SharedMemoryPointToPointNetDevice::SendFrom(Ptr<Packet> packet,
                                const Address& source,
                                const Address& dest,
                                uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);
    return false;
}

Ptr<Node>
SharedMemoryPointToPointNetDevice::GetNode() const
{
    return m_node;
}

void
SharedMemoryPointToPointNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    m_node = node;
}

bool
SharedMemoryPointToPointNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
SharedMemoryPointToPointNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_rxCallback = cb;
}

void
SharedMemoryPointToPointNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    m_promiscCallback = cb;
}

bool
SharedMemoryPointToPointNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
SharedMemoryPointToPointNetDevice::DoMpiReceive(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    Receive(p);
}

Address
SharedMemoryPointToPointNetDevice::GetRemote() const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_channel->GetNDevices() == 2);
    for (std::size_t i = 0; i < m_channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> tmp = m_channel->GetDevice(i);
        if (tmp != this)
        {
            return tmp->GetAddress();
        }
    }
    NS_ASSERT(false);
    // quiet compiler.
    return Address();
}

bool
SharedMemoryPointToPointNetDevice::SetMtu(uint16_t mtu)
{
    NS_LOG_FUNCTION(this << mtu);
    m_mtu = mtu;
    return true;
}

uint16_t
SharedMemoryPointToPointNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    return m_mtu;
}

uint16_t
SharedMemoryPointToPointNetDevice::PppToEther(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0021:
        return 0x0800; // IPv4
    case 0x0057:
        return 0x86DD; // IPv6
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}

uint16_t
SharedMemoryPointToPointNetDevice::EtherToPpp(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0800:
        return 0x0021; // IPv4
    case 0x86DD:
        return 0x0057; // IPv6
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}
//--------------------------
std::string
SharedMemoryPointToPointNetDevice::GetName (){
    std::string name = "";
    if (m_node == NULL) {
        return name;
    }
    // std::cout<<m_node->GetNodeType()<<std::endl;
    if (m_node->GetNodeType()==1){
        name = "switch-" + std::to_string(m_node->GetId()) + "-" + std::to_string(this->GetIfIndex());
    }else{
        name = "host-" + std::to_string(m_node->GetId()) + "-" + std::to_string(this->GetIfIndex());
    }
    
    return name;
}

void
SharedMemoryPointToPointNetDevice::WriteQueueMsg(int subqueueNo,std::string ecnFlag){
    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[subqueueNo];
    int queneLength = subqueue->GetNBytes();       //this is queue len, don't delete static buffer
    // int queneLength = subqueue->GetNBytes() - usedStaticBuffer[subqueueNo];       //this is queue len, delete static buffer
    int threshold = GetQueueThreshold(subqueueNo);
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();


    uint32_t priority = m_switch->qToP[subqueueNo];
    uint64_t priorityLen = m_switch->GetPrioritySize(subqueueNo);
    uint64_t priorityThreshold = m_switch->GetPriorityThreshold(subqueueNo);

    if(m_writeFlag==true){
        m_writeCount++;
        if(m_writeCount%m_writeFrequence==0){
        m_fileFout<<double(Simulator::Now ().GetNanoSeconds ())/1000000000.0<<" "\
        <<subqueueNo<<" "<<queneLength<<" "<<threshold<<" "\
        <<ecnFlag<<" "\
        <<priority<<" "<<priorityLen<<" "<<priorityThreshold<<" "\
        <<std::endl;
        }
    }

}

void
SharedMemoryPointToPointNetDevice::SetWriteFileFolderName(std::string folderName){
    // std::cout<<GetName()<<std::endl;
    m_writeFlag = true;
    m_writeFileName = folderName + GetName() + ".txt";
    std::ofstream fout1(m_writeFileName,std::ios::trunc);      //clear txt
    fout1.close();

    m_fileFout.open(m_writeFileName,std::ios::app);            //write txt and don't clear txt  
}



void
SharedMemoryPointToPointNetDevice::SetDetailFileFolderName(std::string folderName){      //this is used to write detail msg 
    m_detailFlag = true;
    m_detailFileName = folderName + GetName() + ".txt";
    std::ofstream fout1(m_detailFileName,std::ios::trunc);      //clear txt
    fout1.close();

    m_detailFout.open(m_detailFileName,std::ios::app);            //write txt and don't clear txt  
}

void
SharedMemoryPointToPointNetDevice::WriteDetail(std::string action, Ptr<Packet> packet, uint32_t index){

    if(m_detailFlag == false){
        // std::cout<<"write fails"<<std::endl;
        return;
    }
    
    Ipv4Header ipv4Header;
    Ptr<Packet> tempPacket = packet->Copy();
    uint16_t protocol1 = 0;
    if(action!="r"){
        ProcessHeader (tempPacket, protocol1);
    }


    tempPacket->RemoveHeader(ipv4Header);

    Ipv4Address source_address = ipv4Header.GetSource ();
    Ipv4Address dest_address = ipv4Header.GetDestination ();
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint8_t protocol = ipv4Header.GetProtocol ();

    if(protocol == UdpL4Protocol::PROT_NUMBER){
        UdpHeader udpHeader;
        tempPacket->RemoveHeader (udpHeader);
        srcPort = udpHeader.GetSourcePort ();
        dstPort = udpHeader.GetDestinationPort ();
        
    }else if(protocol == TcpL4Protocol::PROT_NUMBER){
        TcpHeader tcpHeader;
        tempPacket->RemoveHeader (tcpHeader);
        srcPort = tcpHeader.GetSourcePort ();
        dstPort = tcpHeader.GetDestinationPort ();
    }
    if(m_node->GetNodeType() == 1 && action=="ed"){
        Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
        Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[index];
        int queneLength = subqueue->GetNBytes();       //this is queue len, don't delete static buffer
        int threshold = GetQueueThreshold(index);
        std::cout<<action<<" "\
        <<Simulator::Now ().GetSeconds ()<<" "\
        <<packet->GetUid()<<" "\
        <<source_address<<":"<<srcPort<<"-->"<<dest_address<<":"<<dstPort<<" "\
        <<index<<" "<<queneLength<<" "<<threshold<<" "\
        <<std::endl;

    }

    // if(m_node->GetNodeType() == 1 && action!="r"){
    //     Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    //     Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[index];
    //     int queneLength = subqueue->GetNBytes();       //this is queue len, don't delete static buffer
    //     int threshold = GetQueueThreshold(index);

    //     m_detailFout<<action<<" "\
    //     <<Simulator::Now ().GetSeconds ()<<" "\
    //     <<packet->GetUid()<<" "\
    //     <<source_address<<":"<<srcPort<<"-->"<<dest_address<<":"<<dstPort<<" "\
    //     <<index<<" "<<queneLength<<" "<<threshold<<" "\
    //     <<std::endl;
    //     /*
    //     std::cout<<action<<" "\
    //     <<Simulator::Now ().GetSeconds ()<<" "\
    //     <<packet->GetUid()<<" "\
    //     <<source_address<<":"<<srcPort<<"-->"<<dest_address<<":"<<dstPort<<" "\
    //     <<index<<" "<<queneLength<<" "<<threshold<<" "\
    //     <<std::endl;
    //     */
    // }else{
    //     m_detailFout<<action<<" "\
    //     <<Simulator::Now ().GetSeconds ()<<" "\
    //     <<packet->GetUid()<<" "\
    //     <<source_address<<":"<<srcPort<<"-->"<<dest_address<<":"<<dstPort<<" "\
    //     <<std::endl;
    //     /*
    //     std::cout<<action<<" "\
    //     <<Simulator::Now ().GetSeconds ()<<" "\
    //     <<packet->GetUid()<<" "\
    //     <<source_address<<":"<<srcPort<<"-->"<<dest_address<<":"<<dstPort<<" "\
    //     <<std::endl;
    //     */
    // }
  
}

int
SharedMemoryPointToPointNetDevice::GetType(void)
{
    return 1;
}

Ptr<Packet> 
SharedMemoryPointToPointNetDevice::PushOutPacket(uint32_t enqueue_index)
{

    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();
    uint32_t left = (this->GetIfIndex() - 1) / 8 * 8 + 1 ;          //beacuse 3 mmu, pushout in different mmu
    uint32_t right = (this->GetIfIndex() - 1) / 8 * 8 + 8;

    uint32_t longest_queue_len = 0;
    Ptr<Packet> packet = NULL;
    uint32_t dequeue_index = 0;

    Ptr<Queue<Packet> >longest_queue = NULL;
    // for(uint32_t i=0; i<m_node->GetNDevices(); i++){

    for(uint32_t i = left; i <= right; i++){
        Ptr<NetDevice> dev = m_node->GetDevice(i);
        if(dev->GetType() == 0){
            continue;       //this is a protocol stack dev
        }
        Ptr<SharedMemoryPointToPointNetDevice> sharedMemoryDev = dev->GetObject<SharedMemoryPointToPointNetDevice>();
        Ptr<MultipleQueue<Packet> > mulqueue = sharedMemoryDev->GetQueue()->GetObject<MultipleQueue<Packet> >();
        for(uint32_t j=0; j<8; j++){
            uint32_t subqueue_len = mulqueue->m_queues[j]->GetNBytes();
            if(subqueue_len >= longest_queue_len){
                longest_queue_len = subqueue_len;
                longest_queue = mulqueue->m_queues[j];
                dequeue_index = j;
            }
        }    
        
    }

    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    if(mulqueue->GetDequeueType() == MultipleQueue<Packet>::DequeueType::PQ){
        
        if(dequeue_index < enqueue_index){  //low can't push high 
            return packet;
        }
    }

    if(longest_queue_len <= staticBuffer){       //the packet in static memory can't be pushed out
        return NULL;
    }

    packet = longest_queue->Dequeue();
    
    if(packet != NULL){
        m_switch->DeleteUsed(packet->GetSize(), dequeue_index);
    }
    return packet;
}

void
SharedMemoryPointToPointNetDevice::ChangeDeqPriorityBytes(Ptr<Packet> packet, uint32_t dequeue_index){
    if(packet == NULL){
        return;
    }
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    uint32_t priority = m_switch->qToP[dequeue_index];
    deqPriorityBytes[priority] += packet->GetSize();
}

void
SharedMemoryPointToPointNetDevice::UpdateDequeueRate(double nanodelay){
    for (uint32_t p=0;p<8;p++){
        double th = 8*deqPriorityBytes[p]/nanodelay*(1000000000)/(m_bps.GetBitRate());
        if (th < 1.0/double(8) || th > 1){
            th = 1;
        }
        deqPriorityRate[p] = th;
        deqPriorityBytes[p] = 0;
    }
}

void
SharedMemoryPointToPointNetDevice::InvokeUpdates(double nanodelay){
    UpdateDequeueRate(nanodelay);
    UpdateNofP();
    Simulator::Schedule(NanoSeconds(nanodelay),&SharedMemoryPointToPointNetDevice::InvokeUpdates,this,nanodelay);
}

double 
SharedMemoryPointToPointNetDevice::GetDequeueRate(uint32_t enqueue_index){
  
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    uint32_t priority = m_switch->qToP[enqueue_index];
    return deqPriorityRate[priority];
}

int
SharedMemoryPointToPointNetDevice::GetQueueThreshold(uint32_t index){
    double threshold = 0;
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    if(m_switch->GetEnqueueMethod() == Switch::NORMAL_ENQUEUE){
        threshold = m_switch->GetQueueThreshold(index);
    }else if(m_switch->GetEnqueueMethod() == Switch::DT){
        threshold = m_switch->GetQueueThreshold(index);
    }else if(m_switch->GetEnqueueMethod() == Switch::MULTILAYER_DT){
        threshold = m_switch->GetQueueThreshold(index);
    }else if(m_switch->GetEnqueueMethod() == Switch::PUSHOUT){
        threshold = m_switch->GetQueueThreshold(index);
    }else if(m_switch->GetEnqueueMethod() == Switch::ABM){
        threshold = m_switch->GetQueueThreshold(index)*GetDequeueRate(index)/GetNP(index);
        if (threshold> UINT32_MAX){
            threshold = UINT32_MAX-1500;
        } 
    }
    return int(threshold);
}

void 
SharedMemoryPointToPointNetDevice::UpdateNofP(){
    // std::cout<<"UpdateNofP"<<std::endl;
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();


    for(int i=0;i<8;i++){
        nofP[i] = m_switch->N[i];         //8 priorityin switch
    }
}

double
SharedMemoryPointToPointNetDevice::GetNP(uint32_t index){
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();


    uint32_t priority = m_switch->qToP[index];
    // std::cout<<nofP[priority]<<std::endl;
    if (nofP[priority]>=1){
        return nofP[priority];
    }else{
        return 1;
    }
}

uint32_t
SharedMemoryPointToPointNetDevice::GetAllStaticBufferUsedSize(){
    uint32_t all = 0;
    for(uint32_t i=0; i<8; i++){
        all += usedStaticBuffer[i];
    }
    // return all; don;t use the func
    return 0;
}

void
SharedMemoryPointToPointNetDevice::InsertMap(uint32_t srcIP, uint32_t dstIP, const uint32_t dstPort, uint32_t queueId)
{
    uint32_t hash_key =  srcIP ^ dstIP ^ dstPort;
    queueIdMap[hash_key] = queueId;
}

uint32_t 
SharedMemoryPointToPointNetDevice::GetQueueId(uint32_t srcIP, uint32_t dstIP, const uint32_t dstPort)
{
    uint32_t hash_key =  srcIP ^ dstIP ^ dstPort;
    uint32_t queueId = 100;                                //this is a invalid subqueue id 
    if (queueIdMap.find(hash_key) != queueIdMap.end())
    {
        queueId = queueIdMap[hash_key];
    }

    return queueId;
}

void
SharedMemoryPointToPointNetDevice::SetThroughputFileFolderName(std::string folderName)
{
    m_throughputFlag = true;
    m_throughputFlagFileName = folderName + GetName() + ".txt";
    std::ofstream fout1(m_throughputFlagFileName,std::ios::trunc);      //clear txt
    fout1.close();

    m_throughputFout.open(m_throughputFlagFileName,std::ios::app);    
}

void
SharedMemoryPointToPointNetDevice::WriteThroughput(double rate)
{
  
    m_throughputFout<<double(Simulator::Now ().GetNanoSeconds ())/1000000000.0<<" "<<rate<<std::endl;

}

void 
SharedMemoryPointToPointNetDevice::SetPtFileFolderName(std::string folderName)
{
    m_ptFlag = true;
    m_ptFileName = folderName + ".txt";
    std::ofstream fout1(m_ptFileName,std::ios::trunc);      //clear txt
    fout1.close();

    m_ptFout.open(m_ptFileName,std::ios::app);  

}

void
SharedMemoryPointToPointNetDevice::WritePt(bool flag)
{   
    if(flag)
    {
        m_ptFout<<"true"<<std::endl;
    }
    
}

void
SharedMemoryPointToPointNetDevice::Update(double nanodelay)
{
    double rate = throughputBytes / throughputUpdateInterval *8;
    WriteThroughput(rate);
    throughputBytes = 0;
    Simulator::Schedule(NanoSeconds(throughputUpdateInterval),&SharedMemoryPointToPointNetDevice::Update,this,nanodelay);
}

void
SharedMemoryPointToPointNetDevice::DropCheck()
{   
    // if(m_dropType == DropType::RR)
    // {
    //     DropRR();
    // }else if(m_dropType == DropType::TOKEN)
    // {
    //     DropTOKEN();
    // }
    //do nothing
}

void
SharedMemoryPointToPointNetDevice::DropRR()
{   
    // std::cout<<"Drop rr"<<std::endl;
    // Ptr<Switch> m_switch = m_node->m_switch;
    Ptr<Switch> m_switch = GetMMUSwitch();

    Ptr<MultipleQueue<Packet> > mulqueue = m_queue->GetObject<MultipleQueue<Packet> >();
    while(1)
    {   
        bool flag = false;
        uint32_t check_index = 0;
        for(uint32_t index = 0; index < mulqueue->m_useqCnt; index++)
        {   
            check_index = (index + last_head_drop_index) % mulqueue->m_useqCnt;
            Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[check_index];
            //don't use static buffer
            if(double(subqueue->GetNBytes() - usedStaticBuffer[check_index]) / double(GetQueueThreshold(check_index)) > 1.0)
            {
                flag = true;
                int64_t the_time = Simulator::Now ().GetNanoSeconds ();
                if(the_time - m_switch->last_head_drop_time < m_switch->head_drop_internal){
                    // can't drop because frequence
                    std::cout<<"can't Drop rr"<<std::endl;
                    return;
                }
                // std::cout<<the_time<<"    "<<last_head_drop_time<<"     " <<the_time - last_head_drop_time<<std::endl;
                Ptr<Packet> packet = mulqueue->Dequeue(check_index);
                if(packet != NULL){
                    m_switch->DeleteUsed(packet->GetSize(), check_index);
                    m_switch->last_head_drop_time = (int64_t)Simulator::Now ().GetNanoSeconds ();
                    // std::cout<<m_switch-> last_head_drop_time<<std::endl;
                }
                std::cout<<"this is drop check"<<std::endl;
                
            }
        }
        if(flag == false)
        {   last_head_drop_index = (check_index + 1) % mulqueue->m_useqCnt;
            break;
        }
    }
}

Ptr<Switch>
SharedMemoryPointToPointNetDevice::GetMMUSwitch()
{   
    
    Ptr<Switch> mmu_switch = nullptr;
    uint32_t port_num =  this->GetIfIndex() - 1;   //0 is stack


    // std::cout<<"p"<<port_num << " "<< this->GetType()<<std::endl;
    if(port_num / 8 > 3)
    {
        std::cout<<"GetMMUSwitch error"<<std::endl;
    }

    if(port_num / 8 >= 3)
    {   
        std::cout<<port_num<<std::endl;
        std::cout<<"where 3? "<<std::endl;
        std::cout<<this->GetType()<<std::endl;
    }
    mmu_switch = m_node->m_switch_group[port_num/8];
    return mmu_switch;
}

void 
SharedMemoryPointToPointNetDevice::ShowMMU()
{   
    std::cout<<"============"<<std::endl;
    for(uint32_t i = 0; i < 4; i++)
    {
        std::cout<<m_node->m_switch_group[i]->GetUsedBuffer()<<std::endl;
    }
}

void
SharedMemoryPointToPointNetDevice::TokenAdd()
{   

    Ptr<Switch> m_switch = GetMMUSwitch();
    if(m_switch->GetEnqueueMethod() != Switch::MULTILAYER_DT){
        return;
    }
    Ptr<Switch> global_switch = m_node->m_switch;
    if(!global_switch->first_flag){
        global_switch->first_flag = true; //only call one time
        m_switch->last_head_drop_port = (this->GetIfIndex() - 1) / 8 * 8 + 1 ; 
        m_switch->last_head_drop_queue = 0;

    }

          
    Time token_add_internal = m_bps.CalculateBytesTxTime (1500) / global_switch->port_num;
    if(token_add_internal < NanoSeconds(1)){
        token_add_internal = NanoSeconds(1);
    }
    global_switch->token_num += 8;
    if(global_switch->token_num > global_switch->max_token_num)
    {
        global_switch->token_num = global_switch->max_token_num;
    }
    
    // std::cout<<"+ token: "<<GetToken()<<"    "<<token_add_internal<<std::endl;
    // std::cout<<Simulator::Now ().GetNanoSeconds ()<<std::endl;
    if(Simulator::Now ().GetNanoSeconds () < 1100000000){
        Simulator::Schedule (token_add_internal, &SharedMemoryPointToPointNetDevice::TokenAdd, this);
    }
    if(global_switch->token_num > 0 && m_dropType == DropType::TOKEN)
    {   
        // std::cout<<"will delete"<<std::endl;
        DropTOKEN();
    }else if(global_switch->token_num > 0 && m_dropType == DropType::LONGTOKEN)
    {   
        // std::cout<<"will delete"<<std::endl;
        DropLONGTOKEN();
    }
    return;
    
}

void
SharedMemoryPointToPointNetDevice::TokenDelete(uint32_t pkt_size)
{   
    Ptr<Switch> m_switch = GetMMUSwitch();
    if(m_switch->GetEnqueueMethod() != Switch::MULTILAYER_DT){
        return;
    }
    Ptr<Switch> global_switch = m_node->m_switch;
    double ceil_num = 1.0 * pkt_size / global_switch->ceil_size;
    global_switch->token_num -= std::ceil(ceil_num);
    
    // std::cout<<"- pktsize: "<<pkt_size<<"   "<<std::ceil(ceil_num)<< " " <<GetToken()<<std::endl;
}

int64_t
SharedMemoryPointToPointNetDevice::GetToken()
{   
    Ptr<Switch> global_switch = m_node->m_switch;
    return global_switch->token_num;
}

void
SharedMemoryPointToPointNetDevice::DropTOKEN()
{   
    Ptr<Switch> m_switch = GetMMUSwitch();
    uint32_t left = (this->GetIfIndex() - 1) / 8 * 8 + 1 ;          //beacuse 3 mmu, head_drop in different mmu
    uint32_t right = (this->GetIfIndex() - 1) / 8 * 8 + 8;

    uint32_t temp_last_head_drop_port = m_switch->last_head_drop_port;
    uint32_t temp_last_head_drop_queue = m_switch->last_head_drop_queue;
    uint32_t max_queue_no = m_queue->GetObject<MultipleQueue<Packet> >()->m_useqCnt;
    
    // std::cout<<"token drop"<<std::endl;
    while(1){
        //RR
        temp_last_head_drop_queue += 1;   //next_queue
        if(temp_last_head_drop_queue >= max_queue_no){
            temp_last_head_drop_queue = 0; //next port
            temp_last_head_drop_port += 1;
            if(temp_last_head_drop_port > right || temp_last_head_drop_port >= m_node->GetNDevices()){
                temp_last_head_drop_port = left;
            }
        }

        if(temp_last_head_drop_port > right || temp_last_head_drop_port >= m_node->GetNDevices()){
            temp_last_head_drop_port = left;
        }

        // std::cout<<temp_last_head_drop_port<<"   "<<temp_last_head_drop_queue<<std::endl;
        Ptr<NetDevice> dev = m_node->GetDevice(temp_last_head_drop_port);
        if(dev->GetType() == 0){
            if(temp_last_head_drop_port == m_switch->last_head_drop_port && temp_last_head_drop_queue == m_switch->last_head_drop_queue){
                //no one need head-drop
                break;
            }
            continue;       //this is a protocol stack dev
        }
        // std::cout<<"3"<<std::endl;
        Ptr<SharedMemoryPointToPointNetDevice> sharedMemoryDev = dev->GetObject<SharedMemoryPointToPointNetDevice>();
        Ptr<MultipleQueue<Packet> > mulqueue = sharedMemoryDev->GetQueue()->GetObject<MultipleQueue<Packet> >();
        // std::cout<<"4"<<std::endl;
        Ptr<Queue<Packet> >subqueue = mulqueue->m_queues[temp_last_head_drop_queue];
        if(double(subqueue->GetNBytes() - usedStaticBuffer[temp_last_head_drop_queue]) / double(GetQueueThreshold(temp_last_head_drop_queue)) > 1.0){
            
            Ptr<Packet> packet = mulqueue->Dequeue(temp_last_head_drop_queue);
            if(packet != NULL){
                m_switch->DeleteUsed(packet->GetSize(), temp_last_head_drop_queue);
                TokenDelete(packet->GetSize());
                //store msg
                m_switch->last_head_drop_port = temp_last_head_drop_port;
                m_switch->last_head_drop_queue = temp_last_head_drop_queue;

                // std::cout<<"head-rr: "<<m_node->GetId()<<" "<<temp_last_head_drop_port<<"-"<<temp_last_head_drop_queue<<std::endl;
                break;
            }
            
        }
        
        if(temp_last_head_drop_port == m_switch->last_head_drop_port && temp_last_head_drop_queue == m_switch->last_head_drop_queue){
            //no one need head-drop
            break;
        }
        
    }
    // std::cout<<"666"<<std::endl;
}

void
SharedMemoryPointToPointNetDevice::DropLONGTOKEN()
{   

    Ptr<Switch> m_switch = GetMMUSwitch();
    uint32_t left = (this->GetIfIndex() - 1) / 8 * 8 + 1 ;          //beacuse 3 mmu, pushout in different mmu
    uint32_t right = (this->GetIfIndex() - 1) / 8 * 8 + 8;

    uint32_t longest_queue_len = 0;
    Ptr<Packet> packet = NULL;
    uint32_t dequeue_index = 0;
    uint32_t max_queue_no = m_queue->GetObject<MultipleQueue<Packet> >()->m_useqCnt;
    uint32_t long_dev_index = 0;

    Ptr<Queue<Packet> >longest_queue = NULL;
    for(uint32_t i = left; i <= right && i < m_node->GetNDevices(); i++){
        Ptr<NetDevice> dev = m_node->GetDevice(i);
        if(dev->GetType() == 0){
            continue;       //this is a protocol stack dev
        }
        Ptr<SharedMemoryPointToPointNetDevice> sharedMemoryDev = dev->GetObject<SharedMemoryPointToPointNetDevice>();
        Ptr<MultipleQueue<Packet> > mulqueue = sharedMemoryDev->GetQueue()->GetObject<MultipleQueue<Packet> >();
        for(uint32_t j=0; j<max_queue_no; j++){
            uint32_t subqueue_len = mulqueue->m_queues[j]->GetNBytes();
            if(subqueue_len >= longest_queue_len){
                longest_queue_len = subqueue_len;
                longest_queue = mulqueue->m_queues[j];
                dequeue_index = j;
                long_dev_index = i;
            }
        }    
    }
    if(longest_queue_len <= staticBuffer){       //the packet in static memory can't be pushed out
        return;
    }
    if(double(longest_queue->GetNBytes() - usedStaticBuffer[dequeue_index]) / double(GetQueueThreshold(dequeue_index)) > 1.0){
        packet = longest_queue->Dequeue();
        if(packet != NULL){
            m_switch->DeleteUsed(packet->GetSize(), dequeue_index);
            // std::cout<<"head-long: "<<m_node->GetId()<<" "<<long_dev_index<<"-"<<dequeue_index<<std::endl;
            TokenDelete(packet->GetSize());    
        }
    }
}

void
SharedMemoryPointToPointNetDevice::WriteQueryLoss()
{  
    if(m_node->m_queryLossFlag != true){
        return;
    }
    m_node->m_queryLossFout<<Simulator::Now ().GetNanoSeconds ()<<" ";

    uint32_t port_num =  this->GetIfIndex() - 1;   //0 is stack
    uint32_t mmu_num = port_num/8;
    m_node->m_queryLossFout<<mmu_num<<" ";

    for(int i=0;i<4;i++){
        double mmurate = 1.0 * m_node->m_switch_group[i]->GetUsedBuffer() / m_node->m_switch_group[i]->GetMaxBuffer();
        m_node->m_queryLossFout<<mmurate<<" ";
    }
    for(int i=0;i<4;i++){
        double mmurate = 1.0 * m_node->m_switch_group[i]->send_rate;
        m_node->m_queryLossFout<<mmurate<<" ";
    }
    m_node->m_queryLossFout<<std::endl;
}



} // namespace ns3
