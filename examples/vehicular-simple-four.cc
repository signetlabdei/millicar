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

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleTwo");

using namespace ns3;
using namespace mmwave_vehicular;

/**
  This script creates two pairs of vehicles moving at 20 m/s. The two groups are virtually positioned in two different lanes.
  In the same group, the vehicles are positioned one in front of the other at 20 m distance, and they exchange packets through a UDP application.
  Communication is done via a wireless channel at 60 GHz.
  The aim of this example is to check whether the interference was evaluated correctly when different groups
  of nodes are transmitting in the same slot, sharing the same cell.
*/

int main (int argc, char *argv[])
{

  uint32_t packetSize = 1024; // bytes
  Time startTime = Seconds (1.5);
  Time endTime = Seconds (10.0);
  uint8_t mcs = 12;

  double speed = 20; // m/s
  CommandLine cmd;
  cmd.AddValue ("vehicleSpeed", "The speed of the vehicle", speed);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (false));
  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::Frequency", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularSpectrumPropagationLossModel::Frequency", DoubleValue (60.0e9));

  // create the nodes
  NodeContainer group;
  group.Create (3);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (group);

  group.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  group.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (20,0,0));
  group.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group.Get (2)->GetObject<MobilityModel> ()->SetPosition (Vector (10,20,0));
  group.Get (2)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  NetDeviceContainer devs = helper->InstallMmWaveVehicularNetDevices (group);

  InternetStackHelper internet;
  internet.Install (group);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devs);

  helper->PairDevices(devs);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (group.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  staticRouting = ipv4RoutingHelper.GetStaticRouting (group.Get (2)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0: " << group.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1: " << group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 2: " << group.Get (2)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  Ptr<mmwave::MmWaveAmc> m_amc = CreateObject <mmwave::MmWaveAmc> (helper->GetConfigurationParameters());
  double availableRate = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 0.001; // bps
  uint16_t port_1 = 4000;  // well-known echo port number
  uint16_t port_2 = 4001;
  uint32_t maxPacketCount = 1;
  packetSize = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 8 - 28 - 2;
  Time interPacketInterval =  Seconds(double((packetSize * 8) / availableRate));

  UdpServerHelper server (port_1);

  ApplicationContainer apps = server.Install (group.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds(14.0));

  UdpServerHelper server2 (port_2);

  apps = server2.Install (group.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds(14.0));

  UdpClientHelper client (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port_1);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (group.Get (0));
  apps.Start (startTime);
  apps.Stop (endTime);

  UdpClientHelper client2 (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port_2);
  client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client2.Install (group.Get (2));
  apps.Start (startTime);
  apps.Stop (endTime);

  Simulator::Stop (Seconds(14.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
