/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2020 University of Padova, Dep. of Information Engineering,
*   SIGNET lab.
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
*/

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SAP_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SAP_H_

#include <ns3/packet-burst.h>
#include <ns3/lte-mac-sap.h>
#include <ns3/lte-rlc-am.h>
#include <ns3/spectrum-value.h>
#include <ns3/mmwave-phy-mac-common.h>

namespace ns3 {

namespace millicar {

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
  virtual void AddTransportBlock (Ptr<PacketBurst> pb, mmwave::TtiAllocInfo info) = 0;

  /**
   * \brief Called by the upper layer to prepare the PHY for the reception from
   *        another device
   * \param rnti the rnti of the transmitting device
   */
  virtual void PrepareForReception (uint16_t rnti) = 0;

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
   * \param timingInfo the structure containing the timing information
   */
  virtual void SlotIndication (mmwave::SfnSf timingInfo) = 0;

  /**
   * \brief Reports the SINR meausured with a certain device
   * \param sinr the SINR
   * \param rnti RNTI of the transmitting device
   * \param numSym size of the transport block that generated the report in
            number of OFDM symbols
   * \param tbSize size of the transport block that generated the report in
            number of bytes
   */
  virtual void SlSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize) = 0;

};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_SAP_H_ */
