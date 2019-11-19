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

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SAP_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SAP_H_

#include <ns3/packet-burst.h>
#include <ns3/mmwave-phy-mac-common.h>

namespace ns3 {

namespace mmwave {

class MmWaveSidelinkPhySapProvider
{
public:
  virtual ~MmWaveSidelinkPhySapProvider ()
  {
  }

  /**
   * \brief Called by the upper layers to fill PHY's buffer
   * \param pb burst of packets to be forwarded to the PHY layer
   * \param info information about slot allocation necessary to determine the transmission parameters
   */
  virtual void AddTransportBlock (Ptr<PacketBurst> pb, SlotAllocInfo info) = 0;

};

class MmWaveSidelinkPhySapUser
{
public:
  virtual ~MmWaveSidelinkPhySapUser ()
  {
  }

  /**
   * \brief Called by the PHY to notify the MAC of the reception of a new PHY-PDU
   * \param p packet
   */
  virtual void ReceivePhyPdu (Ptr<Packet> p) = 0;

  /**
   * \brief Trigger the start from a new slot (input from PHY layer)
   * \param frameNo frame number
   * \param subframeNo subframe number
   */
  virtual void SlotIndication (SfnSf) = 0;

};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_SAP_H_ */
