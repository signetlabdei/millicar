/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
*   along with this program; if not, write to the Free Software:100cento

*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


#include "mmwave-sidelink-phy.h"
#include <ns3/mmwave-spectrum-value-helper.h>
#include <ns3/mmwave-mac-pdu-tag.h>
#include <ns3/mmwave-mac-pdu-header.h>
#include <ns3/double.h>
#include <ns3/pointer.h>

namespace ns3 {

namespace mmwave {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkPhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveSidelinkPhy);

MmWaveSidelinkPhy::MmWaveSidelinkPhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

MmWaveSidelinkPhy::MmWaveSidelinkPhy (Ptr<MmWaveSidelinkSpectrumPhy> spectrumPhy)
{
  NS_LOG_FUNCTION (this);
  m_sidelinkSpectrumPhy = spectrumPhy;
  Simulator::ScheduleNow (&MmWaveSidelinkPhy::StartSlot, this, 0);
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
                   DoubleValue (30.0),         //TBD zml
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
                    MakeDoubleChecker<double> ())
    .AddAttribute ("SpectrumPhy",
                   "The SpectrumPhy associated to this MmWavePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&MmWaveSidelinkPhy::GetSpectrumPhy),
                   MakePointerChecker <MmWaveSidelinkSpectrumPhy> ());
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

void
MmWaveSidelinkPhy::SetConfigurationParameters (Ptr<MmWavePhyMacCommon> ptrConfig)
{
  m_phyMacConfig = ptrConfig;
}

Ptr<MmWavePhyMacCommon>
MmWaveSidelinkPhy::GetConfigurationParameters (void) const
{
  return m_phyMacConfig;
}

// Ptr<SpectrumValue>
// MmWaveSidelinkPhy::CreateTxPowerSpectralDensity ()
// {
//   Ptr<SpectrumValue> psd = MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (m_phyMacConfig, m_txPower, m_subChannelsForTx);
//   return psd;
// }
//
void
MmWaveSidelinkPhy::StartSlot (uint16_t slotNum)
{
   NS_LOG_FUNCTION (this);
//   m_frameNum = frameNum;
//   m_sfNum = sfNum;
//   m_slotNum = static_cast<uint8_t> (slotNum);
//   m_lastSlotStart = Simulator::Now ();
//   m_varTtiNum = 0;
//
//   // Call MAC before doing anything in PHY
//   // m_phySapUser->SlotIndication (SfnSf (m_frameNum, m_sfNum, m_slotNum));   // trigger mac
//   // TODO: need to define a deterministic scheduling procedure. We could do it
//   // directly here OR define a dummy MAC as discussed
//
//   // update the current slot object, and insert DL/UL CTRL allocations.
//   // That will not be true anymore when true TDD pattern will be used.
//   if (SidelinkSlotAllocInfoExists (SfnSf (frameNum, sfNum, slotNum)))
//     {
//       m_currSlotAllocInfo = RetrieveSidelinkSlotAllocInfo (SfnSf (frameNum, sfNum, slotNum));
//     }
//   else
//     {
//       m_currSlotAllocInfo = SidelinkSlotAllocInfo (SfnSf (frameNum, sfNum, slotNum));
//     }
//
//   std::vector<uint8_t> rbgBitmask (m_phyMacConfig->GetNumRb (), 1);
//
//   NS_ASSERT ((m_currSlotAllocInfo.m_sfnSf.m_frameNum == m_frameNum)
//              && (m_currSlotAllocInfo.m_sfnSf.m_sfNum == m_sfNum
//                  && m_currSlotAllocInfo.m_sfnSf.m_slotNum == m_slotNum));
//
//   auto currentSci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_sci;
//   auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * Time (currentSci->m_symStart);
//
//   Simulator::Schedule (nextVarTtiStart, &MmWaveSidelinkPhy::StartVarTti, this);
}
//
// void
// MmWaveSidelinkPhy::StartVarTti ()
// {
//   NS_LOG_FUNCTION (this);
//   Time varTtiPeriod;
//   const VarTtiAllocInfo & currSlot = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum];
//
//   m_currTbs = currSlot.m_sci->m_tbSize;
//   m_receptionEnabled = false;
//
//   if (currSlot.m_sci->m_type == SciInfoElement::DATA)
//     {
//       varTtiPeriod = SlData (currSlot.m_sci);
//     }
//   else
//     {
//       NS_FATAL_ERROR("There are no different slot types defined.");
//     }
//
//   Simulator::Schedule (varTtiPeriod, &MmWaveSidelinkPhy::EndVarTti, this);
// }
//
// void
// MmWaveSidelinkPhy::EndVarTti ()
// {
//   NS_LOG_FUNCTION (this);
//   NS_LOG_INFO ("Executed varTti " << (+m_varTtiNum) + 1 << " of " << m_currSlotAllocInfo.m_varTtiAllocInfo.size ());
//
//   if (m_varTtiNum == m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1)
//     {
//       // end of slot
//       SfnSf retVal = SfnSf (m_frameNum, m_sfNum, m_slotNum);
//
//       auto slotsPerSubframe = m_phyMacConfig->GetSlotsPerSubframe ();
//       auto subframesPerFrame = m_phyMacConfig->GetSubframesPerFrame ();
//
//       retVal.m_frameNum += (m_sfNum + (m_slotNum + 1) / slotsPerSubframe) / subframesPerFrame;
//       retVal.m_sfNum = (m_sfNum + (m_slotNum + 1) / slotsPerSubframe) % subframesPerFrame;
//       retVal.m_slotNum = (m_slotNum + 1) % slotsPerSubframe;
//
//       Simulator::Schedule (m_lastSlotStart + Seconds(m_phyMacConfig->GetSlotPeriod ()) - Simulator::Now (), &MmWaveSidelinkPhy::StartSlot, this, retVal.m_frameNum, retVal.m_sfNum, retVal.m_slotNum);
//     }
//   else
//     {
//       m_varTtiNum++;
//       Time nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * Time (m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_sci->m_symStart);
//
//       Simulator::Schedule (nextVarTtiStart + m_lastSlotStart - Simulator::Now (), &MmWaveSidelinkPhy::StartVarTti, this);
//     }
//
//   m_receptionEnabled = false;
// }
//
// Time
// MmWaveSidelinkPhy::SlData(const std::shared_ptr<SciInfoElement> &sci)
// {
//   NS_LOG_FUNCTION (this);
//   SetSubChannelsForTransmission (FromRBGBitmaskToRBAssignment (sci->m_rbgBitmask));
//   Time varTtiPeriod = Seconds(m_phyMacConfig->GetSymbolPeriod ()) * sci->m_numSym;
//
//   Ptr<PacketBurst> pktBurst = GetPacketBurst (SfnSf (m_frameNum, m_sfNum, m_slotNum));
//   if (pktBurst && pktBurst->GetNPackets () > 0)
//     {
//       std::list< Ptr<Packet> > pkts = pktBurst->GetPackets ();
//       MmWaveMacPduTag tag;
//       pkts.front ()->PeekPacketTag (tag);
//
//       // LteRadioBearerTag bearerTag;
//       // if (!pkts.front ()->PeekPacketTag (bearerTag))
//       //   {
//       //     NS_FATAL_ERROR ("No radio bearer tag");
//       //   }
//     }
//   else
//     {
//       NS_LOG_WARN ("Send an empty PDU .... ");
//       // sometimes the UE will be scheduled when no data is queued
//       // in this case, send an empty PDU
//       MmWaveMacPduTag tag (SfnSf (m_frameNum, m_sfNum, m_slotNum));
//       Ptr<Packet> emptyPdu = Create <Packet> ();
//       MmWaveMacPduHeader header;
//       MacSubheader subheader (3, 0);    // lcid = 3, size = 0
//       header.AddSubheader (subheader);
//       emptyPdu->AddHeader (header);
//       emptyPdu->AddPacketTag (tag);
//       pktBurst = CreateObject<PacketBurst> ();
//       pktBurst->AddPacket (emptyPdu);
//     }
//
//   Simulator::Schedule (NanoSeconds (1.0), &MmWaveSidelinkPhy::SendDataChannels, this,
//                        pktBurst,
//                        varTtiPeriod - NanoSeconds (2.0),
//                        m_varTtiNum,
//                        sci->m_mcs,
//                        sci->m_tbSize,
//                        FromRBGBitmaskToRBAssignment (sci->m_rbgBitmask));
//   return varTtiPeriod;
// }
//
// void
// MmWaveSidelinkPhy::SendDataChannels (Ptr<PacketBurst> pb,
//   Time duration,
//   uint8_t slotInd,
//   uint8_t mcs,
//   uint32_t size,
//   std::vector<int> rbBitmap)
// {
//   m_sidelinkSpectrumPhy->StartTxDataFrames (pb, duration, slotInd, mcs, size, rbBitmap);
// }
//
// void
// MmWaveSidelinkPhy::SetSubChannelsForTransmission (std::vector <int> mask)
// {
//   Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
//   NS_ASSERT (txPsd);
//   m_sidelinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
// }
//
// bool
// MmWaveSidelinkPhy::SidelinkSlotAllocInfoExists (const SfnSf &retVal) const
// {
//   NS_LOG_FUNCTION (this);
//   for (const auto & alloc : m_slotAllocInfo)
//     {
//       if (alloc.m_sfnSf == retVal)
//         {
//           return true;
//         }
//     }
//   return false;
// }
//
// SidelinkSlotAllocInfo
// MmWaveSidelinkPhy::RetrieveSidelinkSlotAllocInfo ()
// {
//   NS_LOG_FUNCTION (this);
//   SidelinkSlotAllocInfo ret = *m_slotAllocInfo.begin ();
//   m_slotAllocInfo.erase(m_slotAllocInfo.begin ());
//   return ret;
// }
//
// SidelinkSlotAllocInfo
// MmWaveSidelinkPhy::RetrieveSidelinkSlotAllocInfo (const SfnSf &sfnsf)
// {
//   NS_LOG_FUNCTION (this);
//   // NS_LOG_FUNCTION ("ccId:" << +GetCcId () << " slot " << sfnsf);
//
//   for (auto allocIt = m_slotAllocInfo.begin(); allocIt != m_slotAllocInfo.end (); ++allocIt)
//     {
//       if (allocIt->m_sfnSf == sfnsf)
//         {
//           SidelinkSlotAllocInfo ret = *allocIt;
//           m_slotAllocInfo.erase (allocIt);
//           return ret;
//         }
//     }
//
//   NS_FATAL_ERROR("Didn't found the slot");
//   return SidelinkSlotAllocInfo (sfnsf);
// }
//
// std::vector<int>
// MmWaveSidelinkPhy::FromRBGBitmaskToRBAssignment (const std::vector<uint8_t> rbgBitmask) const
// {
//
//   std::vector<int> ret;
//
//   for (uint32_t i = 0; i < rbgBitmask.size (); ++i)
//     {
//       if (rbgBitmask.at (i) == 1)
//         {
//           for (uint32_t k = 0; k < m_phyMacConfig->GetNumRbPerRbg (); ++k)
//             {
//               ret.push_back ((i * m_phyMacConfig->GetNumRbPerRbg ()) + k);
//             }
//         }
//     }
//
//   return ret;
// }
//
//

}

}
