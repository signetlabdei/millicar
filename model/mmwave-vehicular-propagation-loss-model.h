/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York
*   University
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
*
*/

#ifndef MMWAVE_VEHICULAR_PROPAGATION_LOSS_MODEL_H_
#define MMWAVE_VEHICULAR_PROPAGATION_LOSS_MODEL_H_

#include "ns3/propagation-loss-model.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include <ns3/vector.h>
#include <ns3/mmwave-phy-mac-common.h>
#include <map>

/*
 * This propagation loss model for vehicular communications has been implemented based on the 3GPP TR 37.885 v15.2.0 (2019-01).
 *
 * 3rd Generation Partnership Project;
 * Technical Specification Group Radio Access Network;
 * (Release 14)
 *
 * */

namespace ns3 {

namespace millicar {

struct channelCondition
{
  char m_channelCondition;
  double m_shadowing;
  Vector m_position;
};

// map store the path loss scenario(LOS,NLOS,OUTAGE) of each propagation channel
typedef std::map< std::pair< Ptr<MobilityModel>, Ptr<MobilityModel> >, channelCondition> channelConditionMap_t;

class MmWaveVehicularPropagationLossModel : public PropagationLossModel
{
  public:

    static TypeId GetTypeId (void);
    MmWaveVehicularPropagationLossModel ();

    /**
     * \param minLoss the minimum loss (dB)
     *
     * no matter how short the distance, the total propagation loss (in
     * dB) will always be greater or equal than this value
     */
    void SetMinLoss (double minLoss);

    /**
     * \return the minimum loss.
     */
    double GetMinLoss (void) const;

    /**
     * \param freq the operating frequency (Hz)
     */
    void SetFrequency (double freq);

    /**
     * \returns the current frequency (Hz)
     */
    double GetFrequency (void) const;

    char GetChannelCondition (Ptr<MobilityModel> a, Ptr<MobilityModel> b);

    std::string GetScenario ();

    double GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

  private:

    MmWaveVehicularPropagationLossModel (const MmWaveVehicularPropagationLossModel &o);
    MmWaveVehicularPropagationLossModel & operator = (const MmWaveVehicularPropagationLossModel &o);

    virtual double DoCalcRxPower (double txPowerDbm,
                                  Ptr<MobilityModel> a,
                                  Ptr<MobilityModel> b) const;
    virtual int64_t DoAssignStreams (int64_t stream);
    void UpdateConditionMap (Ptr<MobilityModel> a, Ptr<MobilityModel> b, channelCondition cond) const;

    /**
     * \param distance3D: the 3D distance between tx and rx
     * \param hA: the height of device A
     * \param hB: the height of device B
     *
     * \returns the additional NLOSv loss
     */
    double GetAdditionalNlosVLoss (double distance3D, double hA, double hB) const;

    double m_frequency;
    double m_lambda;
    double m_minLoss;
    mutable channelConditionMap_t m_channelConditionMap;
    std::string m_channelConditions;
    std::string m_scenario;
    bool m_optionNlosEnabled;
    Ptr<NormalRandomVariable> m_norVar;
    Ptr<LogNormalRandomVariable> m_logNorVar;
    Ptr<UniformRandomVariable> m_uniformVar;
    bool m_shadowingEnabled;
    double m_percType3Vehicles;
};

} // namespace millicar

} // namespace ns3

#endif
