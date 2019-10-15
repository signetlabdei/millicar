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
#include "ns3/mmwave-spectrum-value-helper.h"
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

MmWaveSidelinkPhy::MmWaveSidelinkPhy (Ptr<MmWaveSpectrumPhy> dlPhy, Ptr<MmWaveSpectrumPhy> ulPhy)
  : MmWavePhy (dlPhy, ulPhy)
{
  NS_LOG_FUNCTION (this);
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
MmWaveSidelinkPhy::StartSlot (uint16_t frameNum, uint8_t subframeNum, uint16_t slotNum)
{
  NS_LOG_FUNCTION (this);
  m_frameNum = frameNum;
  m_sfNum = subframeNum;
  m_slotNum = static_cast<uint8_t> (slotNum);
  m_lastSlotStart = Simulator::Now ();
  // m_varTtiNum = 0;
  //
  // // Call MAC before doing anything in PHY
  // m_phySapUser->SlotIndication (SfnSf (m_frameNum, m_subframeNum, m_slotNum, 0));   // trigger mac
  //
  // // update the current slot object, and insert DL/UL CTRL allocations.
  // // That will not be true anymore when true TDD pattern will be used.
  // if (SlotAllocInfoExists (SfnSf (frameNum, sfNum, slotNum, m_varTtiNum)))
  //   {
  //     m_currSlotAllocInfo = RetrieveSlotAllocInfo (SfnSf (frameNum, sfNum, slotNum, m_varTtiNum));
  //   }
  // else
  //   {
  //     m_currSlotAllocInfo = SlotAllocInfo (SfnSf (frameNum, sfNum, slotNum, m_varTtiNum));
  //   }
  //
  // std::vector<uint8_t> rbgBitmask (m_phyMacConfig->GetBandwidthInRbg (), 1);
  // VarTtiAllocInfo dlCtrlSlot (std::make_shared<DciInfoElementTdma> (0, 1, DciInfoElementTdma::DL, DciInfoElementTdma::CTRL, rbgBitmask));
  // VarTtiAllocInfo ulCtrlSlot (std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, DciInfoElementTdma::UL, DciInfoElementTdma::CTRL, rbgBitmask));
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_front (dlCtrlSlot);
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_back (ulCtrlSlot);
  //
  // NS_ASSERT ((m_currSlotAllocInfo.m_sfnSf.m_frameNum == m_frameNum)
  //            && (m_currSlotAllocInfo.m_sfnSf.m_subframeNum == m_subframeNum
  //                && m_currSlotAllocInfo.m_sfnSf.m_slotNum == m_slotNum));
  //
  // NS_LOG_INFO ("UE " << m_rnti << " start slot " << m_currSlotAllocInfo.m_sfnSf <<
  //              " composed by the following allocations, total " << m_currSlotAllocInfo.m_varTtiAllocInfo.size ());
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
  //       {VarTtiAllocInfo dlCtrlSlot (std::make_shared<DciInfoElementTdma> (0, 1, DciInfoElementTdma::DL, DciInfoElementTdma::CTRL, rbgBitmask));
  // VarTtiAllocInfo ulCtrlSlot (std::make_shared<DciInfoElementTdma> (m_phyMacConfig->GetSymbolsPerSlot () - 1, 1, DciInfoElementTdma::UL, DciInfoElementTdma::CTRL, rbgBitmask));
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_front (dlCtrlSlot);
  // m_currSlotAllocInfo.m_varTtiAllocInfo.push_back (ulCtrlSlot);
  //         direction = "DL";
  //       }
  //     NS_LOG_INFO ("Allocation from sym " << static_cast<uint32_t> (alloc.m_dci->m_symStart) <<
  //                  " to sym " << static_cast<uint32_t> (alloc.m_dci->m_numSym + alloc.m_dci->m_symStart) <<
  //                  " direction " << direction << " type " << type);
  //   }
  //
  // auto currentDci = m_currSlotAllocInfo.m_varTtiAllocInfo[m_varTtiNum].m_dci;
  // auto nextVarTtiStart = m_phyMacConfig->GetSymbolPeriod () * currentDci->m_symStart;
  //
  // Simulator::Schedule (nextVarTtiStart, &MmWaveUePhy::StartVarTti, this);
}

}

}
