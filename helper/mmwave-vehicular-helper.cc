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

#include "mmwave-vehicular-helper.h"
#include "ns3/log.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/antenna-array-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularHelper"); // TODO check if this has to be defined here

namespace mmwave {

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularHelper); // TODO check if this has to be defined here

MmWaveVehicularHelper::MmWaveVehicularHelper ()
{
  NS_LOG_FUNCTION (this);
}

MmWaveVehicularHelper::~MmWaveVehicularHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MmWaveVehicularHelper::GetTypeId ()
{
  static TypeId tid =
  TypeId ("ns3::MmWaveVehicularHelper")
  .SetParent<Object> ()
  .AddConstructor<MmWaveVehicularHelper> ()
  .AddAttribute ("PropagationLossModel",
                 "The type of path-loss model to be used. "
                 "The allowed values for this attributes are the type names "
                 "of any class inheriting from ns3::PropagationLossModel.",
                 StringValue (""),
                 MakeStringAccessor (&MmWaveVehicularHelper::SetPropagationLossModelType),
                 MakeStringChecker ())
  .AddAttribute ("SpectrumPropagationLossModel",
                 "The type of fast fading model to be used. "
                 "The allowed values for this attributes are the type names "
                 "of any class inheriting from ns3::SpectrumPropagationLossModel.",
                 StringValue (""),
                 MakeStringAccessor (&MmWaveVehicularHelper::SetSpectrumPropagationLossModelType),
                 MakeStringChecker ())
  .AddAttribute ("PropagationDelayModel",
                 "The type of propagation delay model to be used. "
                 "The allowed values for this attributes are the type names "
                 "of any class inheriting from ns3::PropagationDelayModel.",
                 StringValue (""),
                 MakeStringAccessor (&MmWaveVehicularHelper::SetPropagationDelayModelType),
                 MakeStringChecker ())
  ;

  return tid;
}

void
MmWaveVehicularHelper::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  // intialize the RNTI counter
  m_rntiCounter = 0;

  // create the channel
  m_channel = CreateObject<SingleModelSpectrumChannel> ();
  if (!m_propagationLossModelType.empty ())
  {
    ObjectFactory factory (m_propagationLossModelType);
    m_channel->AddPropagationLossModel (factory.Create<PropagationLossModel> ());
  }
  if (!m_spectrumPropagationLossModelType.empty ())
  {
    ObjectFactory factory (m_spectrumPropagationLossModelType);
    m_channel->AddSpectrumPropagationLossModel (factory.Create<SpectrumPropagationLossModel> ());
  }
  if (!m_propagationDelayModelType.empty ())
  {
    ObjectFactory factory (m_propagationDelayModelType);
    m_channel->SetPropagationDelayModel (factory.Create<PropagationDelayModel> ());
  }
}

void
MmWaveVehicularHelper::SetConfigurationParameters (Ptr<MmWavePhyMacCommon> conf)
{
  NS_LOG_FUNCTION (this);
  m_phyMacConfig = conf;
}

NetDeviceContainer
MmWaveVehicularHelper::InstallMmWaveVehicularNetDevices (NodeContainer nodes)
{
  NS_LOG_FUNCTION (this);

  Initialize (); // run DoInitialize if necessary

  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Node> node = *i;

      // create the device
      Ptr<MmWaveVehicularNetDevice> device = InstallSingleMmWaveVehicularNetDevice (node, ++m_rntiCounter);

      // assign an address
      device->SetAddress (Mac64Address::Allocate ());

      devices.Add (device);
    }

  // configure the devices to enable communication between them
  PairDevices (devices);

  return devices;
}

Ptr<MmWaveVehicularNetDevice>
MmWaveVehicularHelper::InstallSingleMmWaveVehicularNetDevice (Ptr<Node> node, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);

  // create the antenna
  Ptr<AntennaArrayModel> aam = CreateObject<AntennaArrayModel> ();

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  ssp->SetMobility (node->GetObject<MobilityModel> ());
  ssp->SetAntenna (aam);
  NS_ASSERT_MSG (m_channel, "First create the channel");
  ssp->SetChannel (m_channel);

  // add the spectrum phy to the spectrum channel
  m_channel->AddRx (ssp);

  //TODO connect the rx callback to the sink
  //rx_ssp->SetPhyRxDataEndOkCallback (MakeCallback (&MmWaveVehicularSpectrumPhyTestCase1::Rx, this));

  // create and configure the chunk processor
  Ptr<mmWaveChunkProcessor> pData = Create<mmWaveChunkProcessor> ();
  pData->AddCallback (MakeCallback (&MmWaveSidelinkSpectrumPhy::UpdateSinrPerceived, ssp));
  ssp->AddDataSinrChunkProcessor (pData);

  // create the phy
  NS_ASSERT_MSG (m_phyMacConfig, "First set the configuration parameters");
  Ptr<MmWaveSidelinkPhy> phy = CreateObject<MmWaveSidelinkPhy> (ssp, m_phyMacConfig);

  // create the mac
  Ptr<MmWaveSidelinkMac> mac = CreateObject<MmWaveSidelinkMac> (m_phyMacConfig);
  mac->SetRnti (rnti);

  // connect phy and mac
  phy->SetPhySapUser (mac->GetPhySapUser ());
  mac->SetPhySapProvider (phy->GetPhySapProvider ());

  // create the device
  Ptr<MmWaveVehicularNetDevice> device = CreateObject<MmWaveVehicularNetDevice> (phy, mac);

  return device;
}

void
MmWaveVehicularHelper::PairDevices (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // define the scheduling pattern
  // NOTE we assume a fixed scheduling pattern which periodically repeats at
  // each subframe. Each slot in the subframe is assigned to a different user.
  // If the number of devices is greater than the number of slots per subframe
  // an assert is raised
  // TODO update this part to enable a more flexible configuration of the
  // scheduling pattern
  NS_ASSERT_MSG (devices.GetN () <= m_phyMacConfig->GetSlotsPerSubframe (), "Too many devices");
  std::vector<uint16_t> pattern (m_phyMacConfig->GetSlotsPerSubframe ());
  for (uint16_t i = 0; i < devices.GetN (); i++)
  {
    Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (devices.Get (i));
    pattern.at (i) = di->GetMac ()->GetRnti ();
    NS_LOG_DEBUG ("slot " << i << " assigned to rnti " << pattern.at (i));
  }

  for (NetDeviceContainer::Iterator i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (*i);
      di->GetMac ()->SetSfAllocationInfo (pattern);

      for (NetDeviceContainer::Iterator j = devices.Begin (); j != devices.End (); ++j)
      {
        Ptr<MmWaveVehicularNetDevice> dj = DynamicCast<MmWaveVehicularNetDevice> (*j);

        if (*i != *j)
        {
          // initialize the <MAC address, RNTI> map of the devices
          di->RegisterDevice (dj->GetAddress (), dj->GetMac ()->GetRnti ());

          // register the associated devices in the PHY
          di->GetPhy ()->AddDevice (dj->GetMac ()->GetRnti (), dj);
        }
      }

    }
}

void
MmWaveVehicularHelper::SetPropagationLossModelType (std::string plm)
{
  NS_LOG_FUNCTION (this);
  m_propagationLossModelType = plm;
}

void
MmWaveVehicularHelper::SetSpectrumPropagationLossModelType (std::string splm)
{
  NS_LOG_FUNCTION (this);
  m_spectrumPropagationLossModelType = splm;
}

void
MmWaveVehicularHelper::SetPropagationDelayModelType (std::string pdm)
{
  NS_LOG_FUNCTION (this);
  m_propagationDelayModelType = pdm;
}

} // namespace mmwave
} // namespace ns3
