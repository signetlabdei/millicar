/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
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

#ifndef MMWAVE_SIDELINK_SPECTRUM_SIGNAL_PARAMETERS_H
#define MMWAVE_SIDELINK_SPECTRUM_SIGNAL_PARAMETERS_H

#include <ns3/spectrum-signal-parameters.h>

namespace ns3 {

class PacketBurst;

namespace mmwave {

class MmWaveSidelinkControlMessage;

struct MmwaveSidelinkSpectrumSignalParameter : public SpectrumSignalParameters
{

  // inherited from SpectrumSignalParameters
  virtual Ptr<SpectrumSignalParameters> Copy ();

  /**
  * default constructor
  */
  MmwaveSidelinkSpectrumSignalParameters ();

  /**
  * copy constructor
  */
  MmwaveSidelinkSpectrumSignalParameters (const MmwaveSidelinkSpectrumSignalParameters& p);

  Ptr<PacketBurst> packetBurst;

  std::list<Ptr<MmWaveSidelinkControlMessage>> ctrlMsgList;

  uint8_t slotInd;

  bool pss;

};

} // namespace mmwave

}  // namespace ns3


#endif /* MMWAVE_SIDELINK_SPECTRUM_SIGNAL_PARAMETERS_H */
