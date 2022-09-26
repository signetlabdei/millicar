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

#include <ns3/mac64-address.h>
#include <ns3/net-device.h>
#include "ns3/lte-pdcp.h"
#include "ns3/lte-radio-bearer-info.h"
#include "ns3/epc-tft-classifier.h"
#include <ns3/uniform-planar-array.h>
#include "mmwave-sidelink-phy.h"
#include "mmwave-sidelink-mac.h"

#ifndef SRC_MMWAVE_VEHICULAR_NET_DEVICE_H_
#define SRC_MMWAVE_VEHICULAR_NET_DEVICE_H_

namespace ns3 {

namespace millicar {

  /**
   * store information on active radio bearer instance
   *
   */
class SidelinkRadioBearerInfo : public LteRadioBearerInfo
{
public:
  SidelinkRadioBearerInfo (void) {};
  virtual ~SidelinkRadioBearerInfo (void) {};

  uint16_t m_rnti; //!< rnti of the other endpoint of this bearer
};

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
   * \brief Returns valid LteRlc TypeId based on the parameter passed
   * \param rlcType string representing the selected RLC type
   * \return the type id of the proper RLC class
   */
  TypeId GetRlcType (std::string rlcType);

  /**
   * \brief Packet forwarding to the MAC layer by calling DoTransmitPdu
   * \param p packet to be used to fire the callbacks
   */
  void Receive (Ptr<Packet> p);

  /**
   * \brief a logical channel with instances of PDCP/RLC layers is created and associated to a specific receiving device
   * \param bearerId identifier of the tunnel between two devices
   * \param destRnti the rnti of the destination
   * \param dest IP destination address
  */
  void ActivateBearer (const uint8_t bearerId, const uint16_t destRnti, const Address& dest);
  
  /**
   * \brief Set UniformPlanarArray object 
   * \param antenna antenna to mount on the device 
   */
  void SetAntennaArray (Ptr<UniformPlanarArray> antenna);
  
  /**
   * \brief Get a UniformPlanarArray object
   * \return the antenna mounted on the devide
   */
  Ptr<UniformPlanarArray> GetAntennaArray () const;

protected:
  NetDevice::ReceiveCallback m_rxCallback; //!< callback that is fired when a packet is received

private:
  Ptr<MmWaveSidelinkMac> m_mac; //!< pointer to the MAC instance to be associated to the NetDevice
  Ptr<MmWaveSidelinkPhy> m_phy; //!< pointer to the PHY instance to be associated to the NetDevice
  std::map<uint8_t, Ptr<SidelinkRadioBearerInfo>> m_bearerToInfoMap; //!< map to store RLC and PDCP instances associated to a specific bearer ID
  Mac64Address m_macAddr; //!< MAC address associated to the NetDevice
  mutable uint16_t m_mtu; //!< MTU associated to the NetDevice
  uint8_t m_bidCounter = 0; //!< counter of bearer connections activated for a single UE
  uint8_t m_lcidCounter = 0; //!< counter of logical channels created on this NetDevice
  std::map<uint8_t,uint8_t> m_bid2lcid; //!< map from the BID to a unique LCID
  uint32_t m_ifIndex;
  bool m_linkUp; //!< boolean that indicates if the link is UP (true) or DOWN (false)
  Ptr<Node> m_node; //!< pointer to the node associated to the NetDevice
  EpcTftClassifier m_tftClassifier;
  std::string m_rlcType;
  
  Ptr<UniformPlanarArray> m_antenna; //!< antenna mounted on the device

  /**
   * Return the LCID associated to a certain bearer
   * \param bearerId the bearer
   * \return the logical channel ID
   */
  uint8_t BidToLcid(const uint8_t bearerId) const;
};

class PdcpSpecificSidelinkPdcpSapUser : public LtePdcpSapUser
{
public:
  /**
   * Constructor
   *
   * \param netDevice the specific netDevice to be connected to the PDCP SAP
   */
   PdcpSpecificSidelinkPdcpSapUser (Ptr<MmWaveVehicularNetDevice> netDevice);

  // Interface implemented from LtePdcpSapUser
  virtual void ReceivePdcpSdu (ReceivePdcpSduParameters params);

private:
  PdcpSpecificSidelinkPdcpSapUser ();
  Ptr<MmWaveVehicularNetDevice> m_netDevice; ///< NetDevice
};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_VEHICULAR_NET_DEVICE_H_ */
