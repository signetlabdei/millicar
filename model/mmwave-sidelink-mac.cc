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

#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/mmwave-amc.h"
#include "ns3/lte-mac-sap.h"
#include "mmwave-sidelink-mac.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkMac");

namespace mmwave_vehicular {

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
  // TODO update this method
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
                     "ns3::mmwave_vehicular::MmWaveSidelinkMac::SlSchedulingTracedCallback")
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
  NS_ASSERT_MSG (!m_sfAllocInfo.empty (), "First set the scheduling patter");
  if(m_sfAllocInfo[timingInfo.m_slotNum] == m_rnti) // check if this slot is associated to the user who required it
  {
    // compute the number of available symbols
    uint32_t availableSymbols = m_phyMacConfig->GetSymbPerSlot ();
    uint8_t symStart = 0; // indicates the next available symbol in the slot

    while (availableSymbols > 0 && m_txBuffer.size () > 0)
    {
      // retrieve the first element of the buffer
      LteMacSapProvider::TransmitPduParameters pduInfo = m_txBuffer.front ();
      Ptr<Packet> pdu = pduInfo.pdu; // the packet to transmit
      uint16_t rntiDest = pduInfo.rnti; // the RNTI of the destination node

      uint8_t mcs = GetMcs (rntiDest); // select the MCS

      // compute the number of symbols needed to transmit the packet
      uint32_t requiredSymbols = m_amc->GetNumSymbolsFromTbsMcs (pdu->GetSize ()*8, mcs);

      // compute the corresponding number of bits
      uint32_t requiredBits = m_amc->GetTbSizeFromMcsSymbols(mcs, requiredSymbols);

      if (requiredSymbols <= availableSymbols)
      {
        // create a new transport block
        Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
        pb->AddPacket(pdu);

        mmwave::SlotAllocInfo info;
        info.m_slotIdx = timingInfo.m_slotNum; // the TB will be sent in this slot
        info.m_rnti = rntiDest; // the RNTI of the destination node
        info.m_dci.m_rnti = m_rnti; // my RNTI
        info.m_dci.m_numSym = requiredSymbols; // the number of symbols required to tx the packet
        info.m_dci.m_symStart = symStart; // index of the first available symbol
        info.m_dci.m_mcs = mcs;
        info.m_dci.m_tbSize = requiredBits / 8; // the TB size in bytes
        info.m_slotType = mmwave::SlotAllocInfo::DATA; // the TB carries data

        // forward the transport block to the PHY
        m_phySapProvider->AddTransportBlock(pb, info);

        // remove the packet from the buffer
        m_txBuffer.pop_front ();

        // update the number of available bits
        availableSymbols -= requiredSymbols;

        // update index to the next available symbol
        symStart = symStart + requiredSymbols + 1;

        // fire the scheduling trace
        SlSchedulingCallback traceInfo;
        traceInfo.frame = timingInfo.m_frameNum;
        traceInfo.subframe = timingInfo.m_sfNum;
        traceInfo.slotNum = timingInfo.m_slotNum;
        traceInfo.symStart = symStart;
        traceInfo.numSym = requiredSymbols;
        traceInfo.mcs = mcs;
        traceInfo.tbSize = requiredBits / 8;
        traceInfo.txRnti = m_rnti;
        traceInfo.rxRnti = rntiDest;
        m_schedulingTrace (traceInfo);
      }
      else
      {
        // the remaining available bits are not enough to transmit the next
        // packet in the buffer
        NS_LOG_INFO ("Available bits " << m_amc->GetTbSizeFromMcsSymbols(mcs, availableSymbols) << " next pdu size " << pduInfo.pdu->GetSize () * 8);
        break;
      }
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

void
MmWaveSidelinkMac::DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params)
{
  NS_LOG_FUNCTION (this);

  // insert the packet at the end of the buffer
  m_txBuffer.push_back (params);
}

void
MmWaveSidelinkMac::DoReceivePhyPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this << p);

  LteMacSapUser::ReceivePduParameters rxPduParams;

  // TODO pick the right lcid associated to this communication. As discussed, this can be done via a dedicated SidelinkBearerTag
  rxPduParams.p = p;
  rxPduParams.rnti = m_rnti;
  rxPduParams.lcid = 1;

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
  int mcs; // the selected MCS will be stored in this variable
  if (m_slCqiReported.find (rnti) != m_slCqiReported.end () )
  {
    m_slCqiReported.at (rnti).push_back (m_amc->CreateCqiFeedbackWbTdma (sinr, numSym, tbSize, mcs));
  }
  else
  {
    std::vector<int> cqiTemp;
    cqiTemp.push_back (m_amc->CreateCqiFeedbackWbTdma (sinr, numSym, tbSize, mcs));
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
      mcs = m_amc->GetMcsFromCqi(*std::min_element (cqi.begin(), cqi.end()));
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
