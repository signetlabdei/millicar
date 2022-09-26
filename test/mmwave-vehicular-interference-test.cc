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
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/internet-module.h"
#include "ns3/config.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularInterferenceTestSuite");

using namespace ns3;
using namespace mmwave;
using namespace millicar;

/**
  The aim of this test is to check whether the interference was evaluated correctly when different groups
  of nodes are transmitting in the same slot, sharing the same cell.
  Communication is done using an ideal channel; for this reason, the position of the vehicles does not influence the results. It has been specified just for the sake of giving context to the example.
*/

class MmWaveVehicularInterferenceTestCase : public TestCase
{
public:
  /**
   * Constructor
   */
  MmWaveVehicularInterferenceTestCase ();

  /**
   * Destructor
   */
  virtual ~MmWaveVehicularInterferenceTestCase ();

  void StartTest (double txPower);

  /**
   * This method run the test
   */
  virtual void DoRun (void);

private:

  /**
   * Callback sink to keep track of the SINR evaluated by the devices associated to GROUP 1
   * \param sinr SpectrumValue corresponding to the evaluated SINR
   */
  void CheckSinrPerceivedG1 (const SpectrumValue& sinr);

  /**
   * Callback sink to keep track of the SINR evaluated by the devices associated to GROUP 2
   * \param sinr SpectrumValue corresponding to the evaluated SINR
   */
  void CheckSinrPerceivedG2 (const SpectrumValue& sinr);

  double g_expectedSinrG1; //!< SINR expected in GROUP 1
  double g_expectedSinrG2; //!< SINR expected in GROUP 2

};

MmWaveVehicularInterferenceTestCase::MmWaveVehicularInterferenceTestCase ()
  : TestCase ("MmwaveVehicular test case (does nothing)")
{
}

MmWaveVehicularInterferenceTestCase::~MmWaveVehicularInterferenceTestCase ()
{
}

void
MmWaveVehicularInterferenceTestCase::CheckSinrPerceivedG1 (const SpectrumValue& sinr)
{
  double actualSinr = Sum (sinr) / sinr.GetSpectrumModel ()->GetNumBands ();

  NS_LOG_DEBUG ("GROUP 1 | Expected SINR: " << g_expectedSinrG1 << "W - Actual SINR: " << actualSinr << "W");
  NS_TEST_ASSERT_MSG_EQ_TOL (actualSinr, g_expectedSinrG1, 1e-2, "Got unexpected value");
}

void
MmWaveVehicularInterferenceTestCase::CheckSinrPerceivedG2 (const SpectrumValue& sinr)
{
  double actualSinr = Sum (sinr) / sinr.GetSpectrumModel ()->GetNumBands ();

  NS_LOG_DEBUG ("GROUP 2 | Expected SINR: " << g_expectedSinrG2 << "W - Actual SINR: " << actualSinr << "W");
  NS_TEST_ASSERT_MSG_EQ_TOL (actualSinr, g_expectedSinrG2, 1e-2, "Got unexpected value");
}

void
MmWaveVehicularInterferenceTestCase::DoRun (void)
{
  double txPower = 30; // dBm

  std::cout << "\nTEST 1" << std::endl;
  g_expectedSinrG1 = 0;
  g_expectedSinrG2 = 0;
  StartTest (txPower);

  txPower = 27; //dBm

  std::cout << "\nTEST 2" << std::endl;
  g_expectedSinrG1 = 0;
  g_expectedSinrG2 = 0;

  StartTest (txPower);

}

void
MmWaveVehicularInterferenceTestCase::StartTest (double txPower)
{
  uint32_t packetSize = 1024; // bytes
  Time startTime = Seconds (1.5);
  Time endTime = Seconds (4.0);
  uint8_t mcs = 12;
  double speed = 20; // m/s

  Config::SetDefault ("ns3::MmWaveSidelinkMac::Mcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (60.0e9));

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

  group2.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (20,120,0));
  group2.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, -speed, 0));

  group2.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (20,140,0));
  group2.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, -speed, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (3);
  helper->SetChannelModelType ("Ideal");
  NetDeviceContainer devs1 = helper->InstallMmWaveVehicularNetDevices (group1);
  NetDeviceContainer devs2 = helper->InstallMmWaveVehicularNetDevices (group2);

  // Only the tx power of vehicles of the second group is tuned.
  DynamicCast<MmWaveVehicularNetDevice>(devs2.Get(0))->GetPhy()->SetTxPower(txPower);

  Ptr<MmWaveSidelinkPhy> slPhy_2 = DynamicCast<MmWaveVehicularNetDevice>(devs2.Get(1))->GetPhy();
  slPhy_2->SetTxPower(txPower);

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

  uint16_t port = 4000;  // well-known echo port number

  NS_LOG_INFO ("Create applications for group number 1.");

  UdpServerHelper server1 (port);

  ApplicationContainer apps1 = server1.Install (group1.Get (1));
  apps1.Start (Seconds (1.0));
  apps1.Stop (Seconds(4.0));

  UdpEchoClientHelper client1 (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client1.SetAttribute ("MaxPackets", UintegerValue (1));
  client1.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
  client1.SetAttribute ("PacketSize", UintegerValue (1024));
  apps1 = client1.Install (group1.Get (0));
  apps1.Start (startTime);
  apps1.Stop (endTime);

  NS_LOG_INFO ("Create applications for group number 2.");

  UdpServerHelper server2 (port);

  ApplicationContainer apps2 = server2.Install (group2.Get (1));
  apps2.Start (Seconds (1.0));
  apps2.Stop (Seconds(4.0));

  UdpEchoClientHelper client2 (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  client2.SetAttribute ("MaxPackets", UintegerValue (1));
  client2.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps2 = client2.Install (group2.Get (0));
  apps2.Start (startTime);
  apps2.Stop (endTime);
  
  // Mandatory to install buildings helper even if there are no buildings, 
  // otherwise V2V-Urban scenario does not work 
  BuildingsHelper::Install (group1);
  BuildingsHelper::Install (group2);
  
  // ---------------------------------

  double kT_dBm_Hz = -174.0;  // dBm/Hz
  double kT_W_Hz = std::pow (10.0, (kT_dBm_Hz - 30) / 10.0);
  double noiseFigureLinear = std::pow (10.0, 5 / 10.0); // Noise Figure is equal to 5 dB by default
  double noisePowerSpectralDensity =  kT_W_Hz * noiseFigureLinear;
  double noisePower = noisePowerSpectralDensity * 1e8; // 1e8 is the bandwidth used for transmission

  // Transmitted Power is 30 dBm by default, which is the equivalent to 1 W
  double txPowerLinear1 = std::pow (10.0, (30 - 30) / 10.0);
  double txPowerLinear2 = std::pow (10.0, (txPower - 30) / 10.0);
  std::cout << "---- GROUP 1 ----" << std::endl;
  std::cout << "Transmitted Power: " << txPowerLinear1 << "W" << std::endl;
  std::cout << "Noise Power: "<< noisePower << std::endl;
  g_expectedSinrG1 = txPowerLinear1/(txPowerLinear2+noisePower);
  std::cout << "Expected SINR: "<< g_expectedSinrG1 << std::endl;

  // create and configure the chunk processor
  Ptr<MmWaveSidelinkPhy> slPhy_1 = DynamicCast<MmWaveVehicularNetDevice>(devs1.Get(1))->GetPhy();
  Ptr<mmWaveChunkProcessor> pData_1 = Create<mmWaveChunkProcessor> ();
  pData_1->AddCallback (MakeCallback (&MmWaveVehicularInterferenceTestCase::CheckSinrPerceivedG1, this));
  slPhy_1->GetSpectrumPhy()->AddDataSinrChunkProcessor (pData_1);

  std::cout << "---- GROUP 2 ----" << std::endl;
  std::cout << "Transmitted Power: " << txPowerLinear2 << "W" << std::endl;
  std::cout << "Noise Power: "<< noisePower << std::endl;
  g_expectedSinrG2 = txPowerLinear2/(txPowerLinear1+noisePower);
  std::cout << "Expected SINR: "<< g_expectedSinrG2 << std::endl;

  Ptr<mmWaveChunkProcessor> pData_2 = Create<mmWaveChunkProcessor> ();
  pData_2->AddCallback (MakeCallback (&MmWaveVehicularInterferenceTestCase::CheckSinrPerceivedG2, this));
  slPhy_2->GetSpectrumPhy()->AddDataSinrChunkProcessor (pData_2);

  Simulator::Stop (Seconds(4.0));
  Simulator::Run ();
  Simulator::Destroy ();

}

class MmWaveVehicularInterferenceTestSuite : public TestSuite
{
public:
  MmWaveVehicularInterferenceTestSuite ();
};

MmWaveVehicularInterferenceTestSuite::MmWaveVehicularInterferenceTestSuite ()
  : TestSuite ("mmwave-vehicular-interference", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new MmWaveVehicularInterferenceTestCase, TestCase::QUICK);
}

static MmWaveVehicularInterferenceTestSuite MmWaveVehicularInterferenceTestSuite;
