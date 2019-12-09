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

#include <ns3/uinteger.h>
#include <ns3/log.h>
#include <ns3/lte-mac-sap.h>
#include <ns3/ipv4-header.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-header.h>
#include <ns3/ipv6-l3-protocol.h>
#include "mmwave-vehicular-net-device.h"

namespace ns3 {

namespace mmwave {

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularNetDevice");

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularNetDevice);

TypeId MmWaveVehicularNetDevice::GetTypeId ()
{
  static TypeId
    tid =
    TypeId ("ns3::MmWaveVehicularNetDevice")
    .SetParent<NetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (30000),
                   MakeUintegerAccessor (&MmWaveVehicularNetDevice::SetMtu,
                                         &MmWaveVehicularNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
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
  return false;
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

void
MmWaveVehicularNetDevice::RegisterDevice (const Address& dest, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG(m_macRnti.find (dest) == m_macRnti.end (), "This device had already been registered ");
  m_macRnti.insert (std::make_pair (dest, rnti));
}

void
MmWaveVehicularNetDevice::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
}

bool
MmWaveVehicularNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_ASSERT_MSG(m_macRnti.find (dest) != m_macRnti.end (), "No registered device corresponding to the destination MAC address.");
  LteMacSapProvider::TransmitPduParameters params;
  params.pdu = packet;
  params.rnti = (m_macRnti.find(dest))->second; // association between destination address and destination RNTI

  m_mac->DoTransmitPdu(params);

  return true;
}

}

}
