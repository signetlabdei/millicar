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

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_

#include "mmwave-sidelink-sap.h"
#include "ns3/mmwave-amc.h"
#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/traced-callback.h"

namespace ns3 {

namespace millicar {

/// structure used for the scheduling info callback
struct SlSchedulingCallback
{
  uint16_t frame; //!< frame number
  uint8_t subframe; //!< subframe number
  uint8_t slotNum; //!< slot number
  uint8_t symStart; //!< index of the starting symbol
  uint8_t numSym; //!< nummber of allocated symbols
  uint8_t mcs; //!< the MCS for transport block
  uint16_t tbSize; //!< the TB size in bytes
  uint16_t txRnti; //!< the RNTI which identifies the sender
  uint16_t rxRnti; //!< the RNTI which identifies the destination
};

class MmWaveSidelinkMac : public Object
{

  friend class MacSidelinkMemberPhySapUser;
  friend class RlcSidelinkMemberMacSapProvider;

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Delete default constructor to avoid misuse
   */
  MmWaveSidelinkMac (void) = delete;

  /**
   * \brief Class constructor
   * \param pmc pointer to the mmwave::MmWavePhyMacCommon instance which specifies the
   *        PHY/MAC parameters
   */
  MmWaveSidelinkMac (Ptr<mmwave::MmWavePhyMacCommon> pmc);

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
  * \param timingInfo the structure containing the timing information
  */
  void DoSlotIndication (mmwave::SfnSf timingInfo);

  /**
  * \brief Get the PHY SAP user
  * \return a pointer to the SAP user
  */
  MmWaveSidelinkPhySapUser* GetPhySapUser () const;

  /**
  * \brief Set the PHY SAP provider
  * \param sap the PHY SAP provider
  */
  void SetPhySapProvider (MmWaveSidelinkPhySapProvider* sap);

  /**
  * \brief return the MAC SAP provider
  * \return the MacSapProvider
  */
  LteMacSapProvider* GetMacSapProvider () const;

  /**
  * \brief assign a proper value to the RNTI associated to a specific user
  * \param rnti value of the rnti
  */
  void SetRnti (uint16_t rnti);

  /**
  * \brief return the RNTI associated to a specific user
  * \return the RNTI
  */
  uint16_t GetRnti () const;

  /**
  * \brief set the subframe allocation pattern
  * \param pattern the allocation pattern. The number of element must be equal to
  *        number of slots per subframe. Each element represents the RNTI of the
  *        device scheduled in the corresponding slot.
  */
  void SetSfAllocationInfo (std::vector<uint16_t> pattern);

  /**
  * \brief Transmit PDU function
  */
  void DoTransmitPdu (LteMacSapProvider::TransmitPduParameters params);

  /**
   * \brief set the callback used to forward data packets up to the NetDevice
   * \param cb the callback
   */
  void SetForwardUpCallback (Callback <void, Ptr<Packet> > cb);

  /**
   * TracedCallback signature for SL scheduling
   *
   * \param params the struct SlSchedulingCallback containing the scheduling info
   */
  typedef void (* SlSchedulingTracedCallback) (SlSchedulingCallback params);

  /**
   * Associate a MAC SAP user instance to the LCID and add it in the map
   * \param lcid Logical Channel ID
   * \param macSapUser LteMacSapUser to be associated to a single LCID and added to the respective map
   */
  void AddMacSapUser (uint8_t lcid, LteMacSapUser* macSapUser);

private:
  // forwarded from PHY SAP
 /**
  * Receive PHY PDU function
  * \param p the packet
  */
  void DoReceivePhyPdu (Ptr<Packet> p);

  /**
  * \brief Based on the SINR reported, the CQI is evaluated and pushed to the
           CQIs history with the latest SINR information
  * \params sinr SpectrumValue instance representing the SINR measured on all
            the spectrum chunks
  * \params rnti RNTI of the transmitting device
  * \params numSym size of the transport block that generated the report in
            number of OFDM symbols
  * \params tbSize size of the transport block that generated the report in bytes
  */
  void DoSlSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize);

  /**
  * \brief Implements RlcSidelinkMemberMacSapProvider::ReportBufferStatus,
  *        reports the RLC buffer status to the MAC
  * \param params the buffer status report
  */
  void DoReportBufferStatus (LteMacSapProvider::ReportBufferStatusParameters params);

  /////////////////////////////////////////////////////////////////////////////

  /**
  * \brief Evaluate the MCS of the link towards a specific device
  * \params rnti the RNTI that identifies the device we want to communicate with
  */
  uint8_t GetMcs (uint16_t rnti);

  /**
  * \brief Decides how to allocate the available resources to the active
  *        logical channels
  * \params timingInfo the SfnSf object containing the frame, subframe and slot
  *         index
  * \returns SlotAllocInfo object containing the scheduling information
  */
  mmwave::SlotAllocInfo ScheduleResources (mmwave::SfnSf timingInfo);

  /**
  * \brief Updates the BSR corresponding to the specified LC by subtracting the
  *        assigned grant
  * \params lcid the logical channel ID
  * \params assignedBytes the assigned grant in bytes
  * \returns updated iterator to the Buffer Status Report map
  */
  std::map<uint8_t, LteMacSapProvider::ReportBufferStatusParameters>::iterator UpdateBufferStatusReport (uint8_t lcid, uint32_t assignedBytes);

  MmWaveSidelinkPhySapUser* m_phySapUser; //!< Sidelink PHY SAP user
  MmWaveSidelinkPhySapProvider* m_phySapProvider; //!< Sidelink PHY SAP provider
  LteMacSapProvider* m_macSapProvider; //!< Sidelink MAC SAP provider
  std::map<uint8_t, LteMacSapUser*> m_lcidToMacSap; //!< map that associates to an LCID the respective MAC SAP
  Ptr<mmwave::MmWavePhyMacCommon> m_phyMacConfig; //!< PHY and MAC configuration pointer
  Ptr<mmwave::MmWaveAmc> m_amc; //!< pointer to AMC instance
  bool m_useAmc; //!< set to true to use adaptive modulation and coding
  uint8_t m_mcs; //!< the MCS used to transmit the packets if AMC is not used
  uint16_t m_rnti; //!< radio network temporary identifier
  std::vector<uint16_t> m_sfAllocInfo; //!< defines the subframe allocation, m_sfAllocInfo[i] = RNTI of the device scheduled for slot i
  std::map<uint16_t, std::list<LteMacSapProvider::TransmitPduParameters>> m_txBufferMap; //!< map containing the <RNTI, tx buffer> pairs
  std::map<uint16_t, std::vector<int>> m_slCqiReported; //!< map containing the <RNTI, CQI> pairs
  Callback<void, Ptr<Packet> > m_forwardUpCallback; //!< upward callback to the NetDevice
  std::map<uint8_t, LteMacSapProvider::ReportBufferStatusParameters> m_bufferStatusReportMap; //!< map containing the <LCID, buffer status in bits> pairs

  // trace sources
  TracedCallback<SlSchedulingCallback> m_schedulingTrace; //!< trace source returning information regarding the scheduling
};

class MacSidelinkMemberPhySapUser : public MmWaveSidelinkPhySapUser
{

public:
  MacSidelinkMemberPhySapUser (Ptr<MmWaveSidelinkMac> mac);

  void ReceivePhyPdu (Ptr<Packet> p) override;

  void SlotIndication (mmwave::SfnSf timingInfo) override;

  void SlSinrReport (const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize) override;

private:
  Ptr<MmWaveSidelinkMac> m_mac;

};

class RlcSidelinkMemberMacSapProvider : public LteMacSapProvider
{
public:
  /**
   * Constructor
   *
   * \param mac the MAC class
   */
  RlcSidelinkMemberMacSapProvider (Ptr<MmWaveSidelinkMac> mac);

  // inherited from LteMacSapProvider
  virtual void TransmitPdu (TransmitPduParameters params);
  virtual void ReportBufferStatus (ReportBufferStatusParameters params);

private:
  Ptr<MmWaveSidelinkMac> m_mac; ///< the MAC class
};

} // mmwave namespace

} // ns3 namespace

#endif /* SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_MAC_H_ */
