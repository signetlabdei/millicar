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

#ifndef MMWAVE_VEHICULAR_TRACES_HELPER_H
#define MMWAVE_VEHICULAR_TRACES_HELPER_H

#include <fstream>
#include <string>
#include <ns3/object.h>
#include <ns3/spectrum-value.h>

namespace ns3 {

namespace millicar {

/**
 * Class that manages the connection to a trace
 * in MmWaveSidelinkSpectrumPhy and prints to a file
 */
class MmWaveVehicularTracesHelper : public Object
{
public:
  /**
   * Constructor for this class
   * \param filename the name of the file
   */
  MmWaveVehicularTracesHelper(std::string filename);

  /**
   * Destructor for this class
   */
  virtual ~MmWaveVehicularTracesHelper();

  /**
   * Method to be attached to the callback in the MmWaveSidelinkSpectrumPhy
   * \param sinr pointer to the SpectrumValue instance representing the SINR
            measured on all the spectrum chunks
   * \param rnti the RNTI of the tranmitting device
   * \param numSym size of the transport block that generated the report in
            number of OFDM symbols
   * \param tbSize size of the transport block that generated the report in bytes
   * \param mcs the MCS of the transmission
   */
  void McsSinrCallback(const SpectrumValue& sinr, uint16_t rnti, uint8_t numSym, uint32_t tbSize, uint8_t mcs);

private:
  std::string m_filename; //!< filename for the output
  std::ofstream m_outputFile; //!< output file

};

}
}

#endif
