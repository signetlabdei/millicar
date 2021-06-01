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

#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/mmwave-amc.h"
#include "ns3/lte-mac-sap.h"
#include "ns3/lte-radio-bearer-tag.h"
#include "mmwave-sidelink-mac.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkMac");

namespace millicar {

MacSidelinkMemberPhySapUser::MacSidelinkMemberPhySapUser (Ptr<MmWaveSidelinkMac> mac)
  : m_mac (mac)
{

}

void
MacSidelinkMemberPhySapUser::ReceivePhyPdu (Ptr<Packet> p)
{
  m_mac->DoReceivePhyPdu (p);
}

void
MacSidelinkMemberPhySapUser::SlotIndication (mmwave::SfnSf timingInfo)
{
  m_mac->DoSlotIndication (timingInfo);
}

void
MacSidelinkMemberPhySapUser::SlSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize)
{
  m_mac->DoSlSinrReport (sinr, rnti, numSym, tbSize);
}

//-----------------------------------------------------------------------

RlcSidelinkMemberMacSapProvider::RlcSidelinkMemberMacSapProvider (Ptr<MmWaveSidelinkMac> mac)
  : m_mac (mac)
{

}

void
RlcSidelinkMemberMacSapProvider::TransmitPdu (TransmitPduParameters params)
{
  m_mac->DoTransmitPdu (params);
}

void
RlcSidelinkMemberMacSapProvider::ReportBufferStatus (ReportBufferStatusParameters params)
{
  m_mac->DoReportBufferStatus (params);
}

//-----------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (MmWaveSidelinkMac);

TypeId
MmWaveSidelinkMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveSidelinkMac")
    .SetParent<Object> ()
    .AddAttribute ("Mcs",
                   "If AMC is not used, specify a fixed MCS value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MmWaveSidelinkMac::m_mcs),
                   MakeUintegerChecker<uint8_t> (0, 28))
    .AddAttribute ("UseAmc",
                   "Set to true to use adaptive modulation and coding.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MmWaveSidelinkMac::m_useAmc),
                   MakeBooleanChecker ())
    .AddTraceSource ("SchedulingInfo",
                     "Information regarding the scheduling.",
                     MakeTraceSourceAccessor (&MmWaveSidelinkMac::m_schedulingTrace),
                     "ns3::millicar::MmWaveSidelinkMac::SlSchedulingTracedCallback")
  ;
  return tid;
}

MmWaveSidelinkMac::MmWaveSidelinkMac (Ptr<mmwave::MmWavePhyMacCommon> pmc)
{
  NS_LOG_FUNCTION (this);

  m_phyMacConfig = pmc;

  // initialize the RNTI to 0
  m_rnti = 0;

  // create the PHY SAP USER
  m_phySapUser = new MacSidelinkMemberPhySapUser (this);

  // create the MAC SAP PROVIDER
  m_macSapProvider = new RlcSidelinkMemberMacSapProvider(this);

  // create the mmwave::MmWaveAmc instance
  m_amc = CreateObject <mmwave::MmWaveAmc> (m_phyMacConfig);

  // initialize the scheduling patter
  std::vector<uint16_t> pattern (m_phyMacConfig->GetSlotsPerSubframe (), 0);
  m_sfAllocInfo = pattern;
}

MmWaveSidelinkMac::~MmWaveSidelinkMac (void)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveSidelinkMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_phySapUser;
  Object::DoDispose ();
}

void
MmWaveSidelinkMac::DoSlotIndication (mmwave::SfnSf timingInfo)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_rnti != 0, "First set the RNTI");
  NS_ASSERT_MSG (!m_sfAllocInfo.empty (), "First set the scheduling pattern");
  if(m_sfAllocInfo [timingInfo.m_slotNum] == m_rnti) // check if this slot is associated to the user who required it
  {
    mmwave::SlotAllocInfo allocationInfo = ScheduleResources (timingInfo);

    // associate slot alloc info and pdu
    for (auto it = allocationInfo.m_ttiAllocInfo.begin(); it != allocationInfo.m_ttiAllocInfo.end (); it++)
    {
      // retrieve the tx buffer corresponding to the assigned destination
      auto txBuffer = m_txBufferMap.find (it->m_rnti); // the destination RNTI

      if (txBuffer == m_txBufferMap.end () || txBuffer->second.empty ())
      {
        // discard the tranmission opportunity and go to the next transmission
        continue;
      }

      // otherwise, forward the packet to the PHY
      Ptr<PacketBurst> pb = CreateObject<PacketBurst> ();
      pb->AddPacket (txBuffer->second.front ().pdu);
      m_phySapProvider->AddTransportBlock (pb, *it);
      txBuffer->second.pop_front ();
    }
  }
  else if (m_sfAllocInfo[timingInfo.m_slotNum] != 0) // if the slot is assigned to another device, prepare for reception
  {
    NS_LOG_INFO ("Prepare for reception from rnti " << m_sfAllocInfo[timingInfo.m_slotNum]);
    m_phySapProvider->PrepareForReception (m_sfAllocInfo[timingInfo.m_slotNum]);
  }
  else // the slot is not assigned to any user
  {
    NS_LOG_INFO ("Empty slot");
  }

}

mmwave::SlotAllocInfo
MmWaveSidelinkMac::ScheduleResources (mmwave::SfnSf timingInfo)
{
  mmwave::SlotAllocInfo allocationInfo; // stores all the allocation decisions
  allocationInfo.m_sfnSf = timingInfo;
  allocationInfo.m_numSymAlloc = 0;

  NS_LOG_DEBUG("m_bufferStatusReportMap.size () =\t" << m_bufferStatusReportMap.size ());
  // if there are no active channels return an empty vector
  if (m_bufferStatusReportMap.size () == 0)
  {
    return allocationInfo;
  }

  // compute the total number of available symbols
  uint32_t availableSymbols = m_phyMacConfig->GetSymbPerSlot ();

  NS_LOG_DEBUG("availableSymbols =\t" << availableSymbols);

  // compute the number of available symbols per logical channel
  // NOTE the number of available symbols per LC is rounded down due to the cast
  // to int
  uint32_t availableSymbolsPerLc = availableSymbols / m_bufferStatusReportMap.size ();

  NS_LOG_DEBUG("availableSymbolsPerLc =\t" << availableSymbolsPerLc);

  // TODO start from the last served lc + 1
  auto bsrIt = m_bufferStatusReportMap.begin ();

  uint8_t symStart = 0; // indicates the next available symbol in the slot

  // serve the active logical channels with a Round Robin approach
  while (availableSymbols > 0 && m_bufferStatusReportMap.size () > 0)
  {
    uint16_t rntiDest = bsrIt->second.rnti; // the RNTI of the destination node

    uint8_t mcs = GetMcs (rntiDest); // select the MCS

    NS_LOG_DEBUG("rnti " << rntiDest << " mcs = " << uint16_t(mcs));
    // compute the number of bits for this LC
    uint32_t availableBytesPerLc = m_amc->CalculateTbSize(mcs, availableSymbolsPerLc);

    // compute the number of bits required by this LC
    uint32_t requiredBytes = (bsrIt->second.txQueueSize + bsrIt->second.retxQueueSize + bsrIt->second.statusPduSize);

    // assign a number of bits which is less or equal to the available bits
    uint32_t assignedBytes = 0;
    if (requiredBytes <= availableBytesPerLc)
    {
      assignedBytes = requiredBytes;
    }
    else
    {
      assignedBytes = availableBytesPerLc;
    }

    // compute the number of symbols assigned to this LC
    uint32_t assignedSymbols = m_amc->GetMinNumSymForTbSize (assignedBytes, mcs);

    //if (assignedSymbols <= availableSymbols) // TODO check if needed
    //{
    // create the TtiAllocInfo object
    mmwave::TtiAllocInfo info;
    info.m_ttiIdx = timingInfo.m_slotNum; // the TB will be sent in this slot
    info.m_rnti = rntiDest; // the RNTI of the destination node
    info.m_dci.m_rnti = m_rnti; // my RNTI
    info.m_dci.m_numSym = assignedSymbols; // the number of symbols required to tx the packet
    info.m_dci.m_symStart = symStart; // index of the first available symbol
    info.m_dci.m_mcs = mcs;
    info.m_dci.m_tbSize = assignedBytes; // the TB size in bytes
    info.m_ttiType = mmwave::TtiAllocInfo::TddTtiType::DATA; // the TB carries data

    NS_LOG_DEBUG("info.m_dci.m_tbSize =\t" << info.m_dci.m_tbSize);

    allocationInfo.m_ttiAllocInfo.push_back (info);
    allocationInfo.m_numSymAlloc += assignedSymbols;

    // fire the scheduling trace
    SlSchedulingCallback traceInfo;
    traceInfo.frame = timingInfo.m_frameNum;
    traceInfo.subframe = timingInfo.m_sfNum;
    traceInfo.slotNum = timingInfo.m_slotNum;
    traceInfo.symStart = symStart;
    traceInfo.numSym = assignedSymbols;
    traceInfo.mcs = mcs;
    traceInfo.tbSize = assignedBytes;
    traceInfo.txRnti = m_rnti;
    traceInfo.rxRnti = rntiDest;
    m_schedulingTrace (traceInfo);

    // notify the RLC
    LteMacSapUser* macSapUser = m_lcidToMacSap.find (bsrIt->second.lcid)->second;
    LteMacSapUser::TxOpportunityParameters params;
    params.bytes = assignedBytes;  // the number of bytes to transmit
    params.layer = 0;  // the layer of transmission (MIMO) (NOT USED)
    params.harqId = 0; // the HARQ ID (NOT USED)
    params.componentCarrierId = 0; // the component carrier id (NOT USED)
    params.rnti = rntiDest; // the C-RNTI identifying the destination
    params.lcid = bsrIt->second.lcid; // the logical channel id
    macSapUser->NotifyTxOpportunity (params);

    // update the entry in the m_bufferStatusReportMap (delete it if no
    // further resources are needed)
    bsrIt = UpdateBufferStatusReport (bsrIt->second.lcid, assignedBytes);

    // update the number of available symbols
    availableSymbols -= assignedSymbols;

    // update the availableSymbolsPerLc (if needed)
    if (availableSymbols < availableSymbolsPerLc)
    {
      availableSymbolsPerLc = availableSymbols;
    }

    // update index to the next available symbol
    symStart = symStart + assignedSymbols;

    // if the iterator reached the end of the map, start again
    if (bsrIt == m_bufferStatusReportMap.end ())
    {
      bsrIt = m_bufferStatusReportMap.begin ();
    }

  }
  return allocationInfo;
}

std::map<uint8_t, LteMacSapProvider::ReportBufferStatusParameters>::iterator
MmWaveSidelinkMac::UpdateBufferStatusReport (uint8_t lcid, uint32_t assignedBytes)
{
  // find the corresponding entry in the map
  auto bsrIt = m_bufferStatusReportMap.find (lcid);

  NS_ASSERT_MSG (bsrIt != m_bufferStatusReportMap.end (), "m_bufferStatusReportMap does not contain the required entry");

  // NOTE RLC transmits PDUs in the following priority order:
  // 1) STATUS PDUs
  // 2) retransmissions
  // 3) regular PDUs
  if (bsrIt->second.statusPduSize > assignedBytes)
  {
    bsrIt->second.statusPduSize -= assignedBytes;
    assignedBytes = 0;
  }
  else
  {
    assignedBytes -= bsrIt->second.statusPduSize;
    bsrIt->second.statusPduSize = 0;
  }

  if (bsrIt->second.retxQueueSize > assignedBytes)
  {
    bsrIt->second.retxQueueSize -= assignedBytes;
    assignedBytes = 0;
  }
  else
  {
    assignedBytes -= bsrIt->second.retxQueueSize;
    bsrIt->second.retxQueueSize = 0;
  }

  if (bsrIt->second.txQueueSize > assignedBytes)
  {
    bsrIt->second.txQueueSize -= assignedBytes;
    assignedBytes = 0;
  }
  else
  {
    assignedBytes -= bsrIt->second.txQueueSize;
    bsrIt->second.txQueueSize = 0;
  }

  // delete the entry in the map if no further resources are needed
  if (bsrIt->second.statusPduSize == 0 && bsrIt->second.retxQueueSize == 0  && bsrIt->second.txQueueSize == 0)
  {
    bsrIt = m_bufferStatusReportMap.erase (bsrIt);
  }
  else
  {
    bsrIt++;
  }

  return bsrIt;
}

void
MmWaveSidelinkMac::DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params)
{
  NS_LOG_FUNCTION (this);

  auto bsrIt = m_bufferStatusReportMap.find (params.lcid);
  if (bsrIt != m_bufferStatusReportMap.end ())
  {
    bsrIt->second = params;
    NS_LOG_DEBUG("Update buffer status report for LCID " << uint32_t(params.lcid));
  }
  else
  {
    m_bufferStatusReportMap.insert (std::make_pair (params.lcid, params));
    NS_LOG_DEBUG("Insert buffer status report for LCID " << uint32_t(params.lcid));
  }
}

void
MmWaveSidelinkMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);
  LteRadioBearerTag tag (params.rnti, params.lcid, params.layer);
  params.pdu->AddPacketTag (tag);

  //insert the packet at the end of the buffer
  NS_LOG_DEBUG("Add packet for RNTI " << params.rnti << " LCID " << uint32_t(params.lcid));

  auto it = m_txBufferMap.find (params.rnti);
  if (it == m_txBufferMap.end ())
  {
    std::list<LteMacSapProvider::TransmitPduParameters> txBuffer;
    txBuffer.push_back (params);
    m_txBufferMap.insert (std::make_pair (params.rnti, txBuffer));
  }
  else
  {
    it->second.push_back (params);
  }
}

void
MmWaveSidelinkMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p);
  LteMacSapUser::ReceivePduParameters rxPduParams;

  LteRadioBearerTag tag;
  p->PeekPacketTag (tag);

  // pick the right lcid associated to this communication. As discussed, this can be done via a dedicated SidelinkBearerTag
  rxPduParams.p = p;
  rxPduParams.rnti = tag.GetRnti ();
  rxPduParams.lcid = tag.GetLcid ();

  NS_LOG_DEBUG ("Received a packet " << rxPduParams.rnti << " " << (uint16_t)rxPduParams.lcid);

  LteMacSapUser* macSapUser = m_lcidToMacSap.find(rxPduParams.lcid)->second;
  macSapUser->ReceivePdu (rxPduParams);
}

MmWaveSidelinkPhySapUser*
MmWaveSidelinkMac::GetPhySapUser () const
{
  NS_LOG_FUNCTION (this);
  return m_phySapUser;
}

void
MmWaveSidelinkMac::SetPhySapProvider (MmWaveSidelinkPhySapProvider* sap)
{
  NS_LOG_FUNCTION (this);
  m_phySapProvider = sap;
}

LteMacSapProvider*
MmWaveSidelinkMac::GetMacSapProvider () const
{
  NS_LOG_FUNCTION (this);
  return m_macSapProvider;
}

void
MmWaveSidelinkMac::SetRnti (uint16_t rnti)
{
  m_rnti = rnti;
}

uint16_t
MmWaveSidelinkMac::GetRnti () const
{
  return m_rnti;
}

void
MmWaveSidelinkMac::SetSfAllocationInfo (std::vector<uint16_t> pattern)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (pattern.size () == m_phyMacConfig->GetSlotsPerSubframe (), "The number of pattern elements must be equal to the number of slots per subframe");
  m_sfAllocInfo = pattern;
}

void
MmWaveSidelinkMac::SetForwardUpCallback (Callback <void, Ptr<Packet> > cb)
{
  m_forwardUpCallback = cb;
}

void
MmWaveSidelinkMac::DoSlSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize)
{
  NS_LOG_FUNCTION (this);

  // check if the m_slCqiReported already contains the CQI history for the device
  // with RNTI = rnti. If so, add the new CQI report, otherwise create a new
  // entry.
  uint8_t mcs; // the selected MCS will be stored in this variable
  if (m_slCqiReported.find (rnti) != m_slCqiReported.end () )
  {
    m_slCqiReported.at (rnti).push_back (m_amc->CreateCqiFeedbackWbTdma (sinr, mcs));
  }
  else
  {
    std::vector<int> cqiTemp;
    cqiTemp.push_back (m_amc->CreateCqiFeedbackWbTdma (sinr, mcs));
    m_slCqiReported.insert (std::make_pair(rnti, cqiTemp));
  }
}

uint8_t
MmWaveSidelinkMac::GetMcs (uint16_t rnti)
{
  NS_LOG_FUNCTION (this);

  uint8_t mcs; // the selected MCS
  if (m_useAmc)
  {
    // if AMC is used, select the MCS based on the CQI history
    if (m_slCqiReported.find (rnti) != m_slCqiReported.end ())
    {
      std::vector <int> cqi = m_slCqiReported.find (rnti)->second;
      mcs = m_amc->GetMcsFromCqi(cqi.back ());
    }
    else
    {
      mcs = 0; // if the CQI history not found for this device, use the minimum MCS value
    }
  }
  else
  {
    // if AMC is not used, use a fixed MCS value
    mcs = m_mcs;
  }
  // TODO: update the method to refresh CQI reports and delete older reports
  return mcs;
}

void
MmWaveSidelinkMac::AddMacSapUser (uint8_t lcid, LteMacSapUser* macSapUser)
{
  NS_LOG_FUNCTION (this);
  m_lcidToMacSap.insert(std::make_pair(lcid, macSapUser));
}

} // mmwave namespace

} // ns3 namespace
