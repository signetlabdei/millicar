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


#include "mmwave-sidelink-phy.h"
#include <ns3/mmwave-spectrum-value-helper.h>
#include <ns3/mmwave-mac-pdu-tag.h>
#include <ns3/mmwave-mac-pdu-header.h>
#include <ns3/double.h>
#include <ns3/pointer.h>

namespace ns3 {

namespace millicar {

MacSidelinkMemberPhySapProvider::MacSidelinkMemberPhySapProvider (Ptr<MmWaveSidelinkPhy> phy)
  : m_phy (phy)
{

}

void
MacSidelinkMemberPhySapProvider::AddTransportBlock (Ptr<PacketBurst> pb, mmwave::TtiAllocInfo info)
{
  m_phy->DoAddTransportBlock (pb, info);
}

void
MacSidelinkMemberPhySapProvider::PrepareForReception (uint16_t rnti)
{
  m_phy->DoPrepareForReceptionFrom (rnti);
}

//-----------------------------------------------------------------------

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkPhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveSidelinkPhy);

MmWaveSidelinkPhy::MmWaveSidelinkPhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

MmWaveSidelinkPhy::MmWaveSidelinkPhy (Ptr<MmWaveSidelinkSpectrumPhy> spectrumPhy, Ptr<mmwave::MmWavePhyMacCommon> confParams)
{
  NS_LOG_FUNCTION (this);
  m_sidelinkSpectrumPhy = spectrumPhy;
  m_phyMacConfig = confParams;

  // create the PHY SAP provider
  m_phySapProvider = new MacSidelinkMemberPhySapProvider (this);

  // create the noise PSD
  Ptr<SpectrumValue> noisePsd = mmwave::MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_phyMacConfig, m_noiseFigure);
  m_sidelinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);

  // schedule the first slot
  Simulator::ScheduleNow (&MmWaveSidelinkPhy::StartSlot, this, mmwave::SfnSf (0, 0, 0));
}

MmWaveSidelinkPhy::~MmWaveSidelinkPhy ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MmWaveSidelinkPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveSidelinkPhy")
    .SetParent<Object> ()
    .AddConstructor<MmWaveSidelinkPhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (30.0),
                   MakeDoubleAccessor (&MmWaveSidelinkPhy::SetTxPower,
                                       &MmWaveSidelinkPhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                    "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                    " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                    "\"the difference in decibels (dB) between"
                    " the noise output of the actual receiver to the noise output of an "
                    " ideal receiver with the same overall gain and bandwidth when the receivers "
                    " are connected to sources at the standard noise temperature T0.\" "
                   "In this model, we consider T0 = 290K.",
                    DoubleValue (5.0),
                    MakeDoubleAccessor (&MmWaveSidelinkPhy::SetNoiseFigure,
                                        &MmWaveSidelinkPhy::GetNoiseFigure),
                    MakeDoubleChecker<double> ());
  return tid;
}

void
MmWaveSidelinkPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveSidelinkPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  delete m_phySapProvider;
}

void
MmWaveSidelinkPhy::SetTxPower (double power)
{
  m_txPower = power;
}
double
MmWaveSidelinkPhy::GetTxPower () const
{
  return m_txPower;
}

void
MmWaveSidelinkPhy::SetNoiseFigure (double nf)
{
  m_noiseFigure = nf;

  // update the noise PSD
  Ptr<SpectrumValue> noisePsd = mmwave::MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_phyMacConfig, m_noiseFigure);
  m_sidelinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
}

double
MmWaveSidelinkPhy::GetNoiseFigure () const
{
  return m_noiseFigure;
}

Ptr<MmWaveSidelinkSpectrumPhy>
MmWaveSidelinkPhy::GetSpectrumPhy () const
{
  return m_sidelinkSpectrumPhy;
}

Ptr<mmwave::MmWavePhyMacCommon>
MmWaveSidelinkPhy::GetConfigurationParameters (void) const
{
  return m_phyMacConfig;
}

MmWaveSidelinkPhySapProvider*
MmWaveSidelinkPhy::GetPhySapProvider () const
{
  NS_LOG_FUNCTION (this);
  return m_phySapProvider;
}

void
MmWaveSidelinkPhy::SetPhySapUser (MmWaveSidelinkPhySapUser* sap)
{
  NS_LOG_FUNCTION (this);
  m_phySapUser = sap;
}

void
MmWaveSidelinkPhy::DoAddTransportBlock (Ptr<PacketBurst> pb, mmwave::TtiAllocInfo info)
{
  // create a new entry for the PHY buffer
  PhyBufferEntry e = std::make_pair (pb, info);

  // add the new entry to the buffer
  m_phyBuffer.push_back (e);
}

void
MmWaveSidelinkPhy::StartSlot (mmwave::SfnSf timingInfo)
{
   NS_LOG_FUNCTION (this << " frame " << timingInfo.m_frameNum << " subframe " << timingInfo.m_sfNum << " slot " << timingInfo.m_slotNum);

  // trigger the MAC
  m_phySapUser->SlotIndication (timingInfo);

  while (m_phyBuffer.size () != 0)
  {
    uint8_t usedSymbols = 0; // the symbol index

    // retrieve the first element in the list
    Ptr<PacketBurst> pktBurst;
    mmwave::TtiAllocInfo info;
    std::tie (pktBurst, info) = m_phyBuffer.front ();

    // send the transport block
    if (info.m_ttiType == mmwave::TtiAllocInfo::DATA)
    {
      usedSymbols += SlData (pktBurst, info);
    }
    else if (info.m_ttiType == mmwave::TtiAllocInfo::CTRL)
    {
      NS_FATAL_ERROR ("Control messages are not currently supported");
    }
    else
    {
      NS_FATAL_ERROR ("Unknown TB type");
    }

    // check if we exceeded the slot boundaries
    NS_ASSERT_MSG (usedSymbols <= m_phyMacConfig->GetSymbPerSlot (), "Exceeded number of available symbols");

    // remove the transport block from the buffer
    m_phyBuffer.pop_front ();
  }

  // update the timing information
  timingInfo = UpdateTimingInfo (timingInfo);
  Simulator::Schedule (m_phyMacConfig->GetSlotPeriod (), &MmWaveSidelinkPhy::StartSlot, this, timingInfo);
}

uint8_t
MmWaveSidelinkPhy::SlData (Ptr<PacketBurst> pb, mmwave::TtiAllocInfo info)
{
  NS_LOG_FUNCTION (this);

  // create the tx PSD
  //TODO do we need to create a new psd at each TTI?
  std::vector<int> subChannelsForTx = SetSubChannelsForTransmission ();

  // compute the tx start time (IndexOfTheFirstSymbol * SymbolDuration)
  Time startTime = info.m_dci.m_symStart * m_phyMacConfig->GetSymbolPeriod ();
  NS_ASSERT_MSG (startTime == info.m_dci.m_symStart * m_phyMacConfig->GetSymbolPeriod (), "startTime was not been correctly set");

  // compute the duration of the transmission (NumberOfSymbols * SymbolDuration)
  Time duration = info.m_dci.m_numSym * m_phyMacConfig->GetSymbolPeriod ();

  NS_ASSERT_MSG (duration == info.m_dci.m_numSym * m_phyMacConfig->GetSymbolPeriod (), "duration was not been correctly set");

  // send the transport block
  Simulator::Schedule (startTime, &MmWaveSidelinkPhy::SendDataChannels, this,
                       pb,
                       duration,
                       info,
                       subChannelsForTx);

  return info.m_dci.m_numSym;
}

void
MmWaveSidelinkPhy::SendDataChannels (Ptr<PacketBurst> pb,
  Time duration,
  mmwave::TtiAllocInfo info,
  std::vector<int> rbBitmap)
{
  // retrieve the RNTI of the device we want to communicate with and properly
  // configure the beamforming
  // NOTE: this information is contained in mmwave::TtiAllocInfo.m_rnti parameter
  NS_ASSERT_MSG (m_deviceMap.find (info.m_rnti) != m_deviceMap.end (), "Device not found");
  m_sidelinkSpectrumPhy->ConfigureBeamforming (m_deviceMap.at (info.m_rnti));

  m_sidelinkSpectrumPhy->StartTxDataFrames (pb, duration, info.m_dci.m_mcs, info.m_dci.m_tbSize, info.m_dci.m_numSym, info.m_dci.m_rnti, info.m_rnti, rbBitmap);
}

std::vector<int>
MmWaveSidelinkPhy::SetSubChannelsForTransmission ()
  {
    // create the transmission mask, use all the available subchannels
    std::vector<int> subChannelsForTx (m_phyMacConfig->GetNumRb ());
    for (uint32_t i = 0; i < subChannelsForTx.size (); i++)
    {
      subChannelsForTx.at(i) = i;
    }

    // create the tx PSD
    Ptr<SpectrumValue> txPsd = mmwave::MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (m_phyMacConfig, m_txPower, subChannelsForTx);

    // set the tx PSD in the spectrum phy
    m_sidelinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);

    return subChannelsForTx;
  }

mmwave::SfnSf
MmWaveSidelinkPhy::UpdateTimingInfo (mmwave::SfnSf info) const
{
  NS_LOG_INFO (this);

  uint32_t nextSlot = info.m_slotNum + 1;
  uint32_t nextSf = info.m_sfNum;
  uint32_t nextFrame = info.m_frameNum;

  if (nextSlot == m_phyMacConfig->GetSlotsPerSubframe ())
  {
    nextSlot = 0;
    ++nextSf;

    if (nextSf == m_phyMacConfig->GetSubframesPerFrame ())
    {
      nextSf = 0;
      ++nextFrame;
    }
  }

  // update the mmwave::SfnSf structure
  info.m_slotNum = nextSlot;
  info.m_sfNum = nextSf;
  info.m_frameNum = nextFrame;

  return info;
}

void
MmWaveSidelinkPhy::DoPrepareForReceptionFrom (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_deviceMap.find (rnti) != m_deviceMap.end (), "Cannot find device with rnti " << rnti);
  m_sidelinkSpectrumPhy->ConfigureBeamforming (m_deviceMap.at (rnti));
}

void
MmWaveSidelinkPhy::AddDevice (uint64_t rnti, Ptr<NetDevice> dev)
{
  NS_LOG_FUNCTION (this);

  if (m_deviceMap.find (rnti) == m_deviceMap.end ())
  {
    m_deviceMap.insert (std::make_pair (rnti,dev));
  }
  else
  {
    NS_FATAL_ERROR ("Device with rnti " << rnti << " already present in the map");
  }
}

void
MmWaveSidelinkPhy::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);

  // Forward the received packet to the MAC layer using the PHY SAP USER
  m_phySapUser->ReceivePhyPdu(p);
}

void
MmWaveSidelinkPhy::GenerateSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize, uint8_t mcs)
{
  NS_LOG_FUNCTION (this << rnti << (uint32_t)numSym << tbSize << (uint32_t)mcs);

  double sinrAvg = Sum (sinr) / (sinr.GetSpectrumModel ()->GetNumBands ());
  NS_LOG_INFO ("Average SINR with dev " << rnti << " = " << 10 * std::log10 (sinrAvg));

  // forward the report to the MAC layer
  m_phySapUser->SlSinrReport (sinr, rnti, numSym, tbSize);
}

} // namespace millicar
} // namespace ns3
