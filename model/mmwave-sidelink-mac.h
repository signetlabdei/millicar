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

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_

#include "mmwave-sidelink-sap.h"
#include "mmwave-sidelink-phy.h"
#include "ns3/mmwave-phy-mac-common.h"

namespace ns3 {

namespace mmwave {

class MmWaveSidelinkMac : public Object
{

friend class MacSidelinkMemberPhySapUser;

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief Class constructor
   */
  MmWaveSidelinkMac (void);
  /**
   * \brief Class destructor
   */
  ~MmWaveSidelinkMac (void);
  /**
   * \brief Destructor implementation
   */
  virtual void DoDispose (void);
  /**
  * \brief trigger the start of a new slot with all the necessary information
  *
  * \param SfnSf subframe and slot information
  */
  void DoSlotIndication (SfnSf sfn);
  /**
  * \brief Get the PHY SAP user
  * \return a pointer to the SAP user of the PHY
  */
  MmWaveSidelinkPhySapUser* GetPhySapUser ();

private:
  // forwarded from MAC SAP
 /**
  * Transmit PDU function
  *
  */
  void DoTransmitPdu ();
  // forwarded from PHY SAP
 /**
  * Receive PHY PDU function
  *
  * \param p the packet
  */
  void DoReceivePhyPdu (Ptr<Packet> p);

  MmWaveSidelinkPhySapUser* m_phySapUser; ///< Sidelink PHY SAP user
  Ptr<MmWavePhyMacCommon> m_phyMacConfig; ///< PHY and MAC configuration pointer

};

class MacSidelinkMemberPhySapUser : public MmWaveSidelinkPhySapUser
{

public:
  MacSidelinkMemberPhySapUser (Ptr<MmWaveSidelinkMac> mac);

  void ReceivePhyPdu (Ptr<Packet> p) override;

  void SlotIndication (SfnSf sfn) override;

private:
  Ptr<MmWaveSidelinkMac> m_mac;

};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_ */
