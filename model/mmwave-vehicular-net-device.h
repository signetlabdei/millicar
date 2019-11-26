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

#include <ns3/mac64-address.h>
#include <ns3/net-device.h>
#include "mmwave-sidelink-phy.h"
#include "mmwave-sidelink-mac.h"

#ifndef SRC_MMWAVE_MODEL_MMWAVE_VEHICULAR_NET_DEVICE_H_
#define SRC_MMWAVE_MODEL_MMWAVE_VEHICULAR_NET_DEVICE_H_

namespace ns3 {

namespace mmwave {

class MmWaveVehicularNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);


  /**
   * \brief Class constructor, it is not used
   */
  MmWaveVehicularNetDevice (void);

  /**
   * \brief Class constructor
   * \param phy pointer to the PHY
   * \param mac pointer to the MAC
   *
   */
  MmWaveVehicularNetDevice (Ptr<MmWaveSidelinkPhy> phy, Ptr<MmWaveSidelinkMac> mac);

  /**
   * \brief Class destructor
   */
  virtual ~MmWaveVehicularNetDevice (void);

  /**
   * \brief Destructor implementation
   */
  virtual void DoDispose (void);

  // For the following methods refer to NetDevice doxy

  virtual void SetIfIndex (const uint32_t index);

  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual bool IsLinkUp (void) const;

  virtual void AddLinkChangeCallback (Callback<void> callback);

  virtual bool IsBroadcast (void) const;

  virtual Address GetBroadcast (void) const;

  virtual bool IsMulticast (void) const;

  virtual Address GetMulticast (Ipv4Address multicastGroup) const;

  virtual bool IsBridge (void) const;

  virtual bool IsPointToPoint (void) const;

  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;

  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual void SetReceiveCallback (ReceiveCallback cb);

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);

  virtual bool SupportsSendFrom (void) const;

  /**
   * \brief Associate to the device a univocal (with respect to the transmitting device) RNTI
   * \param address MAC address
   * \param rnti univocal RNTI
   */
  virtual void RegisterDevice (const Address& address, uint16_t rnti);

  /**
   * \brief Set MAC address associated to the NetDevice
   * \param address MAC address
   */
  virtual void SetAddress (Address address);

  /**
   * \brief Returns MAC address associated to the NetDevice
   */
  virtual Address GetAddress (void) const;

  /**
   * \brief Returns pointer to MAC
   */
  virtual Ptr<MmWaveSidelinkMac> GetMac (void) const;

  /**
   * \brief Returns pointer to PHY
   */
  virtual Ptr<MmWaveSidelinkPhy> GetPhy (void) const;

  /**
   * \brief Send a packet to the vehicular stack
   * \param address MAC address of the destination device
   * \param protocolNumber identifies if NetDevice is using IPv4 or IPv6
   */
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);

  /**
   * \brief Set MTU associated to the NetDevice
   * \param mtu size of the Maximum Transmission Unit
   */
  virtual bool SetMtu (const uint16_t mtu);

  /**
   * \brief Returns MTU associated to the NetDevice
   */
  virtual uint16_t GetMtu (void) const;

  /**
   * \brief Packet forwarding to the MAC layer by calling DoTransmitPdu
   * \param p packet to be used to fire the callbacks
   */
  void Receive (Ptr<Packet> p);

protected:
  NetDevice::ReceiveCallback m_rxCallback; //!< callback that is fired when a packet is received

private:
  Ptr<MmWaveSidelinkMac> m_mac; //!< pointer to the MAC instance to be associated to the NetDevice
  Ptr<MmWaveSidelinkPhy> m_phy; //!< pointer to the PHY instance to be associated to the NetDevice
  Mac64Address m_macAddr; //!< MAC address associated to the NetDevice
  mutable uint16_t m_mtu; //!< MTU associated to the NetDevice
  std::map<Address, uint16_t> m_macRnti; //!< map that associates each MAC address to a specific univocal (with respect to the transmitting device) RNTI
  uint32_t m_ifIndex;
  bool m_linkUp; //!< boolean that indicates if the link is UP (true) or DOWN (false)
  Ptr<Node> m_node; //!< pointer to the node associated to the NetDevice
};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_VEHICULAR_NET_DEVICE_H_ */
