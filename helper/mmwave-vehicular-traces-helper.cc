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

#include "mmwave-vehicular-traces-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveVehicularTracesHelper");

namespace millicar {

NS_OBJECT_ENSURE_REGISTERED (MmWaveVehicularTracesHelper);

MmWaveVehicularTracesHelper::MmWaveVehicularTracesHelper (std::string filename)
: m_filename(filename)
{
  NS_LOG_FUNCTION (this);

  if(!m_outputFile.is_open())
  {
    m_outputFile.open(m_filename.c_str());
    if (!m_outputFile.is_open ())
    {
      NS_FATAL_ERROR ("Could not open tracefile");
    }
  }
}

MmWaveVehicularTracesHelper::~MmWaveVehicularTracesHelper ()
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveVehicularTracesHelper::McsSinrCallback(const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize, uint8_t mcs)
{
  double sinrAvg = Sum (sinr) / (sinr.GetSpectrumModel ()->GetNumBands ());
  m_outputFile << Simulator::Now().GetSeconds() << "\t" << rnti << "\t" << 10 * std::log10 (sinrAvg) << "\t" << (uint32_t)numSym << "\t" << tbSize << "\t" << (uint32_t)mcs << std::endl;
}

}

}
