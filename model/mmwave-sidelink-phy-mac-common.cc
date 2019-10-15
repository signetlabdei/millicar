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
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


#include "mmwave-sidelink-phy-mac-common.h"
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/double.h>
#include <ns3/string.h>
#include <ns3/attribute-accessor-helper.h>
#include <algorithm>

namespace ns3 {

namespace mmwave {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkPhyMacCommon");

NS_OBJECT_ENSURE_REGISTERED (MmWaveSidelinkPhyMacCommon);

TypeId
MmWaveSidelinkPhyMacCommon::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveSidelinkPhyMacCommon")
    .SetParent<Object> ()
    .AddConstructor<MmWaveSidelinkPhyMacCommon> ()
    .AddAttribute ("NumReferenceSymbols",
                   "Number of reference symbols per slot",
                   UintegerValue (6),
                   MakeUintegerAccessor (&MmWaveSidelinkPhyMacCommon::m_numRefSymbols),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("CenterFreq",
                   "The center frequency in Hz",
                   DoubleValue (28e9),
                   MakeDoubleAccessor (&MmWaveSidelinkPhyMacCommon::m_centerFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Bandwidth",
                   "The system bandwidth in Hz",
                   DoubleValue (400e6),
                   MakeDoubleAccessor (&MmWaveSidelinkPhyMacCommon::SetBandwidth,
                                       &MmWaveSidelinkPhyMacCommon::GetBandwidth),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NumRbPerRbg",
                   "Number of resource blocks per resource block group",
                   UintegerValue (1),
                   MakeUintegerAccessor (&MmWaveSidelinkPhyMacCommon::m_numRbPerRbg),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Numerology",
                   "The 3gpp numerology to be used",
                   UintegerValue (4),
                   MakeUintegerAccessor (&MmWaveSidelinkPhyMacCommon::SetNumerology,
                                         &MmWaveSidelinkPhyMacCommon::GetNumerology),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SymbolsPerSlot",
                   "Number of symbols in one slot, including 2 of control",
                   UintegerValue (14),
                   MakeUintegerAccessor (&MmWaveSidelinkPhyMacCommon::m_symbolsPerSlot),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("TbDecodeLatency",
                   "TB decode latency",
                   UintegerValue (100.0),
                   MakeUintegerAccessor (&MmWaveSidelinkPhyMacCommon::m_tbDecodeLatencyUs),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

MmWaveSidelinkPhyMacCommon::MmWaveSidelinkPhyMacCommon ()
  : m_symbolPeriod (0.00000416),
  m_symbolsPerSlot (14),
  m_slotPeriod (0.0001),
  m_fixedTtisPerSlot (8),
  m_subframesPerFrame (10),
  m_numRefSymbols (6),
  m_numRbPerRbg (1),
  m_numerology (4),
  m_subcarrierSpacing (15e3),
  m_rbNum (72),
  m_numRefScPerRb (1),
  m_numSubCarriersPerRb (12),
  m_centerFrequency (28e9),
  m_bandwidth (400e6),
  m_bandwidthConfigured (false),
  m_tbDecodeLatencyUs (100.0),
  m_maxTbSizeBytes (0x7FFF)
{
  NS_LOG_INFO ("MmWaveSidelinkPhyMacCommon constructor");
}


void
MmWaveSidelinkPhyMacCommon::DoInitialize (void)
{
  NS_LOG_INFO ("Initialized MmWaveSidelinkPhyMacCommon");
}

void
MmWaveSidelinkPhyMacCommon::DoDispose (void)
{
}

MmWaveSidelinkPhyMacCommon::~MmWaveSidelinkPhyMacCommon (void)
{
}

Time MmWaveSidelinkPhyMacCommon::GetSymbolPeriod(void) const
{
  return m_symbolPeriod;
}

uint8_t
MmWaveSidelinkPhyMacCommon::GetSymbolsPerSlot (void) const
{
  return m_symbolsPerSlot;
}

Time
MmWaveSidelinkPhyMacCommon::GetSlotPeriod (void) const
{
  return m_slotPeriod;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetVarTtisPerSlot (void) const
{
  return m_fixedTtisPerSlot;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetSubframesPerFrame (void) const
{
  return m_subframesPerFrame;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetSlotsPerSubframe (void) const
{
  return m_slotsPerSubframe;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetNumReferenceSymbols (void)
{
  return m_numRefSymbols;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetNumScsPerRb (void) const
{
  return m_numSubCarriersPerRb;
}

double
MmWaveSidelinkPhyMacCommon::GetSubcarrierSpacing (void) const
{
  return m_subcarrierSpacing;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetNumRefScPerRb (void) const
{
  return m_numRefScPerRb;
}

// for TDMA, number of reference subcarriers across entire bandwidth (default to 1/4th of SCs)
uint32_t
MmWaveSidelinkPhyMacCommon::GetNumRefScPerSym (void) const
{
  return m_numSubCarriersPerRb * m_rbNum  / 4;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetNumRbPerRbg (void) const
{
  return m_numRbPerRbg;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetNumerology (void) const
{
  return m_numerology;
}

double
MmWaveSidelinkPhyMacCommon::GetBandwidth (void) const
{
  return (GetSubcarrierSpacing () * GetNumScsPerRb () * m_rbNum);
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetBandwidthInRbg () const
{
  return m_rbNum / m_numRbPerRbg;
}

/*
 * brief: bandwidth in number of RBs
 */
uint32_t
MmWaveSidelinkPhyMacCommon::GetBandwidthInRbs () const
{
  return m_rbNum;
}


double
MmWaveSidelinkPhyMacCommon::GetCenterFrequency (void) const
{
  return m_centerFrequency;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetTbDecodeLatency (void) const
{
  return m_tbDecodeLatencyUs;
}

uint32_t
MmWaveSidelinkPhyMacCommon::GetMaxTbSize (void) const
{
  return m_maxTbSizeBytes;
}

void
MmWaveSidelinkPhyMacCommon::SetSymbolPeriod (double prdSym)
{
  m_symbolPeriod = Seconds (prdSym);
}

void
MmWaveSidelinkPhyMacCommon::SetSymbolsPerSlot (uint8_t numSym)
{
  m_symbolsPerSlot = numSym;
}

void
MmWaveSidelinkPhyMacCommon::SetSlotPeriod (double period)
{
  m_slotPeriod = Seconds (period);
}

void
MmWaveSidelinkPhyMacCommon::SetVarTtiPerSlot (uint32_t numVarTti)
{
  m_fixedTtisPerSlot = numVarTti;
}

void
MmWaveSidelinkPhyMacCommon::SetSubframePerFrame (uint32_t numSf)
{
  m_subframesPerFrame = numSf;
}

void
MmWaveSidelinkPhyMacCommon::SetNumReferenceSymbols (uint32_t refSym)
{
  m_numRefSymbols = refSym;
}

void
MmWaveSidelinkPhyMacCommon::SetNumScsPrRb (uint32_t numScs)
{
  m_numSubCarriersPerRb = numScs;
}

void
MmWaveSidelinkPhyMacCommon::SetNumRefScPerRb (uint32_t numRefSc)
{
  m_numRefScPerRb = numRefSc;
}

void
MmWaveSidelinkPhyMacCommon::SetRbNum (uint32_t numRB)
{
  m_rbNum = numRB;
}

/*
 * brief
 * rbgSize size of RBG in number of resource blocks
 */
void
MmWaveSidelinkPhyMacCommon::SetNumRbPerRbg (uint32_t rbgSize)
{
  m_numRbPerRbg = rbgSize;
}

void
MmWaveSidelinkPhyMacCommon::SetNumerology (uint32_t numerology)
{
  NS_ASSERT_MSG ( (0 <= numerology) && (numerology <= 5), "Numerology not defined.");

  m_numerology = numerology;
  m_slotsPerSubframe  = std::pow (2, numerology);
  m_slotPeriod = Seconds (0.001 / m_slotsPerSubframe);
  m_symbolPeriod = (m_slotPeriod / m_symbolsPerSlot);
  m_numSubCarriersPerRb = 12;
  m_subcarrierSpacing = 15 * std::pow (2, numerology) * 1000;

  NS_ASSERT_MSG (m_bandwidthConfigured, "Bandwidth not configured, bandwidth has to be configured in order to configure properly the numerology");

  m_rbNum = m_bandwidth / (m_subcarrierSpacing * m_numSubCarriersPerRb);

  NS_LOG_INFO (" Numerology configured:" << m_numerology <<
               " slots per subframe: " << m_slotsPerSubframe <<
               " slot period:" << m_slotPeriod <<
               " symbol period:" << m_symbolPeriod <<
               " subcarrier spacing: " << m_subcarrierSpacing <<
               " number of RBs: " << m_rbNum );
}

/*
 * brief Set bandwidth value in Hz
 * param bandwidth the bandwidth value in Hz
 */
void
MmWaveSidelinkPhyMacCommon::SetBandwidth (double bandwidth)
{
  m_bandwidth = bandwidth;
  m_bandwidthConfigured = true;
}

void
MmWaveSidelinkPhyMacCommon::SetCentreFrequency (double fc)
{
  m_centerFrequency = fc;
}

void
MmWaveSidelinkPhyMacCommon::SetTbDecodeLatency (uint32_t us)
{
  m_tbDecodeLatencyUs = us;
}

void
MmWaveSidelinkPhyMacCommon::SetMaxTbSize (uint32_t bytes)
{
  m_maxTbSizeBytes = bytes;
}

void
SlotAllocInfo::Merge (const SlotAllocInfo &other)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (other.m_type != NONE && m_type != NONE);
  NS_ASSERT (other.m_sfnSf == m_sfnSf);

  if (other.m_type * m_type == 6)
    {
      m_type = BOTH;
    }

  m_numSymAlloc += other.m_numSymAlloc;

  for (auto extAlloc : other.m_varTtiAllocInfo)
    {
      m_varTtiAllocInfo.push_front (extAlloc);
    }

  // Sort over the symStart of the DCI (VarTtiAllocInfo::operator <)
  std::sort (m_varTtiAllocInfo.begin (), m_varTtiAllocInfo.end ());
}

bool
SlotAllocInfo::ContainsDataAllocation () const
{
  NS_LOG_FUNCTION (this);
  for (const auto & allocation : m_varTtiAllocInfo)
    {
      if (allocation.m_dci->m_type == DciInfoElementTdma::DATA)
        {
          return true;
        }
    }
  return false;
}

}

}
