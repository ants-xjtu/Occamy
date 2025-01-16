
#include "cdf.h"
#include "cdf.cc"
#include "abm_cdf.cc"
#include "abm_cdf.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "iostream"
#include "vector"
#include "ns3/flow-monitor-helper.h"
#include "ns3/double-binary-tree.h"
#include <cmath>
#include <random>

// # define PACKET_SIZE 1400
# define PACKET_SIZE 1448     //pkt is 1.5K
# define GIGA 1000000000
#define WEB_PORT_START 4444
#define WEB_PORT_END 20000
#define REQUEST_PORT_START 20010
#define REQUEST_PORT_END 65530
using namespace ns3;

double START_TIME = 1;
double FLOW_LAUNCH_START_TIME = 1.01;
double FLOW_LAUNCH_END_TIME = 1.03;
double END_TIME = FLOW_LAUNCH_END_TIME + 0.08;
double webLoad = 0.1;
uint32_t nPrior = 2; // number queues in switch ports
unsigned randomSeed = 8;
uint32_t bufferSize = 4*1024*1024;  //4MB
double requestSizeRate = 1.0;
double requestFlowRate = 200.0;        //possion request flow rate
std::string method = "pBuffer";
double alpha = 8;
std::string tcpProtocol = "DCTCP";
uint32_t rto = 5 * 1000; // in MicroSeconds, 5 milliseconds.

uint32_t SERVER_COUNT = 16;
uint32_t SPINE_COUNT = 8;
uint32_t LEAF_COUNT = 8;
uint32_t LINK_COUNT = 1;

uint64_t spineLeafCapacity = 100; //Gbps
uint64_t leafServerCapacity = 100; //Gbps
double linkLatency = 10;
double spineLeafLatency = 2;
double leafServerLatency = 18;

std::vector<uint32_t> WEB_PORT(LEAF_COUNT*SERVER_COUNT, WEB_PORT_START);
std::vector<uint32_t> REQUEST_PORT(LEAF_COUNT*SERVER_COUNT, REQUEST_PORT_START);

Ipv4InterfaceContainer serverNics[8][16];
NodeContainer servers[8];

//create double binary tree
ns3::DoubleBinaryTree dbt[5];
uint32_t root = 127;

template<typename T>
T rand_range (T min, T max)
{
	return min + ((double)max - min) * rand () / RAND_MAX;
}

double poission_gen_interval(double avg_rate)
{
	if (avg_rate > 0)
		return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
	else
		return 0;
}

//set ......
uint32_t reduceEndNum[5] = {0};
uint32_t reduceFlowNum[5] = {0};
uint32_t broadFlowNum[5] = {0};
uint32_t broadEndNum[5] = {0};
uint32_t allreduceNum = 3;
uint32_t allreduceEndNum = 0;
uint64_t allreduceSize = 3 * 1024 * 1024;

uint32_t endFlowNum = 0;

void reduce(uint32_t allreduce_id);
void broadcast(uint32_t allreduce_id);
void allreduce(bool isReduce, uint32_t allreduceId);

void startflow(uint32_t node_id, uint32_t node_id_s, uint32_t prior, uint64_t flow_size, double start_time, bool isReduce, uint32_t allreduceId)
{
    uint16_t port = WEB_PORT[node_id_s]++;
    if (port > WEB_PORT_END)
    {
        port = WEB_PORT_START;
        WEB_PORT[node_id_s] = WEB_PORT_START;
    }
    uint32_t rxLeaf = node_id_s / SERVER_COUNT;
    uint32_t rxServer = node_id_s % SERVER_COUNT;
    uint32_t txLeaf = node_id / SERVER_COUNT;
    uint32_t txServer = node_id % SERVER_COUNT;
    InetSocketAddress ad (serverNics[rxLeaf][rxServer].GetAddress (0), port);
    Address sinkAddress(ad);
    Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
    bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));           
    bulksend->SetAttribute ("SendSize", UintegerValue (flow_size));
    bulksend->SetAttribute ("MaxBytes", UintegerValue(flow_size));
    bulksend->SetAttribute("priorityCustom", UintegerValue(prior));
    bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
    bulksend->SetAttribute("InitialCwnd", UintegerValue (120));    //thsi is bdp
    bulksend->SetAttribute("priority", UintegerValue(prior));
    bulksend->SetStartTime (Seconds(start_time));
    bulksend->SetStopTime (Seconds (END_TIME));
    servers[txLeaf].Get (txServer)->AddApplication(bulksend);
    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApp = sink.Install (servers[rxLeaf].Get(rxServer));
    sinkApp.Get(0)->SetAttribute("TotalQueryBytes", UintegerValue(flow_size));
    sinkApp.Get(0)->SetAttribute("priority", UintegerValue(0)); // ack packets are prioritized
    sinkApp.Get(0)->SetAttribute("priorityCustom", UintegerValue(0)); // ack packets are prioritized
    sinkApp.Get(0)->SetAttribute("senderPriority", UintegerValue(prior));
    sinkApp.Get(0)->SetAttribute("allreducePattern", ns3::BooleanValue(isReduce));
    sinkApp.Get(0)->SetAttribute("allreduceId", UintegerValue(allreduceId));
    Ptr<PacketSink> sink_ptr = DynamicCast<PacketSink>(sinkApp.Get(0));
    sink_ptr->SetFlowEndCallback(MakeCallback(&allreduce));
    sinkApp.Start (Seconds(start_time));
    sinkApp.Stop (Seconds (END_TIME));
}

void allreduce(bool isReduce, uint32_t allreduceId)
{   
    endFlowNum++;
    //std::cout << reduceEndNum << " " << broadEndNum  <<std::endl;
    if(isReduce)
    {
        reduceEndNum[allreduceId] ++;
        if(reduceEndNum[allreduceId] >= reduceFlowNum[allreduceId])
        {
            std::cout << Simulator::Now().GetNanoSeconds() << std::endl;
            broadcast(allreduceId);
        }
    }
    else
    {
        broadEndNum[allreduceId] ++;
        //std::cout << broadEndNum[allreduceId] << std::endl;
        if(broadEndNum[allreduceId] >= broadFlowNum[allreduceId])
        {
            allreduceEndNum ++;
            std::cout << Simulator::Now().GetNanoSeconds() << std::endl;
            if(Simulator::Now() < Seconds(FLOW_LAUNCH_END_TIME)) reduce(allreduceId);
        }
    }
    // if(reduceEndNum >= reduceFlowNum && broadEndNum >= broadFlowNum && Simulator::Now() < Seconds(FLOW_LAUNCH_END_TIME))
    // {
    //     std::cout << Simulator::Now().GetNanoSeconds() << std::endl;
    //     reduce();
    //     broadcast();
    // }
}

void broadcast(uint32_t allreduceId)
{
    for (uint32_t txLeaf = 0; txLeaf < LEAF_COUNT; txLeaf++)
    {   
        for (uint32_t txServer = 0; txServer < SERVER_COUNT; txServer++)
        {   
            uint32_t prior = rand_range(2, nPrior);
            if(nPrior == 2)
            {
                prior = 1;  //same queue
            }
            uint32_t node_id = txLeaf * SERVER_COUNT + txServer;
            //set ......
            double start_time = 0;
            //if(broadEndNum[allreduceId] == 0) start_time = FLOW_LAUNCH_START_TIME;

            //set ......
            uint64_t flow_size = allreduceSize;
            if(dbt[allreduceId].HasS0(node_id))
            {
                uint32_t node_id_s0 = dbt[allreduceId].GetNodeIdS0(node_id);
                //std::cout << node_id_s0 << " " << dbt.m_left_tree0[node_id] << " " << dbt.m_left_tree1[node_id] << std::endl;
                startflow(node_id, node_id_s0, prior, flow_size, start_time, false, allreduceId);
                broadFlowNum[allreduceId] ++;
            }
            if(dbt[allreduceId].HasS1(node_id))
            {
                uint32_t node_id_s1 = dbt[allreduceId].GetNodeIdS1(node_id);
                //std::cout << node_id_s1 << " " << dbt.m_right_tree0[node_id] << " " << dbt.m_right_tree1[node_id] << std::endl;
                startflow(node_id, node_id_s1, prior, flow_size, start_time, false, allreduceId);
                broadFlowNum[allreduceId] ++;
            }
        }
    }
    std::cout<<"broadcast flow: " << allreduceId << ": "<<broadFlowNum[allreduceId] <<std::endl;
    std::cout<<"==================================="<<std::endl;
}

void reduce(uint32_t allreduceId)
{
    root = 127;
    dbt[allreduceId].SetRoot(root);
    dbt[allreduceId].BuildTree();
    std::cout << root << std::endl;
    for (uint32_t txLeaf = 0; txLeaf < LEAF_COUNT; txLeaf++)
    {   
        for (uint32_t txServer = 0; txServer < SERVER_COUNT; txServer++)
	    {   
            uint32_t prior = rand_range(2, nPrior);
            if(nPrior == 2)
            {
                prior = 1;  //same queue
            }
            uint32_t node_id = txLeaf * SERVER_COUNT + txServer;
            //set ......
            double start_time = 0;
            //if(reduceEndNum[allreduceId] == 0) start_time = FLOW_LAUNCH_START_TIME;
            if(reduceEndNum[allreduceId] == 0) start_time = START_TIME;

            //set ......
            uint64_t flow_size = allreduceSize;
            //std::cout << flow_size << std::endl;
            if(node_id != root)
            {
                if(dbt[allreduceId].HasP0(node_id))
                {
                    uint32_t node_id_p0 = dbt[allreduceId].GetNodeIdP0(node_id);
                    //std::cout << node_id_p0 << " " << dbt.m_parent0[node_id] << std::endl;
                    startflow(node_id, node_id_p0, prior, flow_size, start_time, true, allreduceId);
                    reduceFlowNum[allreduceId] ++;
                }
                if(dbt[allreduceId].HasP1(node_id))
                {
                    uint32_t node_id_p1 = dbt[allreduceId].GetNodeIdP1(node_id);
                    //std::cout << node_id_p1 << " " << dbt.m_parent1[node_id]<< std::endl;
                    startflow(node_id, node_id_p1, prior, flow_size, start_time, true, allreduceId);
                    reduceFlowNum[allreduceId] ++;
                }
            }
        }
    }
    std::cout<<"reduce flow: " << allreduceId << ": "<<reduceFlowNum[allreduceId] <<std::endl;
    std::cout<<"==================================="<<std::endl;
}

int
main (int argc, char *argv[])
{   
    dbt[0].SetNodeNum(LEAF_COUNT * SERVER_COUNT - 1);
    dbt[1].SetNodeNum(LEAF_COUNT * SERVER_COUNT - 1);
    dbt[2].SetNodeNum(LEAF_COUNT * SERVER_COUNT - 1);
    dbt[3].SetNodeNum(LEAF_COUNT * SERVER_COUNT - 1);
    dbt[4].SetNodeNum(LEAF_COUNT * SERVER_COUNT - 1);
    //set use cmd ....

    // dbt.SetRoot(root);
    // dbt.BuildTree();
    
    Time::SetResolution (Time::NS);
    CommandLine cmd (__FILE__);

	cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
	cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
	cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);

    
    cmd.AddValue ("webLoad", "Load of the network, 0.0 - 1.0", webLoad);

   
	cmd.AddValue ("nPrior", "number of priorities", nPrior);

    
	cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("bufferSize", "the buffer size of the shared memory switch", bufferSize);
	cmd.AddValue ("requestSizeRate", "the rate is equal with requestSize / bufferSize", requestSizeRate);
    cmd.AddValue ("requestFlowRate", "possion flow rate of request query", requestFlowRate);
    cmd.AddValue("method", "the buffer management", method);
    cmd.AddValue("alpha", "the alpha of queue", alpha);

    
    cmd.AddValue("tcpProtocol", "the tcp protocol", tcpProtocol);

    
	cmd.AddValue ("rto", "min Retransmission timeout value in MicroSeconds", rto);



    //create topo============================================
   

    
    cmd.AddValue ("serverCount", "The Server count", SERVER_COUNT);
	cmd.AddValue ("spineCount", "The Spine count", SPINE_COUNT);
	cmd.AddValue ("leafCount", "The Leaf count", LEAF_COUNT);
    cmd.AddValue ("linkCount", "The Link count", LINK_COUNT);
    cmd.AddValue ("spineLeafCapacity", "Spine <-> Leaf capacity in Gbps", spineLeafCapacity);
	cmd.AddValue ("leafServerCapacity", "Leaf <-> Server capacity in Gbps", leafServerCapacity);
	cmd.AddValue ("linkLatency", "linkLatency in microseconds", linkLatency);
    cmd.Parse (argc, argv);

    uint32_t requestSize = requestSizeRate * bufferSize;
    
    //set_allreduce size
    if(webLoad == 0.1)
    {
        allreduceSize = 16 * 1024;
    }
    else if(webLoad == 0.2)
    {
        allreduceSize = 32 * 1024;
    }
    else if(webLoad == 0.3)
    {
        allreduceSize = 64 * 1024;
    }
    else if(webLoad == 0.4)
    {
        allreduceSize = 128 * 1024;
    }
    else if(webLoad == 0.5)
    {
        allreduceSize = 256 * 1024;
    }
    else if(webLoad == 0.6)
    {
        allreduceSize = 512 * 1024;
    }
    else if(webLoad == 0.65)
    {
        allreduceSize = 786 * 1024;
    }
    else if(webLoad == 0.7)
    {
        allreduceSize = 1024 * 1024;
    }
    else if(webLoad == 0.72)
    {
        allreduceSize = 1280 * 1024;
    }
    else if(webLoad == 0.74)
    {
        allreduceSize = 1536 * 1024;
    }
    else if(webLoad == 0.76)
    {
        allreduceSize = 1792 * 1024;
    }
    else if(webLoad == 0.8)
    {
        allreduceSize = 2 * 1024 * 1024;
    }
    else if(webLoad == 0.9)
    {
        allreduceSize = 2304 * 1024;
    }
    else if(webLoad == 1.0)
    {
        allreduceSize = 2560 * 1024;
    }
    else if(webLoad == 1.1)
    {
        allreduceSize = 2816 * 1024;
    }
    else if(webLoad == 1.2)
    {
        allreduceSize = 3072 * 1024;
    }
    else if(webLoad == 1.3)
    {
        allreduceSize = 4 * 1024 * 1024;
    }

    std::string keyName = method + "-";
    if(alpha==10 || alpha==0.25 || alpha==16)
    {
        keyName = keyName + std::to_string(alpha).substr(0,4) + "~";
    }
    else if(alpha==0.125)
    {
        keyName = keyName + std::to_string(alpha).substr(0,5) + "~";
    }
    else
    {
        keyName = keyName + std::to_string(alpha).substr(0,3) + "~";
    }

    keyName = keyName + tcpProtocol + "-" + \
    std::to_string(webLoad).substr(0,4) + "-" + \
    std::to_string(requestSizeRate).substr(0,3) + "-" + \
    std::to_string(requestFlowRate).substr(0,5) + "-" + \
    std::to_string(bufferSize) + "-" + \
    std::to_string(nPrior);


    std::cout<<keyName<<std::endl;

    uint64_t SPINE_LEAF_CAPACITY = spineLeafCapacity * GIGA;
	uint64_t LEAF_SERVER_CAPACITY = leafServerCapacity * GIGA;
	Time LINK_LATENCY = MicroSeconds(linkLatency);
    Time SPINE_LEAF_LINK_LATENCY = MicroSeconds(spineLeafLatency);
    Time LEAF_SERVER_LINK_LATENCY = MicroSeconds(leafServerLatency);
    uint32_t RTTBytes = 8 * LEAF_SERVER_CAPACITY / 8  * linkLatency * 1e-6 ; // 8 links visited in roundtrip according to the topology, divided by 8 for bytes        

    Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (10))); // syn retry interval
	Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MicroSeconds (rto)) );  //(MilliSeconds (5))
	Config::SetDefault ("ns3::TcpSocketBase::RTTBytes", UintegerValue ( RTTBytes ));  //(MilliSeconds (5))
	Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (NanoSeconds (10))); //(MicroSeconds (100))
	Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (200))); //TimeValue (MicroSeconds (80))
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1073725440)); //1073725440
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1073725440));
	Config::SetDefault ("ns3::TcpSocket::ConnCount", UintegerValue (6));  // Syn retry count
	Config::SetDefault ("ns3::TcpSocketBase::Timestamp", BooleanValue (true));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (PACKET_SIZE));
	Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
	Config::SetDefault ("ns3::TcpSocket::PersistTimeout", TimeValue (Seconds (20)));

    Config::SetDefault ("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));

    Config::SetDefault ("ns3::SharedMemoryPointToPointNetDevice::UpdataInterval", UintegerValue (1*8*10*1000));
    Config::SetDefault ("ns3::MultipleQueue<Packet>::UseqCnt", UintegerValue(nPrior));
    Config::SetDefault ("ns3::MultipleQueue<Packet>::DequeueType", EnumValue (MultipleQueue<Packet>::DequeueType::DRR));


    //set TCP
    if(tcpProtocol == "DCTCP")
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpDctcp::GetTypeId()));
        Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
    }
    else if(tcpProtocol == "CUBIC")
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpCubic::GetTypeId()));
    }
    else if(tcpProtocol == "RENO")
    {   
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpLinuxReno::GetTypeId()));
        // Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpDctcp::GetTypeId()));
        // Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("Off"));
    }
    else if(tcpProtocol == "NEWRENO")
    {   
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpNewReno::GetTypeId()));
    }
    else if(tcpProtocol == "HPCC")
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpAdvanced::GetTypeId()));
        Config::SetDefault("ns3::TcpSocketState::initCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::minCCRate", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::maxCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::AI", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
		Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::useHpcc", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
		Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency * 4 * 2 + 2 * double(PACKET_SIZE * 8) / (LEAF_SERVER_CAPACITY))));
    }
    else if(tcpProtocol == "POWERTCP")
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpAdvanced::GetTypeId()));
        Config::SetDefault("ns3::TcpSocketState::initCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::minCCRate", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::maxCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::AI", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
		Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::usePowerTcp", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
		Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency * 4 * 2 + 2 * double(PACKET_SIZE * 8) / (LEAF_SERVER_CAPACITY))));
    }
    else if(tcpProtocol == "THETAPOWERTCP")
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpAdvanced::GetTypeId()));
        Config::SetDefault("ns3::TcpSocketState::initCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::minCCRate", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::maxCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::AI", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::mThreshHpcc", UintegerValue(5));
		Config::SetDefault("ns3::TcpSocketState::fastReactHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::sampleFeedbackHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::useThetaPowerTcp", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::multipleRateHpcc", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::targetUtil", DoubleValue(0.95));
		Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency * 4 * 2 + 2 * double(PACKET_SIZE * 8) / (LEAF_SERVER_CAPACITY))));
    }
    else if(tcpProtocol == "TIMELY")
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (ns3::TcpAdvanced::GetTypeId()));
		// Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
		Config::SetDefault("ns3::TcpSocketState::initCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::minCCRate", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::maxCCRate", DataRateValue(DataRate(LEAF_SERVER_CAPACITY)));
		Config::SetDefault("ns3::TcpSocketState::AI", DataRateValue(DataRate("100Mbps")));
		Config::SetDefault("ns3::TcpSocketState::HighAI", DataRateValue(DataRate("150Mbps")));
		Config::SetDefault("ns3::TcpSocketState::useTimely", BooleanValue(true));
		Config::SetDefault("ns3::TcpSocketState::baseRtt", TimeValue(MicroSeconds(linkLatency * 4 * 2 + 2 * double(PACKET_SIZE * 8) / (LEAF_SERVER_CAPACITY))));
    }
    else
    {
        std::cout << "Error in CC configuration" << std::endl;
		return 0;
    }

    NodeContainer spines;
	spines.Create (SPINE_COUNT);
	NodeContainer leaves;
	leaves.Create (LEAF_COUNT);
    for (uint32_t i = 0; i < LEAF_COUNT; i++) 
    {
        servers[i].Create (SERVER_COUNT);
    }

    //switch setting
    double qAlpha[8] = {1, 1, 1, 1, 1 ,1, 1, 1};
    for(int i=0;i<8;i++)
    {
        qAlpha[i] = alpha;
    }
    double pAlpha[8] = {10000, 10000, 10000, 10000, 10000 ,10000, 10000, 10000};
    // double pAlpha[8] = {128, 128, 128, 128, 128 ,128, 128, 128};
    int qToP[8] = {0,1,2,3,4,5,6,7};

    Switch::EnqueueMethod enqueueType = Switch::DT;
    Switch::DequeueMethod dequeueType = Switch::NORMAL_DEQUEUE;
    if(method=="DT")
    {
        enqueueType = Switch::DT;
        dequeueType = Switch::NORMAL_DEQUEUE;
    }else if(method=="pBuffer")
    {
        enqueueType = Switch::MULTILAYER_DT;
        dequeueType = Switch::NORMAL_DEQUEUE;
        Config::SetDefault ("ns3::SharedMemoryPointToPointNetDevice::DropType", StringValue("TOKEN"));
    }else if(method=="ABM")
    {
        enqueueType = Switch::ABM;
        dequeueType = Switch::NORMAL_DEQUEUE;
    }else if(method=="PUSHOUT"){
        enqueueType = Switch::PUSHOUT;
        dequeueType = Switch::NORMAL_DEQUEUE;
    }else if(method=="Long-pBuffer")
    {
        enqueueType = Switch::MULTILAYER_DT;
        dequeueType = Switch::NORMAL_DEQUEUE;
        Config::SetDefault ("ns3::SharedMemoryPointToPointNetDevice::DropType", StringValue("LONGTOKEN"));
    }

    
    //reno or other protocol don't change ecn
    int ecn_threshold = 2*bufferSize;
    if(tcpProtocol == "DCTCP"){
        ecn_threshold = 720 * 1024;
        // ecn_threshold = 65 * PACKET_SIZE;
    }

    for(uint32_t i=0;i<SPINE_COUNT;i++)
    {
        spines.Get(i)->SetNodeType(1);
        spines.Get(i)->m_switch->SetBuffer(bufferSize,0);
        spines.Get(i)->m_switch->SetEcnThreshold(ecn_threshold);
        spines.Get(i)->m_switch->SetPriorityAlpha(pAlpha);
        spines.Get(i)->m_switch->SetQueueAlpha(qAlpha);
        spines.Get(i)->m_switch->SetQueueToPriority(qToP);
        spines.Get(i)->m_switch->SetEnqueueMethod(enqueueType);
        spines.Get(i)->m_switch->SetDequeueMethod(dequeueType);
        spines.Get(i)->m_switch->head_drop_internal = 20;

        spines.Get(i)->m_switch->port_num = 8;

        for(uint32_t j = 0; j < 4; j++)
        {
            spines.Get(i)->m_switch_group[j]->SetBuffer(bufferSize,0);
            spines.Get(i)->m_switch_group[j]->SetEcnThreshold(ecn_threshold);
            spines.Get(i)->m_switch_group[j]->SetPriorityAlpha(pAlpha);
            spines.Get(i)->m_switch_group[j]->SetQueueAlpha(qAlpha);
            spines.Get(i)->m_switch_group[j]->SetQueueToPriority(qToP);
            spines.Get(i)->m_switch_group[j]->SetEnqueueMethod(enqueueType);
            spines.Get(i)->m_switch_group[j]->SetDequeueMethod(dequeueType);
            spines.Get(i)->m_switch_group[j]->head_drop_internal = 20;
        }
        
    }

    for(uint32_t i=0;i<LEAF_COUNT ;i++)
    {
        leaves.Get(i)->SetNodeType(1);
        leaves.Get(i)->m_switch->SetBuffer(bufferSize,0);
        leaves.Get(i)->m_switch->SetEcnThreshold(ecn_threshold);
        leaves.Get(i)->m_switch->SetPriorityAlpha(pAlpha);
        leaves.Get(i)->m_switch->SetQueueAlpha(qAlpha);
        leaves.Get(i)->m_switch->SetQueueToPriority(qToP);
        leaves.Get(i)->m_switch->SetEnqueueMethod(enqueueType);
        leaves.Get(i)->m_switch->SetDequeueMethod(dequeueType);
        leaves.Get(i)->m_switch->head_drop_internal = 20;

        leaves.Get(i)->m_switch->port_num = 24;

        for(uint32_t j = 0; j < 4; j++)
        {
            leaves.Get(i)->m_switch_group[j]->SetBuffer(bufferSize,0);
            leaves.Get(i)->m_switch_group[j]->SetEcnThreshold(ecn_threshold);
            leaves.Get(i)->m_switch_group[j]->SetPriorityAlpha(pAlpha);
            leaves.Get(i)->m_switch_group[j]->SetQueueAlpha(qAlpha);
            leaves.Get(i)->m_switch_group[j]->SetQueueToPriority(qToP);
            leaves.Get(i)->m_switch_group[j]->SetEnqueueMethod(enqueueType);
            leaves.Get(i)->m_switch_group[j]->SetDequeueMethod(dequeueType);
            leaves.Get(i)->m_switch_group[j]->head_drop_internal = 20;
        }
    }

    
    //install protocol stack
    InternetStackHelper internet;
	Ipv4GlobalRoutingHelper globalRoutingHelper;
	internet.SetRoutingHelper (globalRoutingHelper);

	internet.Install(spines);
	internet.Install(leaves);
	for (uint32_t i = 0; i < LEAF_COUNT; i++) 
    {
		internet.Install(servers[i]);
	}


    //create the link and give device ip address

    SharedMemoryPointToPointHelper p2p;
	Ipv4AddressHelper ipv4;
	


    ipv4.SetBase ("10.1.0.0", "255.255.252.0");
        

    //leaf-server
    p2p.SetQueue("ns3::MultipleQueue");
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
	p2p.SetChannelAttribute ("Delay", TimeValue(LEAF_SERVER_LINK_LATENCY));

    for (uint32_t leaf = 0; leaf < LEAF_COUNT; leaf++) 
    {
        ipv4.NewNetwork ();
        for (uint32_t server = 0; server < SERVER_COUNT; server++) 
        {
            NodeContainer nodeContainer = NodeContainer (leaves.Get (leaf), servers[leaf].Get (server));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
            serverNics[leaf][server] = ipv4.Assign(netDeviceContainer.Get(1));
            ipv4.Assign(netDeviceContainer.Get(0));

        }
    }

    //leaf-spine
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
	p2p.SetChannelAttribute ("Delay", TimeValue(SPINE_LEAF_LINK_LATENCY));

    for (uint32_t leaf = 0; leaf < LEAF_COUNT; leaf++) 
    {
        for (uint32_t spine = 0; spine < SPINE_COUNT; spine++) 
        {
            for (uint32_t link = 0; link < LINK_COUNT; link++)
            {
                ipv4.NewNetwork ();
                NodeContainer nodeContainer = NodeContainer (leaves.Get (leaf), spines.Get (spine));
                NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
                Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);


            }
        }
    }


    //create map
    Ptr<SharedMemoryPointToPointNetDevice> emptySharedMemoryPointToPointNetDevice = nullptr;

    if (randomSeed == 0)
	{
		srand ((unsigned)time (NULL));
	}
	else
	{
		srand (randomSeed);
	}
    // uint32_t flowCount = 0;
    // uint32_t flow_num = 0;
    // std::cout<<"reduce flow: "<<reduceFlowNum<<std::endl;
    // std::cout<<"==================================="<<std::endl;
    reduce(uint32_t(1));
    reduce(uint32_t(2));
    // reduce(uint32_t(3));
    // reduce(uint32_t(4));

    uint32_t burst_num = 0;
    int fan = SERVER_COUNT;   
    uint32_t requestFlowSize = double(requestSize) / double(fan);
    for (uint32_t incastLeaf=0; incastLeaf < LEAF_COUNT; incastLeaf++)
    {   
        
        for (uint32_t incastServer = 0; incastServer < SERVER_COUNT; incastServer++)
        {
            double startTime = START_TIME + poission_gen_interval (requestFlowRate);
            uint32_t flowSize = requestFlowSize;
            uint32_t prior = rand_range(1, 1);
            while (startTime < FLOW_LAUNCH_END_TIME && startTime > START_TIME)
            {
                std::vector< std::vector<uint32_t> > burstSenderServer (LEAF_COUNT, std::vector<uint32_t>(SERVER_COUNT, 0));
                for(uint32_t count = 0; count < SERVER_COUNT; count++)
                {
                    uint32_t txLeaf = rand() % LEAF_COUNT;
                    uint32_t txServer = rand() % SERVER_COUNT;
                    while(txLeaf==incastLeaf || burstSenderServer[txLeaf][txServer] == 1)
                    {
                        txLeaf = rand() % LEAF_COUNT;
                        txServer = rand() % SERVER_COUNT;
                    }
                    burstSenderServer[txLeaf][txServer] = 1;
                    uint16_t port = REQUEST_PORT[incastLeaf * SERVER_COUNT + incastServer]++;
                    if (port > REQUEST_PORT_END)
                    {
                        port = REQUEST_PORT_START;
                        REQUEST_PORT[incastLeaf * SERVER_COUNT + incastServer] = REQUEST_PORT_START;
                    }
                    InetSocketAddress ad (serverNics[incastLeaf][incastServer].GetAddress (0), port);
                    Address sinkAddress(ad);
                    Ptr<BulkSendApplication> bulksend = CreateObject<BulkSendApplication>();
                    bulksend->SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
                    bulksend->SetAttribute ("SendSize", UintegerValue (flowSize));
                    bulksend->SetAttribute ("MaxBytes", UintegerValue(flowSize));
                    bulksend->SetAttribute("priorityCustom", UintegerValue(prior));
                    bulksend->SetAttribute("Remote", AddressValue(sinkAddress));
                    bulksend->SetAttribute("InitialCwnd", UintegerValue (flowSize / PACKET_SIZE + 1));
                    bulksend->SetAttribute("priority", UintegerValue(prior));
                    bulksend->SetStartTime (Seconds (startTime));
                    bulksend->SetStopTime (Seconds (END_TIME));
                    servers[txLeaf].Get (txServer)->AddApplication(bulksend);

                    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
                    ApplicationContainer sinkApp = sink.Install (servers[incastLeaf].Get(incastServer));
                    sinkApp.Get(0)->SetAttribute("TotalQueryBytes", UintegerValue(flowSize));
                    sinkApp.Get(0)->SetAttribute("priority", UintegerValue(0)); // ack packets are prioritized
                    sinkApp.Get(0)->SetAttribute("priorityCustom", UintegerValue(0)); // ack packets are prioritized
                    sinkApp.Get(0)->SetAttribute("senderPriority", UintegerValue(prior));
                    // sinkApp.Start (Seconds (startTime));
                    sinkApp.Start (Seconds (START_TIME));
                    sinkApp.Stop (Seconds (END_TIME));

                    uint32_t src_ip = serverNics[txLeaf][txServer].GetAddress (0).Get();
                    uint32_t dst_ip = serverNics[incastLeaf][incastServer].GetAddress (0).Get();
                    uint32_t dst_port = port;
                    emptySharedMemoryPointToPointNetDevice->InsertMap(src_ip,dst_ip,dst_port,prior);
                }
                startTime += poission_gen_interval (requestFlowRate);
                burst_num++;
            }
        }
    }

    std::cout<<"burst: "<<burst_num<<std::endl;

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    Simulator::Stop (Seconds (END_TIME+10));
    Simulator::Run ();
    

    std::string resultFloder = "../../exp/sigcomm/100g_allreduce/";
    std::string xmlName = resultFloder + keyName + ".xml";
    flowMonitor->SerializeToXmlFile(xmlName, true, true);
    std::cout<<"finish"<<std::endl;
    Simulator::Destroy ();
    return 0;
}
