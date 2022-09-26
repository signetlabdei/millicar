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

#ifndef MMWAVE_VEHICULAR_HELPER_H
#define MMWAVE_VEHICULAR_HELPER_H

#include "ns3/mmwave-vehicular.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/spectrum-channel.h"
#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/mmwave-vehicular-traces-helper.h"
#include "ns3/object-factory.h"

namespace ns3 {

namespace millicar {

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
   * \param conf pointer to mmwave::MmWavePhyMacCommon
   */
  void SetConfigurationParameters (Ptr<mmwave::MmWavePhyMacCommon> conf);

  /**
   * Retrieve pointer to the object that lists all the configuration parameters
   * \return a pointer to a MmWavePhyMacCommon object
   */
  Ptr<mmwave::MmWavePhyMacCommon> GetConfigurationParameters () const;
  
  /**
   * Set the beamforming delay model type
   * \param pdm the type id of the beamforming model to use
   */
  void SetBeamformingModelType (std::string type);

  /**
   * Associate the devices in the container
   * \param devices the NetDeviceContainer with the devices
   */
  void PairDevices (NetDeviceContainer devices);

  /**
   * Configure the numerology index
   * \param index numerology index, used to define PHY layer parameters
   */
  void SetNumerology (uint8_t index);
                                   
  /**
   * Configure the type of channel model to be used
   * \param model string representing the channel model to be used
   */
  void SetChannelModelType (std::string model);

  /**
   * Configure the scheduling pattern for a specific group of devices
   * \param devices the NetDeviceContainer with the devices
   * \return a vector of integers representing the scheduling pattern
  */
  std::vector<uint16_t> CreateSchedulingPattern (NetDeviceContainer devices);

  /**
   * Identifies the supported scheduling pattern policies
   */
  enum SchedulingPatternOption_t {DEFAULT = 1,
                                   OPTIMIZED = 2};

  /**
  * Set the scheduling pattern option type
  * \param spo the enum representing the scheduling pattern policy to be adopted
  */
  void SetSchedulingPatternOptionType (SchedulingPatternOption_t spo);

  /**
  * Returns the adopted scheduling pattern policy
  * \return the adopted scheduling pattern policy
  */
  SchedulingPatternOption_t GetSchedulingPatternOptionType () const;

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
   * Create and configure the spectrum channel
   * \param model string representing the type of channel model to be created
   * \return pointer to the SpectrumChannel object
   */
  Ptr<SpectrumChannel> CreateSpectrumChannel (std::string model) const;

  Ptr<SpectrumChannel> m_channel; //!< the SpectrumChannel
  Ptr<mmwave::MmWavePhyMacCommon> m_phyMacConfig; //!< the configuration parameters
  uint16_t m_rntiCounter; //!< a counter to set the RNTIs
  uint8_t m_numerologyIndex; //!< numerology index
  double m_bandwidth; //!< system bandwidth
  std::string m_channelModelType; //!< the type of channel model to be used
  SchedulingPatternOption_t m_schedulingOpt; //!< the type of scheduling pattern policy to be adopted
  
  ObjectFactory m_bfModelFactory; //!< beamforming model object factory
  Ptr<MmWaveVehicularTracesHelper> m_phyTraceHelper; //!< Ptr to an helper for the physical layer traces

};

} // namespace millicar
} // namespace ns3

#endif /* MMWAVE_VEHICULAR_HELPER_H */
