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

  uint8_t mcs = 9; // TODO: just a placeholder for testing purposes

  if(m_sfAllocInfo[timingInfo.m_slotNum] == m_rnti) // check if this slot is associated to the user who required it
  {
    Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
    SlotAllocInfo info;

    info.m_dci.m_rnti = m_rnti;
    info.m_dci.m_numSym = 14;
    info.m_dci.m_mcs = mcs;
    info.m_dci.m_tbSize = m_amc->GetTbSizeFromMcsSymbols(mcs, info.m_dci.m_numSym) / 8; // this method returns the size in number of bits, that are then converted in number of bytes

    Ptr<Packet> pkt = Create <Packet> (info.m_dci.m_tbSize);

    pb->AddPacket(pkt);

    m_phySapProvider->AddTransportBlock(pb, info);

  }
  else
  {
    NS_LOG_DEBUG("This slot was not associated to this user.");
  }

}

void
MmWaveSidelinkMac::DoTransmitPdu ()
{
  // TODO: to be defined when upper layers will be added
}

void
MmWaveSidelinkMac::DoReceivePhyPdu (Ptr<Packet> p)
{

}

MmWaveSidelinkPhySapUser*
MmWaveSidelinkMac::GetPhySapUser ()
{
  return m_phySapUser;
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
