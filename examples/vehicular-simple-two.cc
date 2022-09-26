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

#include "ns3/mmwave-sidelink-spectrum-phy.h"
#include "ns3/mmwave-vehicular-net-device.h"
#include "ns3/mmwave-vehicular-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/spectrum-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config.h"
#include "ns3/command-line.h"
#include "ns3/node-list.h"

NS_LOG_COMPONENT_DEFINE ("VehicularSimpleTwo");

using namespace ns3;
using namespace millicar;

void PrintGnuplottableNodeListToFile (std::string filename);
/**
This script creates two pairs of vehicles moving at 20 m/s. The two groups are virtually positioned in two different lanes.
In the same group, the vehicles are positioned one in front of the other at 20 m distance, and they exchange packets through a UDP application.
Communication is done via a wireless channel.
The aim of this example is to check whether the interference was evaluated correctly when different groups
of nodes are transmitting in the same slot, sharing the same cell.
*/
int main (int argc, char *argv[])
{
  // system parameters
  double bandwidth = 100.0e6; // bandwidth in Hz
  double frequency = 28e9; // the carrier frequency
  uint32_t numerology = 3; // the numerology

  // applications
  uint32_t packetSize = 1024; // UDP packet size in bytes
  uint32_t startTime = 50; // application start time in milliseconds
  uint32_t endTime = 2000; // application end time in milliseconds
  uint32_t interPacketInterval = 1000; // interpacket interval in microseconds

  // mobility
  double speed = 20; // speed of the vehicles m/s
  double intraGroupDistance = 10; // distance between two vehicles belonging to the same group
  double interGroupInitialDistance = 40; // initial distance between the two groups
  double laneDistance = 5.0; // distance between the two lanes
  double antennaHeight = 2.0; // the height of the antenna
  std::string scenario = "V2V-Urban";
  
  CommandLine cmd;
  cmd.AddValue ("vehicleSpeed", "The speed of the vehicle", speed);
  cmd.AddValue ("scenario", "set the vehicular scenario", scenario);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::MmWaveSidelinkMac::UseAmc", BooleanValue (true));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (frequency));
  
  Config::SetDefault ("ns3::MmWaveVehicularHelper::Bandwidth", DoubleValue (bandwidth));
  Config::SetDefault ("ns3::MmWaveVehicularHelper::Numerology", UintegerValue (numerology));
  
  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (10)));
  
  // create the nodes
  NodeContainer group1, group2;
  group1.Create (2);
  group2.Create (2);

  // create the mobility models
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (group1);
  mobility.Install (group2);

  group1.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (-(interGroupInitialDistance / 2 + intraGroupDistance) , -laneDistance / 2, antennaHeight));
  group1.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  group1.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (-(interGroupInitialDistance / 2), -laneDistance / 2, antennaHeight));
  group1.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  group2.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (interGroupInitialDistance / 2, laneDistance / 2, antennaHeight));
  group2.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (-speed, 0, 0));

  group2.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (interGroupInitialDistance / 2 + intraGroupDistance, laneDistance / 2, antennaHeight));
  group2.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (-speed, 0, 0));

  // create and configure the helper
  Ptr<MmWaveVehicularHelper> helper = CreateObject<MmWaveVehicularHelper> ();
  helper->SetNumerology (numerology);
  helper->SetNumerology (3);
  helper->SetChannelModelType (scenario);
  NetDeviceContainer devs1 = helper->InstallMmWaveVehicularNetDevices (group1);
  NetDeviceContainer devs2 = helper->InstallMmWaveVehicularNetDevices (group2);

  // install the internet stack
  InternetStackHelper internet;
  internet.Install (group1);
  internet.Install (group2);

  // assign the IP addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devs1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  i = ipv4.Assign (devs2);
  
  // Mandatory to install buildings helper even if there are no buildings, 
  // otherwise V2V-Urban scenario does not work
  BuildingsHelper::Install (group1);
  BuildingsHelper::Install (group2);

  // connect the devices belonging to the same group
  helper->PairDevices(devs1);
  helper->PairDevices(devs2);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (group1.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0 group 1: " << group1.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1 group 1: " << group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  staticRouting = ipv4RoutingHelper.GetStaticRouting (group2.Get (0)->GetObject<Ipv4> ());
  staticRouting->SetDefaultRoute (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () , 2 );

  NS_LOG_DEBUG("IPv4 Address node 0 group 2: " << group2.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());
  NS_LOG_DEBUG("IPv4 Address node 1 group 2: " << group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ());

  // setup the applications
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (0xFFFFFFFF));
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds (interPacketInterval)));
  Config::SetDefault ("ns3::UdpClient::PacketSize", UintegerValue (packetSize));

  // create the applications for group 1
  uint32_t port = 4000;
  UdpServerHelper server11 (port);
  ApplicationContainer apps = server11.Install (group1.Get (1));

  UdpServerHelper server10 (port);
  apps.Add (server10.Install (group1.Get (0)));

  UdpClientHelper client10 (group1.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  apps.Add (client10.Install (group1.Get (0)));

  UdpClientHelper client11 (group1.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  apps.Add (client11.Install (group1.Get (1)));

  // create the applications for group 2
  UdpServerHelper server21 (port);
  apps.Add(server21.Install (group2.Get (1)));

  UdpClientHelper client20 (group2.Get (1)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  apps.Add (client20.Install (group2.Get (0)));

  UdpClientHelper client21 (group2.Get (0)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (), port);
  apps.Add (client21.Install (group2.Get (1)));

  // set the application start/end time
  apps.Start (MilliSeconds (startTime));
  apps.Stop (MilliSeconds (endTime));

  PrintGnuplottableNodeListToFile ("scenario.txt");

  Simulator::Stop (MilliSeconds (endTime + 1000));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

void
PrintGnuplottableNodeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  outFile << "set xrange [-200:200]; set yrange [-200:200]" << std::endl;
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<MmWaveVehicularNetDevice> vdev = node->GetDevice (j)->GetObject <MmWaveVehicularNetDevice> ();
          if (vdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << vdev->GetMac ()->GetRnti ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 7 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;

              // Simulator::Schedule (Seconds (1), &PrintHelper::UpdateGnuplottableNodeListToFile, filename, node);
            }
        }
    }
}
