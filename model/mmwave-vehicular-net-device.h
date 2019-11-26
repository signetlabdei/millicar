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

private:
  Ptr<MmWaveSidelinkMac> m_mac; //!< pointer to the MAC instance to be associated to the NetDevice
  Ptr<MmWaveSidelinkPhy> m_phy; //!< pointer to the PHY instance to be associated to the NetDevice
  Mac64Address m_macAddr; //!< MAC address associated to the NetDevice
  mutable uint16_t m_mtu; //!< MTU associated to the NetDevice
  std::map<Address, uint16_t> m_macRnti; //!< map that associates each MAC address to a specific univocal (with respect to the transmitting device) RNTI
};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_VEHICULAR_NET_DEVICE_H_ */
