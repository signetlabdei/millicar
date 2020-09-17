/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 * Copyright (c) 2020 University of Padova, Dep. of Information Engineering,
 * SIGNET lab.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <ns3/uinteger.h>
#include <ns3/node.h>
#include <ns3/log.h>
#include <ns3/object-map.h>
#include <ns3/ipv4-header.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-header.h>
#include <ns3/ipv6-l3-protocol.h>
#include "ns3/epc-tft.h"
#include "ns3/lte-rlc-um.h"
#include "ns3/lte-rlc-tm.h"
#include "ns3/lte-radio-bearer-tag.h"
#include "mmwave-sidelink-mac.h"
#include "mmwave-vehicular-net-device.h"

namespace ns3 {

namespace millicar {

PdcpSpecificSidelinkPdcpSapUser::PdcpSpecificSidelinkPdcpSapUser (Ptr<MmWaveVehicularNetDevice> netDevice)
  : m_netDevice (netDevice)
{

}

void
PdcpSpecificSidelinkPdcpSapUser::ReceivePdcpSdu (ReceivePdcpSduParameters params)
{
  m_netDevice->Receive (params.pdcpSdu);
}

//-----------------------------------------------------------------------

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularNetDevice");

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularNetDevice);

TypeId MmWaveVehicularNetDevice::GetTypeId ()
{
  static TypeId
    tid =
    TypeId ("ns3::MmWaveVehicularNetDevice")
    .SetParent<NetDevice> ()
    .AddAttribute ("SidelinkRadioBearerMap", "List of SidelinkRadioBearerMap by BID",
                   ObjectMapValue (),
                   MakeObjectMapAccessor (&MmWaveVehicularNetDevice::m_bearerToInfoMap),
                   MakeObjectMapChecker<SidelinkRadioBearerInfo> ())
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&MmWaveVehicularNetDevice::SetMtu,
                                         &MmWaveVehicularNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
     .AddAttribute ("RlcType",
                  "Set the RLC mode to use (AM not supported for now)",
                   StringValue ("LteRlcTm"),
                   MakeStringAccessor (&MmWaveVehicularNetDevice::m_rlcType),
                   MakeStringChecker ())
  ;

  return tid;
}

MmWaveVehicularNetDevice::MmWaveVehicularNetDevice (void)
{
  NS_LOG_FUNCTION (this);
}

MmWaveVehicularNetDevice::MmWaveVehicularNetDevice (Ptr<MmWaveSidelinkPhy> phy, Ptr<MmWaveSidelinkMac> mac)
{
  NS_LOG_FUNCTION (this);
  m_phy = phy;
  m_mac = mac;
}

MmWaveVehicularNetDevice::~MmWaveVehicularNetDevice (void)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveVehicularNetDevice::DoDispose (void)
{
  NetDevice::DoDispose ();
}

void
MmWaveVehicularNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}

uint32_t
MmWaveVehicularNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
MmWaveVehicularNetDevice::GetChannel (void) const
{
  return 0;
}

bool
MmWaveVehicularNetDevice::IsLinkUp (void) const
{
  return m_linkUp;
}

void
MmWaveVehicularNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
}

bool
MmWaveVehicularNetDevice::IsBroadcast (void) const
{
  return true;
}

Address
MmWaveVehicularNetDevice::GetBroadcast (void) const
{
  return Mac48Address::GetBroadcast ();
}

bool
MmWaveVehicularNetDevice::IsMulticast (void) const
{
  return false;
}

Address
MmWaveVehicularNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  return Mac48Address ("01:00:5e:00:00:00");
}

bool
MmWaveVehicularNetDevice::IsBridge (void) const
{
  return false;
}

bool
MmWaveVehicularNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
MmWaveVehicularNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_FATAL_ERROR ("Send from not supported");
  return false;
}

Ptr<Node>
MmWaveVehicularNetDevice::GetNode (void) const
{
  return m_node;
}

void
MmWaveVehicularNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

bool
MmWaveVehicularNetDevice::NeedsArp (void) const
{
  return false;
}

Address
MmWaveVehicularNetDevice::GetMulticast (Ipv6Address addr) const
{
  Address dummy;
  return dummy;
}

void
MmWaveVehicularNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_rxCallback = cb;
}

void
MmWaveVehicularNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{

}

bool
MmWaveVehicularNetDevice::SupportsSendFrom (void) const
{
  return false;
}

void
MmWaveVehicularNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_macAddr = Mac64Address::ConvertFrom (address);
}

Address
MmWaveVehicularNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_macAddr;
}

Ptr<MmWaveSidelinkMac>
MmWaveVehicularNetDevice::GetMac (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mac;
}

Ptr<MmWaveSidelinkPhy>
MmWaveVehicularNetDevice::GetPhy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

bool
MmWaveVehicularNetDevice::SetMtu (const uint16_t mtu)
{
  m_mtu = mtu;
  return true;
}

uint16_t
MmWaveVehicularNetDevice::GetMtu (void) const
{
  return m_mtu;
}

TypeId
MmWaveVehicularNetDevice::GetRlcType (std::string rlcType)
{
  if (rlcType == "LteRlcSm")
  {
    return LteRlcSm::GetTypeId ();
  }
  else if (rlcType == "LteRlcUm")
  {
    return LteRlcUm::GetTypeId ();
  }
  else if (rlcType == "LteRlcTm")
  {
    return LteRlcTm::GetTypeId ();
  }
  else
  {
    NS_FATAL_ERROR ("Unknown RLC type");
  }
}

void
MmWaveVehicularNetDevice::ActivateBearer(const uint8_t bearerId, const uint16_t destRnti, const Address& dest)
{
  NS_LOG_FUNCTION(this << bearerId);
  uint8_t lcid = bearerId; // set the LCID to be equal to the bearerId
  // future extensions could consider a different mapping
  // and will actually exploit the following map
  m_bid2lcid.insert(std::make_pair(bearerId, lcid));

  NS_ASSERT_MSG(m_bearerToInfoMap.find (bearerId) == m_bearerToInfoMap.end (),
    "There's another bearer associated to this bearerId: " << uint32_t(bearerId));

  EpcTft::PacketFilter slFilter;
  slFilter.remoteAddress= Ipv4Address::ConvertFrom(dest);

  Ptr<Node> node = GetNode ();
  Ptr<Ipv4> nodeIpv4 = node->GetObject<Ipv4> ();
  int32_t interface =  nodeIpv4->GetInterfaceForDevice (this);
  Ipv4Address src = nodeIpv4->GetAddress (interface, 0).GetLocal ();
  slFilter.localAddress= Ipv4Address::ConvertFrom(src);
  //slFilter.direction= EpcTft::DOWNLINK;
  slFilter.remoteMask= Ipv4Mask("255.255.255.255");
  slFilter.localMask= Ipv4Mask("255.255.255.255");

  NS_LOG_DEBUG(this << " Add filter for " << Ipv4Address::ConvertFrom(dest));

  Ptr<EpcTft> tft = Create<EpcTft> (); // Create a new tft
  tft->Add (slFilter); // Add the packet filter

  m_tftClassifier.Add(tft, bearerId);

  // Create RLC instance with specific RNTI and LCID
  ObjectFactory rlcObjectFactory;
  rlcObjectFactory.SetTypeId (GetRlcType(m_rlcType));
  Ptr<LteRlc> rlc = rlcObjectFactory.Create ()->GetObject<LteRlc> ();

  rlc->SetLteMacSapProvider (m_mac->GetMacSapProvider());
  rlc->SetRnti (destRnti); // this is the rnti of the destination
  rlc->SetLcId (lcid);

  // Call to the MAC method that created the SAP for binding the MAC instance on this node to the RLC instance just created
  m_mac->AddMacSapUser(lcid, rlc->GetLteMacSapUser());

  Ptr<LtePdcp> pdcp = CreateObject<LtePdcp> ();
  pdcp->SetRnti (destRnti); // this is the rnti of the destination
  pdcp->SetLcId (lcid);

  // Create the PDCP SAP that connects the PDCP instance to this NetDevice
  LtePdcpSapUser* pdcpSapUser = new PdcpSpecificSidelinkPdcpSapUser (this);
  pdcp->SetLtePdcpSapUser (pdcpSapUser);
  pdcp->SetLteRlcSapProvider (rlc->GetLteRlcSapProvider ());
  rlc->SetLteRlcSapUser (pdcp->GetLteRlcSapUser ());
  rlc->Initialize (); // this is needed to trigger the BSR procedure if RLC SM is selected

  Ptr<SidelinkRadioBearerInfo> rbInfo = CreateObject<SidelinkRadioBearerInfo> ();
  rbInfo->m_rlc= rlc;
  rbInfo->m_pdcp = pdcp;
  rbInfo->m_rnti = destRnti;

  NS_LOG_DEBUG(this << " MmWaveVehicularNetDevice::ActivateBearer() bid: " << (uint32_t)bearerId << " rnti: " << destRnti);

  // insert the tuple <lcid, pdcpSapProvider> in the map of this NetDevice, so that we are able to associate it to them later
  m_bearerToInfoMap.insert (std::make_pair (bearerId, rbInfo));
}

void
MmWaveVehicularNetDevice::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_DEBUG ("Received packet at: " << Simulator::Now().GetSeconds() << "s");
  uint8_t ipType;

  p->CopyData (&ipType, 1);
  ipType = (ipType>>4) & 0x0f;

  if (ipType == 0x04)
  {
    m_rxCallback (this, p, Ipv4L3Protocol::PROT_NUMBER, Address ());
  }
  else if (ipType == 0x06)
  {
    m_rxCallback (this, p, Ipv6L3Protocol::PROT_NUMBER, Address ());
  }
  else
  {
    NS_ABORT_MSG ("MmWaveVehicularNetDevice::Receive - Unknown IP type...");
  }
}

bool
MmWaveVehicularNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this);

  // classify the incoming packet
  uint32_t id = m_tftClassifier.Classify (packet, EpcTft::UPLINK, protocolNumber);
  NS_ASSERT ((id & 0xFFFFFF00) == 0);
  uint8_t bid = (uint8_t) (id & 0x000000FF);
  uint8_t lcid = BidToLcid(bid);

  // get the SidelinkRadioBearerInfo
  NS_ASSERT_MSG(m_bearerToInfoMap.find (bid) != m_bearerToInfoMap.end (), "No logical channel associated to this communication");
  auto bearerInfo = m_bearerToInfoMap.find (bid)->second;

  LtePdcpSapProvider::TransmitPdcpSduParameters params;
  params.pdcpSdu = packet;
  params.rnti = bearerInfo->m_rnti;
  params.lcid = lcid;

  NS_LOG_DEBUG(this << " MmWaveVehicularNetDevice::Send() bid " << (uint32_t)bid << " lcid " << (uint32_t)lcid << " rnti " << bearerInfo->m_rnti);

  packet->RemoveAllPacketTags (); // remove all tags in case there is any

  params.pdcpSdu = packet;
  bearerInfo->m_pdcp->GetLtePdcpSapProvider()->TransmitPdcpSdu (params);

  return true;
}

uint8_t
MmWaveVehicularNetDevice::BidToLcid(const uint8_t bearerId) const
{
  NS_ASSERT_MSG(m_bid2lcid.find(bearerId) != m_bid2lcid.end(),
    "BearerId to LCID mapping not found " << bearerId);
  return m_bid2lcid.find(bearerId)->second;
}

}

}
