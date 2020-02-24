/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2020 University of Padova, Dep. of Information Engineering,
*   SIGNET lab.
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleThree");

using namespace ns3;
using namespace millicar;

/**
  This script creates two pairs of vehicles moving at 20 m/s. The two groups are virtually positioned in the same lane, one in front of the other at 20 m distance.
  Also in the same group, the vehicles are positioned one in front of the other at 20 m distance, and they exchange packets through a UDP application.
  Communication is done via a wireless channel.
  The aim of this example is to check whether the interference was evaluated correctly when different groups
  of nodes are transmitting in the same slot, sharing the same cell. It is basically the same as vehicular-simple-two, except for the position of the vehicles.
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
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (28.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::Frequency", DoubleValue (28.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularSpectrumPropagationLossModel::Frequency", DoubleValue (28.0e9));

  // create the nodes
  NodeContainer group1, group2;
  group1.Create (2);
  group2.Create (2);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (group1);
  mobility.Install (group2);

  group1.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  group1.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group1.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (0,20,0));
  group1.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group2.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,40,0));
  group2.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group2.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (0,60,0));
  group2.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetPropagationLossModelType ("ns3::MmWaveVehicularPropagationLossModel");
  helper->SetSpectrumPropagationLossModelType ("ns3::MmWaveVehicularSpectrumPropagationLossModel");
  NetDeviceContainer devs1 = helper->InstallMmWaveVehicularNetDevices (group1);
  NetDeviceContainer devs2 = helper->InstallMmWaveVehicularNetDevices (group2);

  InternetStackHelper internet;
  internet.Install (group1);
  internet.Install (group2);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devs1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  i = ipv4.Assign (devs2);

  helper->PairDevices(devs1);
  helper->PairDevices(devs2);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (group1.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0 group 1: " << group1.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1 group 1: " << group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  staticRouting = ipv4RoutingHelper.GetStaticRouting (group2.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0 group 2: " << group2.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1 group 2: " << group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  Ptr<mmwave::MmWaveAmc> m_amc = CreateObject <mmwave::MmWaveAmc> (helper->GetConfigurationParameters());
  double availableRate = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 0.001; // bps
  uint16_t port = 4000;  // well-known echo port number
  uint32_t maxPacketCount = 500000;
  packetSize = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 8 - 28;
  Time interPacketInterval =  Seconds(double((packetSize * 8) / availableRate));

  NS_LOG_INFO ("Create applications for group number 1.");

  UdpServerHelper server1 (port);

  ApplicationContainer apps1 = server1.Install (group1.Get (1));
  apps1.Start (Seconds (1.0));
  apps1.Stop (Seconds(14.0));

  UdpEchoClientHelper client1 (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client1.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps1 = client1.Install (group1.Get (0));
  apps1.Start (startTime);
  apps1.Stop (endTime);

  NS_LOG_INFO ("Create applications for group number 2.");

  UdpServerHelper server2 (port);

  ApplicationContainer apps2 = server2.Install (group2.Get (1));
  apps2.Start (Seconds (1.0));
  apps2.Stop (Seconds(14.0));

  UdpEchoClientHelper client2 (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps2 = client2.Install (group2.Get (0));
  apps2.Start (startTime);
  apps2.Stop (endTime);

  Simulator::Stop (Seconds(14.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
