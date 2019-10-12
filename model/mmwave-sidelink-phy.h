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
*   along with this program; if not, write to the Free Software:100cento

*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


#ifndef SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_PHY_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_PHY_H_

#include <ns3/mmwave-phy.h>
#include <ns3/ptr.h>
#include <map>

namespace ns3 {

namespace mmwave {

class MmWaveSidelinkPhy : public MmWavePhy
{

public:
  MmWaveSidelinkPhy ();

  MmWaveSidelinkPhy (Ptr<MmWaveSpectrumPhy> ulPhy, Ptr<MmWaveSpectrumPhy> dlPhy);

  virtual ~MmWaveSidelinkPhy ();

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  // void SetTxPower (double pow);
  // double GetTxPower () const;
  //
  // void SetNoiseFigure (double pf);
  // double GetNoiseFigure () const;
  //
  // bool SendPacket (Ptr<Packet> packet);
  //
  Ptr<SpectrumValue> CreateTxPowerSpectralDensity ();
  //
  // void DoSetSubChannels ();
  //
  // void SetSubChannelsForReception (std::vector <int> mask);
  // std::vector <int> GetSubChannelsForReception (void);
  //
  // void SetSubChannelsForTransmission (std::vector <int> mask);
  // std::vector <int> GetSubChannelsForTransmission (void);
  //
  // void DoSendControlMessage (Ptr<MmWaveControlMessage> msg);
  //
  // Ptr<MmWaveSpectrumPhy> GetDlSpectrumPhy () const;
  // Ptr<MmWaveSpectrumPhy> GetUlSpectrumPhy () const;
  //
  // void SubframeIndication (uint16_t frameNum, uint8_t subframeNum);
  // void StartSlot ();
  // void EndSlot ();

};

}

}

#endif /* SRC_MMWAVE_MODEL_MMWAVE_SIDELINK_PHY_H_ */
