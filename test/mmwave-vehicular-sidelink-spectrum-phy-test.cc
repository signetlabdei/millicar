/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/test.h"

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularSidelinkSpectrumPhyTestSuite");

using namespace ns3;
using namespace mmwave_vehicular;

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
  NS_LOG_DEBUG ("Rx event");
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
  Ptr<mmwave::mmWaveChunkProcessor> pData = Create<mmwave::mmWaveChunkProcessor> ();
  pData->AddCallback (MakeCallback (&MmWaveSidelinkSpectrumPhy::UpdateSinrPerceived, rx_ssp));
  pData->AddCallback (MakeCallback (&MmWaveVehicularSidelinkSpectrumPhyTestCase1::UpdateSinrPerceived, this));
  rx_ssp->AddDataSinrChunkProcessor (pData);

  // create the tx psd
  Ptr<mmwave::MmWavePhyMacCommon> pmc = CreateObject<mmwave::MmWavePhyMacCommon> ();
  double txp = 30.0; // transmission power in dBm
  std::vector<int> subChannelsForTx (72);
  // create the transmission mask, use all the available subchannels
  for (uint8_t i = 0; i < subChannelsForTx.size (); i++)
  {
    subChannelsForTx [i] = i;
  }
  Ptr<SpectrumValue> txPsd = mmwave::MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (pmc, txp, subChannelsForTx);
  tx_ssp->SetTxPowerSpectralDensity (txPsd);

  // set the rx noise psd
  double noiseFigure = 5.0; // noise figure in dB
  Ptr<SpectrumValue> noisePsd = mmwave::MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (pmc, noiseFigure);
  rx_ssp->SetNoisePowerSpectralDensity (noisePsd);

  // set the device for the receiving side, as the MmWaveSidelinkSpectrumPhy checks the RNTI of the receiver
  // to drop or not the packet
  uint16_t rxRnti = 1;
  Ptr<MmWaveSidelinkPhy> phy = CreateObject<MmWaveSidelinkPhy> (rx_ssp, pmc);
  Ptr<MmWaveSidelinkMac> mac = CreateObject<MmWaveSidelinkMac> (pmc);
  phy->SetPhySapUser (mac->GetPhySapUser ());
  mac->SetPhySapProvider (phy->GetPhySapProvider ());
  mac->SetRnti (rxRnti);
  Ptr<MmWaveVehicularNetDevice> device = CreateObject<MmWaveVehicularNetDevice> (phy, mac);
  NodeContainer nc;
  nc.Create(1);
  rx_ssp->SetDevice (device);
  device->SetNode(nc.Get(0));
  nc.Get(0)->AddDevice(device);

  // send an empty packet burst
  Ptr<Packet> p = Create<Packet> (20);
  Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
  pb->AddPacket (p);
  Time duration = MilliSeconds (1); // packet duration
  uint8_t slotInd = 0; // slot index
  uint8_t mcs = 0; // MCS
  uint8_t numSym = 14; // number of symbols dedicated to the transport block
  uint8_t size = 20; // size of the transport block

  // send the transport block through the spectrum channel
  tx_ssp->StartTxDataFrames (pb, duration, slotInd, mcs, size, numSym, 0, rxRnti, subChannelsForTx);

  // compute the expected SINR
  m_expectedSinr = txp + 20 * log10 (3e8 / (4 * M_PI * distance * pmc->GetCenterFrequency ())) + 114 - noiseFigure - 10 * log10 (pmc->GetSystemBandwidth () / 1e6);

  Simulator::Stop (MilliSeconds(2));
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
