/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2019 University of Padova, Dep. of Information Engineering, SIGNET lab.
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

#ifndef MMWAVE_VEHICULAR_HELPER_H
#define MMWAVE_VEHICULAR_HELPER_H

#include "ns3/mmwave-vehicular.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/spectrum-channel.h"
#include "ns3/mmwave-phy-mac-common.h"

namespace ns3 {

namespace mmwave {

class MmWaveVehicularNetDevice;

/**
 * This class is used for the creation of MmWaveVehicularNetDevices and
 * their configuration
 */
class MmWaveVehicularHelper : public Object
{
public:
  /**
   * Constructor
   */
  MmWaveVehicularHelper (void);

  /**
   * Destructor
   */
  virtual ~MmWaveVehicularHelper (void);

  // inherited from Object
  static TypeId GetTypeId (void);

  /**
   * Install a MmWaveVehicularNetDevice on each node in the container
   * \param nodes the node container
   * \return a NetDeviceContainer containing the installed devices
   */
  NetDeviceContainer InstallMmWaveVehicularNetDevices (NodeContainer nodes);

  /**
   * Set the configuration parameters
   * \param conf pointer to MmWavePhyMacCommon
   */
  void SetConfigurationParameters (Ptr<MmWavePhyMacCommon> conf);

  /**
   * Set the propagation loss model type
   * \param plm the type id of the propagation loss model to use
   */
  void SetPropagationLossModelType (std::string plm);

  /**
   * Set the spectrum propagation loss model type
   * \param splm the type id of the spectrum propagation loss model to use
   */
  void SetSpectrumPropagationLossModelType (std::string splm);

  /**
   * Set the propagation delay model type
   * \param pdm the type id of the propagation delay model to use
   */
  void SetPropagationDelayModelType (std::string pdm);

protected:
  // inherited from Object
  virtual void DoInitialize (void) override;

private:
  /**
   * Install a MmWaveVehicularNetDevice on the node
   * \param n the node
   * \param rnti the RNTI
   * \return pointer to the installed NetDevice
   */
  Ptr<MmWaveVehicularNetDevice> InstallSingleMmWaveVehicularNetDevice (Ptr<Node> n, uint16_t rnti);

  /**
   * Associate the devices in the container
   * \param devices the NetDeviceContainer with the devices
   */
  void PairDevices (NetDeviceContainer devices);

  Ptr<SpectrumChannel> m_channel; //!< the SpectrumChannel
  Ptr<MmWavePhyMacCommon> m_phyMacConfig; //!< the configuration parameters
  uint16_t m_rntiCounter; //!< a counter to set the RNTIs
  std::string m_propagationLossModelType; //!< the type id of the propagation loss model to be used
  std::string m_spectrumPropagationLossModelType; //!< the type id of the spectrum propagation loss model to be used
  std::string m_propagationDelayModelType; //!< the type id of the delay model to be used
};

} // namespace mmwave
} // namespace ns3

#endif /* MMWAVE_VEHICULAR_HELPER_H */
