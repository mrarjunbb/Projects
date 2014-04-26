#include "ApplicationUtil.h"
NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

class PacketHolder {
public:
    PacketHolder(Ptr<Packet> _packet) {
        this->packet = _packet;
    }
    void SendCallback(Ptr<Socket> socket, uint32_t val) {
        std::cout<<"send callback"<<std::endl;
        socket->Send(this->packet);
        //socket->Close();
    }
    Ptr<Packet> packet;

};


void DCNET(int numRounds);
std::string hexStr(byte *data, int len)
{
    std::stringstream ss;
    ss<<std::hex;
    for(int i(0); i<len; ++i)
        ss<<(int)data[i];
    return ss.str();
}

int randomBitGeneratorWithProb(double p)
{
    double rndDouble = (double)rand() / RAND_MAX;
    return rndDouble > p;
}

bool AcceptConnectionRequest(Ptr<Socket> socket,const Address& address) {
    return true;
}


void ReceivePublicKey (Ptr<Socket> socket)
{
    int available = socket->GetRxAvailable ();
    std::cout<<"available="<<available<<std::endl;
    std::cout<<"Debug : Inside dcnet receive public key \n";
    Ptr<Node> recvnode = socket->GetNode();
    //int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);

    Ptr<Packet> recPacket = socket->Recv();
    stage1RecvPacketCount +=1; //increment received packet count for stage 1

available = socket->GetRxAvailable ();
std::cout<<"available="<<available<<std::endl;

/*
    uint8_t *buffer = new uint8_t[recPacket->GetSize()+1];
    recPacket->CopyData(buffer,recPacket->GetSize());

    SecByteBlock pubKey((byte *)buffer,recPacket->GetSize());

    MyTag recTag;
    recPacket->PeekPacketTag(recTag);
    int tagVal =int(recTag.GetSimpleValue());
    std::ostringstream s;
    s<<tagVal;
    std::string ss(s.str());
    int srcNodeIndex = atoi(ss.c_str());
    std::string recvData = hexStr(pubKey.BytePtr(),pubKey.SizeInBytes());

    DH dh;
    dh.AccessGroupParameters().Initialize(p, q, g);
    SecByteBlock sharedKey(ApplicationUtil::getInstance()->getDhAgreedLength());

    dh.Agree(sharedKey, ApplicationUtil::getInstance()->getPrivateKeyFromMap(recNodeIndex),pubKey);

    ApplicationUtil::getInstance()->putSecretKeyInGlobalMap(recNodeIndex,srcNodeIndex,sharedKey);
*/
    publicKeyCounter--;
    std::cout<<"Public key counter :"<< publicKeyCounter<< "\n";
    //socket->Close();
    if(publicKeyCounter == 0)
    {
        std::cout<<"Debug : calling simulator loop \n";
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
        double jitter = uv->GetValue(0.01,10);
        std::cout << "simulatorloop random="<<jitter<< std::endl;

        //Simulator::Schedule (Seconds(jitter),&SimulatorLoop, tid,c,ipInterfaceContainer);
    }


}

void ReceivePublicKeyCallback(Ptr<Socket> socket,const Address& address) {
    std::cout<<"receivepublickeycallback"<<std::endl;
    socket->SetRecvCallback(MakeCallback(&ReceivePublicKey));
}


map<int,Ptr<Socket> > sendPublicKey_ReceiverSocketMap;
map<int,Ptr<Socket> > sendPublicKey_SenderSocketMap;

static void SendPublicKey (SecByteBlock pub, int index1, int index2)
{
    std::cout<<"*******************"<<std::endl;
    std::cout<<"Debug : Inside dcnet send public key index1="<<index1<<" index2="<<index2<<std::endl;


    Ptr<Packet> sendPacket = Create<Packet> ((uint8_t*)pub.BytePtr(),(uint8_t) pub.SizeInBytes());
    MyTag sendTag;
    sendTag.SetSimpleValue(index1);
    sendPacket->AddPacketTag(sendTag);

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    int randomport = uv->GetInteger(1025,65530);
    randomport = 9803;

    Ptr<Socket> recvNodeSink = NULL;
    int retval = 0;
    bool receiver_exists = sendPublicKey_ReceiverSocketMap.find(index2) != sendPublicKey_ReceiverSocketMap.end();
    if(receiver_exists) {
        recvNodeSink = sendPublicKey_ReceiverSocketMap[index2];
    }
    else {

        recvNodeSink = Socket::CreateSocket (c.Get (index2), tid);
        InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),randomport);
        std::cout<<"localaddress="<<localAddress<<std::endl;
        retval = recvNodeSink->Bind (localAddress);
        std::cout<<"recvnode bind="<<retval<<std::endl;
        recvNodeSink->Listen();
        //recvNodeSink->SetRecvCallback (MakeCallback (&ReceivePublicKey));
        recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
		MakeCallback(&ReceivePublicKeyCallback));

        sendPublicKey_ReceiverSocketMap[index2] = recvNodeSink;
    }

    InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (index2, 0), randomport);
    Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
    retval = sourceNodeSocket->Connect (remoteSocket);
    std::cout<<"sourcenode connect="<<retval<<std::endl;



    //PacketHolder* ph = new PacketHolder(sendPacket);
    //sourceNodeSocket->SetSendCallback(MakeCallback(&PacketHolder::SendCallback,ph));

    retval = sourceNodeSocket->Send(sendPacket);
    std::cout<<"sourcenode send="<<retval<<std::endl;
    stage1SentPacketCount += 1;//increment sent packet counter for stage1
    //std::string sendData = hexStr(pub.BytePtr(),pub.SizeInBytes());

    sourceNodeSocket->Close();
}


void generateKeys(int index)
{
    try {
        ApplicationUtil *appUtil = ApplicationUtil::getInstance();
        DH dh;
        AutoSeededRandomPool rnd;

        dh.AccessGroupParameters().Initialize(p, q, g);

        if(!dh.GetGroupParameters().ValidateGroup(rnd, 3))
            throw runtime_error("Failed to validate prime and generator");


        p = dh.GetGroupParameters().GetModulus();
        q = dh.GetGroupParameters().GetSubgroupOrder();
        g = dh.GetGroupParameters().GetGenerator();

        Integer v = ModularExponentiation(g, q, p);
        if(v != Integer::One())
            throw runtime_error("Failed to verify order of the subgroup");

        //////////////////////////////////////////////////////////////

        SecByteBlock priv(dh.PrivateKeyLength());
        SecByteBlock pub(dh.PublicKeyLength());
        dh.GenerateKeyPair(rnd, priv, pub);

        //////////////////////////////////////////////////////////////

        appUtil->putPrivateKeyInMap(index,priv);
        appUtil->putPublicKeyInMap(index,pub);
        appUtil->setDhAgreedLength(dh.AgreedValueLength());

        //	std::cout<<"Dh key length "<< index <<" : "<<dh.AgreedValueLength()<<"\n";
    }
    catch(const CryptoPP::Exception& e)
    {
        std::cerr << "Crypto error : "<< e.what() << std::endl;
    }

    catch(const std::exception& e)
    {
        std::cerr << "Standard error : "<<e.what() << std::endl;
    }
}

void DisplayMeasurements()
{
    std::cout<<"Shared Message after "<<MessageLength<<" rounds is : "<<sharedMessage.str()<<"\n";
    std::cout<<"Sent Packet Count Stage 1: "<<stage1SentPacketCount<<"\n";
    std::cout<<"Sent Packet Count Stage 2: "<<stage2SentPacketCount<<"\n";
    std::cout<<"Sent Recv Count Stage 1: "<<stage1RecvPacketCount<<"\n";
    std::cout<<"Sent Recv Count Stage 2: "<<stage2RecvPacketCount<<"\n";

    stage1Latency = (stage1EndTime.front().GetSeconds() - stage1StartTime.front().GetSeconds());
    std::cout<<"Stage 1 latency: "<<stage1Latency<<"\n";

    stage2Latency = (stage2EndTime.front().GetSeconds() - stage2StartTime.front().GetSeconds());
    std::cout<<"Stage 2 latency: "<<stage2Latency<<"\n";

    totalLatency = (stage1Latency + stage2Latency);
    // std::cout<<"goodPut: "<<goodPut<<"\n";

    totalTimeEnd = Simulator::Now();
    totalRunningTime = totalTimeEnd.GetSeconds() - totalTimeStart.GetSeconds();
    std::cout<<"Total time taken : "<<totalRunningTime<<" seconds\n";

    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
//output to csv

    if(option == 1)
        appUtil->writeOutputToFile((char*)"NumNodesvsMeasurements.csv",option,numNodes,MessageLength,totalLatency,totalRunningTime);
    else if(option == 2)
        appUtil->writeOutputToFile((char*)"MsgLengthvsMeasurements.csv",option,numNodes,MessageLength,totalLatency,totalRunningTime);

}

void DCNET( int numRounds)
{
    std::cout<<"Debug : Inside dcnet\n";

    //finished
    if(numRounds >= MessageLength) {
        DisplayMeasurements();
        return;
    }

    //Symmetric key generation
    for(int ind =0 ; ind < (int)numNodes; ind++)
    {
        generateKeys(ind);
    }

    ApplicationUtil *appUtil = ApplicationUtil::getInstance();

    //send the public key to everyone
    for (int index1 = 0; index1 < (int)numNodes; index1++)
    {

        for (int index2 = 0; index2 < (int)numNodes; index2++)
        {
            if(index1 != index2)
            {
                std::cout<<"Debug : Inside dcnet  1\n";
                Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
                double jitter = uv->GetValue(100,200);
                std::cout << "sendpublickey random="<<jitter<< std::endl;
                Simulator::Schedule (Seconds(jitter),&SendPublicKey, appUtil->getPublicKeyFromMap(index1),index1,index2);
            }
        }
    }


}

int main (int argc, char *argv[])
{

    NS_LOG_UNCOND("Inside Main");

    ApplicationUtil *appUtil = ApplicationUtil::getInstance();

    CommandLine cmd;

    std::cout<<"argc : "<<argc<<"\n";

    cmd.AddValue ("numNodes", "Number of Nodes", numNodes);
    cmd.AddValue ("message", "Actual Message", Message);
    cmd.AddValue ("option", "Changing numnodes or messagelength", option);
    //cmd.AddValue ("sender", "Sender of the message (actually anonymous)", sender);

    cmd.Parse (argc, argv);
    // Convert to time object
    //Time interPacketInterval = Seconds (interval);

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                        StringValue (phyMode));


    c.Create (numNodes);
    for(int nodeind = 0; nodeind < numNodes; nodeind++)
    {
        appUtil->putNodeInMap(c.Get(nodeind),nodeind);
    }
    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    if (verbose)
    {
        wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set ("RxGain", DoubleValue (0) );
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel (wifiChannel.Create ());

    // Add a non-QoS upper mac, and disable rate control
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue (phyMode),
                                  "ControlMode",StringValue (phyMode));






    // Set it to adhoc mode
    wifiMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);


    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (distance),
                                   "DeltaY", DoubleValue (distance),
                                   "GridWidth", UintegerValue (10),
                                   "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (c);


    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper list;
    list.Add (staticRouting, 0);



    InternetStackHelper internet;
    internet.SetRoutingHelper (list); // has effect on the next Install ()
    internet.Install (c);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    ipInterfaceContainer = ipv4.Assign (devices);

    tid = TypeId::LookupByName ("ns3::TcpSocketFactory");


    AnnouncementPacketCount = (numNodes * numNodes) - numNodes;
    publicKeyCounter = (numNodes * numNodes) - numNodes;
    randomBitCounter = (numNodes * (numNodes-1)/2);


    std::cout<<"Actual Message : "<<Message<<"\n";
    MessageLength = (int)strlen(Message.c_str()) ;
    std::cout<<"Message length:"<<MessageLength<<"\n";
    stage1StartTime.push_back(Simulator::Now());
    totalTimeStart = Simulator::Now();
    RngSeedManager::SetSeed (30);
    Simulator::ScheduleNow (&DCNET, 0);


    if (tracing == true)
    {
        AsciiTraceHelper ascii;
        wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
        wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
        // Trace routing tables
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);


        // To do-- enable an IP-level trace that shows forwarding events only
    }

    Simulator::Run ();
    Simulator::Destroy ();


    return 0;
}
