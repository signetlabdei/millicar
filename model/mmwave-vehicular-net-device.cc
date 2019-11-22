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
MmWaveVehicularNetDevice::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
}

bool
MmWaveVehicularNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  // TODO: need to associate destination address to a proper IP/RNTI/NetDevice of the destination node
  // TODO: need to call DoTransmitPdu of MmWaveSidelinkMac
  return true;
}


}

}
