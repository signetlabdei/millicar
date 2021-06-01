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

#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/mmwave-vehicular-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include <fstream>
#include "ns3/applications-module.h"

using namespace ns3;
using namespace millicar;

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularLinkAdaptationExample");

/**
 * In this example there are two nodes, one is stationary while the other is
 * moving at a constant speed, and they exchange messages at a constant rate.
 * The simulation produces the file sinr-mcs.txt containg the
 * MCS selected for each transmission.
 */
int
main (int argc, char *argv[])
{
  double initialDistance = 10.0; // the initial distance between the two nodes in m
  double finalDistance = 10000.0; // the final distance between the two nodes in m
  double speed = 100.0; // the speed of the moving node in m/s
  double frequency = 60e9; // the carrier frequency

  double endTime = (finalDistance - initialDistance) / speed; // time required for the simulation

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (true));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (frequency));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::ChannelCondition", StringValue ("l"));
  Config::SetDefault ("ns3::MmWaveVehicularHelper::SchedulingPatternOption", EnumValue(2)); // use 2 for SchedulingPatternOption=OPTIMIZED, 1 or SchedulingPatternOption=DEFAULT
  Config::SetDefault ("ns3::MmWaveVehicularNetDevice::RlcType", StringValue("LteRlcUm"));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (50*1024));

  // create the nodes
  NodeContainer n;
  n.Create (2);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (n);

  n.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, 0, 0));
  n.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

  n.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (initialDistance, 0, 0));
  n.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

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
  
  UdpClientHelper client (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), 4000);
  client.SetAttribute ("MaxPackets", UintegerValue (0xFFFFF));
  client.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
  client.SetAttribute ("PacketSize", UintegerValue (512));
  ApplicationContainer clientApps = client.Install (n.Get (0));
  clientApps.Start (Seconds(1.0));
  
  UdpEchoServerHelper server (4000);
  ApplicationContainer serverApps = server.Install (n.Get (1));
  serverApps.Start (Seconds (0.0));

  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
