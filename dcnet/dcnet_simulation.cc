#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "DCNET-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/nstime.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/names.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/netanim-module.h"


using namespace ns3;
std::string phyMode ("ErpOfdmRate54Mbps");
Ipv4InterfaceContainer ipInterfaceContainer;
NS_LOG_COMPONENT_DEFINE ("DCNET_simulation");

int 
main (int argc, char *argv[])
{
    NS_LOG_UNCOND("Inside Main josh");
    CommandLine cmd;
    NS_LOG_LOGIC("argc : "<<argc);
	cmd.Parse (argc, argv);
   	int numNodes=100;

	std::string message1="10101";
    std::string message2="1111";
  //  std::string message3="111111";

   uint16_t messagelen1=message1.size();
   uint16_t messagelen2=message2.size();
	//uint16_t messagelen3=message3.size();

	cmd.AddValue ("numNodes", "Number of Nodes", numNodes);
	//cmd.AddValue ("message", "Message to be sent", message);
    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
    NodeContainer c;
	c.Create (numNodes);
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.Set("EnergyDetectionThreshold",DoubleValue(-74 )); 
	wifiPhy.Set("CcaMode1Threshold",DoubleValue(-74 )); 
	wifiPhy.Set("TxGain", DoubleValue (5));
    wifiPhy.Set("RxGain", DoubleValue (4));
    wifiPhy.Set("TxPowerLevels", UintegerValue (1));
    wifiPhy.Set("TxPowerEnd", DoubleValue (35.0)); 
    wifiPhy.Set("TxPowerStart", DoubleValue (35.0)); 
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
                                     "DeltaX", DoubleValue (4.0),
                                     "DeltaY", DoubleValue (20.0),
                                     "GridWidth", UintegerValue (20),
                                     "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel");
	//mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
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
	NS_LOG_INFO ("Create Applications.");
    uint16_t port = 9999;  // well-known echo port number
	DCNETHelper app1 (port,false,message1,messagelen1);
	DCNETHelper app2 (port,false,message2,messagelen2);
	ApplicationContainer apps1 ;
	ApplicationContainer apps2;
    for(int i=0;i<10;i++) {
    	apps1 = app1.Install (c.Get (i));
    	apps1.Start (Seconds (1.0));
	   
	}
	
	
    /*DCNETHelper app1 (port,false,"",messagelen1);
	DCNETHelper app2 (port,false,"",messagelen2);
	DCNETHelper app3 (port,false,"",messagelen3);

    DCNETHelper app4 ( port,true,message1,messagelen1);
    DCNETHelper app5 ( port,true,message2,messagelen2);
	DCNETHelper app6 ( port,true,message3,messagelen3);

	ApplicationContainer apps1 ;
    ApplicationContainer apps2;
    ApplicationContainer apps3;

	ApplicationContainer apps4 ;
    ApplicationContainer apps5;
    ApplicationContainer apps6;
    //first cluster
	for(int i=0;i<29;i++) {
    	apps1 = app1.Install (c.Get (i));
    	apps1.Start (Seconds (1.0));
	   
	}
	apps4 = app4.Install (c.Get (29));    //first master node
    apps4.Start (Seconds (2.0));

	for(int i=30;i<59;i++) {
    	apps2 = app2.Install (c.Get (i));
    	apps2.Start (Seconds (3.0));
    }

	apps5 = app5.Install (c.Get (59));
    apps5.Start (Seconds (4.0));

	for(int i=60;i<99;i++) {
    	apps3 = app3.Install (c.Get (i));
    	apps3.Start (Seconds (5.0));
    }
	apps6 = app6.Install (c.Get (99));
    apps6.Start (Seconds (6.0));*/
  
   	NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds(2000));
	//AnimationInterface anim("dcnet_net_anim.xml");
	//anim.EnablePacketMetadata(true);
   // FlowMonitorHelper flowmon;
	//Ptr<FlowMonitor> monitor = flowmon.InstallAll();    
    Simulator::Run ();
   /*
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
    monitor->SerializeToXmlFile("dcnet_ring.flowmon",true,true);*/
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");

}
