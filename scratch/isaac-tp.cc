#include <ns3/core-module.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/packet-sink.h"
#include "ns3/simulator.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traffic-control-module.h"
#include "ns3/mptcp-socket-base.h"
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>

#include "ns3/enum.h"
#include "ns3/config-store-module.h"
#include "ns3/isaac-module.h"
#include <vector>
#include <algorithm>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MpTcpTestingExample");

int g_nNodes = 0;
int g_upperNodeCapacity = 12;
int* g_arrayWithNodeCapacities;
int GenerateClusterNodes();

int
main (int argc, char *argv[])
{
  g_nNodes = GenerateClusterNodes();//get some random number of nodes between 3 and 10
	cout << "g_nNodes: " << g_nNodes << endl;
	g_arrayWithNodeCapacities = new int[g_nNodes];//a pointer to array to hold cluster node capacities
	cout << "m_nodeCapacities are :";
	for (size_t i = 0; i < g_nNodes; i++)
	{
		
		srand(time(0) + static_cast<unsigned long>(i*11));//i will randomize time so that we have truly random capacity
		g_arrayWithNodeCapacities[i] = 0+(rand() % (g_upperNodeCapacity+1));
		cout << g_arrayWithNodeCapacities[i] << "\t";
	}
	cout << endl;
	Cluster myCluster(g_nNodes, g_arrayWithNodeCapacities, g_upperNodeCapacity);

	int lowerTbr = 22, upperTbr = 23;

	// Find the number of subsets with desired Sum
	if (myCluster.findAndPrintSubsets(g_arrayWithNodeCapacities, g_nNodes, lowerTbr, upperTbr) > 0)
		cout << "Yes Subsets found!!!" << endl;
	else
		cout << "No Subsets found!!!" << endl;

	
// The below value configures the default behavior of global routing.
  // By default, it is disabled.  To respond to interface events, set to true
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue (true));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  Config::SetDefault ("ns3::BulkSendApplication::SendSize", UintegerValue (4364));

  //Enable MPTCP
  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (true));
  Config::SetDefault ("ns3::MpTcpSocketBase::PathManagerMode",
                      EnumValue (MpTcpSocketBase::nDiffPorts));
  // Config::SetDefault ("ns3::MpTcpNdiffPorts::MaxSubflows", UintegerValue (1));

  //Variables Declaration
  uint16_t port = 999;
  uint32_t maxBytes = 1048576; //1MBs


  // // initialize a cluster
  // Cluster myCluster;
  // g_nNodes = myCluster.GenerateClusterNodes (3); //random number of nodes in a cluster
  // g_nNodes = myCluster.GenerateActiveRelays (g_nNodes, 5);// 5 is a randomizer to generate random number of nodes
  // std::cout << "My Cluster has :" << g_nNodes << " nodes\n";
  // std::cout << "My Cluster has :" << g_nNodes << " relays\n";

  //Initialize Internet Stack and Routing Protocols
  InternetStackHelper internet;
  Ipv4AddressHelper ipv4;

  //creating routers, source and destination. Installing internet stack
  NodeContainer host; // NodeContainer for source and destination

  host.Create (2);
  internet.Install (host);
  //enb
  NodeContainer enb;
  enb.Create (1);
  internet.Install (enb);

  // relay
  NodeContainer relay; // NodeContainer for relay
  relay.Create (g_nNodes);
  internet.Install (relay);

 
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enb);
  mobility.Install (host.Get(0));

  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("10"), "Y",
                                 StringValue ("20"), "Rho",
                                 StringValue ("ns3::UniformRandomVariable[Min=0|Max=10]"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"), "Time",
                             StringValue ("2s"), "Speed",
                             StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"), "Bounds",
                             StringValue ("0|100|0|100"));
  mobility.Install (relay);
  mobility.Install (host.Get(1));
  
  /////////////////////creating topology////////////////
  NodeContainer *e0Ri = new NodeContainer[g_nNodes];
  NodeContainer *riH1 = new NodeContainer[g_nNodes];

  NodeContainer e0h0 = NodeContainer (enb.Get (0), host.Get (0));

  NetDeviceContainer *chiE0Ri = new NetDeviceContainer[g_nNodes];


  //loop through the relays
  for (size_t i = 0; i < g_nNodes; i++)
    {
      e0Ri[i] = NodeContainer (enb.Get (0), relay.Get (i));
      riH1[i] = NodeContainer (host.Get (1), relay.Get (i));
    }
  NS_LOG_INFO ("Create channels.");

  PointToPointHelper p2p;

  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms")); //ms

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4InterfaceContainer *intfiE0Ri = new Ipv4InterfaceContainer[g_nNodes];
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  for (size_t i = 0; i < g_nNodes; i++)
    {
      chiE0Ri[i] = p2p.Install (e0Ri[i]);
      intfiE0Ri[i] = ipv4.Assign (chiE0Ri[i]);
    }

  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  NetDeviceContainer ch4e0h0 = p2p.Install (e0h0);

  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer *chiRiH1 = new NetDeviceContainer[g_nNodes];
  Ipv4InterfaceContainer *intfiRiH1 = new Ipv4InterfaceContainer[g_nNodes];
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  for (size_t i = 0; i < g_nNodes; i++)
    {
      chiRiH1[i] = p2p.Install (riH1[i]);
      intfiRiH1[i] = ipv4.Assign (chiRiH1[i]);
    }

  //Later, we add IP addresses.

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i4e0h0 = ipv4.Assign (ch4e0h0);

  //Attach the UEs to an eNB. This will configure each UE according to the eNB configuration, and create an RRC connection between them:

  //Create router nodes, initialize routing database and set up the routing
  //tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  NS_LOG_INFO ("Create applications.");

  //Create bulk send Application

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address ("10.1.1.1"), port));
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

  ApplicationContainer sourceApps;

  sourceApps.Add (source.Install (host.Get (0)));
  sourceApps.Start (NanoSeconds (2.0));
  sourceApps.Stop (Seconds (100.0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (host.Get (1));
  sinkApps.Start (NanoSeconds (1.0));
  sinkApps.Stop (Seconds (100.0));

  //=========== Start the simulation ===========//

  std::cout << "Start Simulation.\n ";

  //tracemetrics trace file
  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("tracemetrics/isaac-mptcp-1048576.tr"));

  ////////////////////////// Use of NetAnim model/////////////////////////////////////////
  AnimationInterface anim ("netanim/mptcp-1048576.xml");

  //NodeContainer server
  anim.SetConstantPosition (host.Get (0), 25.0, 0.0);
  uint32_t resourceIdIconServer =
      anim.AddResource ("/home/ns3/ns-3-dev-git/netanim/icons/server.png");
  anim.UpdateNodeImage (0, resourceIdIconServer);
  anim.UpdateNodeSize (0, 4, 4);

  //enb
  anim.SetConstantPosition (enb.Get (0), 25.0, 10.0);
  uint32_t resourceIdIconEnb = anim.AddResource ("/home/ns3/ns-3-dev-git/netanim/icons/enb.png");
  anim.UpdateNodeImage (2, resourceIdIconEnb);
  anim.UpdateNodeSize (2, 7, 7);
//relays
  for (size_t i = 0; i < g_nNodes; i++)
    {
      int x = i * 10;
      anim.SetConstantPosition (relay.Get (i), x, 20.0);
    }

  uint32_t resourceIdIconPhone =
      anim.AddResource ("/home/ns3/ns-3-dev-git/netanim/icons/phone.png");
  for (size_t i = 0; i < g_nNodes; i++)
    {
      anim.UpdateNodeImage (i + 3, resourceIdIconPhone);
      anim.UpdateNodeSize (i + 3, 4, 4);
    }

  //requester
  anim.SetConstantPosition (host.Get (1), 25.0, 50.0);
  anim.UpdateNodeImage (1, resourceIdIconPhone);
  anim.UpdateNodeSize (1, 4, 4);

  /////////////////////////////////////////////////////////////////////////////////////////////
  //Output config store to txt format
  Config::SetDefault ("ns3::ConfigStore::Filename",
                      StringValue ("config/output-attributesmptcp-1048576.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();

  //flowmon tracefiles
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  //p2p.EnablePcapAll ("pcap/isaac-p2pCapmptcp-1048576");

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();

  NS_LOG_INFO ("Done.");

  std::cout << "Simulation finished "
            << "\n";

  Simulator::Destroy ();
  delete[] g_arrayWithNodeCapacities;//remember to destroy dynamic array holding cluster node  capacities
  monitor->SerializeToXmlFile ("flowmon/isaac-mptcp-1048576flow.xml", true, true);
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}

int GenerateClusterNodes()
{
	srand(time(0));
	g_nNodes = 1 + (rand() % 10);
	if (g_nNodes < 3)
	{
		g_nNodes = 3;
	}
	return g_nNodes;
}