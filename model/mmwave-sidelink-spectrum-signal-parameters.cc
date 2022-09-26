/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
*/

#include <ns3/log.h>
#include <ns3/packet-burst.h>
#include <ns3/ptr.h>
#include "mmwave-sidelink-spectrum-signal-parameters.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveSidelinkSpectrumSignalParameters");

namespace millicar {

MmWaveSidelinkSpectrumSignalParameters::MmWaveSidelinkSpectrumSignalParameters ()
{
  NS_LOG_FUNCTION (this);
}

MmWaveSidelinkSpectrumSignalParameters::MmWaveSidelinkSpectrumSignalParameters (const MmWaveSidelinkSpectrumSignalParameters& p)
  : SpectrumSignalParameters (p)
{
  NS_LOG_FUNCTION (this << &p);
  if (p.packetBurst)
    {
      packetBurst = p.packetBurst->Copy ();
    }
  //ctrlMsgList = p.ctrlMsgList;
  mcs = p.mcs;
  size = p.size;
  rbBitmap = p.rbBitmap;
  pss = p.pss;
  numSym = p.numSym;
  senderRnti = p.senderRnti;
  destinationRnti = p.destinationRnti;
}

Ptr<SpectrumSignalParameters>
MmWaveSidelinkSpectrumSignalParameters::Copy () const
{
  NS_LOG_FUNCTION (this);

  return Create<MmWaveSidelinkSpectrumSignalParameters> (*this);
}

}

}
