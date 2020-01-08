/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/mmwave-vehicular-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/test.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularRateTestSuite");

using namespace ns3;
using namespace mmwave;

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
  // This test creates two nodes moving at 20 m/s, placed at a distance of 10 m.
  // These nodes exchange packets through a UDP application with a fixed sending rate,
  // and they communicate using a wireless channel at 60 GHz.

  uint32_t packetSize = 1024; // bytes
  Time startTime = Seconds (1.5);
  Time endTime = Seconds (60.0);

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

  n.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (10,0,0));
  n.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (20, 0, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
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
  apps.Stop (Seconds(64.0));

  apps.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback (&MmWaveVehicularRateTestCase::Rx, this));

  //
  // Create a UdpClient application to send UDP datagrams from node zero to
  // node one.
  //

  Ptr<MmWaveAmc> m_amc = CreateObject <MmWaveAmc> (helper->GetConfigurationParameters());
  double availableRate = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 0.001; // bps

  uint32_t maxPacketCount = 500000;
  packetSize = m_amc->GetTbSizeFromMcsSymbols(mcs, 14) / 8 - 28;
  Time interPacketInterval =  Seconds(double((packetSize * 8) / availableRate));
  UdpEchoClientHelper client (n.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (n.Get (0));
  apps.Start (startTime);
  apps.Stop (endTime);

  apps.Get(0)->TraceConnectWithoutContext ("Tx", MakeCallback (&MmWaveVehicularRateTestCase::Tx, this));

  Simulator::Stop (Seconds(64.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "----------- MCS :\t\t" << uint32_t(mcs) << " -----------" << std::endl;
  std::cout << "Available Rate:\t\t" << availableRate/1e6 << " Mbps" << std::endl;
  std::cout << "Packets size:\t\t" << packetSize << " Bytes" << std::endl;
  std::cout << "Packets transmitted:\t" << g_txPackets << std::endl;
  std::cout << "Packets received:\t" << g_rxPackets << std::endl;
  std::cout << "Average Throughput:\t" << (double(g_rxPackets*(packetSize+28)*8)/double( g_lastReceived.GetSeconds() - g_firstReceived.GetSeconds()))/1e6 << " Mbps" << std::endl;

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
