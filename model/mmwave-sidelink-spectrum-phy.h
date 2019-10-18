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

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_SPECTRUM_PHY_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_SPECTRUM_PHY_H_

#include <ns3/object-factory.h>
#include <ns3/event-id.h>
#include <ns3/spectrum-value.h>
#include <ns3/mobility-model.h>
#include <ns3/packet.h>
#include <ns3/nstime.h>
#include <ns3/net-device.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-interference.h>
#include <ns3/data-rate.h>
#include <ns3/generic-phy.h>
#include <ns3/packet-burst.h>
#include "mmwave-sidelink-spectrum-signal-parameters.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mmwave-beamforming.h"
#include "ns3/mmwave-interference.h"
#include "ns3/mmwave-control-messages.h"

namespace ns3 {

namespace mmwave {

struct TbInfo_t
{
  Ptr<PacketBurst> packetBurst; ///< Packet burst associated to the transport block
  uint32_t size; ///< Transport block size
  uint8_t mcs; ///< MCS
  std::vector<int> rbBitmap; ///< Resource block bitmap
};

/**
* This method is used by the MmWaveSidelinkSpectrumPhy to notify the PHY that a
* previously started RX attempt has been successfully completed.
*
* @param packet the received Packet
*/
typedef Callback< void, Ptr<Packet> > MmWavePhyRxDataEndOkCallback;

//typedef Callback< void, std::list<Ptr<MmWaveControlMessage> > > MmWavePhyRxCtrlEndOkCallback;

/**
 * \ingroup mmwave
 * \class MmWaveSidelinkSpectrumPhy
 *
 * The MmWaveSidelinkSpectrumPhy models the physical layer of sidelink mode of vehicular
 * networks exploiting mmwave band.
 *
 */
class MmWaveSidelinkSpectrumPhy : public SpectrumPhy
{
public:
  MmWaveSidelinkSpectrumPhy ();
  virtual ~MmWaveSidelinkSpectrumPhy ();

  /**
   *  PHY states
   */
  enum State
  {
    IDLE = 0,
    TX,
    RX_DATA,
    RX_CTRL
  };

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual void DoDispose ();

  void Reset ();
  void ResetSpectrumModel ();

  /**
   * Set the associated NetDevice instance
   *
   * @param d the NetDevice instance
   */
  void SetDevice (Ptr<NetDevice> d);

  /**
   * Get the associated NetDevice instance
   *
   * @return a Ptr to the associated NetDevice instance
   */
  Ptr<NetDevice> GetDevice () const;

  /**
   * Set the mobility model associated with this device.
   *
   * @param m the mobility model
   */
  void SetMobility (Ptr<MobilityModel> m);

  /**
   * Get the associated MobilityModel instance
   *
   * @return a Ptr to the associated MobilityModel instance
   */
  Ptr<MobilityModel> GetMobility ();

  /**
   * Set the channel attached to this device.
   *
   * @param c the channel
   */
  void SetChannel (Ptr<SpectrumChannel> c);

  /**
   *
   * @return returns the SpectrumModel that this SpectrumPhy expects to be used
   * for all SpectrumValues that are passed to StartRx. If 0 is
   * returned, it means that any model will be accepted.
   */
  Ptr<const SpectrumModel> GetRxSpectrumModel () const;

  /**
   * Get the AntennaModel used by the NetDevice for reception
   *
   * @return a Ptr to the AntennaModel used by the NetDevice for reception
   */
  Ptr<AntennaModel> GetRxAntenna ();

  /**
   * set the AntennaModel to be used
   *
   * \param a the Antenna Model
   */
  void SetAntenna (Ptr<AntennaModel> a);

  void SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd);
  void SetTxPowerSpectralDensity (Ptr<SpectrumValue> TxPsd);

  void StartRx (Ptr<SpectrumSignalParameters> params);

  /**
   * \brief Start receive data function
   * \param params Ptr<MmWaveSidelinkSpectrumSignalParameters>
   */
  void StartRxData (Ptr<MmWaveSidelinkSpectrumSignalParameters> params);

  /**
   * \brief Start receive control function
   * \param params Ptr<SpectrumSignalParameters>
   */
  //void StartRxCtrl (Ptr<SpectrumSignalParameters> params);

  Ptr<SpectrumChannel> GetSpectrumChannel ();

  // TODO remove
  // void SetComponentCarrierId (uint8_t componentCarrierId);

  /**
  * Start a transmission of data frame in sidelink
  *
  * @param pb the burst of packets to be transmitted
  * @param duration the duration of the data frame
  *
  * @return true if an error occurred and the transmission was not
  * started, false otherwise.
  */
  bool StartTxDataFrames (Ptr<PacketBurst> pb, Time duration, uint8_t slotInd, uint8_t mcs, uint32_t size, std::vector<int> rbBitmap);

  //bool StartTxControlFrames (std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration);       // control frames from enb to ue

  /**
  * set the callback for the successful end of a RX ctrl frame, as part
  * of the interconnections between the MmWaveSidelinkSpectrumPhy and the PHY
  *
  * @param c the callback
  */
  void SetPhyRxDataEndOkCallback (MmWavePhyRxDataEndOkCallback c);

  //void SetPhyRxCtrlEndOkCallback (MmWavePhyRxCtrlEndOkCallback c);

  /**
  *
  *
  * \param p the new ChunkProcessor to be added to the Data Channel power processing chain
  */
  void AddDataPowerChunkProcessor (Ptr<mmWaveChunkProcessor> p);

  /**
  *
  *
  * \param p the new ChunkProcessor to be added to the data processing chain
  */
  void AddDataSinrChunkProcessor (Ptr<mmWaveChunkProcessor> p);

  /**
  *
  *
  * \param sinr vector of sinr perceived per each RB
  */
  void UpdateSinrPerceived (const SpectrumValue& sinr);

private:
  /**
  * \brief Change state function
  *
  * \param newState the new state to set
  */
  void ChangeState (State newState);
  /// End transmit data function
  void EndTx ();
  /// End receive data function
  void EndRxData ();
  //void EndRxCtrl ();

  Ptr<mmWaveInterference> m_interferenceData; ///< the data interference
  Ptr<MobilityModel> m_mobility; ///< the modility model
  Ptr<NetDevice> m_device; ///< the device // TODO this is never used
  Ptr<SpectrumChannel> m_channel; ///< the channel
  Ptr<const SpectrumModel> m_rxSpectrumModel; ///< the spectrum model
  Ptr<SpectrumValue> m_txPsd; ///< the transmit PSD
  //Ptr<PacketBurst> m_txPacketBurst;

  std::list<TbInfo_t> m_rxTransportBlock; ///< the received with associated structure

  // Should it be MmWaveSidelinkControlMessage?
  //std::list<Ptr<MmWaveControlMessage> > m_rxControlMessageList;

  Time m_firstRxStart; ///< the first receive start
  Time m_firstRxDuration; ///< the first receive duration

  Ptr<AntennaModel> m_antenna; ///< the antenna model

  State m_state; ///< the state

  uint8_t m_componentCarrierId; ///< the component carrier ID

  //MmWavePhyRxCtrlEndOkCallback m_phyRxCtrlEndOkCallback;
  MmWavePhyRxDataEndOkCallback m_phyRxDataEndOkCallback;  ///< the mmwave sidelink phy receive data end ok callback

  SpectrumValue m_sinrPerceived; ///< the perceived SINR

  Ptr<UniformRandomVariable> m_random;

  bool m_dataErrorModelEnabled; ///< when true (default) the phy error model is enabled
  bool m_ctrlErrorModelEnabled; ///< when true (default) the phy error model is enabled for DL ctrl frame

  EventId m_endTxEvent; ///< end transmit event
  EventId m_endRxDataEvent; ///< end receive data event
  //EventId m_endRxCtrlEvent;

};

}

}


#endif /* SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_SPECTRUM_PHY_H_ */