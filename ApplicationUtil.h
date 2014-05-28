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
#include "cryptopp/aes.h" 
#include "cryptopp/sha.h"
#include "cryptopp/modes.h"
#include "cryptopp/integer.h"
#include "cryptopp/osrng.h"
#include "cryptopp/nbtheory.h"
#include <stdexcept>
#include "cryptopp/dh.h"
#include "cryptopp/secblock.h"
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <map>
#include <sstream>
#include <iomanip>

using CryptoPP::AutoSeededRandomPool;
using CryptoPP::ModularExponentiation;
using std::runtime_error;
using CryptoPP::DH;
using CryptoPP::SecByteBlock;
using CryptoPP::StringSink;
using CryptoPP::HexEncoder;
using std::map;
using std::pair;
using namespace ns3;
using namespace CryptoPP;

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

TypeId tid;
Ipv4InterfaceContainer ipInterfaceContainer;
NodeContainer c;
//Ptr<Socket> source;
int option;


int rounds = 0;
int MessageLength = 0;
double time_wait_dcnet=15.0;
double waitTime = 0;
std::stringstream sharedMessage;
int sender = 0;
int numberofmessages=3;
std::string messages[] = {"1010", "101", "10"};
std::string Message = "";
std::string phyMode ("ErpOfdmRate54Mbps");
double distance = 1;  // m
uint32_t packetSize =1024; // bytes
uint32_t numPackets = 60;
int numNodes = 85;  
uint32_t sinkNode = 0;
uint32_t sourceNode = 2;
int dcnet_protocol_start=0;
double interval = 1.0; // seconds
double keyExchangeInterval = 5.0; // seconds
bool verbose = false;
bool tracing = false;
int messageLen=0;	
std::string protocol="udp";
std::string topology="ring";
int publickeyset=0;
int aesKeyLength = SHA256::DIGESTSIZE;
byte AESiv[AES::BLOCKSIZE];
static std::string msgs[20];

SecByteBlock AESkey(0x00, AES::DEFAULT_KEYLENGTH);
AutoSeededRandomPool rnd;
byte iv[AES::BLOCKSIZE];

//measurement variables

int stage1SentPacketCount = 0;
int stage2SentPacketCount = 0;
int stage1RecvPacketCount = 0;
int stage2RecvPacketCount = 0;
int announcementPacketCounter = 0;
double stage1Latency;
double stage2Latency;
std::vector<Time> stage1StartTime;
std::vector<Time> stage2StartTime;
std::vector<Time> stage1EndTime;
std::vector<Time> stage2EndTime;
Time totalTimeStart, totalTimeEnd;
double totalRunningTime;
double totalLatency;



int publicKeyCounter = 0;
int randomBitCounter = 0;

class ApplicationUtil
{	
	private:
	static bool instanceFlag;
	int dhAgreedLength;
	static ApplicationUtil *appUtil;
	ApplicationUtil()
	{
	 //private constructor
	}

	map<int,SecByteBlock> publicKeyMap;
	map<int,SecByteBlock> privateKeyMap;
	map<int,SecByteBlock> dhSecretKeyMapSub;
	map<int,map<int,SecByteBlock> > dhSecretKeyMapGlobal;
	map<int,std::string> dhSecretBitMapSub;
	map<int,map<int,std::string> > dhSecretBitMapGlobal;
	map<Ptr<Node>,int> nodeMap;
	map<int,std::string> announcement;
	map<int, std::string> receivedAnnouncementSubMap;
	map<int, map<int, std::string> > receivedAnnouncement;

public:

	void writeOutputToFile(char* fileName, int option, int numNodes,int length, double latency, double totalTime );
	//static int publicKeyPairCount;
	int getDhAgreedLength()
	{
		return dhAgreedLength;
	}	
	void setDhAgreedLength(int len)
	{
		dhAgreedLength = len;
	}
	
	SecByteBlock getPublicKeyFromMap(int nodeId);
	void putPublicKeyInMap(int nodeId, SecByteBlock key);
	SecByteBlock getPrivateKeyFromMap(int nodeId);
	void putPrivateKeyInMap(int nodeId, SecByteBlock key);
	SecByteBlock getSecretKeyFromGlobalMap(int nodeId,int destNodeId);
	void putSecretKeyInGlobalMap(int nodeId, int destNodeId, SecByteBlock key);

	std::string getSecretBitFromGlobalMap(int nodeId,int destNodeId);
	void putSecretBitInGlobalMap(int nodeId, int destNodeId, std::string value);
	void eraseSecretBitMapGlobal();
	map<int,std::string> getSecretBitSubMap(int nodeId);

	void putNodeInMap(Ptr<Node> node,int index);
	int getNodeFromMap(Ptr<Node> node);
	void putAnnouncementInGlobalMap(int nodeId,std::string value);
	std::string getAnnouncement(int nodeId);
	void putAnnouncementInReceivedMap(int nodeId, int senderNode, std::string value);
	map<int, std::string> getAnnouncementSubMap(int nodeId);
	std::string getReceivedAnnouncement(int nodeId, int senderNodeId);

	static ApplicationUtil* getInstance();	
	
        ~ApplicationUtil()
        {
	  //instanceFlag = false;
        }
};
bool ApplicationUtil::instanceFlag = false;
ApplicationUtil* ApplicationUtil::appUtil = NULL;

ApplicationUtil* ApplicationUtil::getInstance()
{
	if(!instanceFlag)
        {		
		appUtil = new ApplicationUtil();
		instanceFlag = true;
	}
        return appUtil;
    
}		
void ApplicationUtil::putNodeInMap(Ptr<Node> node,int index)
{
	nodeMap.insert(pair<Ptr<Node>,int>(node,index));
}

int ApplicationUtil::getNodeFromMap(Ptr<Node> node)
{
	map<Ptr<Node>,int>::iterator p;
	p = nodeMap.find(node);
	if(p != nodeMap.end())
		return p->second;
	else 
		return -1;	
}
SecByteBlock ApplicationUtil::getPublicKeyFromMap(int nodeId)
{
	map<int,SecByteBlock>::iterator p;
	p = publicKeyMap.find(nodeId);
	if(p != publicKeyMap.end())
		return p->second;
	else 
		return SecByteBlock(0);
		
}

void ApplicationUtil::putPublicKeyInMap(int nodeId, SecByteBlock key)
{
	publicKeyMap.insert(pair<int,SecByteBlock>(nodeId,key));
}

SecByteBlock ApplicationUtil::getPrivateKeyFromMap(int nodeId)
{
	map<int,SecByteBlock>::iterator p;
	p = privateKeyMap.find(nodeId);
	if(p != privateKeyMap.end())
		return p->second;
	else 
		return SecByteBlock(0);
}

void ApplicationUtil::putPrivateKeyInMap(int nodeId, SecByteBlock key)
{
	privateKeyMap.insert(pair<int,SecByteBlock>(nodeId,key));
}	

SecByteBlock ApplicationUtil::getSecretKeyFromGlobalMap(int nodeId, int destNodeId)
{

	map<int,map<int,SecByteBlock> >::iterator p;
	p = dhSecretKeyMapGlobal.find(nodeId);
    if(p != dhSecretKeyMapGlobal.end()) {
		map<int,SecByteBlock>::iterator p1;
		p1 = p->second.find(destNodeId);
		if(p1 != dhSecretKeyMapSub.end())
			return p1->second;
		else 
			return SecByteBlock(0);
    }
	else 
		return SecByteBlock(0);
}

void ApplicationUtil::putSecretKeyInGlobalMap(int nodeId, int destNodeId, SecByteBlock key)
{

	map<int,map<int,SecByteBlock> >::iterator p;
	p = dhSecretKeyMapGlobal.find(nodeId);
	if(p != dhSecretKeyMapGlobal.end())
	{
		
		map<int,SecByteBlock>::iterator p1;
		p1 = p->second.find(destNodeId);	
		if(p1 != p->second.end())
			p->second[destNodeId] = key;
		else
			p->second.insert(pair<int,SecByteBlock>(destNodeId,key));

	}
	else
	{
				
		map<int,SecByteBlock> tempMap;	
		tempMap.insert(pair<int,SecByteBlock>(destNodeId,key));
		dhSecretKeyMapGlobal.insert(pair<int,map<int,SecByteBlock> >(nodeId,tempMap));
	}	

}	

std::string ApplicationUtil::getSecretBitFromGlobalMap(int nodeId, int destNodeId)
{

	map<int,map<int,std::string> >::iterator p;
	p = dhSecretBitMapGlobal.find(nodeId);

	if(p != dhSecretBitMapGlobal.end())
	{
		map<int,std::string>::iterator p1;
		p1 = p->second.find(destNodeId);
		if(p1 != dhSecretBitMapSub.end())
			return p1->second;
		else 
		{
			
			return "-99";
		}
	}
	else 
		{
			
			return "-99";
		}	
}

void ApplicationUtil::putSecretBitInGlobalMap(int nodeId, int destNodeId, std::string value)
{

	map<int,map<int,std::string> >::iterator p;
	p = dhSecretBitMapGlobal.find(nodeId);
	if(p != dhSecretBitMapGlobal.end())
	{
		map<int,std::string>::iterator p1;
		p1 = p->second.find(destNodeId);	
		if(p1 != p->second.end())
			p->second[destNodeId] = value;
		else
			p->second.insert(pair<int,std::string>(destNodeId,value));		
	}
	else
	{	
		map<int,std::string> tempMap;	
		tempMap.insert(pair<int,std::string>(destNodeId,value));
		dhSecretBitMapGlobal.insert(pair<int,map<int,std::string> >(nodeId,tempMap));
	}		
}					

map<int,std::string> ApplicationUtil::getSecretBitSubMap(int nodeId)
{
	map<int,map<int,std::string> >::iterator p;
	map<int,std::string> temp;
	p = dhSecretBitMapGlobal.find(nodeId);
    if(p!= dhSecretBitMapGlobal.end())
		return p->second;
    else {
		
		return temp;
	}
}

void ApplicationUtil::eraseSecretBitMapGlobal()
{
	 map<int,map<int,std::string> >::iterator p;
	 dhSecretBitMapGlobal.erase ( p, dhSecretBitMapGlobal.end() );
}

void ApplicationUtil::putAnnouncementInGlobalMap(int nodeId, std::string value)
{
	map<int,std::string>::iterator p;
	p = announcement.find(nodeId);
	if(p != announcement.end())
		announcement[nodeId] = value;
	else
	announcement.insert(pair<int,std::string>(nodeId,value));
}					

std::string ApplicationUtil::getAnnouncement(int nodeId)
{
	map<int,std::string>::iterator p;
	p = announcement.find(nodeId);
	return p->second;
}
void ApplicationUtil::putAnnouncementInReceivedMap(int nodeId, int senderNode, std::string value)
{
	map<int,map<int,std::string> >::iterator p;
	p = receivedAnnouncement.find(nodeId);
	if(p != receivedAnnouncement.end())
	{
		map<int,std::string>::iterator p1;
		p1 = p->second.find(senderNode);	
		if(p1 != p->second.end())
			p->second[senderNode] = value;
		else
		p->second.insert(pair<int,std::string>(senderNode,value));

	//	std::cout<<"Inserting "<<nodeId<<","<<senderNode<<","<<value<<"\n";

	}
	else
	{	
		map<int,std::string> tempMap;	
		tempMap.insert(pair<int,std::string>(senderNode,value));
		receivedAnnouncement.insert(pair<int,map<int,std::string> >(nodeId,tempMap));
	//	std::cout<<"Inserting "<<nodeId<<","<<senderNode<<","<<value<<"\n";
	}
}
std::string ApplicationUtil::getReceivedAnnouncement(int nodeId, int senderNodeId)
{
	map<int,map<int,std::string> >::iterator p;
	p = receivedAnnouncement.find(nodeId);

	if(p != receivedAnnouncement.end())
	{
		map<int,std::string>::iterator p1;
		p1 = p->second.find(senderNodeId);
		if(p1 != receivedAnnouncementSubMap.end())
			return p1->second;
		else 
		{
			
			return "-99";
		}
	}
	else 
		{
			
			return "-99";
		}	
}


map<int,std::string> ApplicationUtil::getAnnouncementSubMap(int nodeId)
{
	map<int,map<int,std::string> >::iterator p;
	p = receivedAnnouncement.find(nodeId);

	return p->second;
}
//write to csv file

void ApplicationUtil::writeOutputToFile(char* fileName, int option, int numNodes,int length, double latency, double totalTime ) {
   std::ofstream fout;

   fout.open(fileName, std::ios_base::app);

    if(option == 1)
    {
	fout << numNodes << ',' << latency << ','<<totalTime;
    }
    else if(option == 2)
    {
	fout << length << ',' << latency << ','<<totalTime;
    }
	    
    fout << "\n";
    fout.close();
}

