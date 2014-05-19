#include "ApplicationUtil.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

class PacketHolder {
public:
    PacketHolder(Ptr<Packet> _packet) {
        this->packet = _packet;
    }
    void SendCallback(Ptr<Socket> socket, uint32_t val) {
        NS_LOG_LOGIC("send callback");
        socket->Send(this->packet);
        socket->Close();
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
   // double rndDouble = (double)rand() / RAND_MAX;
    //return rndDouble > p;
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	int rand = uv->GetInteger(0,1);
   // std::cout <<"random variable generated is "<<rand << "\n";
    return rand;
}



//sending and receiving announcements
static void SendAnnouncement (Ptr<Socket> sourceNodeSocket, int result, int index)
{
    std::ostringstream ss;
    ss << result;
    std::string message = ss.str();
    Ptr<Packet> sendPacket =
        Create<Packet> ((uint8_t*)message.c_str(),message.size());

    MyTag sendTag;
    sendTag.SetSimpleValue(index);
    sendPacket->AddPacketTag(sendTag);

    //PacketHolder* ph = new PacketHolder(sendPacket);
    //sourceNodeSocket->SetSendCallback(MakeCallback(&PacketHolder::SendCallback,ph));

    int retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
    //sourceNodeSocket->Close();
}

void ReceiveAnnouncement (Ptr<Socket> socket)
{
    announcementPacketCounter-=1;
    Ptr<Packet> recPacket = socket->Recv();
    //stage2RecvPacketCount += 1;//increment recv packet counter for stage2
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    Ptr<Node> recvnode = socket->GetNode();
    int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);
    
    uint8_t *buffer = new uint8_t[recPacket->GetSize()];
    recPacket->CopyData(buffer,recPacket->GetSize());

    std::string recMessage = std::string((char*)buffer);
    recMessage = recMessage.substr (0,messageLen-1);

    MyTag recTag;
    recPacket->PeekPacketTag(recTag);
    int srcNodeIndex =int(recTag.GetSimpleValue());
    NS_LOG_DEBUG("receive announcement from "<<srcNodeIndex);
    appUtil->putAnnouncementInReceivedMap(recNodeIndex, srcNodeIndex, atoi(recMessage.c_str()));
    delete buffer;
    NS_LOG_INFO("announcementPacketCounter="<<announcementPacketCounter);
    if(announcementPacketCounter==0)
    {
        int x=0;
        //xoring outputs
        for(int index=0; index<(int)numNodes; index++)
        {
            x ^= appUtil->getAnnouncement(index);

        }



        sharedMessage<<x;
        announcementPacketCounter = (numNodes * numNodes) - numNodes;
        publicKeyCounter = (numNodes * numNodes) - numNodes;
        randomBitCounter = (numNodes * (numNodes-1)/2);
        stage2EndTime.push_back(Simulator::Now());
        rounds = rounds +1;
        Simulator::ScheduleNow (&DCNET,rounds);
    }
}

void ReceiveAnnouncementCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receiveannouncementcallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveAnnouncement));
}



map<int,Ptr<Socket> > sendAnnouncement_ReceiverSocketMap;


void GenerateAnnouncements()
{
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();

    int bit = Message.at(rounds)-48 ;

    NS_LOG_LOGIC("Current Round : "<<rounds<<" and current bit : "<<bit);
    for(int index = 0; index < (int)numNodes ; index++)
    {

        int result = 0;
        map<int,int> NodeSecretBitMap = appUtil->getSecretBitSubMap(index);

        for (map<int,int>::iterator it=NodeSecretBitMap.begin(); it!=NodeSecretBitMap.end(); ++it)
        {

            NS_LOG_LOGIC("Adj bits of node "<<index<<" : "<<(int)it->second);
            //Exor the adjacent node bits stored in the map
            result ^= (int)it->second;
        }
        if(sender == index)	//exor result with message
        {
            result ^= bit;
        }

        NS_LOG_LOGIC("Result for Node "<<index<<" is : "<<result<<" in round "<<rounds);
        appUtil->putAnnouncementInGlobalMap(index, result);

    }
    
    for (int index1 = 0; index1 < (int)numNodes; index1++)
    {

        for (int index2 = 0; index2 < (int)numNodes; index2++)
        {
            if(index1 != index2)
            {

                int port = 9802;

                Ptr<Socket> recvNodeSink = NULL;
                int retval = 0;
                bool receiver_exists = sendAnnouncement_ReceiverSocketMap.find(index2) != sendAnnouncement_ReceiverSocketMap.end();
                if(receiver_exists) {
                    recvNodeSink = sendAnnouncement_ReceiverSocketMap[index2];
                }
                else {

                    recvNodeSink = Socket::CreateSocket (c.Get (index2), tid);
                    InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
                    NS_LOG_LOGIC("localaddress="<<localAddress);
                    retval = recvNodeSink->Bind (localAddress);
                    NS_LOG_LOGIC("recvnode bind="<<retval);
                    recvNodeSink->Listen();
                    recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveAnnouncement));
                    recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveAnnouncementCallback));

                    sendAnnouncement_ReceiverSocketMap[index2] = recvNodeSink;
                }

                InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (index2, 0), port);
                Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
                retval = sourceNodeSocket->Connect (remoteSocket);
                NS_LOG_LOGIC("sourcenode connect="<<retval);


                Simulator::Schedule (Seconds(0.01),&SendAnnouncement, sourceNodeSocket,appUtil->getAnnouncement(index1), index1);
            }
        }
    }
}

void ReceiveCoinFlip (Ptr<Socket> socket)
{
    Ptr<Packet> recPacket = socket->Recv();
    stage2RecvPacketCount += 1;//increment recv packet counter for stage2
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();

    Ptr<Node> recvnode = socket->GetNode();
    int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);

    uint8_t *buffer = new uint8_t[recPacket->GetSize()];
    recPacket->CopyData(buffer,recPacket->GetSize());

    std::string recMessage = std::string((char*)buffer);
    recMessage = recMessage.substr (0,messageLen-1);

    MyTag recTag;
    recPacket->PeekPacketTag(recTag);
    int srcNodeIndex =int(recTag.GetSimpleValue());

    SecByteBlock key(SHA256::DIGESTSIZE);
    SHA256().CalculateDigest(key, appUtil->getSecretKeyFromGlobalMap(srcNodeIndex,recNodeIndex), appUtil->getSecretKeyFromGlobalMap(srcNodeIndex,recNodeIndex).size());

    //Decryption using the Shared secret key
    CFB_Mode<AES>::Decryption cfbDecryption(key, aesKeyLength, iv);
    cfbDecryption.ProcessData((byte*)recMessage.c_str(), (byte*)recMessage.c_str(), messageLen);


    int value = atoi(recMessage.c_str());
    //put in node's map
    appUtil->putSecretBitInGlobalMap(srcNodeIndex,recNodeIndex,value);
    appUtil->putSecretBitInGlobalMap(recNodeIndex,srcNodeIndex,value);
    delete buffer;
    randomBitCounter--;
    NS_LOG_INFO("randomBitCounter="<<randomBitCounter);
    if(randomBitCounter == 0)
    {
        stage1EndTime.push_back(Simulator::Now());
        stage2StartTime.push_back(Simulator::Now());
        Simulator::Schedule (Seconds(0.01),&GenerateAnnouncements);
    }
}

void ReceiveCoinFlipCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receivemessagecallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveCoinFlip));
}


map<int,Ptr<Socket> > sendMessage_ReceiverSocketMap;

static void SendCoinFlip (std::string message, int index1, int index2)
{
    Ptr<Packet> sendPacket =
        Create<Packet> ((uint8_t*)message.c_str(),message.size());

    MyTag sendTag;
    sendTag.SetSimpleValue(index1);
    sendPacket->AddPacketTag(sendTag);

    Ptr<Socket> recvNodeSink =  NULL;
    int retval = 0;
    int port = 9801;
    bool receiver_exists = sendMessage_ReceiverSocketMap.find(index2) != sendMessage_ReceiverSocketMap.end();
    if(receiver_exists) {
        recvNodeSink = sendMessage_ReceiverSocketMap[index2];
    }
    else {

        recvNodeSink = Socket::CreateSocket (c.Get (index2), tid);
        InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
        NS_LOG_LOGIC("localaddress="<<localAddress);
        retval = recvNodeSink->Bind (localAddress);
        NS_LOG_LOGIC("recvnode bind="<<retval);
        recvNodeSink->Listen();
        recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveCoinFlip));
        recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                        MakeCallback(&ReceiveCoinFlipCallback));

        sendMessage_ReceiverSocketMap[index2] = recvNodeSink;
    }

    InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (index2, 0), port);
    Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
    retval = sourceNodeSocket->Connect (remoteSocket);
    NS_LOG_LOGIC("sourcenode connect="<<retval);

    //PacketHolder ph(sendPacket);
    //sourceNodeSocket->SetSendCallback(MakeCallback(&PacketHolder::SendCallback,&ph));


    retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
    stage2SentPacketCount += 1;//increment sent packet counter for stage2
    //sourceNodeSocket->Close ();
}

static void GenerateCoinFlips(TypeId tid, NodeContainer c, Ipv4InterfaceContainer i)
{
    publicKeyCounter = (numNodes * numNodes) - numNodes;
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    // Generate a random IV
    rnd.GenerateBlock(iv, AES::BLOCKSIZE);

    //sharing the random bit using dh secret key
    for (int index1 = 0; index1 < (int)numNodes; index1++)
    {

        for (int index2 = 0; index2 < (int)numNodes; index2++)
        {
            if(index1 < index2)
            {
                int randomBit = randomBitGeneratorWithProb(0.5);
                NS_LOG_LOGIC("Random bit : "<<randomBit<<" "<<index1<<" "<<index2);

                //put random bit in both the maps - src and dest maps

                appUtil->putSecretBitInGlobalMap(index1,index2,randomBit);
                appUtil->putSecretBitInGlobalMap(index2,index1,randomBit);

                // Calculate a SHA-256 hash over the Diffie-Hellman session key
                SecByteBlock key(SHA256::DIGESTSIZE);
                SHA256().CalculateDigest(key, appUtil->getSecretKeyFromGlobalMap(index1,index2), appUtil->getSecretKeyFromGlobalMap(index1,index2).size());

                std::ostringstream ss;
                ss << randomBit;
                std::string message = ss.str();
                messageLen = (int)strlen(message.c_str()) + 1;

                // Encrypt

                CFB_Mode<AES>::Encryption cfbEncryption(key, aesKeyLength, iv);
                cfbEncryption.ProcessData((byte*)message.c_str(), (byte*)message.c_str(), messageLen);

                Simulator::Schedule(Seconds(0.05),&SendCoinFlip, message,index1,index2);
            }
        }
    }


}



void ReceivePublicKey (Ptr<Socket> socket)
{
    int available = socket->GetRxAvailable ();
    NS_LOG_LOGIC("available="<<available);
    NS_LOG_LOGIC("Debug : Inside dcnet receive public key");
    Ptr<Node> recvnode = socket->GetNode();
    int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);

    Ptr<Packet> recPacket = socket->Recv();
    NS_LOG_LOGIC("packet tags");
    //recPacket->PrintPacketTags(std::cout);
    NS_LOG_LOGIC("done packet tags");
    stage1RecvPacketCount +=1; //increment received packet count for stage 1

    int len = recPacket->GetSize()+1;
    NS_LOG_LOGIC("packet len="<<len);
    uint8_t *buffer = new uint8_t[len];
    memset(buffer,0,len);
    int copiedlen = recPacket->CopyData(buffer,recPacket->GetSize());
    NS_LOG_LOGIC("copied len="<<copiedlen);
    SecByteBlock pubKey((byte *)buffer,recPacket->GetSize());

    MyTag recTag;
    recTag.SetSimpleValue(0);
    bool isTag = recPacket->PeekPacketTag(recTag);
    int srcNodeIndex =int(recTag.GetSimpleValue());
    NS_LOG_LOGIC("isTag="<<isTag<<"tagVal="<<srcNodeIndex);

    //std::string recvData = hexStr(pubKey.BytePtr(),pubKey.SizeInBytes());


    DH dh;
    dh.AccessGroupParameters().Initialize(p, q, g);
    SecByteBlock sharedKey(ApplicationUtil::getInstance()->getDhAgreedLength());

    dh.Agree(sharedKey, ApplicationUtil::getInstance()->getPrivateKeyFromMap(recNodeIndex),pubKey);

    ApplicationUtil::getInstance()->putSecretKeyInGlobalMap(recNodeIndex,srcNodeIndex,sharedKey);

    publicKeyCounter--;
    NS_LOG_INFO("Public key counter :"<< publicKeyCounter);
    //socket->Close();
    if(publicKeyCounter == 0)
    {
        NS_LOG_LOGIC("Debug : calling simulator loop");
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
        double jitter = uv->GetValue(0.01,10);
        std::cout << "simulatorloop random="<<jitter<< std::endl;

        Simulator::Schedule (Seconds(jitter),&GenerateCoinFlips, tid,c,ipInterfaceContainer);
    }


}

void ReceivePublicKeyCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receivepublickeycallback");
    socket->SetRecvCallback(MakeCallback(&ReceivePublicKey));
}


map<int,Ptr<Socket> > sendPublicKey_ReceiverSocketMap;

static void SendPublicKey (SecByteBlock pub, int index1, int index2)
{
    NS_LOG_LOGIC("*******************");
    NS_LOG_LOGIC("Debug : Inside dcnet send public key index1="<<index1<<" index2="<<index2);


    Ptr<Packet> sendPacket = Create<Packet> ((uint8_t*)pub.BytePtr(),(uint8_t) pub.SizeInBytes());
    MyTag sendTag;
    sendTag.SetSimpleValue(index1);
    NS_LOG_LOGIC("sendTagVal="<<int(sendTag.GetSimpleValue()));
    sendPacket->AddPacketTag(sendTag);
    //sendPacket->PrintPacketTags(std::cout);

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
        NS_LOG_LOGIC("localaddress="<<localAddress);
        retval = recvNodeSink->Bind (localAddress);
        NS_LOG_LOGIC("recvnode bind="<<retval);
        recvNodeSink->Listen();
        recvNodeSink->SetRecvCallback (MakeCallback (&ReceivePublicKey));
        recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                        MakeCallback(&ReceivePublicKeyCallback));

        sendPublicKey_ReceiverSocketMap[index2] = recvNodeSink;
    }

    InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (index2, 0), randomport);
    Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
    retval = sourceNodeSocket->Connect (remoteSocket);
    NS_LOG_LOGIC("sourcenode connect="<<retval);



    //PacketHolder* ph = new PacketHolder(sendPacket);
    //sourceNodeSocket->SetSendCallback(MakeCallback(&PacketHolder::SendCallback,ph));

    retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
    stage1SentPacketCount += 1;//increment sent packet counter for stage1
    //std::string sendData = hexStr(pub.BytePtr(),pub.SizeInBytes());

    //sourceNodeSocket->Close();
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
    NS_LOG_INFO("Shared Message after "<<MessageLength<<" rounds is : "<<sharedMessage.str());
    NS_LOG_INFO("Sent Packet Count Stage 1: "<<stage1SentPacketCount);
    NS_LOG_INFO("Sent Packet Count Stage 2: "<<stage2SentPacketCount);
    NS_LOG_INFO("Sent Recv Count Stage 1: "<<stage1RecvPacketCount);
    NS_LOG_INFO("Sent Recv Count Stage 2: "<<stage2RecvPacketCount);

    stage1Latency = (stage1EndTime.front().GetSeconds() - stage1StartTime.front().GetSeconds());
    NS_LOG_LOGIC("Stage 1 latency: "<<stage1Latency);

    stage2Latency = (stage2EndTime.front().GetSeconds() - stage2StartTime.front().GetSeconds());
    NS_LOG_LOGIC("Stage 2 latency: "<<stage2Latency);

    totalLatency = (stage1Latency + stage2Latency);

    totalTimeEnd = Simulator::Now();
    totalRunningTime = totalTimeEnd.GetSeconds() - totalTimeStart.GetSeconds();
    NS_LOG_INFO("Total time taken : "<<totalRunningTime<<" seconds");

    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
//output to csv

    if(option == 1)
        appUtil->writeOutputToFile((char*)"NumNodesvsMeasurements.csv",option,numNodes,MessageLength,totalLatency,totalRunningTime);
    else if(option == 2)
        appUtil->writeOutputToFile((char*)"MsgLengthvsMeasurements.csv",option,numNodes,MessageLength,totalLatency,totalRunningTime);

}

void DCNET( int numRounds)
{
    NS_LOG_INFO("Debug : Inside dcnet rounds="<<numRounds<<"messagelen="<<MessageLength);

    //finished
    if(numRounds >= MessageLength) {
		 Simulator::Stop ();
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
                NS_LOG_LOGIC("Debug : Inside dcnet  1");
                Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
                double jitter = uv->GetValue(100,200);
                NS_LOG_LOGIC("sendpublickey random="<<jitter);
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

    NS_LOG_LOGIC("argc : "<<argc);

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
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
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

    tid = TypeId::LookupByName ("ns3::UdpSocketFactory");


    announcementPacketCounter = (numNodes * numNodes) - numNodes;
    publicKeyCounter = (numNodes * numNodes) - numNodes;
    randomBitCounter = (numNodes * (numNodes-1)/2);


    NS_LOG_LOGIC("Actual Message : "<<Message);
    MessageLength = (int)strlen(Message.c_str()) ;
    NS_LOG_LOGIC("Message length:"<<MessageLength);
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
	AnimationInterface anim("dcnet_net_anim_fully_connected.xml");
	anim.EnablePacketMetadata(true);
    FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();    
    Simulator::Run ();
	monitor->CheckForLostPackets (); 
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	uint32_t txPacketsum = 0;
    uint32_t rxPacketsum = 0;
    uint32_t DropPacketsum = 0;
    uint32_t LostPacketsum = 0;
    double Delaysum = 0;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =stats.begin (); i != stats.end (); i++) {
        txPacketsum += i->second.txPackets;
        rxPacketsum += i->second.rxPackets;
        LostPacketsum += i->second.lostPackets;
        DropPacketsum += i->second.packetsDropped.size();
        Delaysum += i->second.delaySum.GetSeconds();
  
	}
	std::cout << "  All Tx Packets: " << txPacketsum << "\n";
    std::cout << "  All Rx Packets: " << rxPacketsum << "\n";
    std::cout << "  All Delay: " << Delaysum / txPacketsum <<"\n";
    std::cout << "  All Lost Packets: " << LostPacketsum << "\n";
    std::cout << "  All Drop Packets: " << DropPacketsum << "\n";
    std::cout << "  Packets Delivery Ratio: " << ((rxPacketsum *100) /txPacketsum) << "%" << "\n";
    std::cout << "  Packets Lost Ratio: " << ((LostPacketsum *100) /txPacketsum) << "%" << "\n";
    monitor->SerializeToXmlFile("dcnet_fullyconnected.flowmon",true,true);
    Simulator::Destroy ();


    return 0;
}
