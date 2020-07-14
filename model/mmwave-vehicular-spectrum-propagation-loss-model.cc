
/*
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York
*   University
*   Copyright (c) 2016, 2018, 2020 University of Padova, Dep. of Information
*   Engineering, SIGNET lab.
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

#include "mmwave-vehicular-spectrum-propagation-loss-model.h"
#include <ns3/log.h>
#include <ns3/math.h>
#include <ns3/simulator.h>
#include <ns3/node.h>
#include <ns3/double.h>
#include <algorithm>
#include <random>       // std::default_random_engine
#include <ns3/boolean.h>
#include <ns3/integer.h>

namespace ns3 {

namespace millicar {

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularSpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularSpectrumPropagationLossModel);

//Table 7.5-3: Ray offset angles within a cluster, given for rms angle spread normalized to 1.
static const double offSetAlpha[20] = {
  0.0447,-0.0447,0.1413,-0.1413,0.2492,-0.2492,0.3715,-0.3715,0.5129,-0.5129,0.6797,-0.6797,0.8844,-0.8844,1.1481,-1.1481,1.5195,-1.5195,2.1551,-2.1551
};

/*
 * The cross correlation matrix is constructed according to table 7.5-6.
 * All the square root matrix is being generated using the Cholesky decomposition
 * and following the order of [SF,K,DS,ASD,ASA,ZSD,ZSA].
 * The parameter K is ignored in NLOS.
 *
 * The Matlab file to generate the matrices can be found in the mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 * */

static const double sqrtC_UMi_LOS[7][7] = {
  {1, 0, 0, 0, 0, 0, 0},
  {0.5, 0.866025, 0, 0, 0, 0, 0},
  {-0.4, -0.57735, 0.711805, 0, 0, 0, 0},
  {-0.5, 0.057735, 0.468293, 0.726201, 0, 0, 0},
  {-0.4, -0.11547, 0.805464, -0.23482, 0.350363, 0, 0},
  {0, 0, 0, 0.688514, 0.461454, 0.559471, 0},
  {0, 0, 0.280976, 0.231921, -0.490509, 0.11916, 0.782603},
};

static const double sqrtC_UMi_NLOS[6][6] = {
  {1, 0, 0, 0, 0, 0},
  {-0.7, 0.714143, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0},
  {-0.4, 0.168034, 0, 0.90098, 0, 0},
  {0, -0.70014, 0.5, 0.130577, 0.4927, 0},
  {0, 0, 0.5, 0.221981, -0.566238, 0.616522},
};

static const double oxygen_loss[17][2] = {
  {52.0e9, 0.0},
  {53.0e9, 1.0},
  {54.0e9, 2.2},
  {55.0e9, 4.0},
  {56.0e9, 6.6},
  {57.0e9, 9.7},
  {58.0e9, 12.6},
  {59.0e9, 14.6},
  {60.0e9, 15.0},
  {61.0e9, 14.6},
  {62.0e9, 14.3},
  {63.0e9, 10.5},
  {64.0e9, 6.8},
  {65.0e9, 3.9},
  {66.0e9, 1.9},
  {67.0e9, 1.0},
  {68.0e9, 0.0}
};

MmWaveVehicularSpectrumPropagationLossModel::MmWaveVehicularSpectrumPropagationLossModel ()
{
  m_uniformRv = CreateObject<UniformRandomVariable> ();
  m_uniformRvBlockage = CreateObject<UniformRandomVariable> ();
  m_expRv = CreateObject<ExponentialRandomVariable> ();
  m_normalRv = CreateObject<NormalRandomVariable> ();
  m_normalRv->SetAttribute ("Mean", DoubleValue (0));
  m_normalRv->SetAttribute ("Variance", DoubleValue (1));
  m_normalRvBlockage = CreateObject<NormalRandomVariable> ();
  m_normalRvBlockage->SetAttribute ("Mean", DoubleValue (0));
  m_normalRvBlockage->SetAttribute ("Variance", DoubleValue (1));
}

TypeId
MmWaveVehicularSpectrumPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveVehicularSpectrumPropagationLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .AddConstructor<MmWaveVehicularSpectrumPropagationLossModel> ()
    .AddAttribute ("Frequency",
                   "Operating frequency in Hz",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&MmWaveVehicularSpectrumPropagationLossModel::SetFrequency,
                                       &MmWaveVehicularSpectrumPropagationLossModel::GetFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UpdatePeriod",
                   "Enable spatially-consistent UT mobility modeling procedure A, the update period unit is in ms, set to 0 ms to disable update",
                   TimeValue (MilliSeconds (1)),
                   MakeTimeAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_updatePeriod),
                   MakeTimeChecker ())
    .AddAttribute ("Blockage",
                   "Enable blockage model A (sec 7.6.4.1)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_blockage),
                   MakeBooleanChecker ())
    .AddAttribute ("NumNonselfBlocking",
                   "number of non-self-blocking regions",
                   IntegerValue (4),
                   MakeIntegerAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_numNonSelfBloking),
                   MakeIntegerChecker<uint16_t> ())
    .AddAttribute ("BlockerSpeed",
                   "The speed of moving blockers, the unit is m/s",
                   DoubleValue (1),
                   MakeDoubleAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_blockerSpeed),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PortraitMode",
                   "true for portrait mode, false for landscape mode",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_portraitMode),
                   MakeBooleanChecker ())
    .AddAttribute ("OxygenAbsorption",
                   "true for considering oxygen absortion, false for not considering it",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_oxygenAbsorption),
                   MakeBooleanChecker ())
    .AddAttribute ("O2I",
                   "Outdoor to Indoor channel state",
                   BooleanValue (false),
                   MakeBooleanAccessor (&MmWaveVehicularSpectrumPropagationLossModel::m_o2i),
                   MakeBooleanChecker ())
  ;
  return tid;
}

MmWaveVehicularSpectrumPropagationLossModel::~MmWaveVehicularSpectrumPropagationLossModel ()
{

}

void
MmWaveVehicularSpectrumPropagationLossModel::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveVehicularSpectrumPropagationLossModel::AddDevice (Ptr<NetDevice> dev, Ptr<MmWaveVehicularAntennaArrayModel> antenna)
{
  NS_ASSERT_MSG (m_deviceAntennaMap.find (dev) == m_deviceAntennaMap.end (), "Device is already present in the map");
  m_deviceAntennaMap.insert (std::pair <Ptr<NetDevice>, Ptr<MmWaveVehicularAntennaArrayModel>> (dev, antenna));
}

Ptr<SpectrumValue>
MmWaveVehicularSpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                 Ptr<const MobilityModel> a,
                                                 Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);

  // check if the frequency is correctly set
  NS_ASSERT_MSG (m_frequency != 0.0, "Set the operating frequency first!");

  Ptr<SpectrumValue> rxPsd = Copy (txPsd);

  Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);

  Vector locUT = b->GetPosition (); // TODO change this

  // retrieve the antenna of the tx device
  NS_ASSERT_MSG (m_deviceAntennaMap.find (txDevice) != m_deviceAntennaMap.end (), "Antenna not found for device " << txDevice);
  Ptr<MmWaveVehicularAntennaArrayModel> txAntennaArray = m_deviceAntennaMap.at (txDevice);
  NS_LOG_DEBUG ("tx dev " << txDevice << " antenna " << txAntennaArray);

  // retrieve the antenna of the rx device
  NS_ASSERT_MSG (m_deviceAntennaMap.find (txDevice) != m_deviceAntennaMap.end (), "Antenna not found for device " << rxDevice);
  Ptr<MmWaveVehicularAntennaArrayModel> rxAntennaArray = m_deviceAntennaMap.at (rxDevice);
  NS_LOG_DEBUG ("rx dev " << rxDevice << " antenna " << rxAntennaArray);

  /* txAntennaNum[0]-number of vertical antenna elements
   * txAntennaNum[1]-number of horizontal antenna elements*/
  // NOTE: only squared antenna arrays are currently supported
  uint16_t txAntennaNum[2];
  txAntennaNum[0] = sqrt (txAntennaArray->GetTotNoArrayElements ());
  txAntennaNum[1] = txAntennaNum[0];
  NS_LOG_DEBUG ("number of tx antenna elements " << txAntennaNum[0] << " x " << txAntennaNum[1]);

  uint16_t rxAntennaNum[2];
  rxAntennaNum[0] = sqrt (rxAntennaArray->GetTotNoArrayElements ());
  rxAntennaNum[1] = rxAntennaNum[0];
  NS_LOG_DEBUG ("number of rx antenna elements " << rxAntennaNum[0] << " x " << rxAntennaNum[1]);

  if (txAntennaArray->IsOmniTx () || rxAntennaArray->IsOmniTx () )
    {
      NS_LOG_LOGIC ("Omni transmission, do nothing.");
      return rxPsd;
    }

  NS_ASSERT_MSG (a->GetDistanceFrom (b) != 0, "The position of tx and rx devices cannot be the same");

  Vector rxSpeed = b->GetVelocity ();
  Vector txSpeed = a->GetVelocity ();
  Vector relativeSpeed (rxSpeed.x - txSpeed.x,rxSpeed.y - txSpeed.y,rxSpeed.z - txSpeed.z);

  key_t key = std::make_pair (txDevice,rxDevice);
  key_t keyReverse = std::make_pair (rxDevice,txDevice);

  std::map< key_t, Ptr<Params3gpp> >::iterator it = m_channelMap.find (key);
  std::map< key_t, Ptr<Params3gpp> >::iterator itReverse = m_channelMap.find (keyReverse);

  Ptr<Params3gpp> channelParams;

  //Step 2: Assign propagation condition (LOS/NLOS).

  char condition;
  if (DynamicCast<MmWaveVehicularPropagationLossModel> (m_3gppPathloss) != 0)
    {
      condition = m_3gppPathloss->GetObject<MmWaveVehicularPropagationLossModel> ()
        ->GetChannelCondition (a->GetObject<MobilityModel> (),b->GetObject<MobilityModel> ());
    }
  // else if (DynamicCast<MmWave3gppBuildingsPropagationLossModel> (m_3gppPathloss) != 0)
  //   {
  //     condition = m_3gppPathloss->GetObject<MmWave3gppBuildingsPropagationLossModel> ()
  //       ->GetChannelCondition (a->GetObject<MobilityModel> (),b->GetObject<MobilityModel> ());
  //   }
  else
    {
      NS_FATAL_ERROR ("unknown pathloss model");
    }

  bool o2i = m_o2i; // In the current implementation the O2I state is manually
                    // configured.

  //Every m_updatedPeriod, the channel matrix is deleted and a consistent channel update is triggered.
  //When there is a LOS/NLOS switch, a new uncorrelated channel is created.
  //Therefore, LOS/NLOS condition of updating is always consistent with the previous channel.

  //I only update the forward channel.
  if ((it == m_channelMap.end () && itReverse == m_channelMap.end ())
      || (it != m_channelMap.end () && it->second->m_channel.size () == 0)
      || (it != m_channelMap.end () && it->second->m_condition != condition)
      || (itReverse != m_channelMap.end () && itReverse->second->m_channel.size () == 0)
      || (itReverse != m_channelMap.end () && itReverse->second->m_condition != condition))
    {
      NS_LOG_INFO ("Update or create the forward channel");
      NS_LOG_LOGIC ("it == m_channelMap.end () " << (it == m_channelMap.end ()));
      NS_LOG_LOGIC ("itReverse == m_channelMap.end () " << (itReverse == m_channelMap.end ()));
      NS_LOG_LOGIC ("it->second->m_channel.size() == 0 " << (it->second->m_channel.size () == 0));
      NS_LOG_LOGIC ("it->second->m_condition != condition" << (it->second->m_condition != condition));

      //Step 1: The parameters are configured in the example code.
      /*make sure txAngle rxAngle exist, i.e., the position of tx and rx cannot be the same*/
      Angles txAngle (b->GetPosition (), a->GetPosition ());
      Angles rxAngle (a->GetPosition (), b->GetPosition ());
      NS_LOG_DEBUG ("txAngle  " << txAngle.phi << " " << txAngle.theta);
      NS_LOG_DEBUG ("rxAngle " << rxAngle.phi << " " << rxAngle.theta);

      txAngle.phi = txAngle.phi - txAntennaArray->GetOffset ();          //adjustment of the angles due to multi-sector consideration
      NS_LOG_DEBUG ("txAngle with offset PHI " << txAngle.phi);
      rxAngle.phi = rxAngle.phi - rxAntennaArray->GetOffset ();
      NS_LOG_DEBUG ("rxAngle with offset PHI " << rxAngle.phi);

      //Step 2: Assign propagation condition (LOS/NLOS).
      //los, o2i condition is computed above.

      //Step 3: The propagation loss is handled in the mmWavePropagationLossModel class.

      double x = a->GetPosition ().x - b->GetPosition ().x;
      double y = a->GetPosition ().y - b->GetPosition ().y;
      double distance2D = sqrt (x * x + y * y);
      double hTx = a->GetPosition ().z;
      double hRx = b->GetPosition ().z;

      //Draw parameters from table 7.5-6 and 7.5-7 to 7.5-10.
      Ptr<ParamsTable> table3gpp = Get3gppTable (condition, o2i, hTx, hRx, distance2D);

      // Step 4-11 are performed in function GetNewChannel()
      if ((it == m_channelMap.end () && itReverse == m_channelMap.end ())
          || (it != m_channelMap.end () && it->second->m_channel.size () == 0))
        {
          //delete the channel parameter to cause the channel to be updated again.
          //The m_updatePeriod can be configured to be relatively large in order to disable updates.
          if (m_updatePeriod.GetMilliSeconds () > 0)
            {
              NS_LOG_INFO ("Time " << Simulator::Now ().GetSeconds () << " schedule delete for a " << a->GetPosition () << " b " << b->GetPosition ()
                                   << " m_updatePeriod " << m_updatePeriod.GetSeconds ());
              Simulator::Schedule (m_updatePeriod, &MmWaveVehicularSpectrumPropagationLossModel::DeleteChannel,this,a,b);
            }
        }

      double distance3D = a->GetDistanceFrom (b);

      bool channelUpdate = false;
      if (it != m_channelMap.end () && it->second->m_channel.size () == 0)
        {
          //if the channel map is not empty, we only update the channel.
          NS_LOG_DEBUG ("Update forward channel consistently between MobilityModel " << a << " " << b);
          it->second->m_locUT = locUT;
          it->second->m_condition = condition;
          it->second->m_o2i = o2i;
          channelParams = UpdateChannel (it->second, table3gpp, txAntennaArray, rxAntennaArray,
                                         txAntennaNum, rxAntennaNum, rxAngle, txAngle);
          it->second->m_dis3D = distance3D;
          it->second->m_dis2D = distance2D;
          it->second->m_speed = relativeSpeed;
          it->second->m_generatedTime = Now ();
          it->second->m_preLocUT = locUT;
          channelUpdate = true;
        }
      else
        {
          //if the channel map is empty, we create a new channel.
          NS_LOG_INFO ("Create new channel");
          channelParams = GetNewChannel (table3gpp, locUT, condition, o2i, txAntennaArray, rxAntennaArray,
                                         txAntennaNum, rxAntennaNum, rxAngle, txAngle, relativeSpeed, distance2D, distance3D);
        }

      NS_LOG_DEBUG (" --- UPDATE BF VECTOR and LONGTERM vectors --- for new or update? " << channelUpdate);

      // insert the channelParams in the map
      m_channelMap[key] = channelParams;
    }
  else if (itReverse == m_channelMap.end ())                       // Find channel matrix in the forward link
    {
      channelParams = (*it).second;
      NS_LOG_DEBUG ("No need to update the channel");
    }
  else                       // Find channel matrix in the Reverse link
    {
      channelParams = (*itReverse).second;

      NS_LOG_DEBUG ("No need to update the channel");
    }

  // for now, store these BF vectors so that CalLongTerm can use them
  channelParams->m_txW = txAntennaArray->GetBeamformingVectorPanel ();
  channelParams->m_rxW = rxAntennaArray->GetBeamformingVectorPanel ();

  // call CalLongTerm, and get the longTerm params
  auto longTerm = CalLongTerm (channelParams);

  channelParams->m_longTerm = longTerm;

  Ptr<SpectrumValue> bfPsd = CalBeamformingGain (rxPsd, channelParams, longTerm, rxSpeed, txSpeed);

  SpectrumValue bfGain = (*bfPsd) / (*rxPsd);
  uint8_t nbands = bfGain.GetSpectrumModel ()->GetNumBands ();

  NS_LOG_DEBUG ("****** BF gain == " << Sum (bfGain) / nbands << " RX PSD " << Sum (*rxPsd) / nbands
                                        << " a pos " << a->GetPosition ()
                                        << " a antenna ID " << txAntennaArray->GetPlanesId ()
                                        << " b pos " << b->GetPosition ()
                                        << " b antenna ID " << rxAntennaArray->GetPlanesId ());
  return bfPsd;
}

Ptr<SpectrumValue>
MmWaveVehicularSpectrumPropagationLossModel::CalBeamformingGain (Ptr<const SpectrumValue> txPsd, Ptr<Params3gpp> params,
                                       complexVector_t longTerm, Vector rxSpeed, Vector txSpeed) const
{
  NS_LOG_FUNCTION (this);

  Ptr<SpectrumValue> tempPsd = Copy<SpectrumValue> (txPsd);

  //NS_ASSERT_MSG (params->m_delay.size()==params->m_channel.at(0).at(0).size(), "the cluster number of channel and delay spread should be the same");
  //NS_ASSERT_MSG (params->m_txW.size()==params->m_channel.at(0).size(), "the tx antenna size of channel and antenna weights should be the same");
  //NS_ASSERT_MSG (params->m_rxW.size()==params->m_channel.size(), "the rx antenna size of channel and antenna weights should be the same");
  //NS_ASSERT_MSG (params->m_angle.at(0).size()==params->m_channel.at(0).at(0).size(), "the cluster number of channel and AOA should be the same");
  //NS_ASSERT_MSG (params->m_angle.at(1).size()==params->m_channel.at(0).at(0).size(), "the cluster number of channel and ZOA should be the same");

  //channel[rx][tx][cluster]
  uint8_t numCluster = params->m_numCluster;
  //uint8_t txAntenna = params->m_txW.size();
  //uint8_t rxAntenna = params->m_rxW.size();
  //the update of Doppler is simplified by only taking the center angle of each cluster in to consideration.
  Values::iterator vit = tempPsd->ValuesBegin ();
  Bands::const_iterator sbit = tempPsd->ConstBandsBegin(); // sub band iterator

  double slotTime = Simulator::Now ().GetSeconds ();
  complexVector_t doppler;
  for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
    {

      double alpha = 0.0, D = 0.0, vScatt = 0.0, delayedPathsTerm = 0.0; // parameters used to evaluate Doppler effect in delayed paths as described in p. 32 of TR 37.885

      if(cIndex != 0)
        {
         if(m_scenario == "V2V-Highway" || m_scenario == "Extended-V2V-Highway")
         {
          vScatt = 140/3.6; // maximum speed in highway scenario, converted in m/s to be consistent with other speed measures
         }
         else if (m_scenario == "V2V-Urban" || m_scenario == "Extended-V2V-Urban")
         {
          vScatt = 60/3.6; // maximum speed in urban scenario, converted in m/s to be consistent with other speed measures
         }
         D = m_uniformRv->GetValue(-vScatt, vScatt);
         alpha = m_uniformRv->GetValue(0, 1);
         delayedPathsTerm = 2 * alpha * D;
        }

      //cluster angle angle[direction][n],where, direction = 0(aoa), 1(zoa).
      //
      double temp_doppler = 2 * M_PI * ((sin (params->m_angle.at (ZOA_INDEX).at (cIndex) * M_PI / 180) * cos (params->m_angle.at (AOA_INDEX).at (cIndex) * M_PI / 180) * rxSpeed.x
                                        + sin (params->m_angle.at (ZOA_INDEX).at (cIndex) * M_PI / 180) * sin (params->m_angle.at (AOA_INDEX).at (cIndex) * M_PI / 180) * rxSpeed.y
                                        + cos (params->m_angle.at (ZOA_INDEX).at (cIndex) * M_PI / 180) * rxSpeed.z)
                                        + (sin (params->m_angle.at (ZOD_INDEX).at (cIndex) * M_PI / 180) * cos (params->m_angle.at (AOD_INDEX).at (cIndex) * M_PI / 180) * txSpeed.x
                                        + sin (params->m_angle.at (ZOD_INDEX).at (cIndex) * M_PI / 180) * sin (params->m_angle.at (AOD_INDEX).at (cIndex) * M_PI / 180) * txSpeed.y
                                        + cos (params->m_angle.at (ZOD_INDEX).at (cIndex) * M_PI / 180) * txSpeed.z) + delayedPathsTerm)
                                        * slotTime * m_frequency / 3e8;
      doppler.push_back (exp (std::complex<double> (0, temp_doppler)));

    }

  while (vit != tempPsd->ValuesEnd ())
    {
      std::complex<double> subsbandGain (0.0,0.0);
      if ((*vit) != 0.00)
        {
          double fsb = (*sbit).fc;
          for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
            {
              double delay = -2 * M_PI * fsb * (params->m_delay.at (cIndex));
              double tauDelta = 0.0;

              if(cIndex != 0)
              {
                tauDelta = params->m_tauDelta; // when in LOS condition, tau_{\Delta} is equal to zero.
              }

              if(!m_oxygenAbsorption)
              {
                subsbandGain = subsbandGain + longTerm.at (cIndex) * doppler.at (cIndex) * exp (std::complex<double> (0, delay));
              }
              else
              {
                subsbandGain = subsbandGain + longTerm.at (cIndex) * doppler.at (cIndex) * exp (std::complex<double> (0, delay)) / GetOxygenLoss(fsb, params->m_dis3D, params->m_delay.at (cIndex), tauDelta);
              }

            }
          *vit = (*vit) * (norm (subsbandGain));
        }
      vit++;
      sbit++;
    }
  return tempPsd;
}


double
MmWaveVehicularSpectrumPropagationLossModel::GetOxygenLoss (double f, double dist3D, double tau, double tauDelta) const
{
  NS_LOG_FUNCTION (this << f << dist3D << tau << tauDelta);
  double alpha = 0.0, loss = 0.0;

  if(f > oxygen_loss[0][0] && f < oxygen_loss[16][0])
  {
    for (uint8_t idx = 1; idx <= 15; idx++)
    {
      if ( f > oxygen_loss[idx-1][0] && f <= oxygen_loss[idx][0] )
      {
        // interpolation of the oxygen_loss table
        alpha = (oxygen_loss[idx][1] - oxygen_loss[idx-1][1])/(oxygen_loss[idx][0] - oxygen_loss[idx-1][0])*(f - oxygen_loss[idx-1][0]) + oxygen_loss[idx-1][1];
        loss = alpha / 1e3 * (dist3D + 3e8 * (tau + tauDelta));
        NS_LOG_DEBUG ("f (subband) " << f << " alpha " << alpha << " dB/km loss " << loss << " dB");
      }
    }
  }

  return pow(10.0, loss/10); // need to obtain the linear term, since in TR 38.901 the formula is in dB

}

void
MmWaveVehicularSpectrumPropagationLossModel::SetPathlossModel (Ptr<PropagationLossModel> pathloss)
{
  m_3gppPathloss = pathloss;
  if (DynamicCast<MmWaveVehicularPropagationLossModel> (m_3gppPathloss) != 0)
    {
      m_scenario = m_3gppPathloss->GetObject<MmWaveVehicularPropagationLossModel> ()->GetScenario ();
    }
  // else if (DynamicCast<MmWave3gppBuildingsPropagationLossModel> (m_3gppPathloss) != 0)
  //   {
  //     m_scenario = m_3gppPathloss->GetObject<MmWave3gppBuildingsPropagationLossModel> ()->GetScenario ();
  //   }
  else
    {
      NS_FATAL_ERROR ("unknown pathloss model");
    }
}


complexVector_t
MmWaveVehicularSpectrumPropagationLossModel::CalLongTerm (Ptr<Params3gpp> params) const
{
  uint16_t txAntenna = params->m_txW.size ();
  uint16_t rxAntenna = params->m_rxW.size ();

  NS_LOG_DEBUG ("CalLongTerm with txAntenna " << (uint16_t)txAntenna << " rxAntenna " << (uint16_t)rxAntenna);
  //store the long term part to reduce computation load
  //only the small scale fading is need to be updated if the large scale parameters and antenna weights remain unchanged.
  complexVector_t longTerm;
  uint8_t numCluster = params->m_numCluster;

  for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
    {
      std::complex<double> txSum (0,0);
      for (uint16_t txIndex = 0; txIndex < txAntenna; txIndex++)
        {
          std::complex<double> rxSum (0,0);
          for (uint16_t rxIndex = 0; rxIndex < rxAntenna; rxIndex++)
            {
              rxSum = rxSum + params->m_rxW.at (rxIndex) * params->m_channel.at (rxIndex).at (txIndex).at (cIndex);
            }
          txSum = txSum + params->m_txW.at (txIndex) * rxSum;
        }
      longTerm.push_back (txSum);
    }
  return longTerm;

}

Ptr<ParamsTable>
MmWaveVehicularSpectrumPropagationLossModel::Get3gppTable (char condition, bool o2i, double hBS, double hUT, double distance2D) const
{
  double fcGHz = m_frequency / 1e9;
  Ptr<ParamsTable> table3gpp = CreateObject<ParamsTable> ();
  // table3gpp includes the following parameters:
  // numOfCluster, raysPerCluster, uLgDS, sigLgDS, uLgASD, sigLgASD,
  // uLgASA, sigLgASA, uLgZSA, sigLgZSA, uLgZSD, sigLgZSD, offsetZOD,
  // cDS, cASD, cASA, cZSA, uK, sigK, rTau, shadowingStd

  //In NLOS case, parameter uK and sigK are not used and 0 is passed into the SetParams() function.
  if (m_scenario == "V2V-Urban" || m_scenario == "Extended-V2V-Urban")
    {
      if (condition == 'l')
        {
          table3gpp->SetParams (12, // number of clusters
                                20, // number of rays per cluster
                                -0.2 * log10 (1 + fcGHz) - 7.5, // mu DS
                                0.1, // sigma DS
                                -0.1 * log10 (1 + fcGHz) + 1.6, // AOD spread mu
                                0.1, // AOD spread sigma
                                -0.1 * log10 (1 + fcGHz) + 1.6, // AOA spread mu
                                0.1, // AOA spread sigma
                                -0.1 * log10 (1 + fcGHz) + 0.73, // ZOA spread mu
                                -0.04 * log10 (1 + fcGHz) + 0.34, // ZOA spread sigma
                                -0.1 * log10 (1 + fcGHz) + 0.73, // ZOD spread mu
                                -0.04 * log10 (1 + fcGHz) + 0.34, // ZOD spread sigma
                                0, // ZOD offset
                                5, // cluster DS
                                17, // cluster ASD
                                17, // cluster ASA
                                7, // cluster ZSA
                                3.48, // K-factor mu
                                2, // K-factor sigma
                                3, // delay scaling parameter
                                4); // per cluster shadowing std

          for (uint8_t row = 0; row < 7; row++)
            {
              for (uint8_t column = 0; column < 7; column++)
                {
                  table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
      else if (condition == 'n')
        {
          table3gpp->SetParams (19, // number of clusters
                                20, // number of rays per cluster
                                -0.3 * log10 (1 + fcGHz) - 7, // mu DS
                                0.28, // sigma DS
                                -0.08 * log10 (1 + fcGHz) + 1.81, // AOD spread mu
                                0.05 * log10 (1 + fcGHz) + 0.3, // AOD spread sigma
                                -0.08 * log10 (1 + fcGHz) + 1.81, // AOA spread mu
                                0.05 * log10 (1 + fcGHz) + 0.3, // AOA spread sigma
                                -0.04 * log10 (1 + fcGHz) + 0.92, // ZOA spread mu
                                -0.07 * log10 (1 + fcGHz) + 0.41, // ZOA spread sigma
                                -0.04 * log10 (1 + fcGHz) + 0.92, // ZOD spread mu
                                -0.07 * log10 (1 + fcGHz) + 0.41, // ZOD spread sigma
                                0, // ZOD offset
                                11, // cluster DS
                                22, // cluster ASD
                                22, // cluster ASA
                                7, // cluster ZSA
                                0, // K-factor mu NOTE this value is NA
                                0, // K-factor sigma NOTE this value is NA
                                2.1, // delay scaling parameter
                                4); // per cluster shadowing std

          for (uint8_t row = 0; row < 6; row++)
            {
              for (uint8_t column = 0; column < 6; column++)
                {
                  table3gpp->m_sqrtC[row][column] = sqrtC_UMi_NLOS[row][column];
                }
            }
        }
      else if (condition == 'v')
        {
          table3gpp->SetParams (19, // number of clusters
                                20, // number of rays per cluster
                                -0.4 * log10 (1 + fcGHz) - 7, // mu DS
                                0.1, // sigma DS
                                -0.1 * log10 (1 + fcGHz) + 1.7, // AOD spread mu
                                0.1, // AOD spread sigma
                                -0.1 * log10 (1 + fcGHz) + 1.7, // AOA spread mu
                                0.1, // AOA spread sigma
                                -0.04 * log10 (1 + fcGHz) + 0.92, // ZOA spread mu
                                -0.07 * log10 (1 + fcGHz) + 0.41, // ZOA spread sigma
                                -0.04 * log10 (1 + fcGHz) + 0.92, // ZOD spread mu
                                -0.07 * log10 (1 + fcGHz) + 0.41, // ZOD spread sigma
                                0, // ZOD offset
                                11, // cluster DS
                                22, // cluster ASD
                                22, // cluster ASA
                                7, // cluster ZSA
                                0, // K-factor mu
                                4.5, // K-factor sigma
                                2.1, // delay scaling parameter
                                4); // per cluster shadowing std

          for (uint8_t row = 0; row < 7; row++)
            {
              for (uint8_t column = 0; column < 7; column++)
                {
                  table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
      else
        {
          NS_FATAL_ERROR ("Unknown channel condition");
        }
    }
  else if (m_scenario == "V2V-Highway" || m_scenario == "Extended-V2V-Highway")
  {
    if (condition == 'l')
      {
        table3gpp->SetParams (12, // number of clusters
                              20, // number of rays per cluster
                              -8.3, // mu DS
                              0.2, // sigma DS
                              1.4, // AOD spread mu
                              0.1, // AOD spread sigma
                              1.4, // AOA spread mu
                              0.1, // AOA spread sigma
                              -0.1 * log10 (1 + fcGHz) + 0.73, // ZOA spread mu
                              -0.04 * log10 (1 + fcGHz) + 0.34, // ZOA spread sigma
                              -0.1 * log10 (1 + fcGHz) + 0.73, // ZOD spread mu
                              -0.04 * log10 (1 + fcGHz) + 0.34, // ZOD spread sigma
                              0, // ZOD offset
                              5, // cluster DS
                              17, // cluster ASD
                              17, // cluster ASA
                              7, // cluster ZSA
                              9, // K-factor mu
                              3.5, // K-factor sigma
                              3, // delay scaling parameter
                              4); // per cluster shadowing std

        for (uint8_t row = 0; row < 7; row++)
          {
            for (uint8_t column = 0; column < 7; column++)
              {
                table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
              }
          }
      }
    else if (condition == 'v')
      {
        table3gpp->SetParams (19, // number of clusters
                              20, // number of rays per cluster
                              -8.3, // mu DS
                              0.3, // sigma DS
                              1.5, // AOD spread mu
                              0.1, // AOD spread sigma
                              1.5, // AOA spread mu
                              0.1, // AOA spread sigma
                              -0.04 * log10 (1 + fcGHz) + 0.92, // ZOA spread mu
                              -0.07 * log10 (1 + fcGHz) + 0.41, // ZOA spread sigma
                              -0.04 * log10 (1 + fcGHz) + 0.92, // ZOD spread mu
                              -0.07 * log10 (1 + fcGHz) + 0.41, // ZOD spread sigma
                              0, // ZOD offset
                              11, // cluster DS
                              22, // cluster ASD
                              22, // cluster ASA
                              7, // cluster ZSA
                              0, // K-factor mu
                              4.5, // K-factor sigma
                              2.1, // delay scaling parameter
                              4); // per cluster shadowing std

        for (uint8_t row = 0; row < 7; row++)
          {
            for (uint8_t column = 0; column < 7; column++)
              {
                table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
              }
          }
      }
    else if (condition == 'n')
      {
        NS_LOG_WARN ("The fast fading parameters for the NLOS condition in the (Extended)-V2V-Highway scenario are not defined in TR 37.885, use the ones defined in TDoc R1-1803671 instead");
        table3gpp->SetParams (19, // number of clusters
                              20, // number of rays per cluster
                              -7.66, // mu DS
                              -7.62, // sigma DS
                              1.32, // AOD spread mu
                              0.77, // AOD spread sigma
                              1.32, // AOA spread mu
                              0.77, // AOA spread sigma
                              -0.04 * log10 (1 + fcGHz) + 0.92, // ZOA spread mu
                              -0.07 * log10 (1 + fcGHz) + 0.41, // ZOA spread sigma
                              0, // ZOD spread mu NOTE this is not defined in the TDoc
                              0, // ZOD spread sigma NOTE this is not defined in the TDoc
                              0, // ZOD offset
                              0, // cluster DS NOTE this is not defined in the TDoc
                              10, // cluster ASD
                              22, // cluster ASA
                              7, // cluster ZSA
                              0, // K-factor mu NOTE this is not defined in the TDoc
                              0, // K-factor sigma NOTE this is not defined in the TDoc
                              2.1, // delay scaling parameter
                              4); // per cluster shadowing std

        for (uint8_t row = 0; row < 6; row++)
          {
            for (uint8_t column = 0; column < 6; column++)
              {
                table3gpp->m_sqrtC[row][column] = sqrtC_UMi_NLOS[row][column];
              }
          }
      }
    else
      {
        NS_FATAL_ERROR ("Unknown channel condition");
      }

  }
  else
    {
      //Note that the InH-ShoppingMall scenario is not given in the table 7.5-6
      NS_FATAL_ERROR ("unkonw scenarios");
    }

  return table3gpp;

}

void
MmWaveVehicularSpectrumPropagationLossModel::DeleteChannel (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  Ptr<NetDevice> dev1 = a->GetObject<Node> ()->GetDevice (0);
  Ptr<NetDevice> dev2 = b->GetObject<Node> ()->GetDevice (0);
  NS_LOG_INFO ("a position " << a->GetPosition () << " b " << b->GetPosition ());
  Ptr<Params3gpp> params = m_channelMap.find (std::make_pair (dev1,dev2))->second;
  NS_LOG_INFO ("params " << params);
  NS_LOG_INFO ("params m_channel size" << params->m_channel.size ());
  NS_ASSERT_MSG (m_channelMap.find (std::make_pair (dev1,dev2)) != m_channelMap.end (), "Channel not found");
  params->m_channel.clear ();
  m_channelMap[std::make_pair (dev1,dev2)] = params;
}

Ptr<Params3gpp>
MmWaveVehicularSpectrumPropagationLossModel::GetNewChannel (Ptr<ParamsTable>  table3gpp, Vector locUT, char condition, bool o2i,
                                  Ptr<MmWaveVehicularAntennaArrayModel> txAntenna, Ptr<MmWaveVehicularAntennaArrayModel> rxAntenna,
                                  uint16_t *txAntennaNum, uint16_t *rxAntennaNum,  Angles &rxAngle, Angles &txAngle,
                                  Vector speed, double dis2D, double dis3D) const
{
  uint8_t numOfCluster = table3gpp->m_numOfCluster;
  uint8_t raysPerCluster = table3gpp->m_raysPerCluster;
  Ptr<Params3gpp> channelParams = Create<Params3gpp> ();
  //for new channel, the previous and current location is the same.
  channelParams->m_preLocUT = locUT;
  channelParams->m_locUT = locUT;
  channelParams->m_condition = condition;
  channelParams->m_o2i = o2i;
  channelParams->m_generatedTime = Now ();
  channelParams->m_speed = speed;
  channelParams->m_dis2D = dis2D;
  channelParams->m_dis3D = dis3D;
  //Step 4: Generate large scale parameters. All LSPS are uncorrelated.
  doubleVector_t LSPsIndep, LSPs;
  uint8_t paramNum;
  if (condition == 'l')
    {
      paramNum = 7;
    }
  else
    {
      paramNum = 6;
    }
  //Generate paramNum independent LSPs.
  for (uint8_t iter = 0; iter < paramNum; iter++)
    {
      LSPsIndep.push_back (m_normalRv->GetValue ());
    }
  for (uint8_t row = 0; row < paramNum; row++)
    {
      double temp = 0;
      for (uint8_t column = 0; column < paramNum; column++)
        {
          temp += table3gpp->m_sqrtC[row][column] * LSPsIndep.at (column);
        }
      LSPs.push_back (temp);
    }

  /*std::cout << "LSPsIndep:";
  for (uint8_t i = 0; i < paramNum; i++)
  {
          std::cout <<LSPsIndep.at(i)<<"\t";
  }
  std::cout << "\n";
  std::cout << "LSPs:";
  for (uint8_t i = 0; i < paramNum; i++)
  {
          std::cout <<LSPs.at(i)<<"\t";
  }
  std::cout << "\n";*/

  /* Notice the shadowing is updated much frequently (every transmission),
   * therefore it is generated separately in the 3GPP propagation loss model.*/

  double DS,ASD,ASA,ZSA,ZSD,K_factor = 0;
  if (condition == 'l')
    {
      K_factor = LSPs.at (1) * table3gpp->m_sigK + table3gpp->m_uK;
      DS = pow (10, LSPs.at (2) * table3gpp->m_sigLgDS + table3gpp->m_uLgDS);
      ASD = pow (10, LSPs.at (3) * table3gpp->m_sigLgASD + table3gpp->m_uLgASD);
      ASA = pow (10, LSPs.at (4) * table3gpp->m_sigLgASA + table3gpp->m_uLgASA);
      ZSD = pow (10, LSPs.at (5) * table3gpp->m_sigLgZSD + table3gpp->m_uLgZSD);
      ZSA = pow (10, LSPs.at (6) * table3gpp->m_sigLgZSA + table3gpp->m_uLgZSA);
    }
  else
    {
      DS = pow (10, LSPs.at (1) * table3gpp->m_sigLgDS + table3gpp->m_uLgDS);
      ASD = pow (10, LSPs.at (2) * table3gpp->m_sigLgASD + table3gpp->m_uLgASD);
      ASA = pow (10, LSPs.at (3) * table3gpp->m_sigLgASA + table3gpp->m_uLgASA);
      ZSD = pow (10, LSPs.at (4) * table3gpp->m_sigLgZSD + table3gpp->m_uLgZSD);
      ZSA = pow (10, LSPs.at (5) * table3gpp->m_sigLgZSA + table3gpp->m_uLgZSA);

    }
  ASD = std::min (ASD, 104.0);
  ASA = std::min (ASA, 104.0);
  ZSD = std::min (ZSD, 52.0);
  ZSA = std::min (ZSA, 52.0);

  channelParams->m_DS = DS;
  channelParams->m_K = K_factor;

  NS_LOG_INFO ("K-factor=" << K_factor << ",DS=" << DS << ", ASD=" << ASD << ", ASA=" << ASA << ", ZSD=" << ZSD << ", ZSA=" << ZSA);

  //Step 5: Generate Delays.
  doubleVector_t clusterDelay;
  double minTau = 100.0;
  for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
    {
      double tau = -1*table3gpp->m_rTau*DS*log (m_uniformRv->GetValue (0,1));         //(7.5-1)
      if (minTau > tau)
        {
          minTau = tau;
        }
      clusterDelay.push_back (tau);
    }
  channelParams->m_tauDelta = minTau;
  for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
    {
      clusterDelay.at (cIndex) -= minTau;
    }
  std::sort (clusterDelay.begin (), clusterDelay.end ());       //(7.5-2)

  /* since the scaled Los delays are not to be used in cluster power generation,
   * we will generate cluster power first and resume to compute Los cluster delay later.*/

  //Step 6: Generate cluster powers.
  doubleVector_t clusterPower;
  double powerSum = 0;
  for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
    {
      double power = exp (-1 * clusterDelay.at (cIndex) * (table3gpp->m_rTau - 1) / table3gpp->m_rTau / DS) *
        pow (10,-1 * m_normalRv->GetValue () * table3gpp->m_shadowingStd / 10);                       //(7.5-5)
      powerSum += power;
      clusterPower.push_back (power);
    }
  double powerMax = 0;

  for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
    {
      clusterPower.at (cIndex) = clusterPower.at (cIndex) / powerSum;         //(7.5-6)
    }

  doubleVector_t clusterPowerForAngles;       // this power is only for equation (7.5-9) and (7.5-14), not for (7.5-22)
  if (condition == 'l')
    {
      double K_linear = pow (10,K_factor / 10);

      for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
        {
          if (cIndex == 0)
            {
              clusterPowerForAngles.push_back (clusterPower.at (cIndex) / (1 + K_linear) + K_linear / (1 + K_linear));                  //(7.5-8)
            }
          else
            {
              clusterPowerForAngles.push_back (clusterPower.at (cIndex) / (1 + K_linear));                  //(7.5-8)
            }
          if (powerMax < clusterPowerForAngles.at (cIndex))
            {
              powerMax = clusterPowerForAngles.at (cIndex);
            }
        }
    }
  else
    {
      for (uint8_t cIndex = 0; cIndex < numOfCluster; cIndex++)
        {
          clusterPowerForAngles.push_back (clusterPower.at (cIndex));              //(7.5-6)
          if (powerMax < clusterPowerForAngles.at (cIndex))
            {
              powerMax = clusterPowerForAngles.at (cIndex);
            }
        }
    }

  //remove clusters with less than -25 dB power compared to the maxim cluster power;
  //double thresh = pow(10,-2.5);
  double thresh = 0.0032;
  for (uint8_t cIndex = numOfCluster; cIndex > 0; cIndex--)
    {
      if (clusterPowerForAngles.at (cIndex - 1) < thresh * powerMax )
        {
          clusterPowerForAngles.erase (clusterPowerForAngles.begin () + cIndex - 1);
          clusterPower.erase (clusterPower.begin () + cIndex - 1);
          clusterDelay.erase (clusterDelay.begin () + cIndex - 1);
        }
    }
  uint8_t numReducedCluster = clusterPower.size ();

  channelParams->m_numCluster = numReducedCluster;
  // Resume step 5 to compute the delay for LoS condition.
  if (condition == 'l')
    {
      double C_tau = 0.7705 - 0.0433 * K_factor + 2e-4 * pow (K_factor,2) + 17e-6 * pow (K_factor,3);         //(7.5-3)
      for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
        {
          clusterDelay.at (cIndex) = clusterDelay.at (cIndex) / C_tau;             //(7.5-4)
        }
    }

  /*for (uint8_t i = 0; i < clusterPowerForAngles.size(); i++)
  {
          std::cout <<clusterPowerForAngles.at(i)<<"s\t";
  }
  std::cout << "\n";*/

  /*std::cout << "Delay:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterDelay.at(i)<<"s\t";
  }
  std::cout << "\n";
  std::cout << "Power:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterPower.at(i)<<"\t";
  }
  std::cout << "\n";*/

  //step 7: Generate arrival and departure angles for both azimuth and elevation.

  double C_NLOS, C_phi;
  //According to table 7.5-6, only cluster number equals to 8, 10, 11, 12, 19 and 20 is valid.
  //Not sure why the other cases are in Table 7.5-2.
  switch (numOfCluster)       // Table 7.5-2
    {
    case 4:
      C_NLOS = 0.779;
      break;
    case 5:
      C_NLOS = 0.860;
      break;
    case 8:
      C_NLOS = 1.018;
      break;
    case 10:
      C_NLOS = 1.090;
      break;
    case 11:
      C_NLOS = 1.123;
      break;
    case 12:
      C_NLOS = 1.146;
      break;
    case 14:
      C_NLOS = 1.190;
      break;
    case 15:
      C_NLOS = 1.221;
      break;
    case 16:
      C_NLOS = 1.226;
      break;
    case 19:
      C_NLOS = 1.273;
      break;
    case 20:
      C_NLOS = 1.289;
      break;
    default:
      NS_FATAL_ERROR ("Invalide cluster number");
    }

  if (condition == 'l')
    {
      C_phi = C_NLOS * (1.1035 - 0.028 * K_factor - 2e-3 * pow (K_factor,2) + 1e-4 * pow (K_factor,3));         //(7.5-10))
    }
  else
    {
      C_phi = C_NLOS;           //(7.5-10))
    }

  double C_theta;
  switch (numOfCluster)       //Table 7.5-4
    {
    case 8:
      C_NLOS = 0.889;
      break;
    case 10:
      C_NLOS = 0.957;
      break;
    case 11:
      C_NLOS = 1.031;
      break;
    case 12:
      C_NLOS = 1.104;
      break;
    case 19:
      C_NLOS = 1.184;
      break;
    case 20:
      C_NLOS = 1.178;
      break;
    default:
      NS_FATAL_ERROR ("Invalide cluster number");
    }

  if (condition == 'l')
    {
      C_theta = C_NLOS * (1.3086 + 0.0339 * K_factor - 0.0077 * pow (K_factor,2) + 2e-4 * pow (K_factor,3));         //(7.5-15)
    }
  else
    {
      C_theta = C_NLOS;
    }


  doubleVector_t clusterAoa, clusterAod, clusterZoa, clusterZod;
  double angle;
  for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
    {
      angle = 2*ASA*sqrt (-1 * log (clusterPowerForAngles.at (cIndex) / powerMax)) / 1.4 / C_phi;        //(7.5-9)
      clusterAoa.push_back (angle);
      angle = 2*ASD*sqrt (-1 * log (clusterPowerForAngles.at (cIndex) / powerMax)) / 1.4 / C_phi;        //(7.5-9)
      clusterAod.push_back (angle);
      angle = -1*ZSA*log (clusterPowerForAngles.at (cIndex) / powerMax) / C_theta;         //(7.5-14)
      clusterZoa.push_back (angle);
      angle = -1*ZSD*log (clusterPowerForAngles.at (cIndex) / powerMax) / C_theta;
      clusterZod.push_back (angle);
    }

  for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
    {
      int Xn = 1;
      if (m_uniformRv->GetValue (0,1) < 0.5)
        {
          Xn = -1;
        }
      clusterAoa.at (cIndex) = clusterAoa.at (cIndex) * Xn + (m_normalRv->GetValue () * ASA / 7) + rxAngle.phi * 180 / M_PI;        //(7.5-11)
      clusterAod.at (cIndex) = clusterAod.at (cIndex) * Xn + (m_normalRv->GetValue () * ASD / 7) + txAngle.phi * 180 / M_PI;
      if (o2i)
        {
          clusterZoa.at (cIndex) = clusterZoa.at (cIndex) * Xn + (m_normalRv->GetValue () * ZSA / 7) + 90;            //(7.5-16)
        }
      else
        {
          clusterZoa.at (cIndex) = clusterZoa.at (cIndex) * Xn + (m_normalRv->GetValue () * ZSA / 7) + rxAngle.theta * 180 / M_PI;            //(7.5-16)
        }
      clusterZod.at (cIndex) = clusterZod.at (cIndex) * Xn + (m_normalRv->GetValue () * ZSD / 7) + txAngle.theta * 180 / M_PI + table3gpp->m_offsetZOD;        //(7.5-19)

    }

  if (condition == 'l')
    {
      //The 7.5-12 can be rewrite as Theta_n,ZOA = Theta_n,ZOA - (Theta_1,ZOA - Theta_LOS,ZOA) = Theta_n,ZOA - diffZOA,
      //Similar as AOD, ZSA and ZSD.
      double diffAoa = clusterAoa.at (0) - rxAngle.phi * 180 / M_PI;
      double diffAod = clusterAod.at (0) - txAngle.phi * 180 / M_PI;
      double diffZsa = clusterZoa.at (0) - rxAngle.theta * 180 / M_PI;
      double diffZsd = clusterZod.at (0) - txAngle.theta * 180 / M_PI;

      for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
        {
          clusterAoa.at (cIndex) -= diffAoa;              //(7.5-12)
          clusterAod.at (cIndex) -= diffAod;
          clusterZoa.at (cIndex) -= diffZsa;              //(7.5-17)
          clusterZod.at (cIndex) -= diffZsd;

        }
    }

  double rayAoa_radian[numReducedCluster][raysPerCluster];       //rayAoa_radian[n][m], where n is cluster index, m is ray index
  double rayAod_radian[numReducedCluster][raysPerCluster];       //rayAod_radian[n][m], where n is cluster index, m is ray index
  double rayZoa_radian[numReducedCluster][raysPerCluster];       //rayZoa_radian[n][m], where n is cluster index, m is ray index
  double rayZod_radian[numReducedCluster][raysPerCluster];       //rayZod_radian[n][m], where n is cluster index, m is ray index

  for (uint8_t nInd = 0; nInd < numReducedCluster; nInd++)
    {
      for (uint8_t mInd = 0; mInd < raysPerCluster; mInd++)
        {
          double tempAoa = clusterAoa.at (nInd) + table3gpp->m_cASA * offSetAlpha[mInd];              //(7.5-13)
          while (tempAoa > 360)
            {
              tempAoa -= 360;
            }

          while (tempAoa < 0)
            {
              tempAoa += 360;

            }
          NS_ASSERT_MSG (tempAoa >= 0 && tempAoa <= 360, "the AOA should be the range of [0,360]");
          rayAoa_radian[nInd][mInd] = tempAoa * M_PI / 180;

          double tempAod = clusterAod.at (nInd) + table3gpp->m_cASD * offSetAlpha[mInd];
          while (tempAod > 360)
            {
              tempAod -= 360;
            }

          while (tempAod < 0)
            {
              tempAod += 360;
            }
          NS_ASSERT_MSG (tempAod >= 0 && tempAod <= 360, "the AOD should be the range of [0,360]");
          rayAod_radian[nInd][mInd] = tempAod * M_PI / 180;

          double tempZoa = clusterZoa.at (nInd) + table3gpp->m_cZSA * offSetAlpha[mInd];              //(7.5-18)

          while (tempZoa > 360)
            {
              tempZoa -= 360;
            }

          while (tempZoa < 0)
            {
              tempZoa += 360;
            }

          if (tempZoa > 180)
            {
              tempZoa = 360 - tempZoa;
            }

          NS_ASSERT_MSG (tempZoa >= 0&&tempZoa <= 180, "the ZOA should be the range of [0,180]");
          rayZoa_radian[nInd][mInd] = tempZoa * M_PI / 180;

          double tempZod = clusterZod.at (nInd) + 0.375 * pow (10,table3gpp->m_uLgZSD) * offSetAlpha[mInd];             //(7.5-20)

          while (tempZod > 360)
            {
              tempZod -= 360;
            }

          while (tempZod < 0)
            {
              tempZod += 360;
            }
          if (tempZod > 180)
            {
              tempZod = 360 - tempZod;
            }
          NS_ASSERT_MSG (tempZod >= 0&&tempZod <= 180, "the ZOD should be the range of [0,180]");
          rayZod_radian[nInd][mInd] = tempZod * M_PI / 180;
        }
    }
  doubleVector_t angle_degree;
  double sizeTemp = clusterZoa.size ();
  for (uint8_t ind = 0; ind < 4; ind++)
    {
      switch (ind)
        {
        case 0:
          angle_degree = clusterAoa;
          break;
        case 1:
          angle_degree = clusterZoa;
          break;
        case 2:
          angle_degree = clusterAod;
          break;
        case 3:
          angle_degree = clusterZod;
          break;
        default:
          NS_FATAL_ERROR ("Programming Error");
        }

      for (uint8_t nIndex = 0; nIndex < sizeTemp; nIndex++)
        {
          while (angle_degree[nIndex] > 360)
            {
              angle_degree[nIndex] -= 360;
            }

          while (angle_degree[nIndex] < 0)
            {
              angle_degree[nIndex] += 360;
            }

          if (ind == 1 || ind == 3)
            {
              if (angle_degree[nIndex] > 180)
                {
                  angle_degree[nIndex] = 360 - angle_degree[nIndex];
                }
            }
        }
      switch (ind)
        {
        case 0:
          clusterAoa = angle_degree;
          break;
        case 1:
          clusterZoa = angle_degree;
          break;
        case 2:
          clusterAod = angle_degree;
          break;
        case 3:
          clusterZod = angle_degree;
          break;
        default:
          NS_FATAL_ERROR ("Programming Error");
        }
    }

  doubleVector_t attenuation_dB;
  if (m_blockage)
    {
      attenuation_dB = CalAttenuationOfBlockage (channelParams, clusterAoa, clusterZoa);
      for (uint8_t cInd = 0; cInd < numReducedCluster; cInd++)
        {
          clusterPower.at (cInd) = clusterPower.at (cInd) / pow (10,attenuation_dB.at (cInd) / 10);
        }
    }
  else
    {
      attenuation_dB.push_back (0);
    }

  /*std::cout << "BlockedPower:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterPower.at(i)<<"\t";
  }
  std::cout << "\n";

  std::cout << "AOD:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterAod.at(i)<<"'\t";
  }
  std::cout << "\n";

  std::cout << "AOA:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterAoa.at(i)<<"'\t";
  }
  std::cout << "\n";

  std::cout << "ZOD:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterZod.at(i)<<"'\t";
          for (uint8_t d = 0; d < raysPerCluster; d++)
          {
                  std::cout <<rayZod_radian[i][d]<<"\t";
          }
          std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "ZOA:";
  for (uint8_t i = 0; i < numReducedCluster; i++)
  {
          std::cout <<clusterZoa.at(i)<<"'\t";
  }
  std::cout << "\n";*/


  //Step 8: Coupling of rays within a cluster for both azimuth and elevation
  //shuffle all the arrays to perform random coupling
  for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
    {
      std::shuffle (&rayAod_radian[cIndex][0],&rayAod_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 100));
      std::shuffle (&rayAoa_radian[cIndex][0],&rayAoa_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 200));
      std::shuffle (&rayZod_radian[cIndex][0],&rayZod_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 300));
      std::shuffle (&rayZoa_radian[cIndex][0],&rayZoa_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 400));
    }

  //Step 9: Generate the cross polarization power ratios
  //This step is skipped, only vertical polarization is considered in this version

  //Step 10: Draw initial phases
  double2DVector_t clusterPhase;       //rayAoa_radian[n][m], where n is cluster index, m is ray index
  for (uint8_t nInd = 0; nInd < numReducedCluster; nInd++)
    {
      doubleVector_t temp;
      for (uint8_t mInd = 0; mInd < raysPerCluster; mInd++)
        {
          temp.push_back (m_uniformRv->GetValue (-1 * M_PI, M_PI));
        }
      clusterPhase.push_back (temp);
    }
  double losPhase = m_uniformRv->GetValue (-1 * M_PI, M_PI);
  channelParams->m_clusterPhase = clusterPhase;
  channelParams->m_losPhase = losPhase;

  //Step 11: Generate channel coefficients for each cluster n and each receiver and transmitter element pair u,s.

  complex3DVector_t H_NLOS;       // channel coefficients H_NLOS [u][s][n],
  // where u and s are receive and transmit antenna element, n is cluster index.
  uint64_t uSize = rxAntennaNum[0] * rxAntennaNum[1];
  uint64_t sSize = txAntennaNum[0] * txAntennaNum[1];

  uint8_t cluster1st = 0, cluster2nd = 0;       // first and second strongest cluster;
  double maxPower = 0;
  for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
    {
      if (maxPower < clusterPower.at (cIndex))
        {
          maxPower = clusterPower.at (cIndex);
          cluster1st = cIndex;
        }
    }
  maxPower = 0;
  for (uint8_t cIndex = 0; cIndex < numReducedCluster; cIndex++)
    {
      if (maxPower < clusterPower.at (cIndex) && cluster1st != cIndex)
        {
          maxPower = clusterPower.at (cIndex);
          cluster2nd = cIndex;
        }
    }

  NS_LOG_INFO ("1st strongest cluster:" << (int)cluster1st << ", 2nd strongest cluster:" << (int)cluster2nd);

  complex3DVector_t H_usn;       //channel coffecient H_usn[u][s][n];
  //Since each of the strongest 2 clusters are divided into 3 sub-clusters, the total cluster will be numReducedCLuster + 4.

  H_usn.resize (uSize);
  for (uint64_t uIndex = 0; uIndex < uSize; uIndex++)
    {
      H_usn.at (uIndex).resize (sSize);
      for (uint64_t sIndex = 0; sIndex < sSize; sIndex++)
        {
          H_usn.at (uIndex).at (sIndex).resize (numReducedCluster);
        }
    }
  //double slotTime = Simulator::Now ().GetSeconds ();
  // The following for loops computes the channel coefficients
  for (uint64_t uIndex = 0; uIndex < uSize; uIndex++)
    {
      Vector uLoc = rxAntenna->GetAntennaLocation (uIndex,rxAntennaNum);

      for (uint64_t sIndex = 0; sIndex < sSize; sIndex++)
        {

          Vector sLoc = txAntenna->GetAntennaLocation (sIndex,txAntennaNum);

          for (uint8_t nIndex = 0; nIndex < numReducedCluster; nIndex++)
            {
              //Compute the N-2 weakest cluster, only vertical polarization. (7.5-22)
              if (nIndex != cluster1st && nIndex != cluster2nd)
                {
                  std::complex<double> rays (0,0);
                  for (uint8_t mIndex = 0; mIndex < raysPerCluster; mIndex++)
                    {
                      double initialPhase = clusterPhase.at (nIndex).at (mIndex);
                      //lambda_0 is accounted in the antenna spacing uLoc and sLoc.
                      double rxPhaseDiff = 2 * M_PI * (sin (rayZoa_radian[nIndex][mIndex]) * cos (rayAoa_radian[nIndex][mIndex]) * uLoc.x
                                                       + sin (rayZoa_radian[nIndex][mIndex]) * sin (rayAoa_radian[nIndex][mIndex]) * uLoc.y
                                                       + cos (rayZoa_radian[nIndex][mIndex]) * uLoc.z);

                      double txPhaseDiff = 2 * M_PI * (sin (rayZod_radian[nIndex][mIndex]) * cos (rayAod_radian[nIndex][mIndex]) * sLoc.x
                                                       + sin (rayZod_radian[nIndex][mIndex]) * sin (rayAod_radian[nIndex][mIndex]) * sLoc.y
                                                       + cos (rayZod_radian[nIndex][mIndex]) * sLoc.z);
                      //Doppler is computed in the CalBeamformingGain function and is simplified to only account for the center anngle of each cluster.
                      //double doppler = 2*M_PI*(sin(rayZoa_radian[nIndex][mIndex])*cos(rayAoa_radian[nIndex][mIndex])*relativeSpeed.x
                      //		+ sin(rayZoa_radian[nIndex][mIndex])*sin(rayAoa_radian[nIndex][mIndex])*relativeSpeed.y
                      //		+ cos(rayZoa_radian[nIndex][mIndex])*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;
                      rays += exp (std::complex<double> (0, initialPhase))
                        * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                           * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                        * exp (std::complex<double> (0, rxPhaseDiff))
                        * exp (std::complex<double> (0, txPhaseDiff));
                      //*exp(std::complex<double>(0, doppler));
                      //rays += 1;
                    }
                  //rays *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  rays *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  H_usn.at (uIndex).at (sIndex).at (nIndex) = rays;
                }
              else                   //(7.5-28)
                {
                  std::complex<double> raysSub1 (0,0);
                  std::complex<double> raysSub2 (0,0);
                  std::complex<double> raysSub3 (0,0);

                  for (uint8_t mIndex = 0; mIndex < raysPerCluster; mIndex++)
                    {

                      //ZML:Just remind me that the angle offsets for the 3 subclusters were not generated correctly.

                      double initialPhase = clusterPhase.at (nIndex).at (mIndex);
                      double rxPhaseDiff = 2 * M_PI * (sin (rayZoa_radian[nIndex][mIndex]) * cos (rayAoa_radian[nIndex][mIndex]) * uLoc.x
                                                       + sin (rayZoa_radian[nIndex][mIndex]) * sin (rayAoa_radian[nIndex][mIndex]) * uLoc.y
                                                       + cos (rayZoa_radian[nIndex][mIndex]) * uLoc.z);
                      double txPhaseDiff = 2 * M_PI * (sin (rayZod_radian[nIndex][mIndex]) * cos (rayAod_radian[nIndex][mIndex]) * sLoc.x
                                                       + sin (rayZod_radian[nIndex][mIndex]) * sin (rayAod_radian[nIndex][mIndex]) * sLoc.y
                                                       + cos (rayZod_radian[nIndex][mIndex]) * sLoc.z);
                      //double doppler = 2*M_PI*(sin(rayZoa_radian[nIndex][mIndex])*cos(rayAoa_radian[nIndex][mIndex])*relativeSpeed.x
                      //		+ sin(rayZoa_radian[nIndex][mIndex])*sin(rayAoa_radian[nIndex][mIndex])*relativeSpeed.y
                      //		+ cos(rayZoa_radian[nIndex][mIndex])*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;
                      //double delaySpread;
                      switch (mIndex)
                        {
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 17:
                        case 18:
                          //delaySpread= -2*M_PI*(clusterDelay.at(nIndex)+1.28*c_DS)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub2 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub2 +=1;
                          break;
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                          //delaySpread = -2*M_PI*(clusterDelay.at(nIndex)+2.56*c_DS)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub3 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub3 +=1;
                          break;
                        default:                        //case 1,2,3,4,5,6,7,8,19,20
                                                        //delaySpread = -2*M_PI*clusterDelay.at(nIndex)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub1 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub1 +=1;
                          break;
                        }
                    }
                  //raysSub1 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  //raysSub2 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  //raysSub3 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  raysSub1 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  raysSub2 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  raysSub3 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  H_usn.at (uIndex).at (sIndex).at (nIndex) = raysSub1;
                  H_usn.at (uIndex).at (sIndex).push_back (raysSub2);
                  H_usn.at (uIndex).at (sIndex).push_back (raysSub3);

                }
            }
          if (condition == 'l')               //(7.5-29) && (7.5-30)
            {
              std::complex<double> ray (0,0);
              double rxPhaseDiff = 2 * M_PI * (sin (rxAngle.theta) * cos (rxAngle.phi) * uLoc.x
                                               + sin (rxAngle.theta) * sin (rxAngle.phi) * uLoc.y
                                               + cos (rxAngle.theta) * uLoc.z);
              double txPhaseDiff = 2 * M_PI * (sin (txAngle.theta) * cos (txAngle.phi) * sLoc.x
                                               + sin (txAngle.theta) * sin (txAngle.phi) * sLoc.y
                                               + cos (txAngle.theta) * sLoc.z);
              //double doppler = 2*M_PI*(sin(rxAngle.theta)*cos(rxAngle.phi)*relativeSpeed.x
              //		+ sin(rxAngle.theta)*sin(rxAngle.phi)*relativeSpeed.y
              //		+ cos(rxAngle.theta)*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;

              ray = exp (std::complex<double> (0, losPhase))
                * (rxAntenna->GetRadiationPattern (rxAngle.theta,rxAngle.phi)
                   * txAntenna->GetRadiationPattern (txAngle.theta,rxAngle.phi))
                * exp (std::complex<double> (0, rxPhaseDiff))
                * exp (std::complex<double> (0, txPhaseDiff));
              //*exp(std::complex<double>(0, doppler));

              double K_linear = pow (10,K_factor / 10);
              // the LOS path should be attenuated if blockage is enabled.
              H_usn.at (uIndex).at (sIndex).at (0) = sqrt (1 / (K_linear + 1)) * H_usn.at (uIndex).at (sIndex).at (0) + sqrt (K_linear / (1 + K_linear)) * ray / pow (10,attenuation_dB.at (0) / 10);           //(7.5-30) for tau = tau1
              double tempSize = H_usn.at (uIndex).at (sIndex).size ();
              for (uint8_t nIndex = 1; nIndex < tempSize; nIndex++)
                {
                  H_usn.at (uIndex).at (sIndex).at (nIndex) *= sqrt (1 / (K_linear + 1));                   //(7.5-30) for tau = tau2...taunN
                }

            }
        }
    }

  if (cluster1st == cluster2nd)
    {
      clusterDelay.push_back (clusterDelay.at (cluster1st) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (cluster1st) + 2.56 * table3gpp->m_cDS);

      clusterAoa.push_back (clusterAoa.at (cluster1st));
      clusterAoa.push_back (clusterAoa.at (cluster1st));

      clusterZoa.push_back (clusterZoa.at (cluster1st));
      clusterZoa.push_back (clusterZoa.at (cluster1st));

      clusterAod.push_back (clusterAod.at (cluster1st));
      clusterAod.push_back (clusterAod.at (cluster1st));

      clusterZod.push_back (clusterZod.at (cluster1st));
      clusterZod.push_back (clusterZod.at (cluster1st));
    }
  else
    {
      double min, max;
      if (cluster1st < cluster2nd)
        {
          min = cluster1st;
          max = cluster2nd;
        }
      else
        {
          min = cluster2nd;
          max = cluster1st;
        }
      clusterDelay.push_back (clusterDelay.at (min) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (min) + 2.56 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (max) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (max) + 2.56 * table3gpp->m_cDS);

      clusterAoa.push_back (clusterAoa.at (min));
      clusterAoa.push_back (clusterAoa.at (min));
      clusterAoa.push_back (clusterAoa.at (max));
      clusterAoa.push_back (clusterAoa.at (max));

      clusterZoa.push_back (clusterZoa.at (min));
      clusterZoa.push_back (clusterZoa.at (min));
      clusterZoa.push_back (clusterZoa.at (max));
      clusterZoa.push_back (clusterZoa.at (max));

      clusterAod.push_back (clusterAod.at (min));
      clusterAod.push_back (clusterAod.at (min));
      clusterAod.push_back (clusterAod.at (max));
      clusterAod.push_back (clusterAod.at (max));

      clusterZod.push_back (clusterZod.at (min));
      clusterZod.push_back (clusterZod.at (min));
      clusterZod.push_back (clusterZod.at (max));
      clusterZod.push_back (clusterZod.at (max));


    }

  NS_LOG_INFO ("size of coefficient matrix =[" << H_usn.size () << "][" << H_usn.at (0).size () << "][" << H_usn.at (0).at (0).size () << "]");


  /*std::cout << "Delay:";
  for (uint8_t i = 0; i < clusterDelay.size(); i++)
  {
          std::cout <<clusterDelay.at(i)<<"s\t";
  }
  std::cout << "\n";*/

  channelParams->m_channel = H_usn;
  channelParams->m_delay = clusterDelay;

  channelParams->m_angle.clear ();
  channelParams->m_angle.push_back (clusterAoa);
  channelParams->m_angle.push_back (clusterZoa);
  channelParams->m_angle.push_back (clusterAod);
  channelParams->m_angle.push_back (clusterZod);

  return channelParams;

}

Ptr<Params3gpp>
MmWaveVehicularSpectrumPropagationLossModel::UpdateChannel (Ptr<Params3gpp> params3gpp, Ptr<ParamsTable>  table3gpp,
                                  Ptr<MmWaveVehicularAntennaArrayModel> txAntenna, Ptr<MmWaveVehicularAntennaArrayModel> rxAntenna,
                                  uint16_t *txAntennaNum, uint16_t *rxAntennaNum, Angles &rxAngle, Angles &txAngle) const
{
  Ptr<Params3gpp> params = params3gpp;
  uint8_t raysPerCluster = table3gpp->m_raysPerCluster;
  //We first update the current location, the previous location will be updated in the end.


  //Step 4: Get LSP from previous channel
  double DS = params->m_DS;
  double K_factor = params->m_K;

  //Step 5: Update Delays.
  //copy delay from previous channel.
  doubleVector_t clusterDelay;
  for (uint8_t cInd = 0; cInd < params->m_numCluster; cInd++)
    {
      clusterDelay.push_back (params->m_delay.at (cInd));
    }
  //If LOS condition, we need to revert the tau^LOS_n back to tau_n.
  if (params->m_condition == 'l')
    {
      double C_tau = 0.7705 - 0.0433 * K_factor + 2e-4 * pow (K_factor,2) + 17e-6 * pow (K_factor,3);         //(7.5-3)
      for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
        {
          clusterDelay.at (cIndex) = clusterDelay.at (cIndex) * C_tau;
        }
    }
  //update delay based on equation (7.6-9)
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      clusterDelay.at (cIndex) -= (sin (params->m_angle.at (ZOA_INDEX).at (cIndex) * M_PI / 180) * cos (params->m_angle.at (AOA_INDEX).at (cIndex) * M_PI / 180) * params->m_speed.x
                                   + sin (params->m_angle.at (ZOA_INDEX).at (cIndex) * M_PI / 180) * sin (params->m_angle.at (AOA_INDEX).at (cIndex) * M_PI / 180) * params->m_speed.y) * m_updatePeriod.GetSeconds () / 3e8; //(7.6-9)
    }

  /* since the scaled Los delays are not to be used in cluster power generation,
   * we will generate cluster power first and resume to compute Los cluster delay later.*/

  //Step 6: Generate cluster powers.
  doubleVector_t clusterPower;
  double powerSum = 0;
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      double power = exp (-1 * clusterDelay.at (cIndex) * (table3gpp->m_rTau - 1) / table3gpp->m_rTau / DS) *
        pow (10,-1 * m_normalRv->GetValue () * table3gpp->m_shadowingStd / 10);                       //(7.5-5)
      powerSum += power;
      clusterPower.push_back (power);
    }

  // we do not need to compute the cluster power of LOS case, since it is used for generating angles.
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      clusterPower.at (cIndex) = clusterPower.at (cIndex) / powerSum;         //(7.5-6)
    }

  // Resume step 5 to compute the delay for LoS condition.
  if (params->m_condition == 'l')
    {
      double C_tau = 0.7705 - 0.0433 * K_factor + 2e-4 * pow (K_factor,2) + 17e-6 * pow (K_factor,3);         //(7.5-3)
      for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
        {
          clusterDelay.at (cIndex) = clusterDelay.at (cIndex) / C_tau;             //(7.5-4)
        }
    }

  /*std::cout << "Delay:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterDelay.at(i)<<"s\t";
  }
  std::cout << "\n";
  std::cout << "Power:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterPower.at(i)<<"\t";
  }
  std::cout << "\n";*/

  //step 7: Generate arrival and departure angles for both azimuth and elevation.

  //copy the angles from previous channel
  doubleVector_t clusterAoa, clusterZoa, clusterAod, clusterZod;
  /*
   * copy the angles from previous channel
   * need to change the angle according to equations (7.6-11) - (7.6-14)*/
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      clusterAoa.push_back (params->m_angle.at (AOA_INDEX).at (cIndex));
      clusterZoa.push_back (params->m_angle.at (ZOA_INDEX).at (cIndex));
      clusterAod.push_back (params->m_angle.at (AOD_INDEX).at (cIndex));
      clusterZod.push_back (params->m_angle.at (ZOD_INDEX).at (cIndex));
    }
  double v = sqrt (params->m_speed.x * params->m_speed.x + params->m_speed.y * params->m_speed.y);
  if (v > 1e-6)      //Update the angles only when the speed is not 0.
    {
      if (params->m_norRvAngles.size () == 0)
        {
          //initial case
          for (uint8_t cInd = 0; cInd < params->m_numCluster; cInd++)
            {
              doubleVector_t temp;
              temp.push_back (0);                  //initial random angle for AOA
              temp.push_back (0);                  //initial random angle for ZOA
              temp.push_back (0);                  //initial random angle for AOD
              temp.push_back (0);                  //initial random angle for ZOD
              params->m_norRvAngles.push_back (temp);
            }
        }
      for (uint8_t cInd = 0; cInd < params->m_numCluster; cInd++)
        {
          double  timeDiff = Now ().GetSeconds () - params->m_generatedTime.GetSeconds ();
          double ranPhiAOD, ranThetaZOD, ranPhiAOA, ranThetaZOA;
          if (params->m_condition == 'l' && cInd == 0)              //These angles equal 0 for LOS path.
            {
              ranPhiAOD = 0;
              ranThetaZOD = 0;
              ranPhiAOA = 0;
              ranThetaZOA = 0;
            }
          else
            {
              double deltaX = sqrt (pow (params->m_preLocUT.x - params->m_locUT.x, 2) + pow (params->m_preLocUT.y - params->m_locUT.y, 2));
              double R_phi = exp (-1 * deltaX / 50);                  // 50 m is the correlation distance as specified in TR 38.900 Sec 7.6.3.2
              double R_theta = exp (-1 * deltaX / 100);                  // 100 m is the correlation distance as specified in TR 38.900 Sec 7.6.3.2

              //In order to generate correlated uniform random variables, we first generate correlated normal random variables and map the normal RV to uniform RV.
              //Notice the correlation will change if the RV is transformed from normal to uniform.
              //To compensate the distortion, the correlation of the normal RV is computed
              //such that the uniform RV would have the desired correlation when transformed from normal RV.

              //The following formula was obtained from MATLAB numerical simulation.

              if (R_phi * R_phi * (-0.069) + R_phi * 1.074 - 0.002 < 1)                  //When the correlation for normal RV is close to 1, no need to transform.
                {
                  R_phi = R_phi * R_phi * (-0.069) + R_phi * 1.074 - 0.002;
                }
              if (R_theta * R_theta * (-0.069) + R_theta * 1.074 - 0.002 < 1)
                {
                  R_theta = R_theta * R_theta * (-0.069) + R_theta * 1.074 - 0.002;
                }

              //We can generate a new correlated normal RV with the following formula
              params->m_norRvAngles.at (cInd).at (AOD_INDEX) = R_phi * params->m_norRvAngles.at (cInd).at (AOD_INDEX) + sqrt (1 - R_phi * R_phi) * m_normalRv->GetValue ();
              params->m_norRvAngles.at (cInd).at (ZOD_INDEX) = R_theta * params->m_norRvAngles.at (cInd).at (ZOD_INDEX) + sqrt (1 - R_theta * R_theta) * m_normalRv->GetValue ();
              params->m_norRvAngles.at (cInd).at (AOA_INDEX) = R_phi * params->m_norRvAngles.at (cInd).at (AOA_INDEX) + sqrt (1 - R_phi * R_phi) * m_normalRv->GetValue ();
              params->m_norRvAngles.at (cInd).at (ZOA_INDEX) = R_theta * params->m_norRvAngles.at (cInd).at (ZOA_INDEX) + sqrt (1 - R_theta * R_theta) * m_normalRv->GetValue ();

              //The normal RV is transformed to uniform RV with the desired correlation.
              ranPhiAOD = (0.5 * erfc (-1 * params->m_norRvAngles.at (cInd).at (AOD_INDEX) / sqrt (2))) * 2 * M_PI - M_PI;
              ranThetaZOD = (0.5 * erfc (-1 * params->m_norRvAngles.at (cInd).at (ZOD_INDEX) / sqrt (2))) * M_PI - 0.5 * M_PI;
              ranPhiAOA = (0.5 * erfc (-1 * params->m_norRvAngles.at (cInd).at (AOA_INDEX) / sqrt (2))) * 2 * M_PI - M_PI;
              ranThetaZOA = (0.5 * erfc (-1 * params->m_norRvAngles.at (cInd).at (ZOA_INDEX) / sqrt (2))) * M_PI - 0.5 * M_PI;
            }
          clusterAod.at (cInd) += v * timeDiff *
            sin (atan (params->m_speed.y / params->m_speed.x) - clusterAod.at (cInd) * M_PI / 180 + ranPhiAOD) * 180 / (M_PI * params->m_dis2D);
          clusterZod.at (cInd) -= v * timeDiff *
            cos (atan (params->m_speed.y / params->m_speed.x) - clusterAod.at (cInd) * M_PI / 180 + ranThetaZOD) * 180 / (M_PI * params->m_dis3D);
          clusterAoa.at (cInd) -= v * timeDiff *
            sin (atan (params->m_speed.y / params->m_speed.x) - clusterAoa.at (cInd) * M_PI / 180 + ranPhiAOA) * 180 / (M_PI * params->m_dis2D);
          clusterZoa.at (cInd) -= v * timeDiff *
            cos (atan (params->m_speed.y / params->m_speed.x) - clusterAoa.at (cInd) * M_PI / 180 + ranThetaZOA) * 180 / (M_PI * params->m_dis3D);
        }
    }


  double rayAoa_radian[params->m_numCluster][raysPerCluster];       //rayAoa_radian[n][m], where n is cluster index, m is ray index
  double rayAod_radian[params->m_numCluster][raysPerCluster];       //rayAod_radian[n][m], where n is cluster index, m is ray index
  double rayZoa_radian[params->m_numCluster][raysPerCluster];       //rayZoa_radian[n][m], where n is cluster index, m is ray index
  double rayZod_radian[params->m_numCluster][raysPerCluster];       //rayZod_radian[n][m], where n is cluster index, m is ray index

  for (uint8_t nInd = 0; nInd < params->m_numCluster; nInd++)
    {
      for (uint8_t mInd = 0; mInd < raysPerCluster; mInd++)
        {
          double tempAoa = clusterAoa.at (nInd) + table3gpp->m_cASA * offSetAlpha[mInd];              //(7.5-13)
          while (tempAoa > 360)
            {
              tempAoa -= 360;
            }

          while (tempAoa < 0)
            {
              tempAoa += 360;

            }
          NS_ASSERT_MSG (tempAoa >= 0 && tempAoa <= 360, "the AOA should be the range of [0,360]");
          rayAoa_radian[nInd][mInd] = tempAoa * M_PI / 180;

          double tempAod = clusterAod.at (nInd) + table3gpp->m_cASD * offSetAlpha[mInd];
          while (tempAod > 360)
            {
              tempAod -= 360;
            }

          while (tempAod < 0)
            {
              tempAod += 360;
            }
          NS_ASSERT_MSG (tempAod >= 0 && tempAod <= 360, "the AOD should be the range of [0,360]");
          rayAod_radian[nInd][mInd] = tempAod * M_PI / 180;

          double tempZoa = clusterZoa.at (nInd) + table3gpp->m_cZSA * offSetAlpha[mInd];              //(7.5-18)

          while (tempZoa > 360)
            {
              tempZoa -= 360;
            }

          while (tempZoa < 0)
            {
              tempZoa += 360;
            }

          if (tempZoa > 180)
            {
              tempZoa = 360 - tempZoa;
            }

          NS_ASSERT_MSG (tempZoa >= 0&&tempZoa <= 180, "the ZOA should be the range of [0,180]");
          rayZoa_radian[nInd][mInd] = tempZoa * M_PI / 180;

          double tempZod = clusterZod.at (nInd) + 0.375 * pow (10,table3gpp->m_uLgZSD) * offSetAlpha[mInd];             //(7.5-20)

          while (tempZod > 360)
            {
              tempZod -= 360;
            }

          while (tempZod < 0)
            {
              tempZod += 360;
            }
          if (tempZod > 180)
            {
              tempZod = 360 - tempZod;
            }
          NS_ASSERT_MSG (tempZod >= 0&&tempZod <= 180, "the ZOD should be the range of [0,180]");
          rayZod_radian[nInd][mInd] = tempZod * M_PI / 180;
        }
    }

  doubleVector_t angle_degree;
  double sizeTemp = clusterZoa.size ();
  for (uint8_t ind = 0; ind < 4; ind++)
    {
      switch (ind)
        {
        case 0:
          angle_degree = clusterAoa;
          break;
        case 1:
          angle_degree = clusterZoa;
          break;
        case 2:
          angle_degree = clusterAod;
          break;
        case 3:
          angle_degree = clusterZod;
          break;
        default:
          NS_FATAL_ERROR ("Programming Error");
        }

      for (uint8_t nIndex = 0; nIndex < sizeTemp; nIndex++)
        {
          while (angle_degree[nIndex] > 360)
            {
              angle_degree[nIndex] -= 360;
            }

          while (angle_degree[nIndex] < 0)
            {
              angle_degree[nIndex] += 360;
            }

          if (ind == 1 || ind == 3)
            {
              if (angle_degree[nIndex] > 180)
                {
                  angle_degree[nIndex] = 360 - angle_degree[nIndex];
                }
            }
        }
      switch (ind)
        {
        case 0:
          clusterAoa = angle_degree;
          break;
        case 1:
          clusterZoa = angle_degree;
          break;
        case 2:
          clusterAod = angle_degree;
          break;
        case 3:
          clusterZod = angle_degree;
          break;
        default:
          NS_FATAL_ERROR ("Programming Error");
        }
    }
  doubleVector_t attenuation_dB;
  if (m_blockage)
    {
      attenuation_dB = CalAttenuationOfBlockage (params, clusterAoa, clusterZoa);
      for (uint8_t cInd = 0; cInd < params->m_numCluster; cInd++)
        {
          clusterPower.at (cInd) = clusterPower.at (cInd) / pow (10,attenuation_dB.at (cInd) / 10);
        }
    }
  else
    {
      attenuation_dB.push_back (0);
    }

  /*std::cout << "BlockedPower:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterPower.at(i)<<"\t";
  }
  std::cout << "\n";*/

  /*std::cout << "AOD:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterAod.at(i)<<"'\t";
  }
  std::cout << "\n";

  std::cout << "AOA:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterAoa.at(i)<<"'\t";
  }
  std::cout << "\n";

  std::cout << "ZOD:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterZod.at(i)<<"'\t";
          for (uint8_t d = 0; d < raysPerCluster; d++)
          {
                  std::cout <<rayZod_radian[i][d]<<"\t";
          }
          std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "ZOA:";
  for (uint8_t i = 0; i < params->m_numCluster; i++)
  {
          std::cout <<clusterZoa.at(i)<<"'\t";
  }
  std::cout << "\n";*/


  //Step 8: Coupling of rays within a cluster for both azimuth and elevation
  //shuffle all the arrays to perform random coupling


  //since the updating and original generated angles should have the same order of "random coupling",
  //I control the seed of each shuffle, so that the update and original generated angle use the same seed.
  //Is this correct?

  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      std::shuffle (&rayAod_radian[cIndex][0],&rayAod_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 100));
      std::shuffle (&rayAoa_radian[cIndex][0],&rayAoa_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 200));
      std::shuffle (&rayZod_radian[cIndex][0],&rayZod_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 300));
      std::shuffle (&rayZoa_radian[cIndex][0],&rayZoa_radian[cIndex][raysPerCluster],std::default_random_engine (cIndex * 1000 + 400));
    }

  //Step 9: Generate the cross polarization power ratios
  //This step is skipped, only vertical polarization is considered in this version

  //Step 10: Draw initial phases
  double2DVector_t clusterPhase = params->m_clusterPhase;       //rayAoa_radian[n][m], where n is cluster index, m is ray index
  double losPhase = params->m_losPhase;
  // these two should also be generated from previous channel.

  //Step 11: Generate channel coefficients for each cluster n and each receiver and transmitter element pair u,s.

  complex3DVector_t H_NLOS;       // channel coefficients H_NLOS [u][s][n],
  // where u and s are receive and transmit antenna element, n is cluster index.
  uint64_t uSize = rxAntennaNum[0] * rxAntennaNum[1];
  uint64_t sSize = txAntennaNum[0] * txAntennaNum[1];

  uint8_t cluster1st = 0, cluster2nd = 0;       // first and second strongest cluster;
  double maxPower = 0;
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      if (maxPower < clusterPower.at (cIndex))
        {
          maxPower = clusterPower.at (cIndex);
          cluster1st = cIndex;
        }
    }
  maxPower = 0;
  for (uint8_t cIndex = 0; cIndex < params->m_numCluster; cIndex++)
    {
      if (maxPower < clusterPower.at (cIndex) && cluster1st != cIndex)
        {
          maxPower = clusterPower.at (cIndex);
          cluster2nd = cIndex;
        }
    }

  NS_LOG_INFO ("1st strongest cluster:" << (int)cluster1st << ", 2nd strongest cluster:" << (int)cluster2nd);

  complex3DVector_t H_usn;       //channel coffecient H_usn[u][s][n];
  //Since each of the strongest 2 clusters are divided into 3 sub-clusters, the total cluster will be numReducedCLuster + 4.

  H_usn.resize (uSize);
  for (uint64_t uIndex = 0; uIndex < uSize; uIndex++)
    {
      H_usn.at (uIndex).resize (sSize);
      for (uint64_t sIndex = 0; sIndex < sSize; sIndex++)
        {
          H_usn.at (uIndex).at (sIndex).resize (params->m_numCluster);
        }
    }
  //double slotTime = Simulator::Now ().GetSeconds ();
  // The following for loops computes the channel coefficients
  for (uint64_t uIndex = 0; uIndex < uSize; uIndex++)
    {
      Vector uLoc = rxAntenna->GetAntennaLocation (uIndex,rxAntennaNum);

      for (uint64_t sIndex = 0; sIndex < sSize; sIndex++)
        {

          Vector sLoc = txAntenna->GetAntennaLocation (sIndex,txAntennaNum);

          for (uint8_t nIndex = 0; nIndex < params->m_numCluster; nIndex++)
            {
              //Compute the N-2 weakest cluster, only vertical polarization. (7.5-22)
              if (nIndex != cluster1st && nIndex != cluster2nd)
                {
                  std::complex<double> rays (0,0);
                  for (uint8_t mIndex = 0; mIndex < raysPerCluster; mIndex++)
                    {
                      double initialPhase = clusterPhase.at (nIndex).at (mIndex);
                      //lambda_0 is accounted in the antenna spacing uLoc and sLoc.
                      double rxPhaseDiff = 2 * M_PI * (sin (rayZoa_radian[nIndex][mIndex]) * cos (rayAoa_radian[nIndex][mIndex]) * uLoc.x
                                                       + sin (rayZoa_radian[nIndex][mIndex]) * sin (rayAoa_radian[nIndex][mIndex]) * uLoc.y
                                                       + cos (rayZoa_radian[nIndex][mIndex]) * uLoc.z);

                      double txPhaseDiff = 2 * M_PI * (sin (rayZod_radian[nIndex][mIndex]) * cos (rayAod_radian[nIndex][mIndex]) * sLoc.x
                                                       + sin (rayZod_radian[nIndex][mIndex]) * sin (rayAod_radian[nIndex][mIndex]) * sLoc.y
                                                       + cos (rayZod_radian[nIndex][mIndex]) * sLoc.z);
                      //Doppler is computed in the CalBeamformingGain function and is simplified to only account for the center anngle of each cluster.
                      //double doppler = 2*M_PI*(sin(rayZoa_radian[nIndex][mIndex])*cos(rayAoa_radian[nIndex][mIndex])*relativeSpeed.x
                      //		+ sin(rayZoa_radian[nIndex][mIndex])*sin(rayAoa_radian[nIndex][mIndex])*relativeSpeed.y
                      //		+ cos(rayZoa_radian[nIndex][mIndex])*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;
                      rays += exp (std::complex<double> (0, initialPhase))
                        * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                           * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                        * exp (std::complex<double> (0, rxPhaseDiff))
                        * exp (std::complex<double> (0, txPhaseDiff));
                      //*exp(std::complex<double>(0, doppler));
                      //rays += 1;
                    }
                  //rays *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  rays *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  H_usn.at (uIndex).at (sIndex).at (nIndex) = rays;
                }
              else                   //(7.5-28)
                {
                  std::complex<double> raysSub1 (0,0);
                  std::complex<double> raysSub2 (0,0);
                  std::complex<double> raysSub3 (0,0);

                  for (uint8_t mIndex = 0; mIndex < raysPerCluster; mIndex++)
                    {
                      double initialPhase = clusterPhase.at (nIndex).at (mIndex);
                      double rxPhaseDiff = 2 * M_PI * (sin (rayZoa_radian[nIndex][mIndex]) * cos (rayAoa_radian[nIndex][mIndex]) * uLoc.x
                                                       + sin (rayZoa_radian[nIndex][mIndex]) * sin (rayAoa_radian[nIndex][mIndex]) * uLoc.y
                                                       + cos (rayZoa_radian[nIndex][mIndex]) * uLoc.z);
                      double txPhaseDiff = 2 * M_PI * (sin (rayZod_radian[nIndex][mIndex]) * cos (rayAod_radian[nIndex][mIndex]) * sLoc.x
                                                       + sin (rayZod_radian[nIndex][mIndex]) * sin (rayAod_radian[nIndex][mIndex]) * sLoc.y
                                                       + cos (rayZod_radian[nIndex][mIndex]) * sLoc.z);
                      //double doppler = 2*M_PI*(sin(rayZoa_radian[nIndex][mIndex])*cos(rayAoa_radian[nIndex][mIndex])*relativeSpeed.x
                      //		+ sin(rayZoa_radian[nIndex][mIndex])*sin(rayAoa_radian[nIndex][mIndex])*relativeSpeed.y
                      //		+ cos(rayZoa_radian[nIndex][mIndex])*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;
                      //double delaySpread;
                      switch (mIndex)
                        {
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 17:
                        case 18:
                          //delaySpread= -2*M_PI*(clusterDelay.at(nIndex)+1.28*c_DS)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub2 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub2 +=1;
                          break;
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                          //delaySpread = -2*M_PI*(clusterDelay.at(nIndex)+2.56*c_DS)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub3 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub3 +=1;
                          break;
                        default:                        //case 1,2,3,4,5,6,7,8,19,20
                                                        //delaySpread = -2*M_PI*clusterDelay.at(nIndex)*m_phyMacConfig->GetCenterFrequency ();
                          raysSub1 += exp (std::complex<double> (0, initialPhase))
                            * (rxAntenna->GetRadiationPattern (rayZoa_radian[nIndex][mIndex],rayAoa_radian[nIndex][mIndex])
                               * txAntenna->GetRadiationPattern (rayZod_radian[nIndex][mIndex],rayAod_radian[nIndex][mIndex]))
                            * exp (std::complex<double> (0, rxPhaseDiff))
                            * exp (std::complex<double> (0, txPhaseDiff));
                          //*exp(std::complex<double>(0, doppler));
                          //raysSub1 +=1;
                          break;
                        }
                    }
                  //raysSub1 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  //raysSub2 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  //raysSub3 *= sqrt(clusterPower.at(nIndex))/raysPerCluster;
                  raysSub1 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  raysSub2 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  raysSub3 *= sqrt (clusterPower.at (nIndex) / raysPerCluster);
                  H_usn.at (uIndex).at (sIndex).at (nIndex) = raysSub1;
                  H_usn.at (uIndex).at (sIndex).push_back (raysSub2);
                  H_usn.at (uIndex).at (sIndex).push_back (raysSub3);

                }
            }
          if (params->m_condition == 'l')               //(7.5-29) && (7.5-30)
            {
              std::complex<double> ray (0,0);
              double rxPhaseDiff = 2 * M_PI * (sin (rxAngle.theta) * cos (rxAngle.phi) * uLoc.x
                                               + sin (rxAngle.theta) * sin (rxAngle.phi) * uLoc.y
                                               + cos (rxAngle.theta) * uLoc.z);
              double txPhaseDiff = 2 * M_PI * (sin (txAngle.theta) * cos (txAngle.phi) * sLoc.x
                                               + sin (txAngle.theta) * sin (txAngle.phi) * sLoc.y
                                               + cos (txAngle.theta) * sLoc.z);
              //double doppler = 2*M_PI*(sin(rxAngle.theta)*cos(rxAngle.phi)*relativeSpeed.x
              //		+ sin(rxAngle.theta)*sin(rxAngle.phi)*relativeSpeed.y
              //		+ cos(rxAngle.theta)*relativeSpeed.z)*slotTime*m_phyMacConfig->GetCenterFrequency ()/3e8;

              ray = exp (std::complex<double> (0, losPhase))
                * (rxAntenna->GetRadiationPattern (rxAngle.theta,rxAngle.phi)
                   * txAntenna->GetRadiationPattern (txAngle.theta,txAngle.phi))
                * exp (std::complex<double> (0, rxPhaseDiff))
                * exp (std::complex<double> (0, txPhaseDiff));
              //*exp(std::complex<double>(0, doppler));

              double K_linear = pow (10,K_factor / 10);

              H_usn.at (uIndex).at (sIndex).at (0) = sqrt (1 / (K_linear + 1)) * H_usn.at (uIndex).at (sIndex).at (0) + sqrt (K_linear / (1 + K_linear)) * ray / pow (10,attenuation_dB.at (0) / 10);           //(7.5-30) for tau = tau1
              double tempSize = H_usn.at (uIndex).at (sIndex).size ();
              for (uint8_t nIndex = 1; nIndex < tempSize; nIndex++)
                {
                  H_usn.at (uIndex).at (sIndex).at (nIndex) *= sqrt (1 / (K_linear + 1));                   //(7.5-30) for tau = tau2...taunN
                }

            }
        }
    }

  if (cluster1st == cluster2nd)
    {
      clusterDelay.push_back (clusterDelay.at (cluster2nd) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (cluster2nd) + 2.56 * table3gpp->m_cDS);

      clusterAoa.push_back (clusterAoa.at (cluster2nd));
      clusterAoa.push_back (clusterAoa.at (cluster2nd));
      clusterZoa.push_back (clusterZoa.at (cluster2nd));
      clusterZoa.push_back (clusterZoa.at (cluster2nd));
    }
  else
    {
      double min, max;
      if (cluster1st < cluster2nd)
        {
          min = cluster1st;
          max = cluster2nd;
        }
      else
        {
          min = cluster2nd;
          max = cluster1st;
        }
      clusterDelay.push_back (clusterDelay.at (min) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (min) + 2.56 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (max) + 1.28 * table3gpp->m_cDS);
      clusterDelay.push_back (clusterDelay.at (max) + 2.56 * table3gpp->m_cDS);

      clusterAoa.push_back (clusterAoa.at (min));
      clusterAoa.push_back (clusterAoa.at (min));
      clusterAoa.push_back (clusterAoa.at (max));
      clusterAoa.push_back (clusterAoa.at (max));

      clusterZoa.push_back (clusterZoa.at (min));
      clusterZoa.push_back (clusterZoa.at (min));
      clusterZoa.push_back (clusterZoa.at (max));
      clusterZoa.push_back (clusterZoa.at (max));


    }

  NS_LOG_INFO ("size of coefficient matrix =[" << H_usn.size () << "][" << H_usn.at (0).size () << "][" << H_usn.at (0).at (0).size () << "]");


  /*std::cout << "Delay:";
  for (uint8_t i = 0; i < clusterDelay.size(); i++)
  {
          std::cout <<clusterDelay.at(i)<<"s\t";
  }
  std::cout << "\n";*/

  params->m_delay = clusterDelay;
  params->m_channel = H_usn;
  params->m_angle.clear ();
  params->m_angle.push_back (clusterAoa);
  params->m_angle.push_back (clusterZoa);
  params->m_angle.push_back (clusterAod);
  params->m_angle.push_back (clusterZod);
  //update the previous location.

  return params;

}

doubleVector_t
MmWaveVehicularSpectrumPropagationLossModel::CalAttenuationOfBlockage (Ptr<Params3gpp> params,
                                             doubleVector_t clusterAOA, doubleVector_t clusterZOA) const
{
  doubleVector_t powerAttenuation;
  uint8_t clusterNum = clusterAOA.size ();
  for (uint8_t cInd = 0; cInd < clusterNum; cInd++)
    {
      powerAttenuation.push_back (0);          //Initial power attenuation for all clusters to be 0 dB;
    }
  //step a: the number of non-self blocking blockers is stored in m_numNonSelfBloking.

  //step b:Generate the size and location of each blocker
  //generate self blocking (i.e., for blockage from the human body)
  double phi_sb, x_sb, theta_sb, y_sb;
  //table 7.6.4.1-1 Self-blocking region parameters.
  if (m_portraitMode)
    {
      phi_sb = 260;
      x_sb = 120;
      theta_sb = 100;
      y_sb = 80;
    }
  else      // landscape mode
    {
      phi_sb = 40;
      x_sb = 160;
      theta_sb = 110;
      y_sb = 75;
    }

  //generate or update non-self blocking
  if (params->m_nonSelfBlocking.size () == 0)      //generate new blocking regions
    {
      for (uint16_t blockInd = 0; blockInd < m_numNonSelfBloking; blockInd++)
        {
          //draw value from table 7.6.4.1-2 Blocking region parameters
          doubleVector_t table;
          table.push_back (m_normalRvBlockage->GetValue ());              //phi_k: store the normal RV that will be mapped to uniform (0,360) later.
          if (m_scenario == "InH-OfficeMixed" || m_scenario == "InH-OfficeOpen")
            {
              table.push_back (m_uniformRvBlockage->GetValue (15, 45));                  //x_k
              table.push_back (90);                   //Theta_k
              table.push_back (m_uniformRvBlockage->GetValue (5, 15));                  //y_k
              table.push_back (2);                   //r
            }
          else
            {
              table.push_back (m_uniformRvBlockage->GetValue (5, 15));                  //x_k
              table.push_back (90);                   //Theta_k
              table.push_back (5);                   //y_k
              table.push_back (10);                   //r
            }
          params->m_nonSelfBlocking.push_back (table);
        }
    }
  else
    {
      double deltaX = sqrt (pow (params->m_preLocUT.x - params->m_locUT.x, 2) + pow (params->m_preLocUT.y - params->m_locUT.y, 2));
      //if deltaX and speed are both 0, the autocorrelation is 1, skip updating
      if (deltaX > 1e-6 || m_blockerSpeed > 1e-6)
        {
          double corrDis;
          //draw value from table 7.6.4.1-4: Spatial correlation distance for different scenarios.
          if (m_scenario == "InH-OfficeMixed" || m_scenario == "InH-OfficeOpen")
            {
              //InH, correlation distance = 5;
              corrDis = 5;
            }
          else
            {
              if (params->m_o2i)                  // outdoor to indoor
                {
                  corrDis = 5;
                }
              else                   //LOS or NLOS
                {
                  corrDis = 10;
                }
            }
          double R;
          if (m_blockerSpeed > 1e-6)               // speed not equal to 0
            {
              double corrT = corrDis / m_blockerSpeed;
              R = exp (-1 * (deltaX / corrDis + (Now ().GetSeconds () - params->m_generatedTime.GetSeconds ()) / corrT));
            }
          else
            {
              R = exp (-1 * (deltaX / corrDis));
            }

          NS_LOG_INFO ("Distance change:" << deltaX << " Speed:" << m_blockerSpeed
                                          << " Time difference:" << Now ().GetSeconds () - params->m_generatedTime.GetSeconds ()
                                          << " correlation:" << R);

          //In order to generate correlated uniform random variables, we first generate correlated normal random variables and map the normal RV to uniform RV.
          //Notice the correlation will change if the RV is transformed from normal to uniform.
          //To compensate the distortion, the correlation of the normal RV is computed
          //such that the uniform RV would have the desired correlation when transformed from normal RV.

          //The following formula was obtained from MATLAB numerical simulation.

          if (R * R * (-0.069) + R * 1.074 - 0.002 < 1)              //transform only when the correlation of normal RV is smaller than 1
            {
              R = R * R * (-0.069) + R * 1.074 - 0.002;
            }
          for (uint16_t blockInd = 0; blockInd < m_numNonSelfBloking; blockInd++)
            {

              //Generate a new correlated normal RV with the following formula
              params->m_nonSelfBlocking.at (blockInd).at (PHI_INDEX) =
                R * params->m_nonSelfBlocking.at (blockInd).at (PHI_INDEX) + sqrt (1 - R * R) * m_normalRvBlockage->GetValue ();
            }
        }

    }

  //step c: Determine the attenuation of each blocker due to blockers
  for (uint8_t cInd = 0; cInd < clusterNum; cInd++)
    {
      NS_ASSERT_MSG (clusterAOA.at (cInd) >= 0 && clusterAOA.at (cInd) <= 360, "the AOA should be the range of [0,360]");
      NS_ASSERT_MSG (clusterZOA.at (cInd) >= 0 && clusterZOA.at (cInd) <= 180, "the ZOA should be the range of [0,180]");

      //check self blocking
      NS_LOG_INFO ("AOA=" << clusterAOA.at (cInd) << " Block Region[" << phi_sb - x_sb / 2 << "," << phi_sb + x_sb / 2 << "]");
      NS_LOG_INFO ("ZOA=" << clusterZOA.at (cInd) << " Block Region[" << theta_sb - y_sb / 2 << "," << theta_sb + y_sb / 2 << "]");
      if ( std::abs (clusterAOA.at (cInd) - phi_sb) < (x_sb / 2) && std::abs (clusterZOA.at (cInd) - theta_sb) < (y_sb / 2))
        {
          powerAttenuation.at (cInd) += 30;               //anttenuate by 30 dB.
          NS_LOG_INFO ("Cluster[" << (int)cInd << "] is blocked by self blocking region and reduce 30 dB power,"
                       "the attenuation is [" << powerAttenuation.at (cInd) << " dB]");
        }

      //check non-self blocking
      double phiK, xK, thetaK, yK;
      for (uint16_t blockInd = 0; blockInd < m_numNonSelfBloking; blockInd++)
        {
          //The normal RV is transformed to uniform RV with the desired correlation.
          phiK = (0.5 * erfc (-1 * params->m_nonSelfBlocking.at (blockInd).at (PHI_INDEX) / sqrt (2))) * 360;
          while (phiK > 360)
            {
              phiK -= 360;
            }

          while (phiK < 0)
            {
              phiK += 360;
            }

          xK = params->m_nonSelfBlocking.at (blockInd).at (X_INDEX);
          thetaK = params->m_nonSelfBlocking.at (blockInd).at (THETA_INDEX);
          yK = params->m_nonSelfBlocking.at (blockInd).at (Y_INDEX);
          NS_LOG_INFO ("AOA=" << clusterAOA.at (cInd) << " Block Region[" << phiK - xK << "," << phiK + xK << "]");
          NS_LOG_INFO ("ZOA=" << clusterZOA.at (cInd) << " Block Region[" << thetaK - yK << "," << thetaK + yK << "]");

          if ( std::abs (clusterAOA.at (cInd) - phiK) < (xK)
               && std::abs (clusterZOA.at (cInd) - thetaK) < (yK))
            {
              double A1 = clusterAOA.at (cInd) - (phiK + xK / 2);                   //(7.6-24)
              double A2 = clusterAOA.at (cInd) - (phiK - xK / 2);                   //(7.6-25)
              double Z1 = clusterZOA.at (cInd) - (thetaK + yK / 2);                   //(7.6-26)
              double Z2 = clusterZOA.at (cInd) - (thetaK - yK / 2);                   //(7.6-27)
              int signA1, signA2, signZ1, signZ2;
              //draw sign for the above parameters according to table 7.6.4.1-3 Description of signs
              if (xK / 2 < clusterAOA.at (cInd) - phiK && clusterAOA.at (cInd) - phiK <= xK)
                {
                  signA1 = -1;
                }
              else
                {
                  signA1 = 1;
                }
              if (-1 * xK < clusterAOA.at (cInd) - phiK && clusterAOA.at (cInd) - phiK <= -1 * xK / 2)
                {
                  signA2 = -1;
                }
              else
                {
                  signA2 = 1;
                }

              if (yK / 2 < clusterZOA.at (cInd) - thetaK && clusterZOA.at (cInd) - thetaK <= yK)
                {
                  signZ1 = -1;
                }
              else
                {
                  signZ1 = 1;
                }
              if (-1 * yK < clusterZOA.at (cInd) - thetaK && clusterZOA.at (cInd) - thetaK <= -1 * yK / 2)
                {
                  signZ2 = -1;
                }
              else
                {
                  signZ2 = 1;
                }
              double lambda = 3e8 / m_frequency;
              double F_A1 = atan (signA1 * M_PI / 2 * sqrt (M_PI / lambda *
                                                            params->m_nonSelfBlocking.at (blockInd).at (R_INDEX) * (1 / cos (A1 * M_PI / 180) - 1))) / M_PI; //(7.6-23)
              double F_A2 = atan (signA2 * M_PI / 2 * sqrt (M_PI / lambda *
                                                            params->m_nonSelfBlocking.at (blockInd).at (R_INDEX) * (1 / cos (A2 * M_PI / 180) - 1))) / M_PI;
              double F_Z1 = atan (signZ1 * M_PI / 2 * sqrt (M_PI / lambda *
                                                            params->m_nonSelfBlocking.at (blockInd).at (R_INDEX) * (1 / cos (Z1 * M_PI / 180) - 1))) / M_PI;
              double F_Z2 = atan (signZ2 * M_PI / 2 * sqrt (M_PI / lambda *
                                                            params->m_nonSelfBlocking.at (blockInd).at (R_INDEX) * (1 / cos (Z2 * M_PI / 180) - 1))) / M_PI;
              double L_dB = -20 * log10 (1 - (F_A1 + F_A2) * (F_Z1 + F_Z2));                  //(7.6-22)
              powerAttenuation.at (cInd) += L_dB;
              NS_LOG_INFO ("Cluster[" << (int)cInd << "] is blocked by no-self blocking, "
                           "the loss is [" << L_dB << "]" << " dB");

            }
        }
    }
  return powerAttenuation;
}

void
MmWaveVehicularSpectrumPropagationLossModel::SetFrequency (double freq)
{
  m_frequency = freq;
}

double
MmWaveVehicularSpectrumPropagationLossModel::GetFrequency (void) const
{
  return m_frequency;
}

} // namespace millicar

} // namespace ns3
