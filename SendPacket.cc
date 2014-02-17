  
     #include "ns3/core-module.h"
     #include "ns3/network-module.h"
     #include "ns3/mobility-module.h"
     #include "ns3/config-store-module.h"
     #include "ns3/wifi-module.h"
     #include "ns3/internet-module.h"
     #include "ns3/olsr-helper.h"
     #include "ns3/ipv4-static-routing-helper.h"
     #include "ns3/ipv4-list-routing-helper.h"
     #include "MyTag.h"
     #include "LRUcache.h" 
     #include <iostream>
     #include <fstream>
     #include <vector>
     #include <string>
#include "crypto++/aes.h" 
#include "crypto++/modes.h"
#include "crypto++/integer.h"
#include "crypto++/osrng.h"
using CryptoPP::AutoSeededRandomPool;
#include "crypto++/nbtheory.h"
using CryptoPP::ModularExponentiation;

#include "crypto++/dh.h"
using CryptoPP::DH;

#include "crypto++/secblock.h"
using CryptoPP::SecByteBlock;

#include <crypto++/hex.h>
using CryptoPP::HexEncoder;

#include <crypto++/filters.h>
using CryptoPP::StringSink;


     
     NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");
     
     using namespace ns3;
     using namespace CryptoPP;

     static Queue1* q;
     static Hasht* hasht;
     static int requestPackets[14] = {1,4,6,1,18,3,19,4,6,9,17,8,8,17}; 
     void ReceivePacket (Ptr<Socket> socket)
     {
          Ptr<Packet> recPacket = socket->Recv();
          uint8_t *buffer = new uint8_t[recPacket->GetSize()];
          recPacket->CopyData(buffer,recPacket->GetSize());
          std::string recData = std::string((char*)buffer);
          
          MyTag recTag;
          recPacket->PeekPacketTag(recTag);
       	  int tagVal =int(recTag.GetSimpleValue());
          std::ostringstream s;
          s<<tagVal;
          std::string ss(s.str());
          QNode* qN = q->front;
           int check = 0;
	  while(qN!=NULL){
              //  std::ostringstream qval;
              //  qval << qN->pageNumber;
                  
               //   NS_LOG_UNCOND(qval.str());
                  if(qN->pageNumber==tagVal){
                     check = 1;
                     NS_LOG_UNCOND("Already has packet with TagID: "+ss);
                     break;
                }
          qN = qN->next; 
          }
           ReferencePage(q,hasht,tagVal); 
          if(check==0){ 
            // ReferencePage(q,hasht,tagVal);
          
          NS_LOG_UNCOND ("Received one packet: Data: " +recData+"   TagID: "+ss);
     }}
    static std::string msgs[20];
    //static Hasht* hasht;
    //static Queue1* q;
 
     static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                                  uint32_t pktCount, Time pktInterval , int i)
     {
      if (pktCount > 0)
        {
          std::string msgx = msgs[requestPackets[i]]; 
          Ptr<Packet> sendPacket =
                  Create<Packet> ((uint8_t*)msgx.c_str(),pktSize);
          MyTag sendTag;
          sendTag.SetSimpleValue(requestPackets[i]);
          sendPacket->AddPacketTag(sendTag); 
          socket->Send (sendPacket);
          NS_LOG_UNCOND ("Sending one packet: "+msgx);  
          if(i<13){ Simulator::Schedule (pktInterval, &GenerateTraffic, 
                               socket, pktSize,pktCount-1, pktInterval, i+1);}
        }
      else
        {
          socket->Close ();
        }
    }
    
    
    int main (int argc, char *argv[])
    {
      msgs[0]="UCLA";
      msgs[1]="MIT"; msgs[2]="Stanford"; msgs[3]="Berkley";
      msgs[4]="UC Irvine"; msgs[5]="UC San Diego"; msgs[6]="USC";
      msgs[7]="Carnegie Mellon University"; msgs[8]="UPenn";
      msgs[9]="Columbia University"; msgs[10]="Cornell University";
      msgs[11]="Georgia Tech"; msgs[12]="TAMU"; msgs[13]="NYU";
      msgs[14]="UT Austin"; msgs[15]="Arizona State University";
      msgs[16]="NCSU"; msgs[17]="University of Maryland,College Park";
      msgs[18]="Stony Brook University"; 
      msgs[19]="University of Indiana,Bloomington";
      
      q=createQueue(5);
      hasht = createHash(20); 
      
      std::string phyMode ("DsssRate1Mbps");
      double distance = 500;  // m
      uint32_t packetSize = 1000; // bytes
      uint32_t numPackets = 20;
      uint32_t numNodes = 3;  // by default, 5x5
      uint32_t sinkNode = 0;
      uint32_t sourceNode = 2;
      double interval = 1.0; // seconds
      bool verbose = false;
      bool tracing = false;
    
      CommandLine cmd;
    
      cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
      cmd.AddValue ("distance", "distance (m)", distance);
      cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
      cmd.AddValue ("numPackets", "number of packets generated", numPackets);
      cmd.AddValue ("interval", "interval (seconds) between packets", interval);
      cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
      cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
      cmd.AddValue ("numNodes", "number of nodes", numNodes);
      cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
      cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
    
      cmd.Parse (argc, argv);
      // Convert to time object
      Time interPacketInterval = Seconds (interval);
    
      // disable fragmentation for frames below 2200 bytes
      Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
      // turn off RTS/CTS for frames below 2200 bytes
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
      // Fix non-unicast data rate to be the same as that of unicast
      Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                          StringValue (phyMode));
    
     NodeContainer c;
      c.Create (numNodes);
 
/*
        NodeContainer c;
        for ( int i=0;i<3;i++){
        Ptr<MyNode> mynode(new MyNode(i));
        c.Add(mynode);
        }
*/      
       // c.Get(0)->printId();
        std::cout<<"sdfdsfdsf : "<<c.Get(0)->GetId()<<"\n";
      // The below set of helpers will help us to put together the wifi NICs we want
      WifiHelper wifi;
      if (verbose)
        {
          wifi.EnableLogComponents ();  // Turn on all Wifi logging
        }
    
      YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
      // set it to zero; otherwise, gain will be added
      wifiPhy.Set ("RxGain", DoubleValue (-10) ); 
      // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
    
      YansWifiChannelHelper wifiChannel;
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
      wifiPhy.SetChannel (wifiChannel.Create ());
    
      // Add a non-QoS upper mac, and disable rate control
      NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
      wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
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
                                     "GridWidth", UintegerValue (5),
                                     "LayoutType", StringValue ("RowFirst"));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (c);
    
      // Enable OLSR
      OlsrHelper olsr;
      Ipv4StaticRoutingHelper staticRouting;
    
      Ipv4ListRoutingHelper list;
      list.Add (staticRouting, 0);
      list.Add (olsr, 10);
    
      InternetStackHelper internet;
      internet.SetRoutingHelper (list); // has effect on the next Install ()
      internet.Install (c);
    
      Ipv4AddressHelper ipv4;
      NS_LOG_INFO ("Assign IP Addresses.");
      ipv4.SetBase ("10.1.1.0", "255.255.255.0");
      Ipv4InterfaceContainer i = ipv4.Assign (devices);
    
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
      recvSink->Bind (local);
      recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
    
      Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
      InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
      source->Connect (remote);
    
      if (tracing == true)
        {
          AsciiTraceHelper ascii;
          wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
          wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
          // Trace routing tables
          Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
          olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
    
          // To do-- enable an IP-level trace that shows forwarding events only
        }
    
      // Give OLSR time to converge-- 30 seconds perhaps
      Simulator::Schedule (Seconds (30.0), &GenerateTraffic, 
                           source, packetSize, numPackets, interPacketInterval,0);
    
      // Output what we are doing
      NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);
    
      Simulator::Stop (Seconds (132.0));
      Simulator::Run ();
      Simulator::Destroy ();
    
      return 0;
    }
