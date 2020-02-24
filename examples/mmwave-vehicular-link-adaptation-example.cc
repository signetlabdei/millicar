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

using namespace ns3;
using namespace millicar;

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularLinkAdaptationExample");

/**
 * Transmit a packet
 * \param p the packet
 * \param txDev the sender
 */
void Tx (Ptr<Packet> p, Ptr<NetDevice> txDev)
{
 NS_LOG_DEBUG ("TX event");

 txDev->Send (p, Address (), 0);
}

/**
 * Callback sink for the SL scheduling callback
 * \param params struct containing the SL schedulink information
 */
void SlSchedulingCallbackSink (SlSchedulingCallback params)
{
  std::ofstream myfile;
  myfile.open ("mmwave-vehicular-link-adaptation-example-out.txt", std::ios::out | std::ios::app);
  myfile << Simulator::Now ().GetSeconds () << " " << (uint16_t)params.mcs << std::endl;
  myfile.close();
}

/**
 * In this example there are two nodes, one is stationary while the other is
 * moving at a constant speed, and they exchange messages at a constant rate.
 * The SlSchedulingTracedCallback of the node with RNTI 1 is attached to the
 * SlSchedulingCallbackSink, which produces the file
 * mmwave-vehicular-link-adaptation-example-out.txt containg the
 * MCS selected for each transmission.
 */
int
main (int argc, char *argv[])
{
  double initialDistance = 10.0; // the initial distance between the two nodes in m
  double finalDistance = 1000.0; // the final distance between the two nodes in m
  double speed = 10.0; // the speed of the moving node in m/s
  double ipi = 1.0; // the inter-packet interval in s
  double frequency = 60e9; // the carrier frequency

  double endTime = (finalDistance - initialDistance) / speed; // time required for the simulation
  uint16_t numOfTx = std::ceil (endTime / ipi); // the number of transmissions

  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (frequency));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::Frequency", DoubleValue (frequency));
  Config::SetDefault ("ns3::MmWaveVehicularSpectrumPropagationLossModel::Frequency", DoubleValue (frequency));
  Config::SetDefault ("ns3::MmWaveVehicularPropagationLossModel::ChannelCondition", StringValue ("l"));

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

  // Set the routing table
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (n.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0: " << n.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1: " << n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  // connect the SL scehduling callback
  Ptr<MmWaveVehicularNetDevice> mmWaveNetDev = DynamicCast<MmWaveVehicularNetDevice> (devs.Get (0));
  Ptr<MmWaveSidelinkMac> mac = mmWaveNetDev->GetMac ();
  mac->TraceConnectWithoutContext ("SchedulingInfo", MakeCallback (&SlSchedulingCallbackSink));

  for (uint32_t i = 0; i < numOfTx; i++)
  {
    // node 0 tx to node 1
    Ptr<Packet> p = Create<Packet> (100);
    Ipv4Header iph;
    iph.SetDestination (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
    p->AddHeader (iph);
    Simulator::Schedule (MilliSeconds (1000 * i * ipi), &Tx, p, devs.Get (0));

    // node 1 tx to node 0
    Ptr<Packet> p1 = Create<Packet> (100);
    Ipv4Header iph1;
    iph1.SetDestination (n.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
    p1->AddHeader (iph1);
    Simulator::Schedule (MilliSeconds (1000 * i * ipi), &Tx, p1, devs.Get (1));
  }

  std::ofstream myfile;
  myfile.open ("mmwave-vehicular-link-adaptation-example-out.txt", std::ios::out | std::ios::trunc);
  myfile << "Time MCS" << std::endl;
  myfile.close();

  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
