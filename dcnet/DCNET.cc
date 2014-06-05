#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/aodv-helper.h"
#include "MyTag.h"
#include "ns3/nstime.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>

#include <iomanip>
#include "DCNET.h"
#include <unistd.h>
Integer p("0xB10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C6"
		"9A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C0"
		"13ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD70"
		"98488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0"
		"A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708"
		"DF1FB2BC2E4A4371");

Integer g("0xA4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507F"
		"D6406CFF14266D31266FEA1E5C41564B777E690F5504F213"
		"160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1"
		"909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28A"
		"D662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24"
		"855E6EEB22B3B2E5");

Integer q("0xF518AA8781A8DF278ABA4E7D64B7CB9D49462353");
int aesKeyLength = SHA256::DIGESTSIZE;
byte AESiv[AES::BLOCKSIZE];
SecByteBlock AESkey(0x00, AES::DEFAULT_KEYLENGTH);
AutoSeededRandomPool rnd;
byte iv[AES::BLOCKSIZE];

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DCNETApplication");


TypeId DCNET::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DCNET")
    .SetParent<Application> ()
    .AddConstructor<DCNET> ()
	.AddAttribute ("Port", "Port on which we listen/broadcast packets.",
                   UintegerValue (9999),
                   MakeUintegerAccessor (&DCNET::m_port),
                   MakeUintegerChecker<uint16_t> ())

	.AddAttribute ("MasterNode", "Setting whether or node is master node.",
                  	BooleanValue (0),
                    MakeBooleanAccessor (&DCNET::m_is_master_node),
                    MakeBooleanChecker ())

	.AddAttribute ("Message", "Message to be Sent",
                  	StringValue("101"),
                    MakeStringAccessor (&DCNET::m_message),
                    MakeStringChecker ())

	.AddAttribute ("Message Length", "Message Length to be Sent",
                  	  UintegerValue (3),
                   MakeUintegerAccessor (&DCNET::m_message_length),
                   MakeUintegerChecker<uint16_t> ())

	
  ;
  return tid;
}

DCNET::DCNET ()
{

  NS_LOG_FUNCTION (this);
}

DCNET::~DCNET()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
DCNET::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void DCNET::SendPublicKeys(Ptr<Packet> packet, int nodeid)
{
	 ns3::Ipv4Address addr= m_nodeid_nodeaddr[nodeid];
	 InetSocketAddress remoteSocket = InetSocketAddress (addr, m_unicast_port);
	 m_unicast_socket->Connect(remoteSocket);
     int retval= m_unicast_socket->Send(packet);
	 NS_LOG_DEBUG("retval is" <<retval );
 }

void DCNET::SendConfirmation(bool isFirstMessage)
{
	if(isFirstMessage) {
		Ptr <Node> PtrNode = this->GetNode();
		Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> (); 
		Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);  
		Ipv4Address ipAddr = iaddr.GetLocal (); 
		m_nodeaddr_nodeid.insert(pair<ns3::Ipv4Address,int>(ipAddr,m_my_nodeid));
		m_nodeid_nodeaddr.insert(pair<int,ns3::Ipv4Address>(m_my_nodeid,ipAddr));
		
	}
	if(m_nodeaddr_nodeid.size() <3) {
		std::cout << "Not enough nodes participating in dcnet \n";
		NS_LOG_DEBUG("There are not enough nodes interested to participate in DCNET.So exiting simulation\n");
		NS_LOG_DEBUG("my node id is" <<m_my_nodeid );
		
	}
    else 
    {
		
			m_master_nodeid=m_my_nodeid;
			m_num_pub_keys_confirm=m_nodeaddr_nodeid.size()-1;
			m_num_coin_ex_confirm=m_nodeaddr_nodeid.size()-2;
			m_rank=m_my_nodeid;
			m_prng_str_recv=m_nodeaddr_nodeid.size()-1;
			std::ostringstream ss;
			int node_id;
			--m_num_message_repeat;
		    ss << "Start DCNET protocol among nodes-";
		    for (std::map<ns3::Ipv4Address,int>::iterator it = m_nodeaddr_nodeid.begin() ; it != m_nodeaddr_nodeid.end(); ++it){
				node_id=it->second;
				ss << it->first<<":" << it->second <<",";
				if((m_immediate_neighbor < node_id) && (m_immediate_neighbor== m_my_nodeid )) m_immediate_neighbor=node_id;
				if(m_my_start_cluster_node > node_id) m_my_start_cluster_node=node_id;
				if(m_my_nodeid >node_id) {
					m_rank=m_rank+1;
				}
			
		    }
			//std::cout << "myimmediate neighbor id is " << m_immediate_neighbor   << "\n";
			//std::cout << "m_my_start_cluster_node is " << m_my_start_cluster_node  << "\n";
			//std::cout << "my rank is " << m_rank <<"\n";
		    std::string message=ss.str();
		    message.erase(message.find_last_not_of(" \n\r\t")+1);
			message.erase(message.find_last_not_of(",")+1);
		   // NS_LOG_DEBUG("message to be sent is " << message << "\n");
			std::cout << "sending dcnet protocol start message by master node " << m_my_nodeid << "with message " << message << "\n";
			Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
			MyTag sendTag;
			sendTag.SetSimpleValue(MESSAGE_DCNET_INITIATE);
			sendPacket->AddPacketTag(sendTag);
			int retval=m_socket->Send(sendPacket);
			NS_LOG_DEBUG("retval from broad cast message to dcnet initiate is" << retval);
		    GeneratePublickeys();
			GenerateCoinFlips();
	}
}

void DCNET::SendDCNETQueryMessage() {
	std::string message = "200";
	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue(m_my_nodeid);
	sendPacket->AddPacketTag(sendTag);
	int retval= m_socket->Send(sendPacket);
	NS_LOG_DEBUG("retval from broad cast message to dcnet Query is" << retval);
	m_is_master_node=true;
}

void 
DCNET::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
  	m_my_nodeid=GetNode()->GetId();
  	m_unicast_port=9908;
	m_immediate_neighbor=m_my_nodeid;
	m_my_start_cluster_node = m_my_nodeid;
	m_topology="fullyconnected";
	m_rank=1;
	m_dcnet_start=false;
	m_is_master_node=false;
	m_num_message_repeat=2;
	NS_LOG_DEBUG("master node is " << m_is_master_node << "\n");  
	//std::cout << "my node is " << m_my_nodeid << "and my master node is set to " << m_is_master_node << "\n";
  	if (m_socket == 0) {
  		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		m_unicast_socket = Socket::CreateSocket (GetNode (), tid);
		 //unicast port
    	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_unicast_port);
		m_unicast_socket->Bind (local);	
    	m_unicast_socket->Listen();
    	m_unicast_socket->SetRecvCallback (MakeCallback (&DCNET::HandleUnicastRead, this));
  	}	
  	m_message_length=m_message.size();
	//std::cout <<"message to be send anonymously is" << m_message <<"\n";
  	m_time_wait_dcnet=15.0;
	InetSocketAddress remoteSocket =InetSocketAddress (Ipv4Address::GetBroadcast (), m_port);
    m_socket->SetAllowBroadcast (true);
	m_socket->Connect (remoteSocket);
	m_socket->SetRecvCallback (MakeCallback (&DCNET::HandleBroadcastRead,this));
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
    //need to listen?
   	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
	m_socket->Bind (local);	
	m_socket->Listen();
	double jitter = uv->GetValue(3,8);
    //std::cout << "scheduling to send DCNETQuery message in" << jitter << "seconds from node" << m_my_nodeid <<"\n";
    m_sendBroadcastEvent=Simulator::Schedule (Seconds(jitter),&DCNET::SendDCNETQueryMessage,this);
	//NS_LOG_DEBUG("retval  is" << retval);
    m_sendConfirmationEvent= Simulator::Schedule (Seconds(m_time_wait_dcnet),&DCNET::SendConfirmation,this,true);
}


void 
DCNET::StopApplication ()
{
	NS_LOG_FUNCTION (this);
  	if (m_socket != 0) {
		m_socket->Close ();
      	m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	if (m_unicast_socket != 0) {
		m_unicast_socket->Close();
		m_unicast_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	NS_LOG_DEBUG("DCNET Application has been stopped");
}



void DCNET::split(std::map<ns3::Ipv4Address,int> &nodeaddr_id, std::map<int,ns3::Ipv4Address> &nodeid_addr,const std::string &text, char sep)
{
	unsigned start = 0, end = 0;
  	int node_id;
  	std::string node_id_addr;
  	unsigned pos=0;
  	while (text.find(sep, start) != std::string::npos) {
		end = text.find(sep, start);
		node_id_addr=text.substr(start, end - start);
	 	NS_LOG_DEBUG("node_id_addr" << node_id_addr << "\n");
	 	pos=node_id_addr.find(':');
    	// std::cout << "pos is" << pos << "\n";
	 	std::string add= node_id_addr.substr(0,pos);
	 	std::istringstream ( node_id_addr.substr(pos+1) ) >> node_id;
       // std::cout << "my immediate_neighbor is " << m_immediate_neighbor << "\n";
        NS_LOG_DEBUG("currently processing node id " << node_id <<"\n");
		NS_LOG_DEBUG("I am node " <<m_my_nodeid << "\n");
		//std::cout << "my start cluster node is " << m_my_start_cluster_node  << "\n";
	    if((m_immediate_neighbor < node_id) && (m_immediate_neighbor== m_my_nodeid )) m_immediate_neighbor=node_id;
	    if(m_my_start_cluster_node > node_id) m_my_start_cluster_node=node_id;
		//if(m_master_nodeid <node_id )m_master_nodeid= node_id;
		if(m_my_nodeid >node_id) {
			m_rank=m_rank+1;
			
		}
	 	NS_LOG_DEBUG("node is" <<  m_my_nodeid << "address is" << add << "\n");
	 	Ipv4Address addr(add.c_str() );
    	nodeaddr_id.insert(pair<ns3::Ipv4Address,int>(addr,node_id));
    	nodeid_addr.insert(pair<int,ns3::Ipv4Address>(node_id,addr));
     	start = end + 1;
  	}
  	node_id_addr=text.substr(start);
  	pos=node_id_addr.find(':');
  	std::string add= node_id_addr.substr(0,pos);
  	std::istringstream ( node_id_addr.substr(pos+1) ) >> node_id;
	if((m_immediate_neighbor < node_id) && (m_immediate_neighbor== m_my_nodeid )) m_immediate_neighbor=node_id;
	//if(m_master_nodeid <node_id )m_master_nodeid= node_id;
	if(m_my_start_cluster_node > node_id) m_my_start_cluster_node=node_id;
  	NS_LOG_DEBUG("node is" <<  m_my_nodeid << "address is" << add << "\n");
   	Ipv4Address addr(add.c_str() );
    nodeaddr_id.insert(pair<ns3::Ipv4Address,int>(addr,node_id));
    nodeid_addr.insert(pair<int,ns3::Ipv4Address>(node_id,addr));
}

void DCNET::generateKeys(int index)
{
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
    SecByteBlock priv(dh.PrivateKeyLength());
    SecByteBlock pub(dh.PublicKeyLength());
    dh.GenerateKeyPair(rnd, priv, pub);
    m_privateKeyMap.insert(pair<int,SecByteBlock>(index,priv));
    m_publicKeyMap.insert(pair<int,SecByteBlock>(index,pub));
    m_dhAgreedLength=dh.AgreedValueLength();
	m_publickey_gen=true;
}

void DCNET::GeneratePublickeys()
{

	if(!m_publickey_gen)
		generateKeys(m_my_nodeid);
    //else std::cout << "public key for node " <<m_my_nodeid << "is in cache " << "\n";
	SecByteBlock pub= m_publicKeyMap[m_my_nodeid];
	Ptr<Packet> sendPacket = Create<Packet> ((uint8_t*)pub.BytePtr(),(uint8_t) pub.SizeInBytes());
	MyTag sendTag;
	sendTag.SetSimpleValue(MESSAGE_DCNET_PUBLIC_KEYS);
	sendPacket->AddPacketTag(sendTag);
	for (std::map<int,ns3::Ipv4Address>::iterator it = m_nodeid_nodeaddr.begin() ; it != m_nodeid_nodeaddr.end(); ++it){
		int neighbor_nodeid= it->first ;
		if(m_my_nodeid !=neighbor_nodeid) {
			if(std::find(m_public_keys_neighbors.begin(), m_public_keys_neighbors.end(), neighbor_nodeid) == m_public_keys_neighbors.end()) 			{	 m_public_keys_neighbors.push_back(neighbor_nodeid);
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(10,30);
				NS_LOG_DEBUG("Sending public key between node " <<m_my_nodeid << "and node "<< neighbor_nodeid);
				Simulator::Schedule(Seconds(jitter),&DCNET::SendPublicKeys,this,sendPacket, neighbor_nodeid);
		    }
        }
    }
	
}
int randomBitGeneratorWithProb()
{
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	int rand = uv->GetInteger(0,1);
    return rand;
}

std::string PseudoRandomGenerator(unsigned int SEEDSIZE)
{
	NS_LOG_DEBUG("seed size is" << SEEDSIZE << "\n");
	std::string result;
    std::ostringstream convert;
   	for(unsigned int i=0; i<SEEDSIZE; i++) {
   		int randomBit = randomBitGeneratorWithProb();
       	convert << randomBit;
    }
	result = convert.str();
	return result;
}
void DCNET::SendAnnouncementForRing(std::string result, int immediate_neighbor)
{
	std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
	sendTag.SetSimpleValue(MESSAGE_DCNET_ANOUNCEMENT);
	sendPacket->AddPacketTag(sendTag);
	InetSocketAddress remoteAddress = InetSocketAddress (m_nodeid_nodeaddr[immediate_neighbor],m_unicast_port);
	m_unicast_socket->Connect(remoteAddress);
    m_unicast_socket->Send(sendPacket);
}
void DCNET::SendAnnouncementForFullyConnected(std::string result,int neighborid)
{
	NS_LOG_DEBUG("InSendAnnouncementForFullyConnected\n");
	std::string message = result;
    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
	sendTag.SetSimpleValue(MESSAGE_DCNET_ANOUNCEMENT);
	sendPacket->AddPacketTag(sendTag);
	InetSocketAddress remoteAddress = InetSocketAddress (m_nodeid_nodeaddr[neighborid],m_unicast_port);
	m_unicast_socket->Connect(remoteAddress);
    int retval=m_unicast_socket->Send(sendPacket);
	NS_LOG_DEBUG("retval is " <<retval );
   // std::cout <<"retval is " <<retval << "\n";
}

void DCNET::GenerateAnnouncementsForFullyConnected()
{
	std::ostringstream ss;
	int bit;
	for(unsigned  round = 0; round <m_message_length ; round++) {
		if(m_is_master_node) {
			NS_LOG_DEBUG("message to be send is " << m_message );
			bit =  m_message.at(round)-48 ;
			NS_LOG_DEBUG("bit from master node is" << bit );
        }
		int result = 0;
		std::string prgsecretstring;
        for (map<int,std::string>::iterator it=m_prngstringMap.begin(); it!=m_prngstringMap.end(); ++it) {
			//get the adjacent prg string stored in the map
		    prgsecretstring =  (std::string)it->second;
			//std::cout <<"prgsecretstring is " << prgsecretstring << "\n";
            result ^= (prgsecretstring.at(round) - 48);
			//break;
		}
		if(m_is_master_node) {	//exor result with message
			result ^= bit;
		}
		ss << result;
	}
	m_announcementMap.insert(pair<int,std::string>(m_my_nodeid,ss.str() ));
    for (std::map<int,ns3::Ipv4Address>::iterator it=m_nodeid_nodeaddr.begin(); it!=m_nodeid_nodeaddr.end(); ++it) {
		int neighborid=it->first;
		if(m_my_nodeid!=neighborid) {
			NS_LOG_DEBUG("Sending announcement for node " << neighborid << "from node " << m_my_nodeid << "\n");
			NS_LOG_DEBUG("Message is " << ss.str() <<"\n");
			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
			double jitter = uv->GetValue(10,40);
			Simulator::Schedule (Seconds(jitter),&DCNET::SendAnnouncementForFullyConnected,this,ss.str(),neighborid);
		}
	}
		
}

void DCNET::GenerateAnnouncementsForRing(std::string previous_result)
{
	NS_LOG_DEBUG("Currently generating anouncement for node " << m_my_nodeid );
	//std::cout <<"Currently generating anouncement for node " << m_my_nodeid <<"\n" ;
	std::ostringstream ss;
	int bit;
   // std::cout << "previous result is " << previous_result << "and its size is " <<previous_result.size() <<"\n";
	//std::cout << "message length is " << m_message_length << "\n";
    for(unsigned round = 0; round < m_message_length ; round++) {
		if(m_is_master_node) {
			//std::cout << "m_is_masternode is " << m_is_master_node << "for node " << m_my_nodeid << "\n";
			//std::cout <<"message to be send is " << m_message <<"\n";
			NS_LOG_DEBUG("message to be send is " << m_message );
			bit =  m_message.at(round)-48 ;
			NS_LOG_DEBUG("bit from master node is" << bit );
			//std::cout <<"bit from master node is" << bit <<"\n";
        }
		int result=0;
		if(previous_result.size()> 0)
			result =0^(previous_result.at(round) - 48) ;
		std::string prgsecretstring;
		for (std::map<int,std::string>::iterator it=m_prngstringMap.begin(); it!=m_prngstringMap.end(); ++it) {
			//get the adjacent prg string stored in the map
		    prgsecretstring =  (std::string)it->second;
			NS_LOG_DEBUG("prgstring for node " <<m_my_nodeid << "is " << prgsecretstring << "\n");
			//std::cout << "prgstring for node " <<m_my_nodeid << "is " << prgsecretstring << "for round " <<round <<"\n";
			NS_LOG_DEBUG("prgstring for node " <<m_my_nodeid << "is " << prgsecretstring );
            result ^= (prgsecretstring.at(round) - 48);
				//break;
		}
        if(m_is_master_node)
			result ^= bit;
		NS_LOG_DEBUG("message to be send by node 0 is" << result << "\n");
		NS_LOG_DEBUG("message to be send by node 0 is" << result  );
		ss << result;
    }
    if(m_immediate_neighbor!=m_my_nodeid) {
		//std::cout << "sending the anouncement for the node " << m_immediate_neighbor << " and annoucement is" << ss.str()<<"\n";
		NS_LOG_DEBUG("sending the anouncement for the node " << m_immediate_neighbor << " and annoucement is" << ss.str()<<"\n");
		NS_LOG_DEBUG("sending the anouncement for the node " << m_immediate_neighbor << " and annoucement is" << ss.str() );
		Simulator::Schedule (Seconds(0.01),&DCNET::SendAnnouncementForRing,this,ss.str(),m_immediate_neighbor);
	}
	else {
        m_final_message=ss.str();
		Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)m_final_message.c_str(),m_final_message.size());
    	MyTag sendTag;
 		sendTag.SetSimpleValue(MESSAGE_DCNET_FINAL_ANOUNCEMENT);
   		sendPacket->AddPacketTag(sendTag);
	    m_socket->Send(sendPacket);
        std::cout << "Message " << ss.str() <<"will be broadcasted by node " << m_my_nodeid <<"\n";
		NS_LOG_DEBUG("Message " << ss.str() <<"will be broadcasted by node " << m_my_nodeid );
		m_dcnet_start=false;

		
      //		StopApplication();
	}

}

void DCNET::SendCoinFlips(std::string message,int neighbor_node)
{
	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
	int retval;
    sendTag.SetSimpleValue(MESSAGE_DCNET_PRNG_STRING);
   	sendPacket->AddPacketTag(sendTag);
	//std::cout << "sending coin flip for neighbor_node" <<neighbor_node <<"with ipaddress " <<m_nodeid_nodeaddr[neighbor_node] <<"\n";
	InetSocketAddress remoteSocket =  InetSocketAddress (m_nodeid_nodeaddr[neighbor_node],m_unicast_port);
    retval = m_unicast_socket->Connect (remoteSocket);
    NS_LOG_LOGIC("sourcenode connect="<<retval);
	//std::cout << "retval for send coin flips is " <<retval << "\n";
    m_unicast_socket->Send(sendPacket);
}
void DCNET::ReceiveCoinFlip (Ptr<Packet> recPacket,Address from)
{
	int srcNodeId =  m_nodeaddr_nodeid[InetSocketAddress::ConvertFrom(from).GetIpv4 ()];
    uint8_t *buffer = new uint8_t[recPacket->GetSize()+1];
    recPacket->CopyData(buffer,recPacket->GetSize());
    std::string recMessage = std::string((char*)buffer);
    std::string dec=recMessage.substr(0,m_message_length);
	
	//std::cout << "received coin flip on node " << m_my_nodeid << "with coin flip" <<dec <<"\n";
  
	//std ::cout <<"size of m_prngstringMap is "<<m_prngstringMap.size() <<"for node " <<m_my_nodeid << "\n";
	//std::string decrypted, dec;

	/*recMessage = recMessage.substr (0,m_message_length);
	
    SecByteBlock key(SHA256::DIGESTSIZE);
    SHA256().CalculateDigest(key, m_sharedKeyMap[srcNodeId],m_sharedKeyMap[srcNodeId].size());
   
	CFB_Mode<AES>::Decryption cfbDecryption;
	cfbDecryption.SetKeyWithIV( key, AESkey.size(), AESiv );
	StringSource ss( recMessage, true, new StreamTransformationFilter( cfbDecryption,new StringSink( dec ))); 
	NS_LOG_DEBUG("decrypted key from node " << srcNodeId << "on node " << m_my_nodeid << "is " << dec <<"\n";*/
	NS_LOG_DEBUG("recvd message size is " <<dec.size() <<"\n");
	NS_LOG_DEBUG("max  message size is " << m_message_length <<"\n");
	if(dec.size() < m_message_length)
    {
		std::cout << "decrypted key from node " << srcNodeId << "on node " << m_my_nodeid << "is " << dec <<"\n";
		std::cout << "recvd message size is " <<dec.size() <<"\n";
		std::cout << "max  message size is " << m_message_length <<"\n";
		std::cout << "resending request for prng string\n";
		std::string message="Resend prg string";
	    Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    	MyTag sendTag;
 		sendTag.SetSimpleValue(MESSAGE_DCNET_RE_PRNG_STRING);
   		sendPacket->AddPacketTag(sendTag);
		InetSocketAddress remoteAddress = InetSocketAddress (m_nodeid_nodeaddr[srcNodeId],m_unicast_port);
		m_unicast_socket->Connect(remoteAddress);
    	m_unicast_socket->Send(sendPacket);
	}
    else {
	    NS_LOG_DEBUG("number prng to be received is " << m_prng_str_recv <<"for node " <<m_my_nodeid);
		NS_LOG_DEBUG("number prng to be received is " << m_prng_str_recv <<"for node " <<m_my_nodeid );
		m_prngstringMap.insert(pair<int,std::string>(srcNodeId,dec));
		--m_prng_str_recv;
	}
	delete buffer;
	//if(m_prng_str_recv%50==0)
	NS_LOG_DEBUG("number of prng_to_be recvd is " << m_prng_str_recv  <<"on node " << m_my_nodeid << "\n");
	NS_LOG_DEBUG("number of prng_to_be recvd is " << m_prng_str_recv  <<"on node " << m_my_nodeid );
	//std::cout << "number of prng_to_be recvd is " << m_prng_str_recv  <<"on node " << m_my_nodeid << "\n";
  	if(m_prng_str_recv == 0)
    {
		NS_LOG_DEBUG("number of prng_to_be recvd is " << m_prng_str_recv  <<"on node " << m_my_nodeid );
		NS_LOG_DEBUG("number of prng_to_be recvd is " << m_prng_str_recv  <<"on node " << m_my_nodeid << "\n");
		if(!m_is_master_node) {
			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
			double jitter = uv->GetValue(0.1,0.6);
            Simulator::Schedule(Seconds(jitter),&DCNET::SendPrngRecvdToMaster,this);
	 		//SendPrngRecvdToMaster();
		}
	}
}
void DCNET::ReSendCoinFlip(Ptr<Packet> packet,Address from)
{
	int neighbor_nodeid= m_nodeaddr_nodeid[ InetSocketAddress::ConvertFrom (from).GetIpv4 () ];
	std::string prgString = m_prngstringMap[neighbor_nodeid];
	NS_LOG_DEBUG("re- sending prg string from node" << m_my_nodeid << "to node " << neighbor_nodeid <<"is " << prgString );
    //std::cout <<"re- sending prg string from node" << m_my_nodeid << "to node " << neighbor_nodeid <<"is " << prgString << "\n";
	m_prngstringMap.insert(pair<int,std::string>(neighbor_nodeid,prgString));
	// Calculate a SHA-256 hash over the Diffie-Hellman session key
    SecByteBlock key(SHA256::DIGESTSIZE);
    SHA256().CalculateDigest(key, m_sharedKeyMap[neighbor_nodeid],m_sharedKeyMap[neighbor_nodeid].size());
    std::string message = prgString;
    CFB_Mode<AES>::Encryption cfbEncryption;
	cfbEncryption.SetKeyWithIV( key, AESkey.size(), AESiv );
	std::string encrypted, enc;
	StringSource( message, true, new StreamTransformationFilter( cfbEncryption,new StringSink( encrypted )));
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	double jitter = uv->GetValue(1,20);
    Simulator::Schedule(Seconds(jitter),&DCNET::SendCoinFlips,this,encrypted,neighbor_nodeid);	
	
}
		
void DCNET::GenerateCoinFlips()
{
	rnd.GenerateBlock(iv, AES::BLOCKSIZE);
	for (std::map<int,ns3::Ipv4Address>::iterator it = m_nodeid_nodeaddr.begin() ; it != m_nodeid_nodeaddr.end(); ++it){
		int neighbor_nodeid= it->first ;
		
		if(m_my_nodeid <neighbor_nodeid) {
			std::string prgString = PseudoRandomGenerator(m_message_length);
			std::string encrypted=prgString;
            //std::cout <<"sending prg string from node" << m_my_nodeid << "to node " << neighbor_nodeid <<"is " << prgString << "\n";
			m_prngstringMap.insert(pair<int,std::string>(neighbor_nodeid,prgString));
			// Calculate a SHA-256 hash over the Diffie-Hellman session key
           /* SecByteBlock key(SHA256::DIGESTSIZE);
            SHA256().CalculateDigest(key, m_sharedKeyMap[neighbor_nodeid],m_sharedKeyMap[neighbor_nodeid].size());
            std::string message = prgString;
            CFB_Mode<AES>::Encryption cfbEncryption;
			cfbEncryption.SetKeyWithIV( key, AESkey.size(), AESiv );
			std::string encrypted, enc;
			StringSource( message, true, new StreamTransformationFilter( cfbEncryption,new StringSink( encrypted )));*/

			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
			double jitter = uv->GetValue(1,10);
            Simulator::Schedule(Seconds(jitter),&DCNET::SendCoinFlips,this,encrypted,neighbor_nodeid);	
		}
    }
}

void DCNET::ReceivePublicKeys(Ptr<Packet> packet,Address from,uint8_t *buffer )
{
	SecByteBlock pubKey((byte *)buffer,packet->GetSize());
	//int recv_node= m_nodeaddr_nodeid[InetSocketAddress::ConvertFrom(from).GetIpv4 ()];
	//NS_LOG_DEBUG("received public key from node " <<  recv_node << " to node " << nodeid <<"\n");
    DH dh;
    dh.AccessGroupParameters().Initialize(p, q, g);
    SecByteBlock sharedKey(m_dhAgreedLength);
    dh.Agree(sharedKey, m_privateKeyMap[m_my_nodeid],pubKey);
    int srcNodeID= m_nodeaddr_nodeid[ InetSocketAddress::ConvertFrom (from).GetIpv4 () ];
	m_sharedKeyMap.insert(pair<int,SecByteBlock>(srcNodeID,sharedKey));

}
void DCNET::BroadcastFlipMessage()
{
	NS_LOG_DEBUG("Exchange BroadcastFlipMessage\n" );
	std::string message="ExchangeCoinFlips";
	NS_LOG_DEBUG("Exchange BroadcastFlipMessage\n");
	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue( MESSAGE_DCNET_EX_COIN_FLIPS);
	sendPacket->AddPacketTag(sendTag);
    m_socket->Send(sendPacket);
}
void DCNET::BroadcastAnnouncementMessage()
{
	std::string message="ExchangeAnnouncement";
	NS_LOG_DEBUG("Exchange BroadcastAnnouncementMessage\n");
	NS_LOG_DEBUG("Exchange BroadcastAnnouncementMessage\n" );
  	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
    sendTag.SetSimpleValue( MESSAGE_DCNET_EX_ANNOUNCEMENTS);
	sendPacket->AddPacketTag(sendTag);
    m_socket->Send(sendPacket);
}
void DCNET::SendPublicKeyRecvdToMaster()
{
	int master_node_id= m_master_nodeid;
	
	//std::cout << "In SendPublicKeyRecvdToMaster :my masternodeid is" << master_node_id << "\n";
	std::string message="Rcvd_my_publicKeys";
	NS_LOG_DEBUG("Rcvd_my_publicKeys" );
	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
 	sendTag.SetSimpleValue(MESSAGE_DCNET_RECVD_PUB_KEYS);
   	sendPacket->AddPacketTag(sendTag);
	InetSocketAddress remoteAddress = InetSocketAddress (m_nodeid_nodeaddr[master_node_id],m_unicast_port);
	m_unicast_socket->Connect(remoteAddress);
   // m_unicast_socket->Send(sendPacket);
    //std::cout << "SendPublicKeyRecvdToMaster from node " << m_my_nodeid << "to node " << master_node_id << "\n";
}
void DCNET::SendPrngRecvdToMaster()
{
	int master_node_id= m_master_nodeid;
	//std::cout << "Sending SendPrngRecvdToMaster" <<"to my master " << master_node_id <<"\n";
	std::string message="Rcvd_my_coinflips";
	NS_LOG_DEBUG("Rcvd_my_coinflips" );
	Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)message.c_str(),message.size());
    MyTag sendTag;
 	sendTag.SetSimpleValue(MESSAGE_DCNET_RCVD_COIN_FLIPS);
   	sendPacket->AddPacketTag(sendTag);
	InetSocketAddress remoteAddress = InetSocketAddress (m_nodeid_nodeaddr[master_node_id],m_unicast_port);
	m_unicast_socket->Connect(remoteAddress);
    m_unicast_socket->Send(sendPacket);
   // std::cout << "SendPrngRecvdToMaster from node " << m_my_nodeid << "to node " << master_node_id << "\n";
}
void DCNET::CalculateFinalAnnouncement()
{
	//std::cout << "calling calculateFinalAnnouncement on node "<< m_my_nodeid << "\n";
	std::ostringstream ss;
	for(unsigned  round = 0; round <m_message_length ; round++) {
		int result = 0;
		std::string announcementstring;
		NS_LOG_DEBUG("Size of announcment map for node " << m_my_nodeid << "is" <<m_announcementMap.size() <<"\n");
        for (map<int,std::string>::iterator it=m_announcementMap.begin(); it!=m_announcementMap.end(); ++it) {
			 announcementstring =  (std::string)it->second;
			//std::cout <<"announcementstring is " << announcementstring << "\n";
            result ^= (announcementstring.at(round) - 48);
			//break;
		}
		ss << result;
    }
    std::cout << "Message received is " << ss.str() <<" on node " << m_my_nodeid << "\n";	
	m_announcementMap.clear();
	m_dcnet_start=false;
	
	//StopApplication();
	
}

void DCNET::HandleUnicastRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
  	Ptr<Packet> packet;
  	Address from;
  	while ((packet = socket->RecvFrom (from)))
  	{
		int len = packet->GetSize()+1;
      	uint8_t  *buffer = new uint8_t[len];
      	memset(buffer,0,len);
      	int copiedlen = packet->CopyData(buffer,packet->GetSize());
      	NS_LOG_LOGIC("copied len="<<copiedlen);
      	MyTag recTag;
      	packet->PeekPacketTag(recTag);
	  	int message_id =int(recTag.GetSimpleValue());
		if(message_id==MESSAGE_DCNET_PUBLIC_KEYS) {    //public keys message
			ReceivePublicKeys(packet,from,buffer);
			--m_pub_keys_recv;
			//if(m_pub_keys_recv%50==0)
			//std::cout << "number of public keys_to_be recvd is " << m_pub_keys_recv  <<"for node " << m_my_nodeid << "\n";
			NS_LOG_DEBUG("number of public keys_to_be recvd is " << m_pub_keys_recv  <<"for node " << m_my_nodeid  );
			NS_LOG_DEBUG("Yet to be Received public keys for node " << m_pub_keys_recv << "\n");
			if(m_pub_keys_recv==0) {
				NS_LOG_DEBUG("Received all public keys for node " << m_my_nodeid );
        		NS_LOG_DEBUG("Received all public keys for node " << m_my_nodeid << "\n");
				//std::cout << "sending Recvd AllPublicKeys" << "by node " << m_my_nodeid << "\n";
				// SendPublicKeyRecvdToMaster();
            }
	  	}
		if(message_id==MESSAGE_DCNET_PRNG_STRING) {
			ReceiveCoinFlip(packet,from);
		}
		if(message_id==MESSAGE_DCNET_RE_PRNG_STRING) {
			ReSendCoinFlip(packet,from);
		}
		if(message_id==MESSAGE_DCNET_RECVD_PUB_KEYS ) {
			--m_num_pub_keys_confirm;
			NS_LOG_DEBUG("current number of public keys confirmations needed " << m_num_pub_keys_confirm);
			if(m_num_pub_keys_confirm %20==0)
				std::cout << "current number of keys confirmationsneeded " << m_num_pub_keys_confirm <<"by node" << m_my_nodeid <<"\n";
			if(m_num_pub_keys_confirm==0) {
				std::cout << "Node " << m_my_nodeid << "is broad casting Exchange coin flips message " << "\n";
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(0.1,0.6);
            	//Simulator::Schedule(Seconds(jitter),&DCNET::BroadcastFlipMessage,this);
				//BroadcastFlipMessage();
				//GenerateCoinFlips();
				jitter = uv->GetValue(1,10);
            	Simulator::Schedule(Seconds(jitter),&DCNET::GenerateCoinFlips,this);
			}
		}
		if(message_id==MESSAGE_DCNET_RCVD_COIN_FLIPS ) {
			--m_num_coin_ex_confirm;
			NS_LOG_DEBUG("m_num_coin_ex_confirm " << m_num_coin_ex_confirm);
			//std::cout <<"m_num_coin_ex_confirm " << m_num_coin_ex_confirm << "for my node " << m_my_nodeid << "\n";
			if(m_num_coin_ex_confirm==0) {
				std::cout <<"m_num_coin_ex_confirm " << m_num_coin_ex_confirm << "for my node " << m_my_nodeid << "\n";
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(0.1,0.6);
            	Simulator::Schedule(Seconds(jitter),&DCNET::BroadcastAnnouncementMessage,this);
				//BroadcastAnnouncementMessage();
				if(m_topology=="fullyconnected") {
					std::cout << "Now Generating anouncements from master node\n";
					GenerateAnnouncementsForFullyConnected();
				}
				if(m_topology=="ring") {
					if(m_my_start_cluster_node==m_my_nodeid)
						GenerateAnnouncementsForRing("");
				}
			}
		}
		if(message_id==MESSAGE_DCNET_ANOUNCEMENT) {
			std::string recMessage = std::string((char*)buffer);
			//std::cout <<"received anouncement on node " << m_my_nodeid << "\n";
			NS_LOG_DEBUG("received anouncement on node " << m_my_nodeid);
			if(m_topology=="ring") {
				
				NS_LOG_DEBUG("recMessage " << recMessage);
				//std::cout <<"received anouncement " << recMessage <<"on node " << m_my_nodeid << "\n";
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(0.1,0.6);
            	Simulator::Schedule(Seconds(jitter),&DCNET::GenerateAnnouncementsForRing,this,recMessage);
				//GenerateAnnouncementsForRing(recMessage);
			}
			if(m_topology=="fullyconnected") {
				NS_LOG_DEBUG("In fully connected topology\n");
				NS_LOG_DEBUG("Announcement received is " << recMessage << "for node " << m_my_nodeid << "\n");
				int neighbor_nodeid=m_nodeaddr_nodeid[InetSocketAddress::ConvertFrom (from).GetIpv4 ()];
				m_announcementMap.insert(pair<int,std::string>( neighbor_nodeid,recMessage ));
				NS_LOG_DEBUG("current size of announcement map is " <<m_announcementMap.size() <<"for node " << m_my_nodeid <<"\n");
				//check if it received all its announcements
				if(m_announcementMap.size()==m_nodeaddr_nodeid.size()) {
					//calculate final announcement
					CalculateFinalAnnouncement();
					if(m_is_master_node) {
						std::cout <<"m_num_message_repeat" <<m_num_message_repeat <<"\n";
						if(m_num_message_repeat>0) {
							Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
							double jitter = uv->GetValue(1,6);
							Simulator::Schedule(Seconds(jitter),&DCNET::SendConfirmation,this,true);
						}
					}
				}
			}

		}
		
    }
}

void SendPacket(Ptr<Socket> socket,Ptr<Packet> packet,Address from)
{
	socket->SendTo (packet, 0, from);
}
void DCNET::HandleBroadcastRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
  	Address from;
  	Ptr<Node> recvnode = socket->GetNode();
  	//int nodeid= recvnode->GetId();
  	while ((packet = socket->RecvFrom (from)))
  	{
		int len=packet->GetSize()+1;
    	uint8_t *buffer = new uint8_t[len];
		memset(buffer,0,len);
		int copiedlen = packet->CopyData(buffer,packet->GetSize());
    	NS_LOG_LOGIC("copied len="<<copiedlen);
      	MyTag recTag;
      	packet->PeekPacketTag(recTag);
	  	int messageID =int(recTag.GetSimpleValue());
      	std::string recMessage = std::string((char*)buffer);
		//recMessage = recMessage.substr (0,recMessage.size());
	  	delete buffer;
	  	if(recMessage=="200") {      //DCNET Interest message
			NS_LOG_DEBUG("received DCNET query message " << recMessage << " for node " << m_my_nodeid <<"\n");
			NS_LOG_DEBUG("received DCNET query message " << recMessage << " for node " << m_my_nodeid );
			if((! m_accept_dcnet) &&(!m_is_master_node) )  {
				Simulator::Cancel (m_sendBroadcastEvent); //Cancel dcnet broad cast message
				Simulator::Cancel( m_sendConfirmationEvent); //Cancel dcnet broad cast message
				m_accept_dcnet=true;
				m_is_master_node=false;
				packet->RemoveAllPacketTags ();
  				packet->RemoveAllByteTags ();
  				MyTag sendTag;
				std::ostringstream ss;
				ss << messageID;
				m_master_nodeid=messageID;
				std::string DCNET_accept_message= ss.str();
				//std::cout << "dcnet message to be sent is " << DCNET_accept_message << "\n";
				Ptr<Packet> sendPacket =Create<Packet> ((uint8_t*)DCNET_accept_message.c_str(),DCNET_accept_message.size());
				sendTag.SetSimpleValue(m_my_nodeid);
	    		sendPacket->AddPacketTag(sendTag);
				Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
				double jitter = uv->GetValue(0.1,0.6);
				//std::cout << "sending dcnet accpetance message from node" << m_my_nodeid <<"to node" << messageID << "\n";
				Simulator::Schedule(Seconds(jitter),&SendPacket,socket,sendPacket, from);
			}
	  	}
	  	if(messageID==MESSAGE_DCNET_INITIATE) {    //Start dcnet protocol by exchanging pub keys
			if(! m_dcnet_start) {
				unsigned pos = recMessage.find("-");
				std::string dcnet_nodeids = recMessage.substr(pos+1);
				//std::cout << "my cluster node id is " << m_my_start_cluster_node << "\n";
				split(m_nodeaddr_nodeid,m_nodeid_nodeaddr, dcnet_nodeids, ',');
		        if(m_nodeid_nodeaddr.find(m_my_nodeid)!= m_nodeid_nodeaddr.end() ) {
					NS_LOG_DEBUG(recMessage << " on node" << m_my_nodeid );
					m_is_master_node=false;
					//std::cout << "my_m_master_node is " << m_is_master_node << "on node " << m_my_nodeid << "\n";
					m_pub_keys_recv=m_nodeaddr_nodeid.size()-1;
				    NS_LOG_DEBUG("my node id is " << m_my_nodeid << " and number of public keys to be recvd is "  << m_pub_keys_recv);
					//std::cout << "my node id is " << m_my_nodeid << "and rank  is " << m_rank<<"\n";
					//std::cout << "my node id is " << m_my_nodeid << "and immediate neighbor  is " << m_immediate_neighbor<<"\n";
					//std::cout << "my node id is " << m_my_nodeid << "and starting cluser node is " <<m_my_start_cluster_node<<"\n";
					m_prng_str_recv= m_my_nodeid- m_my_start_cluster_node;
					//std::cout << "my node id is " << m_my_nodeid << "and number of prng  required is " << m_prng_str_recv <<"\n";
					NS_LOG_DEBUG("my node id is " << m_my_nodeid << " and my next immediate neighbor is "  << m_immediate_neighbor << "\n");
					NS_LOG_DEBUG("my node id is " << m_my_nodeid << " and my next immediate neighbor is "  << m_immediate_neighbor);
					m_accept_dcnet=true;
					m_dcnet_start=true;
					Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
					double jitter = uv->GetValue(0.1,0.6);
				   	Simulator::Schedule(Seconds(jitter),&DCNET::GeneratePublickeys,this);
					jitter = uv->GetValue(0.1,0.6);
					std::cout << "Generating coin flips for node" <<m_my_nodeid <<"\n";
					Simulator::Schedule(Seconds(jitter),&DCNET::GenerateCoinFlips,this);
					
				}
				else {
					m_immediate_neighbor=m_my_nodeid;
					m_my_start_cluster_node = m_my_nodeid;
					m_rank=1;
					m_accept_dcnet=false;
					m_is_master_node=false;
					m_nodeaddr_nodeid.clear();
					m_nodeid_nodeaddr.clear();
				}
			}
		}
		if(messageID==MESSAGE_DCNET_EX_COIN_FLIPS) { 
			//check if broad cast node id is in its map
			if(m_nodeaddr_nodeid.find(InetSocketAddress::ConvertFrom (from).GetIpv4 ())!= m_nodeaddr_nodeid.end() )  {
				NS_LOG_DEBUG("exchanging coin flips for node" << m_my_nodeid << "at time "<< Simulator::Now ().GetSeconds () << "\n");
				NS_LOG_DEBUG("exchanging coin flips for node" << m_my_nodeid);
				//GenerateCoinFlips();
			}
		}
		if(messageID==MESSAGE_DCNET_EX_ANNOUNCEMENTS) { 
			if(m_nodeaddr_nodeid.find(InetSocketAddress::ConvertFrom (from).GetIpv4 ())!= m_nodeaddr_nodeid.end() ) {
				NS_LOG_DEBUG("exchanging Annoucements for node" << m_my_nodeid << "at time "<< Simulator::Now ().GetSeconds () << "\n");
				NS_LOG_DEBUG("topology is " << m_topology <<"\n");
				NS_LOG_DEBUG("exchanging Annoucements for node" << m_my_nodeid);
				if(m_topology=="ring") {
					if(m_rank==1) {
						m_is_master_node=0;
						//std::cout << " starting to generate announcement by node" << m_my_nodeid << "\n";
						//std::cout << "m_is_master_node is set to " << m_is_master_node << "\n";
						GenerateAnnouncementsForRing("");
					}
                }
				if(m_topology=="fullyconnected") {
					std::cout<<"exchanging announcementd for node " << m_my_nodeid << "\n";
					GenerateAnnouncementsForFullyConnected();
				}
				
			}
		}
		if(messageID==MESSAGE_DCNET_FINAL_ANOUNCEMENT) {
			if(m_nodeaddr_nodeid.find(InetSocketAddress::ConvertFrom (from).GetIpv4 ())!= m_nodeaddr_nodeid.end() ) {
				NS_LOG_DEBUG("received final announcement " << recMessage <<" on node " << m_my_nodeid );
				std::cout << "received final announcement " << recMessage <<" on node " << m_my_nodeid <<"\n";
				m_final_message=recMessage;
				 m_dcnet_start=false;
				if(m_is_master_node) {
					if(m_num_message_repeat>0) {
						Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
						double jitter = uv->GetValue(1,6);
						Simulator::Schedule(Seconds(jitter),&DCNET::SendConfirmation,this,true);
					}
				}
				//StopApplication();
			}
		}
		else
	  	{   //This is dcnet acceptance message
			recMessage = recMessage.substr (0,recMessage.size());
			int node_to_be_recv = atoi(recMessage.c_str());
			//std::cout << "node to be recevd is " << node_to_be_recv << "\n";
			if(	node_to_be_recv==m_my_nodeid && messageID <200) {
				//std::cout << "m_is_master_node is " << m_is_master_node << "for node " << m_my_nodeid << "\n";
				m_is_master_node=true;
				m_nodeaddr_nodeid.insert(pair<ns3::Ipv4Address,int>(InetSocketAddress::ConvertFrom (from).GetIpv4 (),messageID));
				m_nodeid_nodeaddr.insert(pair<int,ns3::Ipv4Address>(messageID,InetSocketAddress::ConvertFrom (from).GetIpv4 ()));
				NS_LOG_DEBUG("received DCNET acceptance message from node " << messageID );
           		
		  	}
			else
			{
				m_is_master_node=false;
			}
	  	}
	  
	}
}

} // Namespace ns3
