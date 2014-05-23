#include "ApplicationUtil.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");
map<int,Ptr<Socket> > sendAnnouncement_ReceiverSocketMap;
map<int,Ptr<Socket> > sendMessage_ReceiverSocketMap;
map<int,Ptr<Socket> > sendPublicKey_ReceiverSocketMap;
static void ReceiveAnnouncementForRing (Ptr<Socket> socket);
static void ReceiveAnnouncementForStar(Ptr<Socket> socket);
static void ReceiveAnnouncementForFullyConnected(Ptr<Socket> socket);
	

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
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	int rand = uv->GetInteger(0,1);
    return rand;
}
std::string PseudoRandomGenerator(unsigned int SEEDSIZE)
{
	std::string result;
    std::ostringstream convert;
   	for(unsigned int i=0; i<SEEDSIZE; i++) {
   		int randomBit = randomBitGeneratorWithProb(0.5);
       	convert << randomBit;
    }
	result = convert.str();
	return result;
}

static void SendAnnouncementForFullyConnected (Ptr<Socket> sourceNodeSocket, std::string result, int index)
{
    std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue(index);
    if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
    else
		 sendPacket->AddByteTag(sendTag);
   int retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
    //sourceNodeSocket->Close();
}

static void SendAnnouncementForStar (Ptr<Socket> sourceNodeSocket, std::string result, int index)
{
   
    std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
	sendTag.SetSimpleValue(index);
	if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
    else
		sendPacket->AddByteTag(sendTag);
    int retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
}

static void SendAnnouncementForRing (Ptr<Socket> sourceNodeSocket, std::string result, int index)
{
    
       std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
	sendTag.SetSimpleValue(index);
	if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
    else
		sendPacket->AddByteTag(sendTag);
    int retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
}

//sendbroadcast announcement

static void SendBroadcastAnnouncement (Ptr<Socket> sourceNodeSocket, std::string result, int index)
{
   
    std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue(index);
	if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
	else
		sendPacket->AddByteTag(sendTag);
    int retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
	sourceNodeSocket->Close();
}

static void ReceiveBroadcastAnnouncement(Ptr<Socket> socket)
{
	Ptr<Packet> recPacket = socket->Recv();
    uint8_t *buffer = new uint8_t[recPacket->GetSize()];
    recPacket->CopyData(buffer,recPacket->GetSize());
    Ptr<Node> recvnode = socket->GetNode();
    if(recvnode==NULL)
		std::cout << "socket node is null\n";
    std::string recMessage = std::string((char*)buffer);
    recMessage = recMessage.substr (0,messageLen-1);
    delete buffer;
	socket->Close();
}

static void ReceiveBroadcastAnnouncementCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receiveannouncementcallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveBroadcastAnnouncement));
}

static void ReceiveAnnouncementForRingCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receiveannouncementcallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveAnnouncementForRing));
}

static void ReceiveAnnouncementForStarCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receiveannouncementcallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveAnnouncementForStar));
}

static void ReceiveAnnouncementForFullyConnectedCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receiveannouncementcallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveAnnouncementForFullyConnected));
}

static void ReceiveAnnouncementForFullyConnected (Ptr<Socket> socket)
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
    if(protocol=="udp")
    	recPacket->PeekPacketTag(recTag);
	else {
		
		recPacket->FindFirstMatchingByteTag(recTag);
    }
    int srcNodeIndex =int(recTag.GetSimpleValue());
    NS_LOG_DEBUG("receive announcement from "<<srcNodeIndex);
    appUtil->putAnnouncementInReceivedMap(recNodeIndex, srcNodeIndex, recMessage);
    delete buffer;
    NS_LOG_INFO("announcementPacketCounter="<<announcementPacketCounter);
	if(announcementPacketCounter==0)
    {
    	for(int round = 0 ; round < MessageLength ; round++) {
			int x=0;
			//xoring outputs
            for(int index=0;index<(int)numNodes;index++) {
				std::string announcement = appUtil->getAnnouncement(index);
				 x ^= announcement.at(round)-48	;					 		
			}
            sharedMessage<<x;					
		}
		
		std::cout << "message is "<< sharedMessage.str() << "\n";
        announcementPacketCounter = (numNodes * numNodes) - numNodes;
        publicKeyCounter = (numNodes * numNodes) - numNodes;
        randomBitCounter = (numNodes * (numNodes-1)/2);
        stage2EndTime.push_back(Simulator::Now());
        rounds = rounds +1;
        Simulator::ScheduleNow (&DCNET,MessageLength);
    }
}
static void ReceiveAnnouncementForStar (Ptr<Socket> socket)
{
    announcementPacketCounter-=1;
    Ptr<Packet> recPacket = socket->Recv();
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    Ptr<Node> recvnode = socket->GetNode();
    int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);
    uint8_t *buffer = new uint8_t[recPacket->GetSize()];
    recPacket->CopyData(buffer,recPacket->GetSize());
    std::string recMessage = std::string((char*)buffer);
    recMessage = recMessage.substr (0,messageLen-1);
	MyTag recTag;
	if(protocol=="udp")
    	recPacket->PeekPacketTag(recTag);
    else {
		  recPacket->FindFirstMatchingByteTag(recTag);
		 // std::cout << "istag is " <<istag << "\n";
	}
    int srcNodeIndex =int(recTag.GetSimpleValue());
	NS_LOG_DEBUG("receive announcement from "<<srcNodeIndex);
    appUtil->putAnnouncementInReceivedMap(recNodeIndex, srcNodeIndex,recMessage.c_str());
    delete buffer;
    NS_LOG_INFO("announcementPacketCounter="<<announcementPacketCounter);
    if(announcementPacketCounter==0)
    {
        //xoring outputs
        for(int round = 0 ; round < MessageLength ; round++)
		{
			int x=0;
			//xoring outputs
            for(int index=0;index<(int)numNodes;index++)
			{
				std::string announcement = appUtil->getAnnouncement(index);
				 x ^= announcement.at(round)-48	;					 		
			}
            sharedMessage<<x;					
		}
       
        std::cout << "Node" <<numNodes-1 <<"is broadcasting the message:" << sharedMessage.str() <<  "\n";
		int port = 9804;
		int retval;
        for(int index = 0; index < (int)numNodes-1 ; index++) {
          
			Ptr<Socket> recvNodeSink  = Socket::CreateSocket (c.Get (index), tid);
        	InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
        	NS_LOG_LOGIC("localaddress="<<localAddress);
        	retval = recvNodeSink->Bind (localAddress);
        	NS_LOG_LOGIC("recvnode bind="<<retval);
        	recvNodeSink->Listen();
        	recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveBroadcastAnnouncement));
        
        }
        InetSocketAddress remoteSocket = InetSocketAddress (Ipv4Address ("255.255.255.255"), port);
        Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (recNodeIndex), tid);
		sourceNodeSocket->SetAllowBroadcast (true);
        retval = sourceNodeSocket->Connect (remoteSocket);
		NS_LOG_LOGIC("sourcenode connect="<<retval);
        Simulator::Schedule (Seconds(0.03),&SendBroadcastAnnouncement, sourceNodeSocket,sharedMessage.str(),(int) numNodes-1); 
        announcementPacketCounter = numNodes-1;
        publicKeyCounter = (numNodes * numNodes) - numNodes;
        randomBitCounter = (numNodes * (numNodes-1)/2);
        stage2EndTime.push_back(Simulator::Now());
        rounds = rounds +1;
        Simulator::ScheduleNow (&DCNET,MessageLength);
    }
}

static void ReceiveAnnouncementForRing (Ptr<Socket> socket)
{
    announcementPacketCounter-=1;
    Ptr<Packet> recPacket = socket->Recv();
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    Ptr<Node> recvnode = socket->GetNode();
    int recNodeIndex = ApplicationUtil::getInstance()->getNodeFromMap(recvnode);
    uint8_t *buffer = new uint8_t[recPacket->GetSize()];
    recPacket->CopyData(buffer,recPacket->GetSize());
	std::string recMessage = std::string((char*)buffer);
    recMessage = recMessage.substr (0,messageLen-1);
	MyTag recTag;
	if(protocol=="udp")
    	recPacket->PeekPacketTag(recTag);
	else {
		
		recPacket->FindFirstMatchingByteTag(recTag);
	}
    int srcNodeIndex =int(recTag.GetSimpleValue());
    delete buffer;
    socket->Close();
    std::string prgsecretstring;
	std::ostringstream ss;
	map<int,std::string> NodeSecretBitMap = appUtil->getSecretBitSubMap(recNodeIndex);
	for(int round = 0; round < MessageLength ; round++) {
		int result=0;
		for (map<int,std::string>::iterator it=NodeSecretBitMap.begin(); it!=NodeSecretBitMap.end(); ++it) {
			//get the adjacent prg string stored in the map
			prgsecretstring =  (std::string)it->second;
			//std::cout << "prg string is" << prgsecretstring << "\n";
			result ^= ((prgsecretstring.at(round) - 48) ^(recMessage.at(round) - 48));
	    }
		ss << result;
	}
	
   	int retval;
    if(recNodeIndex <(int)numNodes-1) {
		int port = 9805;
		Ptr<Socket> recvNodeSink = NULL;
		recvNodeSink = Socket::CreateSocket (c.Get (recNodeIndex+1), tid);
		
		InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
		retval = recvNodeSink->Bind (localAddress);
		NS_LOG_LOGIC("recvnode bind="<<retval);
		
		recvNodeSink->Listen();
	    recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveAnnouncementForRingCallback));
		recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveAnnouncementForRing));
		InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (recNodeIndex+1, 0), port);
		Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (srcNodeIndex), tid);
		retval = sourceNodeSocket->Connect (remoteSocket);
		Simulator::Schedule (Seconds(0.02),&SendAnnouncementForRing, sourceNodeSocket,ss.str(), recNodeIndex);

	}
    else {
         
      	std::cout << "Node" <<recNodeIndex <<"is broadcasting the message " << ss.str() <<  "\n";
		int port = 9806;
		int retval;
		for(int index = 0; index < (int)numNodes-1 ; index++) {
        	Ptr<Socket> recvNodeSink  = Socket::CreateSocket (c.Get (index), tid);
        	InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
        	//NS_LOG_LOGIC("localaddress="<<localAddress);
        	retval = recvNodeSink->Bind (localAddress);
        	//NS_LOG_LOGIC("recvnode bind="<<retval);
        	recvNodeSink->Listen();
			recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveBroadcastAnnouncementCallback));
            recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveBroadcastAnnouncement));
        }
        InetSocketAddress remoteSocket = InetSocketAddress (Ipv4Address ("255.255.255.255"), port);
        Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (recNodeIndex), tid);
		sourceNodeSocket->SetAllowBroadcast (true);
        retval = sourceNodeSocket->Connect (remoteSocket);
		NS_LOG_LOGIC("sourcenode connect="<<retval);
        Simulator::Schedule (Seconds(0.05),&SendBroadcastAnnouncement, sourceNodeSocket,ss.str(),(int) numNodes-1); 

        announcementPacketCounter =  numNodes-1;
        publicKeyCounter = (numNodes * numNodes) - numNodes;
        randomBitCounter = (numNodes * (numNodes-1)/2);
        stage2EndTime.push_back(Simulator::Now());
        rounds = rounds +1;
        Simulator::ScheduleNow (&DCNET,MessageLength);
        
    }
}
static void GenerateAnnouncementsForFullyConnected()
{
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    std::ostringstream ss;
	for(int index = 0; index < (int)numNodes ; index++) {
		for(int round = 0; round < MessageLength ; round++) {
	 		int bit = Message.at(round)-48 ;
			int result = 0;
			std::string prgsecretstring;
			map<int,std::string> NodeSecretBitMap = appUtil->getSecretBitSubMap(index);
            for (map<int,std::string>::iterator it=NodeSecretBitMap.begin(); it!=NodeSecretBitMap.end(); ++it) {
				//get the adjacent prg string stored in the map
		    	prgsecretstring =  (std::string)it->second;
				//std::cout <<"prgsecretstring is " << prgsecretstring << "\n";
                result ^= (prgsecretstring.at(round) - 48);
				//break;
			}
			if(sender == index) {	//exor result with message
				result ^= bit;
			}
			ss << result;
		}
		appUtil->putAnnouncementInGlobalMap(index, ss.str());
		ss.str("");
	}
    for (int index1 = 0; index1 < (int)numNodes; index1++) {
    	for (int index2 = 0; index2 < (int)numNodes; index2++) {
        	if(index1 != index2) {
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
                    recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveAnnouncementForFullyConnected));
                    recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveAnnouncementForFullyConnectedCallback));
                    sendAnnouncement_ReceiverSocketMap[index2] = recvNodeSink;
                }
                InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (index2, 0), port);
                Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
                retval = sourceNodeSocket->Connect (remoteSocket);
                NS_LOG_LOGIC("sourcenode connect="<<retval);
                Simulator::Schedule (Seconds(0.01),&SendAnnouncementForFullyConnected, sourceNodeSocket,appUtil->getAnnouncement(index1), index1);
            }
        }
    }
}

void GenerateAnnouncementsForStar()
{
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
    std::ostringstream ss;
	for(int index = 0; index < (int)numNodes ; index++) {
		map<int,std::string> NodeSecretBitMap = appUtil->getSecretBitSubMap(index);
		for(int round = 0; round < MessageLength ; round++) {
	 		int bit = Message.at(round)-48 ;
			int result = 0;
			std::string prgsecretstring;
			for (map<int,std::string>::iterator it=NodeSecretBitMap.begin(); it!=NodeSecretBitMap.end(); ++it) {
				//get the adjacent prg string stored in the map
		    	prgsecretstring =  (std::string)it->second;
                result ^= (prgsecretstring.at(round) - 48);
				//break;
			}
			if(sender == index) {	//exor result with message
				result ^= bit;
			}
			ss << result;
		}		
		appUtil->putAnnouncementInGlobalMap(index, ss.str());
		ss.str("");
	}
   /* now time to send it to node based on topology */
   for (int index1 = 0; index1 < (int)numNodes-1; index1++) {
		int retval;
		int port = 9802;
    	Ptr<Socket> recvNodeSink  = Socket::CreateSocket (c.Get ((int)numNodes-1), tid);
        InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
        NS_LOG_LOGIC("localaddress="<<localAddress);
        retval = recvNodeSink->Bind (localAddress);
        NS_LOG_LOGIC("recvnode bind="<<retval);
        recvNodeSink->Listen();
        recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveAnnouncementForStar));
        recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveAnnouncementForStarCallback));
        InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress ((int)numNodes-1, 0), port);
        Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (index1), tid);
        retval = sourceNodeSocket->Connect (remoteSocket);
        NS_LOG_LOGIC("sourcenode connect="<<retval);
		Simulator::Schedule (Seconds(0.01),&SendAnnouncementForStar, sourceNodeSocket,appUtil->getAnnouncement(index1), index1);
   }
}

void GenerateAnnouncementsForRing()
{
    ApplicationUtil *appUtil = ApplicationUtil::getInstance();
	std::ostringstream ss;
	map<int,std::string> NodeSecretBitMap = appUtil->getSecretBitSubMap(0);
	for(int round = 0; round < MessageLength ; round++) {
		int bit = Message.at(round)-48 ;
		int result = 0;
		std::string prgsecretstring;
		for (map<int,std::string>::iterator it=NodeSecretBitMap.begin(); it!=NodeSecretBitMap.end(); ++it) {
			//get the adjacent prg string stored in the map
		    prgsecretstring =  (std::string)it->second;
            result ^= (prgsecretstring.at(round) - 48);
				//break;
		}
		result ^= bit;
		ss << result;
    }
	int port = 9805;
    int retval;
    Ptr<Socket> recvNodeSink = NULL;
    recvNodeSink = Socket::CreateSocket (c.Get (1), tid);
    InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (),port);
    NS_LOG_LOGIC("localaddress="<<localAddress);
    retval = recvNodeSink->Bind (localAddress);
    NS_LOG_LOGIC("recvnode bind="<<retval);
    recvNodeSink->Listen();
    recvNodeSink->SetRecvCallback (MakeCallback (&ReceiveAnnouncementForRing));
    recvNodeSink->SetAcceptCallback(MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                                                    MakeCallback(&ReceiveAnnouncementForRingCallback));
    InetSocketAddress remoteSocket = InetSocketAddress (ipInterfaceContainer.GetAddress (1, 0), port);
    Ptr<Socket> sourceNodeSocket = Socket::CreateSocket (c.Get (0), tid);
    retval = sourceNodeSocket->Connect (remoteSocket);
    NS_LOG_LOGIC("sourcenode connect="<<retval);
    Simulator::Schedule (Seconds(0.01),&SendAnnouncementForRing, sourceNodeSocket,ss.str(), 0);
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
	if(protocol=="udp")
    	recPacket->PeekPacketTag(recTag);
	else
		recPacket->FindFirstMatchingByteTag(recTag);
    int srcNodeIndex =int(recTag.GetSimpleValue());
     SecByteBlock key(SHA256::DIGESTSIZE);
    SHA256().CalculateDigest(key, appUtil->getSecretKeyFromGlobalMap(srcNodeIndex,recNodeIndex), appUtil->getSecretKeyFromGlobalMap(srcNodeIndex,recNodeIndex).size());
    std::string decrypted, dec;
	CFB_Mode<AES>::Decryption cfbDecryption;
	cfbDecryption.SetKeyWithIV( key, AESkey.size(), AESiv );
	StringSource ss( recMessage, true, new StreamTransformationFilter( cfbDecryption,new StringSink( dec ))); 
	delete buffer;
    randomBitCounter--;
    NS_LOG_INFO("randomBitCounter="<<randomBitCounter);
    if(randomBitCounter == 0)
    {
        stage1EndTime.push_back(Simulator::Now());
        stage2StartTime.push_back(Simulator::Now());
		if(topology=="ring")
        	Simulator::Schedule (Seconds(0.01),&GenerateAnnouncementsForRing);
		else if(topology=="star") 
			Simulator::Schedule (Seconds(0.01),&GenerateAnnouncementsForStar);
		else if(topology=="fullconnected") 
			 Simulator::Schedule (Seconds(0.01),&GenerateAnnouncementsForFullyConnected);
       else {
			std::cout << "un supported topology\n";
		}
    }
}

void ReceiveCoinFlipCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receivemessagecallback");
    socket->SetRecvCallback(MakeCallback(&ReceiveCoinFlip));
}

static void SendCoinFlip (std::string message, int index1, int index2)
{
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue(index1);
    if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
	else
    	sendPacket->AddByteTag(sendTag);
    Ptr<Socket> recvNodeSink =  NULL;
    int retval = 0;
    int port = 9804;
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
                std::string prgString = PseudoRandomGenerator(MessageLength);
                NS_LOG_LOGIC("Random bit : "<<prgString<<" "<<index1<<" "<<index2);
                appUtil->putSecretBitInGlobalMap(index1,index2,prgString);
                appUtil->putSecretBitInGlobalMap(index2,index1,prgString);
                 // Calculate a SHA-256 hash over the Diffie-Hellman session key
                SecByteBlock key(SHA256::DIGESTSIZE);
                SHA256().CalculateDigest(key, appUtil->getSecretKeyFromGlobalMap(index1,index2), appUtil->getSecretKeyFromGlobalMap(index1,index2).size());
                std::string message = prgString;
                messageLen = (int)strlen(message.c_str()) + 1;
                CFB_Mode<AES>::Encryption cfbEncryption;
			    cfbEncryption.SetKeyWithIV( key, AESkey.size(), AESiv );
				std::string encrypted, enc;
				StringSource( message, true, new StreamTransformationFilter( cfbEncryption,
		    				   new StringSink( encrypted )
							  ) // StreamTransformationFilter      
		        );
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(0.01,5);
                Simulator::Schedule(Seconds(jitter),&SendCoinFlip, encrypted,index1,index2);
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
	bool isTag;
    if(protocol=="udp")
    	isTag= recPacket->PeekPacketTag(recTag);
	else
		isTag =recPacket->FindFirstMatchingByteTag(recTag);
    int srcNodeIndex =int(recTag.GetSimpleValue());
    NS_LOG_LOGIC("isTag="<<isTag<<"tagVal="<<srcNodeIndex);
    DH dh;
    dh.AccessGroupParameters().Initialize(p, q, g);
    SecByteBlock sharedKey(ApplicationUtil::getInstance()->getDhAgreedLength());

    dh.Agree(sharedKey, ApplicationUtil::getInstance()->getPrivateKeyFromMap(recNodeIndex),pubKey);

    ApplicationUtil::getInstance()->putSecretKeyInGlobalMap(recNodeIndex,srcNodeIndex,sharedKey);

    publicKeyCounter--;
    NS_LOG_INFO("Public key counter :"<< publicKeyCounter);
    //socket->Close()
	if(publicKeyCounter%250 ==0)
		std::cout << "public keys need to be received " <<  publicKeyCounter <<"\n";
    if(publicKeyCounter == 0)
    {
        NS_LOG_LOGIC("Debug : calling simulator loop");
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
        double jitter = uv->GetValue(0.01,2);
        Simulator::Schedule (Seconds(jitter),&GenerateCoinFlips, tid,c,ipInterfaceContainer);
    }


}

void ReceivePublicKeyCallback(Ptr<Socket> socket,const Address& address) {
    NS_LOG_LOGIC("receivepublickeycallback");
    socket->SetRecvCallback(MakeCallback(&ReceivePublicKey));
}

static void SendPublicKey (SecByteBlock pub, int index1, int index2)
{
    NS_LOG_LOGIC("*******************");
    NS_LOG_LOGIC("Debug : Inside dcnet send public key index1="<<index1<<" index2="<<index2);
    Ptr<Packet> sendPacket = Create<Packet> ((uint8_t*)pub.BytePtr(),(uint8_t) pub.SizeInBytes());
	MyTag sendTag;
    sendTag.SetSimpleValue(index1);
    NS_LOG_LOGIC("sendTagVal="<<int(sendTag.GetSimpleValue()));
	if(protocol=="udp")
    	sendPacket->AddPacketTag(sendTag);
	else
		sendPacket->AddByteTag(sendTag);
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    int randomport = 9803;
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
    retval = sourceNodeSocket->Send(sendPacket);
    NS_LOG_LOGIC("sourcenode send="<<retval);
    stage1SentPacketCount += 1;//increment sent packet counter for stage1
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
		Simulator::Stop (Seconds(3.0));
        DisplayMeasurements();
		return;
    }

    //Symmetric key generation
    for(int ind =0 ; ind < (int)numNodes; ind++)
    {
        generateKeys(ind);
    }
    std::cout << "symmetric key generation completed\n";
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
				double jitter;
				if(protocol=="tcp")
                	jitter = uv->GetValue(10,40);
				else
					jitter = uv->GetValue(5,30);
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
    cmd.AddValue ("protocol", "Protocol to be used", protocol);
	cmd.AddValue ("topology", "topology to be used", topology);
    cmd.AddValue ("option", "Changing numnodes or messagelength", option);
    //cmd.AddValue ("sender", "Sender of the message (actually anonymous)", sender);

    cmd.Parse (argc, argv);
    // Convert to time object
    //Time interPacketInterval = Seconds (interval);
	std::cout << "topology to be used:" << topology << "\n";
	std::cout << "protocol to be used:" << protocol << "\n";
	std::cout << "number of to be used:" << numNodes << "\n";
	std::cout << "Message  to be sent:" << Message << "\n";
    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
 //   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("6000"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                        StringValue (phyMode));
     Config::SetDefault ("ns3::TcpSocket::SegmentSize",UintegerValue(1448));
	 Config::SetDefault ("ns3::TcpSocket::RcvBufSize",UintegerValue(900000));

	 Config::SetDefault ("ns3::TcpSocket::SndBufSize",UintegerValue(900000));

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
   // wifiPhy.Set ("RxGain", DoubleValue (0) );
	wifiPhy.Set("EnergyDetectionThreshold",DoubleValue(-74 )); 
	wifiPhy.Set("CcaMode1Threshold",DoubleValue(-74 )); 
	wifiPhy.Set("TxGain", DoubleValue (5));
    wifiPhy.Set("RxGain", DoubleValue (4));
    wifiPhy.Set("TxPowerLevels", UintegerValue (1));
    wifiPhy.Set("TxPowerEnd", DoubleValue (35.0)); 
    wifiPhy.Set("TxPowerStart", DoubleValue (35.0)); 
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
	// setup the grid itself: objects are layed out
    // started from (-100,-100) with 20 objects per row, 
    // the x interval between each object is 5 meters
    // and the y interval between each object is 20 meters
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (-100.0),
                                     "MinY", DoubleValue (-100.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (20.0),
                                     "GridWidth", UintegerValue (20),
                                     "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (c);

     OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper list;
    //list.Add (staticRouting, 0);
    list.Add(olsr,0);


    InternetStackHelper internet;
    internet.SetRoutingHelper (list); // has effect on the next Install ()
    internet.Install (c);

    Ipv4AddressHelper ipv4;
    NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    ipInterfaceContainer = ipv4.Assign (devices);
    if(protocol=="udp")
   		tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	else
     	tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
   	if(topology=="fullconnected")
		announcementPacketCounter = (numNodes * numNodes) - numNodes;
	else
		announcementPacketCounter=numNodes-1;
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
	AnimationInterface anim("dcnet_net_anim_ring.xml");
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
    monitor->SerializeToXmlFile("dcnet_ring.flowmon",true,true);
    Simulator::Destroy ();


    return 0;
}

