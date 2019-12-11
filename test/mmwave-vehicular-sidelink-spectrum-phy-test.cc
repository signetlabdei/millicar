/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/test.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularSidelinkSpectrumPhyTestSuite");

using namespace ns3;
using namespace mmwave;

/**
 * This is a test to check if the class MmWaveSidelinkSpectrumPhy correctly
 * computes the SNR.
 */
class MmWaveVehicularSidelinkSpectrumPhyTestCase1 : public TestCase
{
public:
  /**
   * Constructor
   */
  MmWaveVehicularSidelinkSpectrumPhyTestCase1 ();

  /**
   * Destructor
   */
  virtual ~MmWaveVehicularSidelinkSpectrumPhyTestCase1 ();

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

MmWaveVehicularSidelinkSpectrumPhyTestCase1::MmWaveVehicularSidelinkSpectrumPhyTestCase1 ()
  : TestCase ("MmwaveVehicular test case (does nothing)")
{
}

MmWaveVehicularSidelinkSpectrumPhyTestCase1::~MmWaveVehicularSidelinkSpectrumPhyTestCase1 ()
{
}

void
MmWaveVehicularSidelinkSpectrumPhyTestCase1::Rx (Ptr<Packet> p)
{
  NS_LOG_UNCOND ("Rx event");
}

void
MmWaveVehicularSidelinkSpectrumPhyTestCase1::UpdateSinrPerceived (const SpectrumValue& sinr)
{
  double actualSnr = 10 * log10 (Sum (sinr) / sinr.GetSpectrumModel ()->GetNumBands ());
  NS_LOG_DEBUG ("expected SINR " << m_expectedSinr << " actual SINR " << actualSnr << " dB" );
  NS_TEST_EXPECT_MSG_EQ_TOL (actualSnr, m_expectedSinr, 1e-2, "Got unexpected value");
}

void
MmWaveVehicularSidelinkSpectrumPhyTestCase1::DoRun (void)
{
  // This test creates two MmWaveSidelinkSpectrumPhy instances (tx and rx)
  // and connects them through a SpectrumChannel. The SepctrumChannel is
  // configured with a ConstantSpeedPropagationDelayModel and a
  // FriisSpectrumPropagationLossModel.
  // The tx instance sends a dummy signal to the rx, which computes the SNR.
  // The computed SNR is compared with the expected SNR, which is computed
  // offline using the link budget and the Friis formulas.

  double distance = 100; // distance between tx and rx

  // create the mobility model
  Ptr<MobilityModel> tx_mm = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<MobilityModel> rx_mm = CreateObject<ConstantPositionMobilityModel> ();
  tx_mm->SetPosition (Vector (0.0, 0.0, 0.0));
  rx_mm->SetPosition (Vector (distance, 0.0, 0.0));

  // create the antenna
  Ptr<AntennaModel> tx_am = CreateObject<IsotropicAntennaModel> ();
  Ptr<AntennaModel> rx_am = CreateObject<IsotropicAntennaModel> ();

  // create the channel
  SpectrumChannelHelper sh = SpectrumChannelHelper::Default ();
  Ptr<SpectrumChannel> sc = sh.Create ();

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> tx_ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  tx_ssp->SetMobility (tx_mm);
  tx_ssp->SetAntenna (tx_am);
  tx_ssp->SetChannel (sc);

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> rx_ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  rx_ssp->SetMobility (rx_mm);
  rx_ssp->SetAntenna (rx_am);
  rx_ssp->SetChannel (sc);

  // add the rx spectrum phy instance to the spectrum channel
  sc->AddRx (rx_ssp);

  // connect the rx callback to the sink
  rx_ssp->SetPhyRxDataEndOkCallback (MakeCallback (&MmWaveVehicularSidelinkSpectrumPhyTestCase1::Rx, this));

  // create and configure the chunk processor
  Ptr<mmWaveChunkProcessor> pData = Create<mmWaveChunkProcessor> ();
  pData->AddCallback (MakeCallback (&MmWaveSidelinkSpectrumPhy::UpdateSinrPerceived, rx_ssp));
  pData->AddCallback (MakeCallback (&MmWaveVehicularSidelinkSpectrumPhyTestCase1::UpdateSinrPerceived, this));
  rx_ssp->AddDataSinrChunkProcessor (pData);

  // create the tx psd
  Ptr<MmWavePhyMacCommon> pmc = CreateObject<MmWavePhyMacCommon> ();
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
  uint8_t numSym = 14; // number of symbols dedicated to the transport block 
  uint8_t size = 20; // size of the transport block
  uint16_t rnti = 0; // RNTI that identifies the device in the cell (officially, it is set by the MAC)

  // send the transport block through the spectrum channel
  tx_ssp->StartTxDataFrames (pb, duration, slotInd, mcs, size, numSym, rnti, subChannelsForTx);

  // compute the expected SINR
  m_expectedSinr = txp + 20 * log10 (3e8 / (4 * M_PI * distance * pmc->GetCenterFrequency ())) + 114 - noiseFigure - 10 * log10 (pmc->GetSystemBandwidth () / 1e6);

  Simulator::Run ();
  Simulator::Destroy ();

}

/**
 * Test suite for the class MmWaveSidelinkSpectrumPhy
 */
class MmWaveVehicularSidelinkSpectrumPhyTestSuite : public TestSuite
{
public:
  MmWaveVehicularSidelinkSpectrumPhyTestSuite ();
};

MmWaveVehicularSidelinkSpectrumPhyTestSuite::MmWaveVehicularSidelinkSpectrumPhyTestSuite ()
  : TestSuite ("mmwave-vehicular-sidelink-spectrum-phy", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new MmWaveVehicularSidelinkSpectrumPhyTestCase1, TestCase::QUICK);
}

static MmWaveVehicularSidelinkSpectrumPhyTestSuite MmWaveVehicularSidelinkSpectrumPhyTestSuite;
