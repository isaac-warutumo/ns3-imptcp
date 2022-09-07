#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/netanim-module.h"

//flow monitor
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"

//iomanip required for setprecision(3)
#include <iomanip>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("TestRoutingExample");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting",
                      BooleanValue (true)); // enable multi-path routing
  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (true));

  Ptr<Node> n0 = CreateObject<Node> ();
  Ptr<Node> n1 = CreateObject<Node> ();
  Ptr<Node> n2 = CreateObject<Node> ();
  Ptr<Node> n3 = CreateObject<Node> ();
  Ptr<Node> n4 = CreateObject<Node> ();

  Names::Add ("n0", n0);
  Names::Add ("n1", n1);
  Names::Add ("n2", n2);
  Names::Add ("n3", n3);
  Names::Add ("n4", n4);

  NodeContainer n0n1 (n0, n1);
  NodeContainer n1n2 (n1, n2);
  NodeContainer n1n3 (n1, n3);
  NodeContainer n3n4 (n3, n4);
  NodeContainer n2n4 (n2, n4);

  NodeContainer global (n0, n1, n2, n3, n4);

  // create link
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("25Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ps"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);
  NetDeviceContainer d1d2 = p2p.Install (n1n2);
  NetDeviceContainer d2d4 = p2p.Install (n2n4);

  p2p.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
  NetDeviceContainer d1d3 = p2p.Install (n1n3);
  NetDeviceContainer d3d4 = p2p.Install (n3n4);
  // create internet stack
  InternetStackHelper internet;
  internet.Install (global);

  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

  ipv4.SetBase ("10.2.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i3 = ipv4.Assign (d1d3);

  ipv4.SetBase ("10.3.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i4 = ipv4.Assign (d2d4);

  ipv4.SetBase ("10.4.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // install application
  UdpEchoServerHelper echoServer1 (9999);
  ApplicationContainer serverApps1 = echoServer1.Install (n4);
  serverApps1.Start (Seconds (0.0));
  serverApps1.Stop (Seconds (1000.0));

  // n0 -> n4
  UdpEchoClientHelper echoClient1 (i2i4.GetAddress (1), 9999);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (200));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps1 = echoClient1.Install (n0);
  clientApps1.Start (Seconds (0));
  clientApps1.Stop (Seconds (10.0));

  //tracemetrics trace file
  AsciiTraceHelper ascii;

  p2p.EnableAsciiAll (ascii.CreateFileStream ("tracemetrics/second.tr"));

  // dump config
  p2p.EnablePcapAll ("pcap/second");

  AnimationInterface anim ("netanim/isaac-tp.xml");
  anim.SetConstantPosition (n0, 0.0, 10.0);
  anim.SetConstantPosition (n1, 20.0, 10.0);
  anim.SetConstantPosition (n2, 35.0, 5.0);
  anim.SetConstantPosition (n3, 35.0, 15.0);
  anim.SetConstantPosition (n4, 50.0, 10.0);

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}