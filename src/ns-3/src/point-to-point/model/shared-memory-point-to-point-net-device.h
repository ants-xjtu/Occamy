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

#ifndef SHARED_MEMORY_POINT_TO_POINT_NET_DEVICE_H
#define SHARED_MEMORY_POINT_TO_POINT_NET_DEVICE_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/queue-fwd.h"
#include "ns3/traced-callback.h"

#include <cstring>

namespace ns3
{

class SharedMemoryPointToPointChannel;
class ErrorModel;

/**
 * \defgroup point-to-point Point-To-Point Network Device
 * This section documents the API of the ns-3 point-to-point module. For a
 * functional description, please refer to the ns-3 manual here:
 * http://www.nsnam.org/docs/models/html/point-to-point.html
 *
 * Be sure to read the manual BEFORE going down to the API.
 */

/**
 * \ingroup point-to-point
 * \class SharedMemoryPointToPointNetDevice
 * \brief A Device for a Point to Point Network Link.
 *
 * This SharedMemoryPointToPointNetDevice class specializes the NetDevice abstract
 * base class.  Together with a SharedMemoryPointToPointChannel (and a peer
 * SharedMemoryPointToPointNetDevice), the class models, with some level of
 * abstraction, a generic point-to-point or serial link.
 * Key parameters or objects that can be specified for this device
 * include a queue, data rate, and interframe transmission gap (the
 * propagation delay is set in the SharedMemoryPointToPointChannel).
 */
class SharedMemoryPointToPointNetDevice : public NetDevice
{
  public:
    bool m_writeFlag = false;
    std::string m_writeFileName = "switch.txt";
    std::ofstream m_fileFout;
    int m_writeCount = 0;
    int m_writeFrequence = 1;
    void WriteQueueMsg(int subqueueNo, std::string ecnFlag = "null");
    void SetWriteFileFolderName(std::string folderName);
    Ptr<Packet> PushOutPacket(uint32_t enqueue_index);
    int GetQueueThreshold(uint32_t index);
    bool AcceptPacket(Ptr<Packet> packet,uint32_t index,uint32_t unsched);

    //ABM
    double deqPriorityRate[8];
    double deqPriorityBytes[8];
    bool firstTimeUpdate = true;
    void ChangeDeqPriorityBytes(Ptr<Packet> packet, uint32_t dequeue_index);
    void InvokeUpdates(double nanodelay);
    void UpdateDequeueRate(double nanodelay);
    double GetDequeueRate(uint32_t enqueue_index);

    uint64_t updateInterval;
    uint32_t sat;
    double nofP[8];
    void UpdateNofP();
    double GetNP(uint32_t index);


    //static buffer
    uint64_t staticBuffer;
    uint32_t usedStaticBuffer[8] = {0};
    uint32_t GetAllStaticBufferUsedSize();


    //detail func
    bool m_detailFlag = false;
    std::string m_detailFileName = "detail.txt";
    std::ofstream m_detailFout;
    void SetDetailFileFolderName(std::string folderName);
    void WriteDetail(std::string action, Ptr<Packet> packet, uint32_t index = 0);

    //distinguish NetDevice and SharedMemoryNetDevice
    virtual int GetType(void);

    //Get dev name, eg host-1-1 , switch 2-2
    std::string GetName();

    //send packet 
    bool TransmitStartSwitch (Ptr<Packet> p, uint32_t dequeue_index);
    bool TransmitStartNode (Ptr<Packet> p);

    void TransmitCompleteSwitch (void);
    void TransmitCompleteNode (void);

    virtual bool SendNode (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
    virtual bool SendSwitch (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    //map
    static std::unordered_map<uint32_t, uint32_t> queueIdMap;
    void InsertMap(uint32_t srcIP, uint32_t dstIP, const uint32_t dstPort, uint32_t queueId);
    uint32_t GetQueueId(uint32_t srcIP, uint32_t dstIP, const uint32_t dstPort);

    uint64_t txBytesInt=0;    //the total send bytes of the port

    //ABM unscheduled alpha
    double alphaUnsched = 64;

    //throughput func
    uint32_t throughputBytes = 0;
    // double throughputUpdateInterval = 10000000; //0.01s
    double throughputUpdateInterval = 1000000; //0.001s
    bool m_throughputFlag = false;
    bool throughputFirstFlag = true;
    std::string m_throughputFlagFileName = "throughput.txt";
    std::ofstream m_throughputFout;
    void SetThroughputFileFolderName(std::string folderName);
    void WriteThroughput(double rate);
    void Update(double nanodelay);

    //drop-check
    typedef enum {
        RR,
        NONE,
        TOKEN,
        LONGTOKEN,
    } DropType;
    DropType m_dropType;
    void DropCheck();
    void DropRR();

    //reach priority threshold or not
    bool m_ptFlag = false;
    std::string m_ptFileName = "pt.txt";
    std::ofstream m_ptFout;
    void SetPtFileFolderName(std::string folderName);
    void WritePt(bool flag);
    bool firstTime = false;

    //pbufferplus setting
    uint32_t last_head_drop_index = 0;

    //tomahawk setting

    Ptr<Switch> GetMMUSwitch();
    void ShowMMU();

    //token
    void TokenAdd();
    void TokenDelete(uint32_t pkt_size);
    int64_t GetToken();
    void DropTOKEN();
    void DropLONGTOKEN();

    //PC priority
    typedef enum {
        PCPQ,
        PCNONE,
    } PCType;
    PCType m_pcType;

    //query msg
    void WriteQueryLoss();
    
    /**
     * \brief Get the TypeId
     *
     * \return The TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * Construct a SharedMemoryPointToPointNetDevice
     *
     * This is the constructor for the SharedMemoryPointToPointNetDevice.  It takes as a
     * parameter a pointer to the Node to which this device is connected,
     * as well as an optional DataRate object.
     */
    SharedMemoryPointToPointNetDevice();

    /**
     * Destroy a SharedMemoryPointToPointNetDevice
     *
     * This is the destructor for the SharedMemoryPointToPointNetDevice.
     */
    ~SharedMemoryPointToPointNetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    SharedMemoryPointToPointNetDevice& operator=(const SharedMemoryPointToPointNetDevice&) = delete;
    SharedMemoryPointToPointNetDevice(const SharedMemoryPointToPointNetDevice&) = delete;

    /**
     * Set the Data Rate used for transmission of packets.  The data rate is
     * set in the Attach () method from the corresponding field in the channel
     * to which the device is attached.  It can be overridden using this method.
     *
     * \param bps the data rate at which this object operates
     */
    void SetDataRate(DataRate bps);

    /**
     * Set the interframe gap used to separate packets.  The interframe gap
     * defines the minimum space required between packets sent by this device.
     *
     * \param t the interframe gap time
     */
    void SetInterframeGap(Time t);

    /**
     * Attach the device to a channel.
     *
     * \param ch Ptr to the channel to which this object is being attached.
     * \return true if the operation was successful (always true actually)
     */
    bool Attach(Ptr<SharedMemoryPointToPointChannel> ch);

    /**
     * Attach a queue to the SharedMemoryPointToPointNetDevice.
     *
     * The SharedMemoryPointToPointNetDevice "owns" a queue that implements a queueing
     * method such as DropTailQueue or RedQueue
     *
     * \param queue Ptr to the new queue.
     */
    void SetQueue(Ptr<Queue<Packet>> queue);

    /**
     * Get a copy of the attached Queue.
     *
     * \returns Ptr to the queue.
     */
    Ptr<Queue<Packet>> GetQueue() const;

    /**
     * Attach a receive ErrorModel to the SharedMemoryPointToPointNetDevice.
     *
     * The SharedMemoryPointToPointNetDevice may optionally include an ErrorModel in
     * the packet receive chain.
     *
     * \param em Ptr to the ErrorModel.
     */
    void SetReceiveErrorModel(Ptr<ErrorModel> em);

    /**
     * Receive a packet from a connected SharedMemoryPointToPointChannel.
     *
     * The SharedMemoryPointToPointNetDevice receives packets from its connected channel
     * and forwards them up the protocol stack.  This is the public method
     * used by the channel to indicate that the last bit of a packet has
     * arrived at the device.
     *
     * \param p Ptr to the received packet.
     */
    void Receive(Ptr<Packet> p);

    // The remaining methods are documented in ns3::NetDevice*

    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;

    Ptr<Channel> GetChannel() const override;

    void SetAddress(Address address) override;
    Address GetAddress() const override;

    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;

    bool IsLinkUp() const override;

    void AddLinkChangeCallback(Callback<void> callback) override;

    bool IsBroadcast() const override;
    Address GetBroadcast() const override;

    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;

    bool IsPointToPoint() const override;
    bool IsBridge() const override;

    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;

    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;

    bool NeedsArp() const override;

    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;

    Address GetMulticast(Ipv6Address addr) const override;

    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  protected:
    /**
     * \brief Handler for MPI receive event
     *
     * \param p Packet received
     */
    void DoMpiReceive(Ptr<Packet> p);

  private:
    /**
     * \brief Dispose of the object
     */
    void DoDispose() override;

    /**
     * \returns the address of the remote device connected to this device
     * through the point to point channel.
     */
    Address GetRemote() const;

    /**
     * Adds the necessary headers and trailers to a packet of data in order to
     * respect the protocol implemented by the agent.
     * \param p packet
     * \param protocolNumber protocol number
     */
    void AddHeader(Ptr<Packet> p, uint16_t protocolNumber);

    /**
     * Removes, from a packet of data, all headers and trailers that
     * relate to the protocol implemented by the agent
     * \param p Packet whose headers need to be processed
     * \param param An integer parameter that can be set by the function
     * \return Returns true if the packet should be forwarded up the
     * protocol stack.
     */
    bool ProcessHeader(Ptr<Packet> p, uint16_t& param);

    /**
     * Start Sending a Packet Down the Wire.
     *
     * The TransmitStart method is the method that is used internally in the
     * SharedMemoryPointToPointNetDevice to begin the process of sending a packet out on
     * the channel.  The corresponding method is called on the channel to let
     * it know that the physical device this class represents has virtually
     * started sending signals.  An event is scheduled for the time at which
     * the bits have been completely transmitted.
     *
     * \see SharedMemoryPointToPointChannel::TransmitStart ()
     * \see TransmitComplete()
     * \param p a reference to the packet to send
     * \returns true if success, false on failure
     */
    //bool TransmitStart(Ptr<Packet> p);

    /**
     * Stop Sending a Packet Down the Wire and Begin the Interframe Gap.
     *
     * The TransmitComplete method is used internally to finish the process
     * of sending a packet out on the channel.
     */
    //void TransmitComplete();

    /**
     * \brief Make the link up and running
     *
     * It calls also the linkChange callback.
     */
    void NotifyLinkUp();

    /**
     * Enumeration of the states of the transmit machine of the net device.
     */
    enum TxMachineState
    {
        READY, /**< The transmitter is ready to begin transmission of a packet */
        BUSY   /**< The transmitter is busy transmitting a packet */
    };

    /**
     * The state of the Net Device transmit state machine.
     */
    TxMachineState m_txMachineState;

    /**
     * The data rate that the Net Device uses to simulate packet transmission
     * timing.
     */
    DataRate m_bps;

    /**
     * The interframe gap that the Net Device uses to throttle packet
     * transmission
     */
    Time m_tInterframeGap;

    /**
     * The SharedMemoryPointToPointChannel to which this SharedMemoryPointToPointNetDevice has been
     * attached.
     */
    Ptr<SharedMemoryPointToPointChannel> m_channel;

    /**
     * The Queue which this SharedMemoryPointToPointNetDevice uses as a packet source.
     * Management of this Queue has been delegated to the SharedMemoryPointToPointNetDevice
     * and it has the responsibility for deletion.
     * \see class DropTailQueue
     */
    Ptr<Queue<Packet>> m_queue;

    /**
     * Error model for receive packet events
     */
    Ptr<ErrorModel> m_receiveErrorModel;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, before being queued for transmission.
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * The trace source fired when packets coming into the "top" of the device
     * at the L3/L2 transition are dropped before being queued for transmission.
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace (which doesn't mean a lot here
     * in the point-to-point device).
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non-promiscuous trace (which doesn't mean a lot
     * here in the point-to-point device).
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * but are dropped before being forwarded up to higher layers (at the L2/L3
     * transition).
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    /**
     * The trace source fired when a packet begins the transmission process on
     * the medium.
     */
    TracedCallback<Ptr<const Packet>> m_phyTxBeginTrace;

    /**
     * The trace source fired when a packet ends the transmission process on
     * the medium.
     */
    TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;

    /**
     * The trace source fired when the phy layer drops a packet before it tries
     * to transmit it.
     */
    TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;

    /**
     * The trace source fired when a packet begins the reception process from
     * the medium -- when the simulated first bit(s) arrive.
     */
    TracedCallback<Ptr<const Packet>> m_phyRxBeginTrace;

    /**
     * The trace source fired when a packet ends the reception process from
     * the medium.
     */
    TracedCallback<Ptr<const Packet>> m_phyRxEndTrace;

    /**
     * The trace source fired when the phy layer drops a packet it has received.
     * This happens if the receiver is not enabled or the error model is active
     * and indicates that the packet is corrupt.
     */
    TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;

    /**
     * A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device \c hard_start_xmit where
     * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in \c netif_receive_skb.
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

    /**
     * A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device \c hard_start_xmit where
     * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in \c netif_receive_skb.
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;

    Ptr<Node> m_node;                                    //!< Node owning this NetDevice
    Mac48Address m_address;                              //!< Mac48Address of this NetDevice
    NetDevice::ReceiveCallback m_rxCallback;             //!< Receive callback
    NetDevice::PromiscReceiveCallback m_promiscCallback; //!< Receive callback
                                                         //   (promisc data)
    uint32_t m_ifIndex;                                  //!< Index of the interface
    bool m_linkUp;                                       //!< Identify if the link is up or not
    TracedCallback<> m_linkChangeCallbacks;              //!< Callback for the link change event

    static const uint16_t DEFAULT_MTU = 1500; //!< Default MTU

    /**
     * \brief The Maximum Transmission Unit
     *
     * This corresponds to the maximum
     * number of bytes that can be transmitted as seen from higher layers.
     * This corresponds to the 1500 byte MTU size often seen on IP over
     * Ethernet.
     */
    uint32_t m_mtu;

    Ptr<Packet> m_currentPkt; //!< Current packet processed

    /**
     * \brief PPP to Ethernet protocol number mapping
     * \param protocol A PPP protocol number
     * \return The corresponding Ethernet protocol number
     */
    static uint16_t PppToEther(uint16_t protocol);

    /**
     * \brief Ethernet to PPP protocol number mapping
     * \param protocol An Ethernet protocol number
     * \return The corresponding PPP protocol number
     */
    static uint16_t EtherToPpp(uint16_t protocol);
};

} // namespace ns3

#endif /* POINT_TO_POINT_NET_DEVICE_H */
