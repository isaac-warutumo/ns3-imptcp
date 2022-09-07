
// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

//to use pow functionality
#include <cmath>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

int
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::BulkSendApplication::SendSize", UintegerValue (1024));
  //ns3::BulkSendApplication::SendSize "1128" //512
  uint32_t maxBytes = 1048576; //1MBs

  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (2);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  NS_LOG_INFO ("Create channels.");

  //
  // Explicitly create the point-to-point link required by the topology (shown above).
  //
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("70Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ps"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  //
  // Install the internet stack on the nodes
  //
  InternetStackHelper internet;
  internet.Install (nodes);

  //
  // We've got the "hardware" in place.  Now we need to add IP addresses.
  //
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  NS_LOG_INFO ("Create Applications.");

  // Create a BulkSendApplication and install it on node 0
  uint16_t port = 9; // well-known echo port number

  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  //source.SetAttribute ("PacketSize", UintegerValue (1024));
  //
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (NanoSeconds (0.0));
  sourceApps.Stop (Seconds (20.0));

  //
  // Create a PacketSinkApplication and install it on node 1
  //
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (NanoSeconds (0.0));
  sinkApps.Stop (Seconds (20.0));

  //
  // Set up tracing if enabled
  //
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("tracemetrics/tcp-bulk-send.tr");
  pointToPoint.EnableAscii (stream, devices);

  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send1024000.tr"));
  pointToPoint.EnablePcapAll ("pcap/tcp-bulk-send");
  //ipv4.EnableAsciiIpv4All ("tcp-bulk-send1024000.tr");

  AnimationInterface anim ("netanim/tcp-bulk-send1024000.xml");
  anim.SetConstantPosition (nodes.Get (0), 10.0, 10.0);
  anim.SetConstantPosition (nodes.Get (1), 30.0, 10.0);

  // Ptr<FlowMonitor> flowMonitor;
  // FlowMonitorHelper flowHelper;
  // flowMonitor = flowHelper.InstallAll ();

  Config::SetDefault ("ns3::ConfigStore::Filename",
                      StringValue ("config/output-attributestcpbulksend1024000.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (20.0));

  //flow monitor config
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Run ();
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  int totalBytesRx = sink1->GetTotalRx ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin ();
       i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> "
                << t.destinationAddress << ")\n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";

      std::cout << "  Transmission time:   "
                << (i->second.timeLastRxPacket - i->second.timeFirstRxPacket) << "\n";
      //rxbytes*8=bits; nanoseconds=pow(10,9); Mbps=pow(10,6); 
      //Mbps/nanoseconds=pow(10,3)
      std::cout << "  Throughput: "
                << i->second.rxBytes * 8.0 * pow (10, 3) /
                       (i->second.timeLastRxPacket - i->second.timeFirstRxPacket)
                << " Mbps\n";
      std::cout << "  Tx Packets:   " << i->second.txPackets << "\n";
      std::cout << "  Rx Packets:   " << i->second.rxPackets << "\n";
      std::cout << "  Delay Sum:   " << i->second.delaySum << "\n";
      std::cout << "  Average Delay:   " << i->second.delaySum / i->second.rxPackets << "\n";
    }

  // flowMonitor->SerializeToXmlFile ("flowmon/tcp-bulk-sendflow.xml", true, true);

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  std::cout << "Total Bytes Received: " << totalBytesRx << std::endl;
}
