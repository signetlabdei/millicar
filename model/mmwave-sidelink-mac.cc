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
#include "mmwave-sidelink-mac.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkMac");

namespace mmwave {

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
MacSidelinkMemberPhySapUser::SlotIndication (SfnSf timingInfo)
{
  m_mac->DoSlotIndication (timingInfo);
}

//-----------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (MmWaveSidelinkMac);

TypeId
MmWaveSidelinkMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveSidelinkMac")
    .SetParent<Object> ()
    .AddConstructor<MmWaveSidelinkMac> ()
    .AddAttribute ("Mcs",
                   "Modulation and Coding Scheme.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MmWaveSidelinkMac::m_mcs),
                   MakeUintegerChecker<uint8_t> (0, 28))
  ;
  return tid;
}

MmWaveSidelinkMac::MmWaveSidelinkMac (void)
{
  NS_LOG_FUNCTION (this);
  m_phySapUser = new MacSidelinkMemberPhySapUser (this);
  m_amc = CreateObject <MmWaveAmc> (m_phyMacConfig);
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
MmWaveSidelinkMac::DoSlotIndication (SfnSf timingInfo)
{
  NS_LOG_FUNCTION (this);

  if(m_sfAllocInfo[timingInfo.m_slotNum] == m_rnti) // check if this slot is associated to the user who required it
  {
    // compute the available bytes
    uint32_t availableBytes = m_amc->GetTbSizeFromMcsSymbols(m_mcs, m_phyMacConfig->GetSymbPerSlot ()) / 8; // this method returns the size in number of bits, that are then converted in number of bytes

    while (availableBytes > 0 && m_txBuffer.size () > 0)
    {
      // retrieve the first element of the buffer
      LteMacSapProvider::TransmitPduParameters pduInfo = m_txBuffer.front ();
      Ptr<Packet> pdu = pduInfo.pdu; // the packet to transmit
      uint16_t rntiDest = pduInfo.rnti; // the RNTI of the destination node

      // compute the number of symbols needed to transmit the packet
      uint32_t requiredSymbols = m_amc->GetNumSymbolsFromTbsMcs (pdu->GetSize (), m_mcs);

      // compute the corresponding number of bytes
      uint32_t requiredBytes = m_amc->GetTbSizeFromMcsSymbols(m_mcs, requiredSymbols) / 8;

      if (requiredBytes < availableBytes)
      {
        // create a new transport block
        Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
        pb->AddPacket(pdu);

        SlotAllocInfo info;
        info.m_rnti = rntiDest; // the RNTI of the destination node
        info.m_dci.m_rnti = m_rnti; // my RNTI
        info.m_dci.m_numSym = requiredSymbols; // the number of symbols required to tx the packet
        info.m_dci.m_mcs = m_mcs;
        info.m_dci.m_tbSize = requiredBytes;

        // forward the transport block to the PHY
        m_phySapProvider->AddTransportBlock(pb, info);

        // remove the packet from the buffer
        m_txBuffer.pop_front ();

        // update the number of available bytes
        availableBytes-=requiredBytes;
      }
    }
  }
  else
  {
    NS_LOG_DEBUG("This slot was not associated to this user.");
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

} // mmwave namespace

} // ns3 namespace
