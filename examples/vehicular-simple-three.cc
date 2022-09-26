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
#include "ns3/mobility-module.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/internet-module.h"
#include "ns3/config.h"
#include "ns3/command-line.h"

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleThree");

using namespace ns3;
using namespace millicar;

/**
  In this exampls, we considered two groups of vehicles traveling in
  the same direction, either in the same ([sameLane] = true) or different lanes
  ([sameLane] = false). Each group is composed of two vehicles, one behind the
  other, moving at a constant speed of [speed] m/s and keeping a safety distance of
  [intraGroupDistance] m. Within a group, the rear vehicle acts as a server and
  generates data packets which are sent to the front vehicle. We considered an
  ON-OFF traffic model, in which a UDP source keeps switching between the ON and
  the OFF states. During the ON state, the source generates packets at a
  constant rate for [onPeriod] ms, while in the OFF state it stays idle for a random
  amount of time, which follows an exponential distribution with mean
  [offPeriod] ms. All vehicles operate at 28 GHz with a bandwidth of 100 MHz,
  possibly interfering in case of concurrent transmissions, and are equipped
  with a Uniform Planar Array (UPA) of [numAntennaElements] antenna elements to
  establish directional communications.
  The simulation runs for [stopTime] ms, at outputs the overall Packet Reception
  Ratio.
*/

uint32_t g_txPacketsGroup1 = 0; // tx packet counter for group 1
uint32_t g_txPacketsGroup2 = 0; // tx packet counter for group 2
uint32_t g_rxPacketsGroup1 = 0; // rx packet counter for group 1
uint32_t g_rxPacketsGroup2 = 0; // rx packet counter for group 2

static void Tx (Ptr<OutputStreamWrapper> stream, uint8_t group, Ptr<const Packet> p)
{
  *stream->GetStream () << "Tx\t" << Simulator::Now ().GetSeconds () << "\t" << p->GetSize () << std::endl;
  if (group == 1)
  {
    ++g_txPacketsGroup1;
  }
  else if (group == 2)
  {
    ++g_txPacketsGroup2;
  }
}

static void Rx (Ptr<OutputStreamWrapper> stream, uint8_t group, Ptr<const Packet> packet, const Address& from)
{
  Ptr<Packet> newPacket = packet->Copy ();
  SeqTsHeader seqTs;
  newPacket->RemoveHeader (seqTs);
  if (seqTs.GetTs ().GetNanoSeconds () != 0)
  {
    uint64_t delayNs = Simulator::Now ().GetNanoSeconds () - seqTs.GetTs ().GetNanoSeconds ();
    *stream->GetStream () << "Rx\t" << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize() << "\t" <<  delayNs << std::endl;
  }
  else
  {
    *stream->GetStream () << "Rx\t" << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize() << std::endl;
  }

  if (group == 1)
  {
    ++g_rxPacketsGroup1;
  }
  else if (group == 2)
  {
    ++g_rxPacketsGroup2;
  }
}

int main (int argc, char *argv[])
{
  uint32_t startTime = 100; // application start time in milliseconds
  uint32_t stopTime = 5000; // application stop time in milliseconds
  uint32_t onPeriod = 100; // on period duration in milliseconds
  uint32_t offPeriod = 100; // mean duration of the off period in milliseconds
  double dataRate = 100e6; // data rate in bps
  uint32_t mcs = 28; // modulation and coding scheme

  double interGroupDistance = 10; // distance between the two groups in meters
  double speed = 20; // speed m/s
  bool sameLane = true; // if true the two groups lie on the same lane (as a platoon), if false they lie on adjacent lanes

  uint32_t numAntennaElements = 4; // number of antenna elements

  bool orthogonalResources = true; // if true, resouces are orthogonal among the two groups, if false resources are shared
  std::string scenario = "V2V-Highway";

  CommandLine cmd;
  cmd.AddValue ("startTime", "application stop time in milliseconds", startTime);
  cmd.AddValue ("stopTime", "application stop time in milliseconds", stopTime);
  cmd.AddValue ("onPeriod", "on period duration in milliseconds", onPeriod);
  cmd.AddValue ("offPeriod", "mean duration of the off period in milliseconds", offPeriod);
  cmd.AddValue ("dataRate", "data rate in bps", dataRate);
  cmd.AddValue ("mcs", "modulation and coding scheme", mcs);
  cmd.AddValue ("interGroupDistance", "distance between the two groups in meters", interGroupDistance);
  cmd.AddValue ("speed", "the speed of the vehicles in m/s", speed);
  cmd.AddValue ("numAntennaElements", "number of antenna elements", numAntennaElements);
  cmd.AddValue ("orthogonalResources", "if true, resouces are orthogonal among the two groups, if false resources are shared", orthogonalResources);
  cmd.AddValue ("sameLane", "if true the two groups lie on the same lane, if false they lie on adjacent lanes", sameLane);
  cmd.AddValue ("scenario", "set the vehicular scenario", scenario);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (false));
  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (28.0e9));
  Config::SetDefault ("ns3::MmWaveVehicularNetDevice::RlcType", StringValue("LteRlcUm"));
  
  Config::SetDefault ("ns3::MmWaveVehicularHelper::SchedulingPatternOption", EnumValue(2)); // use 2 for SchedulingPatternOption=OPTIMIZED, 1 or SchedulingPatternOption=DEFAULT

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (10)));
  
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (500*1024));

  Config::SetDefault ("ns3::UniformPlanarArray::NumColumns", UintegerValue (std::sqrt (numAntennaElements)));
  Config::SetDefault ("ns3::UniformPlanarArray::NumRows", UintegerValue (std::sqrt (numAntennaElements)));

  // create the nodes
  NodeContainer group1, group2;
  group1.Create (2);
  group2.Create (2);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (group1);
  mobility.Install (group2);

  double intraGroupDistance = std::max (2.0, 2*speed); // distance between cars belonging to the same group in meters
  group1.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  group1.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  group1.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (intraGroupDistance,0,0));
  group1.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  if (sameLane)
  {
    group2.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (intraGroupDistance+interGroupDistance,0,0));
    group2.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (intraGroupDistance*2+interGroupDistance,0,0));

  }
  else
  {
    group2.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (intraGroupDistance,interGroupDistance,0));
    group2.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (intraGroupDistance*2,interGroupDistance,0));
  }
  group2.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  group2.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetChannelModelType (scenario);
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

  if (orthogonalResources)
  {
    // resources are orthogonally partitioned among the two groups
    helper->PairDevices (NetDeviceContainer (devs1, devs2));
  }
  else
  {
    // resources are othogally partitioned among devices belonging to the
    // same group, while shared among the two groups
    helper->PairDevices(devs1);
    helper->PairDevices(devs2);
  }

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (group1.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0 group 1: " << group1.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1 group 1: " << group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  staticRouting = ipv4RoutingHelper.GetStaticRouting (group2.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  // Mandatory to install buildings helper even if there are no buildings, 
  // otherwise V2V-Urban scenario does not work
  BuildingsHelper::Install (group1);
  BuildingsHelper::Install (group2);

  // create the random variables used to setup the applications
  Ptr<ConstantRandomVariable> onPeriodRv = CreateObjectWithAttributes<ConstantRandomVariable> ("Constant", DoubleValue (onPeriod / 1000.0));
  Ptr<ExponentialRandomVariable> offPeriodRv = CreateObjectWithAttributes<ExponentialRandomVariable> ("Mean", DoubleValue (offPeriod / 1000.0));

  // create the appplications for group 1
  uint32_t port = 1234;
  OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port)));
  onoff.SetConstantRate (DataRate (std::to_string (dataRate)+"b/s"));
  onoff.SetAttribute ("OnTime", PointerValue (onPeriodRv));
  onoff.SetAttribute ("OffTime", PointerValue (offPeriodRv));
  ApplicationContainer onOffApps = onoff.Install (group1.Get (0));
  
  PacketSinkHelper sink ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  ApplicationContainer packetSinkApps = sink.Install (group1.Get (1));

  // create the applications for group 2
  onoff.SetAttribute ("Remote", AddressValue (InetSocketAddress (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port)));
  onOffApps.Add (onoff.Install (group2.Get (0)));

  sink.SetAttribute ("Local", AddressValue (InetSocketAddress (Ipv4Address::GetAny (), port)));
  packetSinkApps.Add (sink.Install (group2.Get (1)));

  onOffApps.Start (MilliSeconds (startTime));
  onOffApps.Stop (MilliSeconds (stopTime));

  packetSinkApps.Start (MilliSeconds (0.0));

  // connect the trace sources to the sinks
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("group-1.txt");
  onOffApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&Tx, stream, 1));
  packetSinkApps.Get (0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&Rx, stream, 1));

  stream = asciiTraceHelper.CreateFileStream ("group-2.txt");
  onOffApps.Get(1)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&Tx, stream, 2));
  packetSinkApps.Get (1)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&Rx, stream, 2));

  Simulator::Stop (MilliSeconds(stopTime + 1000));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "PRR " << double(g_rxPacketsGroup1 + g_rxPacketsGroup2) / double(g_txPacketsGroup1 + g_txPacketsGroup2) << std::endl;

  return 0;
}
