/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/test.h"
#include "ns3/mmwave-sidelink-phy.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularSpectrumPhyTestSuite");

using namespace ns3;
using namespace mmwave;

/**
 * In this test, two packets are sent from the tx node the rx node.
 */
class MmWaveVehicularSpectrumPhyTestCase1 : public TestCase
{
public:
  /**
   * Constructor
   */
  MmWaveVehicularSpectrumPhyTestCase1 ();

  /**
   * Destructor
   */
  virtual ~MmWaveVehicularSpectrumPhyTestCase1 ();

private:

  /**
   * This method run the test
   */
  virtual void DoRun (void);

  /**
   * This method is a callback sink which is fired when the rx receives a packet
   * \param p received packet
   */
  void Rx (Ptr<Packet> p);

  /**
   * This method is a callback sink which is fired when the rx updates the SINR
   * estimate
   * \param sinr the sinr value
   */
  void UpdateSinrPerceived (const SpectrumValue& sinr);
};

MmWaveVehicularSpectrumPhyTestCase1::MmWaveVehicularSpectrumPhyTestCase1 ()
  : TestCase ("MmwaveVehicular test case (does nothing)")
{
}

MmWaveVehicularSpectrumPhyTestCase1::~MmWaveVehicularSpectrumPhyTestCase1 ()
{
}

void
MmWaveVehicularSpectrumPhyTestCase1::Rx (Ptr<Packet> p)
{
  NS_LOG_UNCOND ("Rx packet of size " << p->GetSize ());
}

void
MmWaveVehicularSpectrumPhyTestCase1::UpdateSinrPerceived (const SpectrumValue& sinr)
{
  double actualSnr = 10 * log10 (Sum (sinr) / sinr.GetSpectrumModel ()->GetNumBands ());
  NS_LOG_UNCOND ("SINR " << actualSnr << " dB" );
}

void
MmWaveVehicularSpectrumPhyTestCase1::DoRun (void)
{

  double distance = 100; // distance between tx and rx

  // create the tx and rx nodes
  NodeContainer n;
  n.Create (2); // node 0 is tx, node 1 is rx

  // create the tx mobility model and aggregate it with the tx node
  Ptr<MobilityModel> tx_mm = CreateObject<ConstantPositionMobilityModel> ();
  tx_mm->SetPosition (Vector (0.0, 0.0, 0.0));
  n.Get (0)->AggregateObject (tx_mm);

  // create the rx mobility model and aggregate it with the rx node
  Ptr<MobilityModel> rx_mm = CreateObject<ConstantPositionMobilityModel> ();
  rx_mm->SetPosition (Vector (distance, 0.0, 0.0));
  n.Get (1)->AggregateObject (rx_mm);

  // create the tx and rx antenna
  Ptr<AntennaModel> tx_am = CreateObject<IsotropicAntennaModel> ();
  Ptr<AntennaModel> rx_am = CreateObject<IsotropicAntennaModel> ();

  // create the channel
  SpectrumChannelHelper sh = SpectrumChannelHelper::Default ();
  Ptr<SpectrumChannel> sc = sh.Create ();

  // create the configuration
  Ptr<MmWavePhyMacCommon> pmc = CreateObject<MmWavePhyMacCommon> ();

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> tx_ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  tx_ssp->SetMobility (tx_mm);
  tx_ssp->SetAntenna (tx_am);
  tx_ssp->SetChannel (sc);

  // create the tx phy
  Ptr<MmWaveSidelinkPhy> tx_phy = CreateObject<MmWaveSidelinkPhy> (tx_ssp, pmc);

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> rx_ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  rx_ssp->SetMobility (rx_mm);
  rx_ssp->SetAntenna (rx_am);
  rx_ssp->SetChannel (sc);

  // add the rx spectrum phy instance to the spectrum channel
  sc->AddRx (rx_ssp);

  // create the rx phy
  Ptr<MmWaveSidelinkPhy> rx_phy = CreateObject<MmWaveSidelinkPhy> (rx_ssp, pmc);

  // connect the rx callback to the sink
  rx_ssp->SetPhyRxDataEndOkCallback (MakeCallback (&MmWaveVehicularSpectrumPhyTestCase1::Rx, this));

  // create and configure the chunk processor
  Ptr<mmWaveChunkProcessor> pData = Create<mmWaveChunkProcessor> ();
  pData->AddCallback (MakeCallback (&MmWaveSidelinkSpectrumPhy::UpdateSinrPerceived, rx_ssp));
  pData->AddCallback (MakeCallback (&MmWaveVehicularSpectrumPhyTestCase1::UpdateSinrPerceived, this));
  rx_ssp->AddDataSinrChunkProcessor (pData);

  // send a dummy packet burst
  Ptr<Packet> p = Create<Packet> (20); //TODO how to set the size?
  Ptr<PacketBurst> pb1 = CreateObject<PacketBurst> ();
  pb1->AddPacket (p);
  pb1->AddPacket (p);
  tx_phy->AddPacketBurst (pb1);

  Simulator::Stop (Seconds (2));
  Simulator::Run ();
  Simulator::Destroy ();

}

/**
 * Test suite for the class MmWaveSidelinkPhy
 */
class MmWaveVehicularSpectrumPhyTestSuite : public TestSuite
{
public:
  MmWaveVehicularSpectrumPhyTestSuite ();
};

MmWaveVehicularSpectrumPhyTestSuite::MmWaveVehicularSpectrumPhyTestSuite ()
  : TestSuite ("mmwave-vehicular-sidelink-phy-test", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new MmWaveVehicularSpectrumPhyTestCase1, TestCase::QUICK);
}

static MmWaveVehicularSpectrumPhyTestSuite MmWaveVehicularSpectrumPhyTestSuite;
