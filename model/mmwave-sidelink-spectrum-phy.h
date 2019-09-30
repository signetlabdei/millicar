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
#include "mmwave-interference.h"
#include "mmwave-control-messages.h"

namespace ns3 {

namespace mmwave {

struct ExpectedTbInfo_t
{
  uint8_t ndi;
  uint32_t size;
  uint8_t mcs;
  std::vector<int> rbBitmap;
  uint8_t rv;
  double mi;
  bool downlink;
  bool corrupt;
  double tbler;
  uint8_t               symStart;
  uint8_t               numSym;
};

typedef std::map<uint16_t, ExpectedTbInfo_t> ExpectedTbMap_t;

typedef Callback< void, Ptr<Packet> > MmWavePhyRxDataEndOkCallback;
typedef Callback< void, std::list<Ptr<MmWaveControlMessage> > > MmWavePhyRxCtrlEndOkCallback;

class MmWaveSidelinkSpectrumPhy : public SpectrumPhy
{
public:
  MmWaveSidelinkSpectrumPhy ();
  virtual ~MmWaveSidelinkSpectrumPhy ();

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
  void StartRxCtrl (Ptr<SpectrumSignalParameters> params);
  Ptr<SpectrumChannel> GetSpectrumChannel ();

  void SetComponentCarrierId (uint8_t componentCarrierId);

  bool StartTxDataFrames (Ptr<PacketBurst> pb, std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration, uint8_t slotInd);

  bool StartTxControlFrames (std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration);       // control frames from enb to ue

  void SetPhyRxDataEndOkCallback (MmWavePhyRxDataEndOkCallback c);
  void SetPhyRxCtrlEndOkCallback (MmWavePhyRxCtrlEndOkCallback c);

  void AddDataPowerChunkProcessor (Ptr<mmWaveChunkProcessor> p);
  void AddDataSinrChunkProcessor (Ptr<mmWaveChunkProcessor> p);

  void UpdateSinrPerceived (const SpectrumValue& sinr);

  /**
   * Add the transport block that the spectrum should expect to receive.
   *
   * \param rnti the RNTI of the UE
   * \param ndi the New Data Indicator
   * \param tbSize the size of the transport block
   * \param mcs the modulation and coding scheme to use
   * \param map a map of the resource blocks used
   * \param rv the number of retransmissions
   * \param symStart the first symbol of this TB
   * \param numSym the number of symbols of the TB
   */
  void AddExpectedTb (uint16_t rnti, uint8_t ndi, uint32_t tbSize, uint8_t mcs, std::vector<int> map,
                      uint8_t rv, uint8_t symStart, uint8_t numSym);

private:
  void ChangeState (State newState);
  void EndTx ();
  void EndRxData ();
  void EndRxCtrl ();

  Ptr<mmWaveInterference> m_interferenceData;
  Ptr<MobilityModel> m_mobility;
  Ptr<NetDevice> m_device;
  Ptr<SpectrumChannel> m_channel;
  Ptr<const SpectrumModel> m_rxSpectrumModel;
  Ptr<SpectrumValue> m_txPsd;
  //Ptr<PacketBurst> m_txPacketBurst;
  std::list<Ptr<PacketBurst> > m_rxPacketBurstList;

  // Should it be MmWaveSidelinkControlMessage?
  std::list<Ptr<MmWaveControlMessage> > m_rxControlMessageList;

  Time m_firstRxStart;
  Time m_firstRxDuration;

  Ptr<AntennaModel> m_antenna;

  State m_state;

  uint8_t m_componentCarrierId; ///< the component carrier ID

  MmWavePhyRxCtrlEndOkCallback m_phyRxCtrlEndOkCallback;
  MmWavePhyRxDataEndOkCallback m_phyRxDataEndOkCallback;

  TracedCallback<RxPacketTraceParams> m_rxPacketTraceEnb;
  TracedCallback<RxPacketTraceParams> m_rxPacketTraceUe;

  SpectrumValue m_sinrPerceived;

  ExpectedTbMap_t m_expectedTbs;

  Ptr<UniformRandomVariable> m_random;

  bool m_dataErrorModelEnabled;       // when true (default) the phy error model is enabled
  bool m_ctrlErrorModelEnabled;       // when true (default) the phy error model is enabled for DL ctrl frame

  EventId m_endTxEvent;
  EventId m_endRxDataEvent;
  EventId m_endRxCtrlEvent;

};

}

}


#endif /* SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_SPECTRUM_PHY_H_ */
