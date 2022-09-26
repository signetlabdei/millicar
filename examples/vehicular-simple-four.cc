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
#include "ns3/buildings-module.h"
#include "ns3/internet-module.h"
#include "ns3/config.h"
#include "ns3/command-line.h"

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleFour");

using namespace ns3;
using namespace millicar;

/**
  This script creates two pairs of vehicles moving at 20 m/s. The two groups are virtually positioned in two different lanes.
  In the same group, the vehicles are positioned one in front of the other at 20 m distance, and they exchange packets through a UDP application.
  Communication is done via a wireless channel.
  The aim of this example is to check whether the interference was evaluated correctly when different groups
  of nodes are transmitting in the same slot, sharing the same cell.
*/

static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
 SeqTsHeader header;

 p->PeekHeader(header);

 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() << "\t" << header.GetSeq() << "\t" << header.GetTs().GetSeconds() << std::endl;
}

int main (int argc, char *argv[])
{

  Time endTime = Seconds (10.0);
  double bandwidth = 1e8;
  std::string scenario = "V2V-Urban";

  double speed = 20; // m/s
  CommandLine cmd;
  cmd.AddValue ("vehicleSpeed", "The speed of the vehicle", speed);
  cmd.AddValue ("scenario", "set the vehicular scenario", scenario);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (true));
  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (28));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (60.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularNetDevice::RlcType", StringValue("LteRlcUm"));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (50*1024));
  
  Config::SetDefault ("ns3::MmWaveVehicularHelper::SchedulingPatternOption", EnumValue(2)); // use 2 for SchedulingPatternOption=OPTIMIZED, 1 or SchedulingPatternOption=DEFAULT
  Config::SetDefault ("ns3::MmWaveVehicularHelper::Bandwidth", DoubleValue (bandwidth));

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (10)));
  
  // create the nodes
  NodeContainer group;
  group.Create (3);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (group);

  group.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  group.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, -speed, 0));

  group.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (20,0,0));
  group.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  group.Get (2)->GetObject<MobilityModel> ()->SetPosition (Vector (10,20,0));
  group.Get (2)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, speed, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetChannelModelType (scenario);
  NetDeviceContainer devs = helper->InstallMmWaveVehicularNetDevices (group);
  
  // Mandatory to install buildings helper even if there are no buildings, 
  // otherwise V2V-Urban scenario does not work
  BuildingsHelper::Install (group);

  InternetStackHelper internet;
  internet.Install (group);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devs);

  helper->PairDevices(devs);

  // Ipv4StaticRoutingHelper ipv4RoutingHelper;
  //
  // Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (group.Get (0)->GetObject<Ipv4> ());
  // staticRouting->SetDefaultRoute (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );
  //
  // staticRouting = ipv4RoutingHelper.GetStaticRouting (group.Get (2)->GetObject<Ipv4> ());
  // staticRouting->SetDefaultRoute (group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0: " << group.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1: " << group.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 2: " << group.Get (2)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  Ptr<mmwave::MmWaveAmc> m_amc = CreateObject <mmwave::MmWaveAmc> (helper->GetConfigurationParameters());
  double availableRate = m_amc->CalculateTbSize (28, 14) * 8 / 0.001; // bps
  uint16_t port_1 = 4000;  // well-known echo port number
  uint16_t port_2 = 4001;

  UdpEchoServerHelper server (port_1);

  ApplicationContainer apps = server.Install (group.Get (2));
  apps.Start (Seconds (0.0));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream_1 = asciiTraceHelper.CreateFileStream ("user_1.txt");
  apps.Get(0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&Rx, stream_1));

  UdpEchoServerHelper server2 (port_2);

  ApplicationContainer apps2 = server2.Install (group.Get (2));
  apps2.Start (Seconds (0.0));

  Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper.CreateFileStream ("user_2.txt");
  apps2.Get(0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&Rx, stream_2));

  uint32_t maxPacketCount = 800000;
  uint32_t packetSize = 512;
  NS_LOG_UNCOND(availableRate);
  Time interPacketInterval =  Seconds(double((packetSize * 8) / availableRate));

  UdpClientHelper client (group.Get (2)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port_1);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer clientApps = client.Install (group.Get (0));
  clientApps.Start (Seconds(1.0));
  clientApps.Stop (endTime);

  UdpClientHelper client2 (group.Get (2)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port_2);
  client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer clientApps2 = client2.Install (group.Get (1));
  clientApps2.Start (Seconds(1.0));
  clientApps2.Stop (endTime);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds(18.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
