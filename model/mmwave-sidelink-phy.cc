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

MmWaveSidelinkPhy::MmWaveSidelinkPhy (Ptr<MmWaveSpectrumPhy> channelPhy,
                          const Ptr<Node> &n)
  : MmWavePhy (channelPhy)
{
  NS_LOG_FUNCTION (this);
  m_currSlotAllocInfo = SidelinkSfnSf (0,0,0,0);
  Simulator::ScheduleWithContext (n->GetId (), MilliSeconds (0),
                                  &MmWaveSidelinkPhy::StartSlot, this, 0, 0, 0);
}

MmWaveSidelinkPhy::~MmWaveSidelinkPhy ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MmWaveSidelinkPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveSidelinkPhy")
    .SetParent<MmWavePhy> ()
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
                    MakeDoubleAccessor (&MmWaveSidelinkPhy::m_noiseFigure),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("SpectrumPhy",
                   "The SpectrumPhy associated to this MmWavePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&MmWaveSidelinkPhy::GetSpectrumPhy),
                   MakePointerChecker <MmWaveSpectrumPhy> ());
  return tid;
}

void
MmWaveSidelinkPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  MmWavePhy::DoInitialize ();
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
MmWaveSidelinkPhy::SetNoiseFigure (double pf)
{
}

double
MmWaveSidelinkPhy::GetNoiseFigure () const
{
  return m_noiseFigure;
}

Ptr<MmWaveSpectrumPhy>
MmWaveSidelinkPhy::GetSpectrumPhy () const
{
  return m_spectrumPhy;
}

Ptr<SpectrumValue>
MmWaveSidelinkPhy::CreateTxPowerSpectralDensity ()
{
  Ptr<SpectrumValue> psd;
  return psd;
}

void
MmWaveSidelinkPhy::StartSlot (uint16_t frameNum, uint8_t sfNum, uint16_t slotNum)
{
  NS_LOG_FUNCTION (this);
  m_frameNum = frameNum;
  m_sfNum = sfNum;
  m_slotNum = static_cast<uint8_t> (slotNum);
  m_lastSlotStart = Simulator::Now ();
  m_varTtiNum = 0;

  // Call MAC before doing anything in PHY
  //m_phySapUser->SlotIndication (SidelinkSfnSf (m_frameNum, m_sfNum, m_slotNum, 0));   // trigger mac

  // update the current slot object, and insert DL/UL CTRL allocations.
  // That will not be true anymore when true TDD pattern will be used.
  if (SidelinkSlotAllocInfoExists (SidelinkSfnSf (frameNum, sfNum, slotNum, m_varTtiNum)))
    {
      m_currSlotAllocInfo = RetrieveSidelinkSlotAllocInfo (SidelinkSfnSf (frameNum, sfNum, slotNum, m_varTtiNum));
    }
  else
    {
      m_currSlotAllocInfo = SidelinkSlotAllocInfo (SidelinkSfnSf (frameNum, sfNum, slotNum, m_varTtiNum));
    }

  std::vector<uint8_t> rbgBitmask (m_sidelinkPhyMacConfig->GetBandwidthInRbg (), 1);

  // VarTtiAllocInfo dlCtrlSlot (std::make_shared<DciInfoElementTdma> (0, 1, DciInfoElementTdma::DL, DciInfoElementTdma::CTRL, rbgBitmask));
  // VarTtiAllocInfo ulCtrlSlot (std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, DciInfoElementTdma::UL, DciInfoElementTdma::CTRL, rbgBitmask));
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_front (dlCtrlSlot);
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_back (ulCtrlSlot);

  NS_ASSERT ((m_currSlotAllocInfo.m_sidelinkSfnSf.m_frameNum == m_frameNum)
             && (m_currSlotAllocInfo.m_sidelinkSfnSf.m_subframeNum == m_sfNum
                 && m_currSlotAllocInfo.m_sidelinkSfnSf.m_slotNum == m_slotNum));

  // for (const auto & alloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
  //   {
  //     std::string direction, type;
  //     if (alloc.m_dci->m_type == DciInfoElementTdma::CTRL)
  //       {
  //         type = "CTRL";
  //       }
  //     else if (alloc.m_dci->m_type == DciInfoElementTdma::CTRL_DATA)
  //       {
  //         type = "CTRL_DATA";
  //       }
  //     else
  //       {
  //         type = "DATA";
  //       }
  //
  //     if (alloc.m_dci->m_format == DciInfoElementTdma::UL)
  //       {
  //         direction = "UL";
  //       }
  //     else
  //       {
  //         direction = "DL";
  //       }
  //     NS_LOG_INFO ("Allocation from sym " << static_cast<uint32_t> (alloc.m_sci->m_symStart) <<
  //                  " to sym " << static_cast<uint32_t> (alloc.m_sci->m_numSym + alloc.m_sci->m_symStart) <<
  //                  " direction " << direction << " type " << type);
  //   }

  auto currentSci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_sci;
  auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * Time (currentSci->m_symStart);

  Simulator::Schedule (nextVarTtiStart, &MmWaveSidelinkPhy::StartVarTti, this);
}

void
MmWaveSidelinkPhy::StartVarTti ()
{
  NS_LOG_FUNCTION (this);
  Time varTtiPeriod;
  // const VarTtiAllocInfo & currSlot = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum];
  //
  // m_currTbs = currSlot.m_dci->m_tbSize;
  // m_receptionEnabled = false;
  //
  // NS_LOG_DEBUG ("UE " << m_rnti << " frame " << static_cast<uint32_t> (m_frameNum) <<
  //               " subframe " << static_cast<uint32_t> (m_subframeNum) << " slot " <<
  //               static_cast<uint32_t> (m_slotNum) << " sym " <<
  //               static_cast<uint32_t> (currSlot.m_dci->m_symStart));
  //
  // if (currSlot.m_dci->m_type == DciInfoElementTdma::CTRL && currSlot.m_dci->m_format == DciInfoElementTdma::DL)
  //   {
  //     varTtiPeriod = DlCtrl (currSlot.m_dci);
  //   }
  // else if (currSlot.m_dci->m_type == DciInfoElementTdma::CTRL && currSlot.m_dci->m_format == DciInfoElementTdma::UL)
  //   {
  //     varTtiPeriod = UlCtrl (currSlot.m_dci);
  //   }
  // else if (currSlot.m_dci->m_type == DciInfoElementTdma::DATA && currSlot.m_dci->m_format == DciInfoElementTdma::DL)
  //   {
  //     varTtiPeriod = DlData (currSlot.m_dci);
  //   }
  // else if (currSlot.m_dci->m_type == DciInfoElementTdma::DATA && currSlot.m_dci->m_format == DciInfoElementTdma::UL)
  //   {
  //     varTtiPeriod = UlData (currSlot.m_dci);
  //   }

  Simulator::Schedule (varTtiPeriod, &MmWaveSidelinkPhy::EndVarTti, this);
}

void
MmWaveSidelinkPhy::EndVarTti ()
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_INFO ("Executed varTti " << (+m_varTtiNum) + 1 << " of " << m_currSlotAllocInfo.m_varTtiAllocInfo.size ());
  //
  // if (m_tryToPerformLbt)
  //   {
  //     TryToPerformLbt ();
  //     m_tryToPerformLbt = false;
  //   }
  //
  // if (m_varTtiNum == m_currSlotAllocInfo.m_varTtiAllocInfo.size () - 1)
  //   {
  //     // end of slot
  //     SfnSf retVal = SfnSf (m_frameNum, m_subframeNum, m_slotNum, 0).IncreaseNoOfSlots (m_phyMacConfig->GetSlotsPerSubframe (),
  //                                                                                      m_phyMacConfig->GetSubframesPerFrame ());
  //
  //     Simulator::Schedule (m_lastSlotStart + m_phyMacConfig->GetSlotPeriod () -
  //                          Simulator::Now (),
  //                          &MmWaveUePhy::StartSlot,
  //                          this,
  //                          retVal.m_frameNum,
  //                          retVal.m_subframeNum,
  //                          retVal.m_slotNum);
  //   }
  // else
  //   {
  //     m_varTtiNum++;
  //     Time nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () *
  //                            m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci->m_symStart;
  //
  //     Simulator::Schedule (nextVarTtiStart + m_lastSlotStart - Simulator::Now (), &MmWaveUePhy::StartVarTti, this);
  //   }
  //
  // m_receptionEnabled = false;
}


bool
MmWaveSidelinkPhy::SidelinkSlotAllocInfoExists (const SidelinkSfnSf &retVal) const
{
  NS_LOG_FUNCTION (this);
  for (const auto & alloc : m_slotAllocInfo)
    {
      if (alloc.m_sidelinkSfnSf == retVal)
        {
          return true;
        }
    }
  return false;
}

SidelinkSlotAllocInfo
MmWaveSidelinkPhy::RetrieveSidelinkSlotAllocInfo ()
{
  NS_LOG_FUNCTION (this);
  SidelinkSlotAllocInfo ret = *m_slotAllocInfo.begin ();
  m_slotAllocInfo.erase(m_slotAllocInfo.begin ());
  return ret;
}

SidelinkSlotAllocInfo
MmWaveSidelinkPhy::RetrieveSidelinkSlotAllocInfo (const SidelinkSfnSf &sfnsf)
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_FUNCTION ("ccId:" << +GetCcId () << " slot " << sfnsf);

  for (auto allocIt = m_slotAllocInfo.begin(); allocIt != m_slotAllocInfo.end (); ++allocIt)
    {
      if (allocIt->m_sidelinkSfnSf == sfnsf)
        {
          SidelinkSlotAllocInfo ret = *allocIt;
          m_slotAllocInfo.erase (allocIt);
          return ret;
        }
    }

  NS_FATAL_ERROR("Didn't found the slot");
  return SidelinkSlotAllocInfo (sfnsf);
}


}

}
