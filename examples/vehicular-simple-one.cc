/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/mmwave-vehicular-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/config.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleOne");

using namespace ns3;
using namespace mmwave_vehicular;

uint32_t g_rxPackets; // total number of received packets
uint32_t g_txPackets; // total number of transmitted packets

Time g_firstReceived; // timestamp of the first time a packet is received
Time g_lastReceived; // timestamp of the last received packet

static void Tx (Ptr<const Packet> p)
{
 NS_LOG_UNCOND ("TX event");
 g_txPackets++;
}

static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
 g_rxPackets++;
 SeqTsHeader header;

 p->PeekHeader(header);

 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() << "\t" << header.GetSeq() << "\t" << header.GetTs().GetSeconds() << std::endl;

 if (g_rxPackets > 1)
 {

   g_lastReceived = Simulator::Now();
 }
 else
 {
   g_firstReceived = Simulator::Now();
 }
}

int main (int argc, char *argv[])
{
  // This script creates two nodes moving at 20 m/s, placed at a distance of 10 m.
  // These nodes exchange packets through a UDP application,
  // and they communicate using a wireless channel at 60 GHz.


  double endSim = 10.0;
  double xDistanceNode2 = 10;
  double yDistanceNode2 = 0;
  double xSpeedNode2 = 20; // in m/s
  //double pktInterval = 10;
  double bandwidth = 1e8;
  uint8_t mcs = 24;
  bool useAmc = false;

  //RngSeedManager::SetRun(464);

  CommandLine cmd;

  cmd.AddValue ("endSim", "duration of the application", endSim);
  cmd.AddValue ("useAmc", "enable the use of AMC, false by default", useAmc);
  cmd.AddValue ("mcs", "set different mcs", mcs);
  cmd.AddValue ("bandwidth", "used bandwidth", bandwidth);
  //cmd.AddValue ("pktInterval", "inter packet interval, in microseconds", pktInterval);
  cmd.AddValue ("xDistanceNode2", "distance from Node 1, x-coord", xDistanceNode2);
  cmd.AddValue ("yDistanceNode2", "distance from Node 1, y-coord", yDistanceNode2);
  cmd.AddValue ("xSpeedNode2", "speed at which Node 2 is moving in the x axis", xSpeedNode2);
  cmd.Parse (argc, argv);

  Time startTime = Seconds (1.0);
  Time endTime = Seconds (endSim);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (useAmc));

  if (!useAmc)
  {
    Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  }

  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularNetDevice::RlcType", StringValue("LteRlcUm"));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (50*1024));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::Frequency", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::ChannelCondition", StringValue ("l"));
  Config::SetDefault ("ns3::MmWaveVehicularSpectrumPropagationLossModel::Frequency", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularHelper::Bandwidth", DoubleValue (bandwidth));

  // create the nodes
  NodeContainer n;
  n.Create (2);

  // create the mobility models
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (n);

  n.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  n.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (20, 0, 0));

  n.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (xDistanceNode2, yDistanceNode2, 0));
  n.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (xSpeedNode2, 0, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetPropagationLossModelType ("ns3::MmWaveVehicularPropagationLossModel");
  helper->SetSpectrumPropagationLossModelType ("ns3::MmWaveVehicularSpectrumPropagationLossModel");
  NetDeviceContainer devs = helper->InstallMmWaveVehicularNetDevices (n);

  // Install the TCP/IP stack in the two nodes

  InternetStackHelper internet;
  internet.Install (n);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devs);

  // Need to pair the devices in order to create a correspondence between transmitter and receiver
  // and to populate the < IP addr, RNTI > map.
  helper->PairDevices(devs);

  // Set the routing table
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (n.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0: " << n.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1: " << n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  NS_LOG_INFO ("Create Applications.");
  //
  // Create a UdpEchoServer application on node one.
  //
  uint16_t port = 4000;  // well-known echo port number
  UdpEchoServerHelper server (port);
  ApplicationContainer echoApps = server.Install (n.Get (1));
  echoApps.Start (Seconds (0.0));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("stats.txt");
  echoApps.Get(0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&Rx, stream));

  Ptr<mmwave::MmWaveAmc> m_amc = CreateObject <mmwave::MmWaveAmc> (helper->GetConfigurationParameters());
  // the target rate is the highest possibile
  //NS_LOG_UNCOND(m_amc->GetTbSizeFromMcsSymbols(28, 14));
  double availableRate = m_amc->GetTbSizeFromMcsSymbols(28, 14) / 0.001; // bps

  uint32_t maxPacketCount = 800000;
  // we chose the packetSize as the lowest possibile in order to avoid problems with RLC's TxOpportunities
  //packetSize = m_amc->GetTbSizeFromMcsSymbols(0, 14) / 8 - 28 - 2;
  Time interPacketInterval =  Seconds(double((512 * 8) / availableRate));
  //NS_LOG_UNCOND(double((512 * 8) / availableRate));
  //Time interPacketInterval = MicroSeconds(pktInterval);
  //NS_LOG_UNCOND("txed " << (endSim - 1) / interPacketInterval.GetSeconds());
  UdpClientHelper client (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (512));
  ApplicationContainer apps = client.Install (n.Get (0));
  apps.Start (Seconds(1.0));
  apps.Stop (endTime);

  apps.Get(0)->TraceConnectWithoutContext ("Tx", MakeCallback (&Tx));

  Simulator::Stop (endTime + Seconds (6));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "----------- Parameters ---------- " << std::endl;
  std::cout << "endSim:\t" << endSim << std::endl;
  std::cout << "useAmc:\t" << useAmc << std::endl;
  if(!useAmc)
  {
    std::cout << "mcs:\t" << uint32_t(mcs) << std::endl;
  }
  std::cout << "xDistanceNode2:\t" << xDistanceNode2 << std::endl;
  std::cout << "yDistanceNode2:\t" << yDistanceNode2 << std::endl;
  std::cout << "xSpeedNode2:\t" << xSpeedNode2 << " m/s" << std::endl;

  std::cout << "----------- Statistics -----------" << std::endl;
  std::cout << "Available Rate:\t\t" << availableRate/1e6 << " Mbps" << std::endl;
  std::cout << "Packets size:\t\t" << 512 << " Bytes" << std::endl;
  //std::cout << "Packets transmitted:\t" << g_txPackets << std::endl;
  std::cout << "Packets received:\t" << g_rxPackets << std::endl;
  std::cout << "Average Throughput:\t" << (double(g_rxPackets)*(double(512)*8)/double( g_lastReceived.GetSeconds() - g_firstReceived.GetSeconds()))/1e6 << " Mbps" << std::endl;

  return 0;
}
