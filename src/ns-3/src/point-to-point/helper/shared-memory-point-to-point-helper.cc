/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/abort.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/packet.h"
#include "ns3/shared-memory-point-to-point-channel.h"
#include "ns3/shared-memory-point-to-point-net-device.h"
#include "ns3/simulator.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"
#include "ns3/shared-memory-point-to-point-remote-channel.h"
#endif

#include "shared-memory-point-to-point-helper.h"

#include "ns3/trace-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SharedMemoryPointToPointHelper");

SharedMemoryPointToPointHelper::SharedMemoryPointToPointHelper()
{
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_deviceFactory.SetTypeId("ns3::SharedMemoryPointToPointNetDevice");
    m_channelFactory.SetTypeId("ns3::SharedMemoryPointToPointChannel");
    m_enableFlowControl = true;
}

void
SharedMemoryPointToPointHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    m_deviceFactory.Set(n1, v1);
}

void
SharedMemoryPointToPointHelper::SetChannelAttribute(std::string n1, const AttributeValue& v1)
{
    m_channelFactory.Set(n1, v1);
}

void
SharedMemoryPointToPointHelper::DisableFlowControl()
{
    m_enableFlowControl = false;
}

void
SharedMemoryPointToPointHelper::EnablePcapInternal(std::string prefix,
                                       Ptr<NetDevice> nd,
                                       bool promiscuous,
                                       bool explicitFilename)
{
    //
    // All of the Pcap enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system.  We can only deal with devices of type SharedMemoryPointToPointNetDevice.
    //
    Ptr<SharedMemoryPointToPointNetDevice> device = nd->GetObject<SharedMemoryPointToPointNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("SharedMemoryPointToPointHelper::EnablePcapInternal(): Device "
                    << device << " not of type ns3::SharedMemoryPointToPointNetDevice");
        return;
    }

    PcapHelper pcapHelper;

    std::string filename;
    if (explicitFilename)
    {
        filename = prefix;
    }
    else
    {
        filename = pcapHelper.GetFilenameFromDevice(prefix, device);
    }

    Ptr<PcapFileWrapper> file = pcapHelper.CreateFile(filename, std::ios::out, PcapHelper::DLT_PPP);
    pcapHelper.HookDefaultSink<SharedMemoryPointToPointNetDevice>(device, "PromiscSniffer", file);
}

void
SharedMemoryPointToPointHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                                        std::string prefix,
                                        Ptr<NetDevice> nd,
                                        bool explicitFilename)
{
    //
    // All of the ascii enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system.  We can only deal with devices of type SharedMemoryPointToPointNetDevice.
    //
    Ptr<SharedMemoryPointToPointNetDevice> device = nd->GetObject<SharedMemoryPointToPointNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("SharedMemoryPointToPointHelper::EnableAsciiInternal(): Device "
                    << device << " not of type ns3::SharedMemoryPointToPointNetDevice");
        return;
    }

    //
    // Our default trace sinks are going to use packet printing, so we have to
    // make sure that is turned on.
    //
    Packet::EnablePrinting();

    //
    // If we are not provided an OutputStreamWrapper, we are expected to create
    // one using the usual trace filename conventions and do a Hook*WithoutContext
    // since there will be one file per context and therefore the context would
    // be redundant.
    //
    if (!stream)
    {
        //
        // Set up an output stream object to deal with private ofstream copy
        // constructor and lifetime issues.  Let the helper decide the actual
        // name of the file given the prefix.
        //
        AsciiTraceHelper asciiTraceHelper;

        std::string filename;
        if (explicitFilename)
        {
            filename = prefix;
        }
        else
        {
            filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
        }

        Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream(filename);

        //
        // The MacRx trace source provides our "r" event.
        //
        asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<SharedMemoryPointToPointNetDevice>(device,
                                                                                     "MacRx",
                                                                                     theStream);

        //
        // The "+", '-', and 'd' events are driven by trace sources actually in the
        // transmit queue.
        //
        Ptr<Queue<Packet>> queue = device->GetQueue();
        asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(queue,
                                                                             "Enqueue",
                                                                             theStream);
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(queue, "Drop", theStream);
        asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(queue,
                                                                             "Dequeue",
                                                                             theStream);

        // PhyRxDrop trace source for "d" event
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<SharedMemoryPointToPointNetDevice>(device,
                                                                                  "PhyRxDrop",
                                                                                  theStream);

        return;
    }

    //
    // If we are provided an OutputStreamWrapper, we are expected to use it, and
    // to providd a context.  We are free to come up with our own context if we
    // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
    // compatibility and simplicity, we just use Config::Connect and let it deal
    // with the context.
    //
    // Note that we are going to use the default trace sinks provided by the
    // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
    // but the default trace sinks are actually publicly available static
    // functions that are always there waiting for just such a case.
    //
    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    oss << "/NodeList/" << nd->GetNode()->GetId() << "/DeviceList/" << deviceid
        << "/$ns3::SharedMemoryPointToPointNetDevice/MacRx";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::SharedMemoryPointToPointNetDevice/TxQueue/Enqueue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::SharedMemoryPointToPointNetDevice/TxQueue/Dequeue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::SharedMemoryPointToPointNetDevice/TxQueue/Drop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::SharedMemoryPointToPointNetDevice/PhyRxDrop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer
SharedMemoryPointToPointHelper::Install(NodeContainer c)
{
    NS_ASSERT(c.GetN() == 2);
    return Install(c.Get(0), c.Get(1));
}

NetDeviceContainer
SharedMemoryPointToPointHelper::Install(Ptr<Node> a, Ptr<Node> b)
{
    NetDeviceContainer container;

    Ptr<SharedMemoryPointToPointNetDevice> devA = m_deviceFactory.Create<SharedMemoryPointToPointNetDevice>();
    devA->SetAddress(Mac48Address::Allocate());
    a->AddDevice(devA);
    Ptr<Queue<Packet>> queueA = m_queueFactory.Create<Queue<Packet>>();
    devA->SetQueue(queueA);
    Ptr<SharedMemoryPointToPointNetDevice> devB = m_deviceFactory.Create<SharedMemoryPointToPointNetDevice>();
    devB->SetAddress(Mac48Address::Allocate());
    b->AddDevice(devB);
    Ptr<Queue<Packet>> queueB = m_queueFactory.Create<Queue<Packet>>();
    devB->SetQueue(queueB);
    if (m_enableFlowControl)
    {
        // Aggregate NetDeviceQueueInterface objects
        Ptr<NetDeviceQueueInterface> ndqiA = CreateObject<NetDeviceQueueInterface>();
        ndqiA->GetTxQueue(0)->ConnectQueueTraces(queueA);
        devA->AggregateObject(ndqiA);
        Ptr<NetDeviceQueueInterface> ndqiB = CreateObject<NetDeviceQueueInterface>();
        ndqiB->GetTxQueue(0)->ConnectQueueTraces(queueB);
        devB->AggregateObject(ndqiB);
    }

    Ptr<SharedMemoryPointToPointChannel> channel = nullptr;

    // If MPI is enabled, we need to see if both nodes have the same system id
    // (rank), and the rank is the same as this instance.  If both are true,
    // use a normal p2p channel, otherwise use a remote channel
#ifdef NS3_MPI
    bool useNormalChannel = true;
    if (MpiInterface::IsEnabled())
    {
        uint32_t n1SystemId = a->GetSystemId();
        uint32_t n2SystemId = b->GetSystemId();
        uint32_t currSystemId = MpiInterface::GetSystemId();
        if (n1SystemId != currSystemId || n2SystemId != currSystemId)
        {
            useNormalChannel = false;
        }
    }
    if (useNormalChannel)
    {
        m_channelFactory.SetTypeId("ns3::SharedMemoryPointToPointChannel");
        channel = m_channelFactory.Create<SharedMemoryPointToPointChannel>();
    }
    else
    {
        m_channelFactory.SetTypeId("ns3::PointToPointRemoteChannel");
        channel = m_channelFactory.Create<PointToPointRemoteChannel>();
        Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver>();
        Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver>();
        mpiRecA->SetReceiveCallback(MakeCallback(&SharedMemoryPointToPointNetDevice::Receive, devA));
        mpiRecB->SetReceiveCallback(MakeCallback(&SharedMemoryPointToPointNetDevice::Receive, devB));
        devA->AggregateObject(mpiRecA);
        devB->AggregateObject(mpiRecB);
    }
#else
    channel = m_channelFactory.Create<SharedMemoryPointToPointChannel>();
#endif

    devA->Attach(channel);
    devB->Attach(channel);
    container.Add(devA);
    container.Add(devB);

    return container;
}

NetDeviceContainer
SharedMemoryPointToPointHelper::Install(Ptr<Node> a, std::string bName)
{
    Ptr<Node> b = Names::Find<Node>(bName);
    return Install(a, b);
}

NetDeviceContainer
SharedMemoryPointToPointHelper::Install(std::string aName, Ptr<Node> b)
{
    Ptr<Node> a = Names::Find<Node>(aName);
    return Install(a, b);
}

NetDeviceContainer
SharedMemoryPointToPointHelper::Install(std::string aName, std::string bName)
{
    Ptr<Node> a = Names::Find<Node>(aName);
    Ptr<Node> b = Names::Find<Node>(bName);
    return Install(a, b);
}

} // namespace ns3
