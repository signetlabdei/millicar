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
#include "ns3/test.h"
#include "ns3/buildings-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/config.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularRateTestSuite");

using namespace ns3;
using namespace mmwave;
using namespace millicar;

/**
 * This is a test to check if the designed vehicular stack (MAC and PHY) is able
 * to run on a basic scenario: two vehicle moving at constant velocity and constant distance.
 * The distance increases among different tests of the suite.
 */
class MmWaveVehicularRateTestCase : public TestCase
{
public:
  /**
   * Constructor
   */
  MmWaveVehicularRateTestCase ();

  /**
   * Destructor
   */
  virtual ~MmWaveVehicularRateTestCase ();

  void StartTest (uint8_t mcs);

  /**
   * This method run the test
   */
  virtual void DoRun (void);

private:

  /**
   * Callback sink fired when the rx receives a packet
   * \param p received packet
   */
  void Rx (Ptr<const Packet> p);

  /**
   * Callback sink fired when the tx transmits a packet
   * \param p transmitted packet
   */
  void Tx (Ptr<const Packet> p);

  uint32_t g_rxPackets; //!< total number of received packets on a single test-case
  uint32_t g_txPackets; //!< total number of transmitted packets on a single test-case

  Time g_firstReceived; //!< timestamp of the first time a packet is received on a single test-case
  Time g_lastReceived; //!< timestamp of the last received packet on a single test-case

};

MmWaveVehicularRateTestCase::MmWaveVehicularRateTestCase ()
  : TestCase ("MmwaveVehicular test case (does nothing)")
{
}

MmWaveVehicularRateTestCase::~MmWaveVehicularRateTestCase ()
{
}

void
MmWaveVehicularRateTestCase::Tx (Ptr<const Packet> p)
{
  NS_LOG_DEBUG ("TX event");
  g_txPackets++;
}

void
MmWaveVehicularRateTestCase::Rx (Ptr<const Packet> p)
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

void
MmWaveVehicularRateTestCase::DoRun (void)
{
  uint8_t mcs = 0;

  while (mcs <= 28)
  {
    g_txPackets = 0;
    g_rxPackets = 0;
    // perform the test
    StartTest (mcs);
    NS_TEST_ASSERT_MSG_EQ (g_txPackets, g_rxPackets, "The channel is ideal, no packet should be lost.");
    mcs++;

  }
}

void
MmWaveVehicularRateTestCase::StartTest (uint8_t mcs)
{
  // This test creates two nodes that exchange packets through a UDP application
  // with a fixed sending rate.

  Time startTime = MilliSeconds (100);
  Time endTime = MilliSeconds (500);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (false));
  Config::SetDefault ("ns3::LteRlcTm::MaxTxBufferSize", UintegerValue (1024 * 1024 * 1024)); // we want to avoid buffer drops
  Config::SetDefault ("ns3::MmWaveVehicularNetDevice::Mtu", UintegerValue (65535)); // set equal to the IP MTU

  // create the nodes
  NodeContainer n;
  n.Create (2);

  // create the mobility models
  // NOTE: the position does not matter since we are not applying any channel
  // model, we just set it to avoid failures
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (n);

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetChannelModelType ("V2V-Urban");
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
  
  // Mandatory to install buildings helper even if there are no buildings, 
  // otherwise V2V-Urban scenario does not work
  BuildingsHelper::Install (n);
  
  //
  // Create a UdpEchoServer application on node one.
  //
  uint16_t port = 4000;
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (n.Get (1));
  apps.Start (MilliSeconds (0));

  apps.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback (&MmWaveVehicularRateTestCase::Rx, this));

  //
  // Create a UdpClient application to send UDP datagrams from node zero to
  // node one.
  //
  Ptr<MmWaveAmc> m_amc = CreateObject <MmWaveAmc> (helper->GetConfigurationParameters());

  // The user is assigned to a single slot per subframe.
  // Each slot has 14 OFDM symbols, hence the theoretical available rate is:
  // bitPerSymbol * 14 / 1 ms
  uint32_t availableBytesPerSlot = m_amc->CalculateTbSize(mcs, 14);
  double availableRate =  availableBytesPerSlot * 8 * 1e3; // bps

  // Configure the application to send a single packet per subframe, which has
  // to occupy all the available resources in the slot
  uint32_t headerSize = 30; // header sizes (UDP, IP, PDCP, RLC)
  uint32_t packetSize = availableBytesPerSlot - headerSize; // TB size - header sizes (UDP, IP, PDCP, RLC)
  TimeValue interPacketInterval =  MilliSeconds (1);
  UdpEchoClientHelper client (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));
  client.SetAttribute ("Interval", interPacketInterval);
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (n.Get (0));
  apps.Start (startTime);
  apps.Stop (endTime);

  apps.Get(0)->TraceConnectWithoutContext ("Tx", MakeCallback (&MmWaveVehicularRateTestCase::Tx, this));

  Simulator::Stop (endTime + Seconds (1.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "----------- MCS :\t\t" << uint32_t(mcs) << " -----------" << std::endl;
  std::cout << "Available Rate:\t\t" << availableRate/1e6 << " Mbps" << std::endl;
  std::cout << "Packets size:\t\t" << packetSize << " Bytes" << std::endl;
  std::cout << "Packets transmitted:\t" << g_txPackets << std::endl;
  std::cout << "Packets received:\t" << g_rxPackets << std::endl;
  std::cout << "Average Throughput:\t" << (double(g_rxPackets)*double(packetSize + headerSize)*8/double( g_lastReceived.GetSeconds() - g_firstReceived.GetSeconds()))/1e6 << " Mbps" << std::endl;

}

/**
 * Test suite for the class MmWaveSidelinkSpectrumPhy
 */
class MmWaveVehicularRateTestSuite : public TestSuite
{
public:
  MmWaveVehicularRateTestSuite ();
};

MmWaveVehicularRateTestSuite::MmWaveVehicularRateTestSuite ()
  : TestSuite ("mmwave-vehicular-rate", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new MmWaveVehicularRateTestCase, TestCase::QUICK);
}

static MmWaveVehicularRateTestSuite MmWaveVehicularRateTestSuite;
