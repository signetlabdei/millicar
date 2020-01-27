/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/mmwave-vehicular-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-module.h"
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
 NS_LOG_DEBUG ("TX event");
 g_txPackets++;
}

static void Rx (Ptr<const Packet> p)
{
 g_rxPackets++;

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

  uint32_t packetSize = 1024; // bytes
  Time startTime = Seconds (1.5);
  Time endTime = Seconds (30.0);
  uint8_t mcs = 12;

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (false));
  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::Frequency", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularSpectrumPropagationLossModel::Frequency", DoubleValue (60.0e9));

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

  n.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (250,0,0));
  n.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (20, 0, 0));

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
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (n.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds(34.0));

  apps.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback (&Rx));

  //
  // Create a UdpClient application to send UDP datagrams from node zero to
  // node one.
  //

  Ptr<mmwave::MmWaveAmc> m_amc = CreateObject <mmwave::MmWaveAmc> (helper->GetConfigurationParameters());
  double availableRate = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 0.001; // bps

  uint32_t maxPacketCount = 500000;
  packetSize = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 8 - 28 - 2;
  Time interPacketInterval =  Seconds(double((packetSize * 8) / availableRate));
  UdpEchoClientHelper client (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (n.Get (0));
  apps.Start (startTime);
  apps.Stop (endTime);

  apps.Get(0)->TraceConnectWithoutContext ("Tx", MakeCallback (&Tx));

  Simulator::Stop (Seconds(34.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "----------- MCS :\t\t" << uint32_t(mcs) << " -----------" << std::endl;
  std::cout << "Available Rate:\t\t" << availableRate/1e6 << " Mbps" << std::endl;
  std::cout << "Packets size:\t\t" << packetSize << " Bytes" << std::endl;
  std::cout << "Packets transmitted:\t" << g_txPackets << std::endl;
  std::cout << "Packets received:\t" << g_rxPackets << std::endl;
  std::cout << "Average Throughput:\t" << (double(g_rxPackets)*double(packetSize+28)*8/double( g_lastReceived.GetSeconds() - g_firstReceived.GetSeconds()))/1e6 << " Mbps" << std::endl;

  return 0;
}
