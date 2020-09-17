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

#include "mmwave-vehicular-helper.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/mmwave-vehicular-antenna-array-model.h"
#include "ns3/mmwave-vehicular-spectrum-propagation-loss-model.h"
#include "ns3/pointer.h"
#include "ns3/config.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularHelper"); // TODO check if this has to be defined here

namespace millicar {

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularHelper); // TODO check if this has to be defined here

MmWaveVehicularHelper::MmWaveVehicularHelper ()
: m_phyTraceHelper{CreateObject<MmWaveVehicularTracesHelper>("sinr-mcs.txt")} // TODO name as attribute
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
  .AddAttribute ("Numerology",
                 "Numerology to use for the definition of the frame structure."
                 "2 : subcarrier spacing will be set to 60 KHz"
                 "3 : subcarrier spacing will be set to 120 KHz",
                 UintegerValue (2),
                 MakeUintegerAccessor (&MmWaveVehicularHelper::SetNumerology),
                 MakeUintegerChecker<uint8_t> ())
  .AddAttribute ("Bandwidth",
                 "Bandwidth in Hz",
                 DoubleValue (1e8),
                 MakeDoubleAccessor (&MmWaveVehicularHelper::m_bandwidth),
                 MakeDoubleChecker<double> ())
  .AddAttribute ("SchedulingPatternOption",
                 "The type of scheduling pattern option to be used for resources assignation."
                 "Default   : one single slot per subframe for each device"
                 "Optimized : each slot of the subframe is used",
                 EnumValue(DEFAULT),
                 MakeEnumAccessor (&MmWaveVehicularHelper::SetSchedulingPatternOptionType,
                                   &MmWaveVehicularHelper::GetSchedulingPatternOptionType),
                 MakeEnumChecker(DEFAULT, "Default",
                                 OPTIMIZED, "Optimized"))
  ;

  return tid;
}

void
MmWaveVehicularHelper::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  // intialize the RNTI counter
  m_rntiCounter = 0;
  
  // if the PHY layer configuration object was not set manually, create it 
  if (!m_phyMacConfig)
  {
    m_phyMacConfig = CreateObjectWithAttributes<mmwave::MmWavePhyMacCommon> ("Numerology", EnumValue (m_numerologyIndex),   
                                                                             "Bandwidth", DoubleValue (m_bandwidth));
  }

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

  // 3GPP vehicular channel needs proper configuration
  if (m_spectrumPropagationLossModelType == "ns3::MmWaveVehicularSpectrumPropagationLossModel")
  {
    Ptr<MmWaveVehicularSpectrumPropagationLossModel> threeGppSplm = DynamicCast<MmWaveVehicularSpectrumPropagationLossModel> (m_channel->GetSpectrumPropagationLossModel ());
    PointerValue plm;
    m_channel->GetAttribute ("PropagationLossModel", plm);
    
    Ptr<MmWaveVehicularPropagationLossModel> pathloss = DynamicCast<MmWaveVehicularPropagationLossModel> (plm.Get<PropagationLossModel> ());
    pathloss->SetFrequency (m_phyMacConfig->GetCenterFrequency());
    
    threeGppSplm->SetPathlossModel (pathloss); // associate pathloss and fast fading models
    threeGppSplm->SetFrequency (m_phyMacConfig->GetCenterFrequency()); // set correct value of frequency
    
  }
}

void
MmWaveVehicularHelper::SetConfigurationParameters (Ptr<mmwave::MmWavePhyMacCommon> conf)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_rntiCounter == 0, "The PHY configuration should be set before the installation of any device.");
  m_phyMacConfig = conf;
}

Ptr<mmwave::MmWavePhyMacCommon>
MmWaveVehicularHelper::GetConfigurationParameters () const
{
  NS_LOG_FUNCTION (this);
  return m_phyMacConfig;
}

void
MmWaveVehicularHelper::SetNumerology (uint8_t index)
{
  NS_LOG_FUNCTION (this);
  m_numerologyIndex = index;
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

  return devices;
}

Ptr<MmWaveVehicularNetDevice>
MmWaveVehicularHelper::InstallSingleMmWaveVehicularNetDevice (Ptr<Node> node, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);

  // create the antenna
  Ptr<MmWaveVehicularAntennaArrayModel> aam = CreateObject<MmWaveVehicularAntennaArrayModel> ();

  // create and configure the tx spectrum phy
  Ptr<MmWaveSidelinkSpectrumPhy> ssp = CreateObject<MmWaveSidelinkSpectrumPhy> ();
  NS_ASSERT_MSG (node->GetObject<MobilityModel> (), "Missing mobility model");
  ssp->SetMobility (node->GetObject<MobilityModel> ());
  ssp->SetAntenna (aam);
  NS_ASSERT_MSG (m_channel, "First create the channel");
  ssp->SetChannel (m_channel);

  // add the spectrum phy to the spectrum channel
  m_channel->AddRx (ssp);

  // create and configure the chunk processor
  Ptr<mmwave::mmWaveChunkProcessor> pData = Create<mmwave::mmWaveChunkProcessor> ();
  pData->AddCallback (MakeCallback (&MmWaveSidelinkSpectrumPhy::UpdateSinrPerceived, ssp));
  ssp->AddDataSinrChunkProcessor (pData);

  // create the phy
  NS_ASSERT_MSG (m_phyMacConfig, "First set the configuration parameters");
  Ptr<MmWaveSidelinkPhy> phy = CreateObject<MmWaveSidelinkPhy> (ssp, m_phyMacConfig);

  // connect the rx callback of the spectrum object to the sink
  ssp->SetPhyRxDataEndOkCallback (MakeCallback (&MmWaveSidelinkPhy::Receive, phy));

  // connect the callback to report the SINR
  ssp->SetSidelinkSinrReportCallback (MakeCallback (&MmWaveSidelinkPhy::GenerateSinrReport, phy));

  if(m_phyTraceHelper != 0)
  {
    ssp->SetSidelinkSinrReportCallback (MakeCallback (&MmWaveVehicularTracesHelper::McsSinrCallback, m_phyTraceHelper));
  }

  // create the mac
  Ptr<MmWaveSidelinkMac> mac = CreateObject<MmWaveSidelinkMac> (m_phyMacConfig);
  mac->SetRnti (rnti);

  // connect phy and mac
  phy->SetPhySapUser (mac->GetPhySapUser ());
  mac->SetPhySapProvider (phy->GetPhySapProvider ());

  // create and configure the device
  Ptr<MmWaveVehicularNetDevice> device = CreateObject<MmWaveVehicularNetDevice> (phy, mac);
  node->AddDevice (device);
  device->SetNode (node);
  ssp->SetDevice (device);

  // connect the rx callback of the mac object to the rx method of the NetDevice
  mac->SetForwardUpCallback(MakeCallback(&MmWaveVehicularNetDevice::Receive, device));

  // initialize the channel (if needed)
  Ptr<MmWaveVehicularSpectrumPropagationLossModel> splm = DynamicCast<MmWaveVehicularSpectrumPropagationLossModel> (m_channel->GetSpectrumPropagationLossModel ());
  if (splm)
    splm->AddDevice (device, aam);

  return device;
}

void
MmWaveVehicularHelper::PairDevices (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // TODO update this part to enable a more flexible configuration of the
  // scheduling pattern

  std::vector<uint16_t> pattern = CreateSchedulingPattern(devices);

  uint8_t bearerId = 1;

  for (NetDeviceContainer::Iterator i = devices.Begin (); i != devices.End (); ++i)
    {

      Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (*i);
      Ptr<Node> iNode = di->GetNode ();
      Ptr<Ipv4> iNodeIpv4 = iNode->GetObject<Ipv4> ();
      NS_ASSERT_MSG (iNodeIpv4 != 0, "Nodes need to have IPv4 installed before pairing can be activated");

      di->GetMac ()->SetSfAllocationInfo (pattern); // this is called ONCE for each NetDevice

      for (NetDeviceContainer::Iterator j = i + 1; j != devices.End (); ++j)
      {
        Ptr<MmWaveVehicularNetDevice> dj = DynamicCast<MmWaveVehicularNetDevice> (*j);
        Ptr<Node> jNode = dj->GetNode ();
        Ptr<Ipv4> jNodeIpv4 = jNode->GetObject<Ipv4> ();
        NS_ASSERT_MSG (jNodeIpv4 != 0, "Nodes need to have IPv4 installed before pairing can be activated");

        // initialize the <IP address, RNTI> map of the devices
        int32_t interface =  jNodeIpv4->GetInterfaceForDevice (dj);
        Ipv4Address diAddr = iNodeIpv4->GetAddress (interface, 0).GetLocal ();
        Ipv4Address djAddr = jNodeIpv4->GetAddress (interface, 0).GetLocal ();

        // register the associated devices in the PHY
        di->GetPhy ()->AddDevice (dj->GetMac ()->GetRnti (), dj);
        dj->GetPhy ()->AddDevice (di->GetMac ()->GetRnti (), di);

        // bearer activation by creating a logical channel between the two devices
        NS_LOG_DEBUG("Activation of bearer between " << diAddr << " and " << djAddr);
        NS_LOG_DEBUG("Bearer ID: " << uint32_t(bearerId) << " - Associate RNTI " << di->GetMac ()->GetRnti () << " to " << dj->GetMac ()->GetRnti ());

        di->ActivateBearer(bearerId, dj->GetMac ()->GetRnti (), djAddr);
        dj->ActivateBearer(bearerId, di->GetMac ()->GetRnti (), diAddr);
        bearerId++;
      }


    }
}

std::vector<uint16_t>
MmWaveVehicularHelper::CreateSchedulingPattern (NetDeviceContainer devices)
{
  // The maximum supported number of vehicles in the group is equal to the available number of slots per subframe
  // TODO implement scheduling pattern that support groups with a number of vehicle greater than the number of slots per subframe
  NS_ABORT_MSG_IF (devices.GetN () > m_phyMacConfig->GetSlotsPerSubframe (), "Too many devices");

  // NOTE fixed scheduling pattern, set in configuration time and assumed the same for each subframe

  uint8_t slotPerSf = m_phyMacConfig->GetSlotsPerSubframe ();
  std::vector<uint16_t> pattern;

  switch (m_schedulingOpt)
  {
    case DEFAULT:
    {
      // Each slot in the subframe is assigned to a different user.
      // If (numDevices < numSlots), the remaining available slots are unused
      pattern = std::vector<uint16_t> (slotPerSf);
      for (uint16_t i = 0; i < devices.GetN (); i++)
      {
        Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (devices.Get (i));
        pattern.at(i) = di->GetMac ()->GetRnti ();
        NS_LOG_DEBUG ("slot " << i << " assigned to rnti " << di->GetMac ()->GetRnti ());
      }
      break;
    }
    case OPTIMIZED:
    {
      // Each slot in the subframe is used
      uint8_t slotPerDev = std::floor ( slotPerSf / devices.GetN ());
      uint8_t remainingSlots = slotPerSf % devices.GetN ();

      NS_LOG_DEBUG("Minimum number of slots per device = " << (uint16_t)slotPerDev);
      NS_LOG_DEBUG("Available slots = " << (uint16_t)slotPerSf);

      uint8_t slotCnt = 0;

      for (uint16_t i = 0; i < devices.GetN (); i++)
      {
        Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (devices.Get (i));

        for (uint8_t j = 0; j < slotPerDev; j++)
        {
          pattern.push_back(di->GetMac ()->GetRnti ());
          NS_LOG_DEBUG ("slot " << uint16_t(slotCnt) << " assigned to rnti " << di->GetMac ()->GetRnti ());
          slotCnt++;
        }
      }

      NS_LOG_DEBUG("Remaining slots = " << (uint16_t)remainingSlots);
      for (uint16_t i = 0; i < remainingSlots; i++)
      {
        Ptr<MmWaveVehicularNetDevice> di = DynamicCast<MmWaveVehicularNetDevice> (devices.Get (i));
        pattern.push_back(di->GetMac ()->GetRnti ());
        NS_LOG_DEBUG ("slot " << uint16_t(slotCnt) << " assigned to rnti " << di->GetMac ()->GetRnti ());
        slotCnt++;
      }
      break;
    }
    default:
    {
      NS_FATAL_ERROR("Programming Error.");
    }
  }

  return pattern;
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

void
MmWaveVehicularHelper::SetSchedulingPatternOptionType (SchedulingPatternOption_t spo)
{
  NS_LOG_FUNCTION (this);
  m_schedulingOpt = spo;
}

MmWaveVehicularHelper::SchedulingPatternOption_t
MmWaveVehicularHelper::GetSchedulingPatternOptionType () const
{
  NS_LOG_FUNCTION (this);
  return m_schedulingOpt;
}

} // namespace millicar
} // namespace ns3
