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
 * This is a test to check if the class MmWaveSidelinkSpectrumPhy correctly
 * computes the SNR.
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

  double m_expectedSinr; //!< expected average SINR
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
  NS_LOG_UNCOND ("Rx event");
}

void
MmWaveVehicularSpectrumPhyTestCase1::UpdateSinrPerceived (const SpectrumValue& sinr)
{
  double actualSnr = 10 * log10 (Sum (sinr) / sinr.GetSpectrumModel ()->GetNumBands ());
  NS_LOG_DEBUG ("expected SINR " << m_expectedSinr << " actual SINR " << actualSnr << " dB" );
  NS_TEST_EXPECT_MSG_EQ_TOL (actualSnr, m_expectedSinr, 1e-2, "Got unexpected value");
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


  double txp = 30.0; // transmission power in dBm
  std::vector<int> subChannelsForTx (72);
  // create the transmission mask, use all the available subchannels
  for (uint8_t i = 0; i < subChannelsForTx.size (); i++)
  {
    subChannelsForTx [i] = i;
  }
  Ptr<SpectrumValue> txPsd = MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (pmc, txp, subChannelsForTx);
  tx_ssp->SetTxPowerSpectralDensity (txPsd);

  // set the rx noise psd
  double noiseFigure = 5.0; // noise figure in dB
  Ptr<SpectrumValue> noisePsd = MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (pmc, noiseFigure);
  rx_ssp->SetNoisePowerSpectralDensity (noisePsd);

  // send an empty packet burst
  Ptr<Packet> p = Create<Packet> (20);
  Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
  pb->AddPacket (p);
  Time duration = MilliSeconds (1); // packet duration
  uint8_t slotInd = 0; // slot index
  uint8_t mcs = 28; // MCS
  uint8_t size = 20; // size of the transport block

  // send the transport block through the spectrum channel
  tx_ssp->StartTxDataFrames (pb, duration, slotInd, mcs, size, subChannelsForTx);

  // compute the expected SINR
  m_expectedSinr = txp + 20 * log10 (3e8 / (4 * M_PI * distance * pmc->GetCenterFrequency ())) + 114 - noiseFigure - 10 * log10 (pmc->GetSystemBandwidth () / 1e6);

  Simulator::Run ();
  Simulator::Destroy ();

}

/**
 * Test suite for the class MmWaveSidelinkSpectrumPhy
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
