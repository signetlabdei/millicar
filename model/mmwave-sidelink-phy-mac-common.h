/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef SRC_MMWAVE_MODEL_SIDELINK_PHY_MAC_COMMON_H
#define SRC_MMWAVE_MODEL_SIDELINK_PHY_MAC_COMMON_H

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <deque>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/string.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/component-carrier.h>
#include <ns3/enum.h>
#include <memory>


namespace ns3 {

namespace mmwave {

struct SidelinkSfnSf
{
  SidelinkSfnSf () = default;

  SidelinkSfnSf (uint16_t frameNum, uint8_t sfNum, uint16_t slotNum, uint8_t varTtiNum)
    : m_frameNum (frameNum),
    m_subframeNum (sfNum),
    m_slotNum (slotNum),
    m_varTtiNum (varTtiNum)
  {
  }

  uint64_t
  Encode () const
  {
    uint64_t ret = 0ULL;
    ret = (static_cast<uint64_t> (m_frameNum) << 32 ) |
      (static_cast<uint64_t> (m_subframeNum) << 24) |
      (static_cast<uint64_t> (m_slotNum) << 8) |
      (static_cast<uint64_t> (m_varTtiNum));
    return ret;
  }

  static uint64_t
  Encode (const SidelinkSfnSf &p)
  {
    uint64_t ret = 0ULL;
    ret = (static_cast<uint64_t> (p.m_frameNum) << 32 ) |
      (static_cast<uint64_t> (p.m_subframeNum) << 24) |
      (static_cast<uint64_t> (p.m_slotNum) << 8) |
      (static_cast<uint64_t> (p.m_varTtiNum));
    return ret;
  }

  void
  Decode (uint64_t sfn)
  {
    m_frameNum    = (sfn & 0x0000FFFF00000000) >> 32;
    m_subframeNum = (sfn & 0x00000000FF000000) >> 24;
    m_slotNum     = (sfn & 0x0000000000FFFF00) >> 8;
    m_varTtiNum   = (sfn & 0x00000000000000FF);
  }

  static SidelinkSfnSf
  FromEncoding (uint64_t sfn)
  {
    SidelinkSfnSf ret;
    ret.m_frameNum    = (sfn & 0x0000FFFF00000000) >> 32;
    ret.m_subframeNum = (sfn & 0x00000000FF000000) >> 24;
    ret.m_slotNum     = (sfn & 0x0000000000FFFF00) >> 8;
    ret.m_varTtiNum   = (sfn & 0x00000000000000FF);
    return ret;
  }

  SidelinkSfnSf
  IncreaseNoOfSlots (uint32_t slotsPerSubframe, uint32_t subframesPerFrame) const
  {
    return IncreaseNoOfSlotsWithLatency (1, slotsPerSubframe, subframesPerFrame);
  }

  SidelinkSfnSf
  CalculateUplinkSlot (uint32_t ulSchedDelay, uint32_t slotsPerSubframe, uint32_t subframesPerFrame) const
  {
    return IncreaseNoOfSlotsWithLatency (ulSchedDelay, slotsPerSubframe, subframesPerFrame);
  }

  SidelinkSfnSf
  IncreaseNoOfSlotsWithLatency (uint32_t latency, uint32_t slotsPerSubframe, uint32_t subframesPerFrame) const
  {
    SidelinkSfnSf retVal = *this;
    // currently the default value of L1L2 latency is set to 2 and is interpreted as in the number of slots
    // will be probably reduced to order of symbols
    retVal.m_frameNum += (this->m_subframeNum + (this->m_slotNum + latency) / slotsPerSubframe) / subframesPerFrame;
    retVal.m_subframeNum = (this->m_subframeNum + (this->m_slotNum + latency) / slotsPerSubframe) % subframesPerFrame;
    retVal.m_slotNum = (this->m_slotNum + latency) % slotsPerSubframe;
    return retVal;
  }

  /**
   * \brief Add to this SidelinkSfnSf a number of slot indicated by the first parameter
   * \param slotN Number of slot to add
   * \param slotsPerSubframe Number of slot per subframe
   * \param subframesPerFrame Number of subframes per frame
   */
  void
  Add (uint32_t slotN, uint32_t slotsPerSubframe, uint32_t subframesPerFrame)
  {
    m_frameNum += (m_subframeNum + (m_slotNum + slotN) / slotsPerSubframe) / subframesPerFrame;
    m_subframeNum = (m_subframeNum + (m_slotNum + slotN) / slotsPerSubframe) % subframesPerFrame;
    m_slotNum = (m_slotNum + slotN) % slotsPerSubframe;
  }

  /**
   * \brief operator < (less than)
   * \param rhs other SidelinkSfnSf to compare
   * \return true if this SidelinkSfnSf is less than rhs
   *
   * The comparison is done on m_frameNum, m_subframeNum, and m_slotNum: not on varTti
   */
  bool operator < (const SidelinkSfnSf& rhs) const
  {
    if (m_frameNum < rhs.m_frameNum)
      {
        return true;
      }
    else if ((m_frameNum == rhs.m_frameNum ) && (m_subframeNum < rhs.m_subframeNum))
      {
        return true;
      }
    else if (((m_frameNum == rhs.m_frameNum ) && (m_subframeNum == rhs.m_subframeNum)) && (m_slotNum < rhs.m_slotNum))
      {
        return true;
      }
    else
      {
        return false;
      }
  }

  /**
   * \brief operator ==, compares only frame, subframe, and slot
   * \param o other instance to compare
   * \return true if this instance and the other have the same frame, subframe, slot
   *
   * Used in the MAC operation, in which the varTti is not used (or, it is
   * its duty to fill it).
   *
   * To check the varTti, please use this operator and IsTtiEqual()
   */
  bool operator == (const SidelinkSfnSf &o) const
  {
    return (m_frameNum == o.m_frameNum) && (m_subframeNum == o.m_subframeNum)
           && (m_slotNum == o.m_slotNum);
  }

  /**
   * \brief Compares frame, subframe, slot, and varTti
   * \param o other instance to compare
   * \return true if this instance and the other have the same frame, subframe, slot, and varTti
   *
   * Used in PHY or in situation where VarTti is needed.
   */
  bool IsTtiEqual (const SidelinkSfnSf &o) const
  {
    return (*this == o) && (m_varTtiNum == o.m_varTtiNum);
  }

  uint16_t m_frameNum   {0}; //!< Frame Number
  uint8_t m_subframeNum {0}; //!< SubFrame Number
  uint16_t m_slotNum    {0}; //!< Slot number (a slot is made by 14 symbols)
  uint8_t m_varTtiNum   {0}; //!< Equivalent to symStart: it is the symbol in which the SidelinkSfnSf starts
};

struct SidelinkTbInfoElement
{
  SidelinkTbInfoElement () :
    m_isUplink (0), m_varTtiIdx (0), m_rbBitmap (0), m_rbShift (0), m_rbStart (
      0), m_rbLen (0), m_symStart (0), m_numSym (0), m_resAlloc (0), m_mcs (
      0), m_tbSize (0), m_ndi (0), m_rv (0)
  {
  }

  bool m_isUplink;              // is uplink grant?
  uint8_t m_varTtiIdx;          // var tti index
  uint32_t m_rbBitmap;          // Resource Block Group bitmap
  uint8_t m_rbShift;            // shift for res alloc type 1
  uint8_t m_rbStart;            // starting RB index for uplink res alloc type 0
  uint16_t m_rbLen;
  uint8_t m_symStart;           // starting symbol index for flexible TTI scheme
  uint8_t m_numSym;             // number of symbols for flexible TTI scheme
  uint8_t m_resAlloc;           // resource allocation type
  uint8_t m_mcs;
  uint32_t m_tbSize;
  uint8_t m_ndi;
  uint8_t m_rv;
};

/**
 * \brief Scheduling information. Despite the name, it is not TDMA.
 */
struct SidelinkDciInfoElementTdma
{
  enum DciFormat
  {
    DL = 0,
    UL = 1
  };

  /**
   * \brief The VarTtiType enum
   */
  enum VarTtiType
  {
    CTRL_DATA = 0, //!< Not used anywhere
    DATA = 1,      //!< Used for DL/UL DATA
    CTRL = 2       //!< Used for DL/UL CTRL
  };

  /**
   * \brief Constructor used in MmWaveUePhy to build local DCI for DL and UL control
   * \param symStart Sym start
   * \param numSym Num sym
   * \param rbgBitmask Bitmask of RBG
   */
  SidelinkDciInfoElementTdma (uint8_t symStart, uint8_t numSym, DciFormat format, VarTtiType type,
                      const std::vector<uint8_t> &rbgBitmask)
    : m_format (format),
    m_symStart (symStart),
    m_numSym (numSym),
    m_type (type),
    m_rbgBitmask (rbgBitmask)
  {
  }

  /**
   * \brief Construct to build brand new DCI. Please remember to update manually
   * the HARQ process ID and the RBG bitmask
   *
   * \param rnti
   * \param format
   * \param symStart
   * \param numSym
   * \param mcs
   * \param tbs
   * \param ndi
   * \param rv
   */
  SidelinkDciInfoElementTdma (uint16_t rnti, DciFormat format, uint8_t symStart,
                      uint8_t numSym, uint8_t mcs, uint32_t tbs, uint8_t ndi,
                      uint8_t rv, VarTtiType type)
    : m_rnti (rnti), m_format (format), m_symStart (symStart),
    m_numSym (numSym), m_mcs (mcs), m_tbSize (tbs), m_ndi (ndi), m_rv (rv), m_type (type)
  {
  }


  /**
   * \brief Copy constructor except for some values that have to be overwritten
   * \param symStart Sym start
   * \param numSym Num sym
   * \param ndi New Data Indicator: 0 for Retx, 1 for New Data
   * \param rv Retransmission value
   * \param o Other object from which copy all that is not specified as parameter
   */
  SidelinkDciInfoElementTdma (uint8_t symStart, uint8_t numSym, uint8_t ndi, uint8_t rv,
                      const SidelinkDciInfoElementTdma &o)
    : m_rnti (o.m_rnti),
      m_format (o.m_format),
      m_symStart (symStart),
      m_numSym (numSym),
      m_mcs (o.m_mcs),
      m_tbSize (o.m_tbSize),
      m_ndi (ndi),
      m_rv (rv),
      m_type (o.m_type),
      m_harqProcess (o.m_harqProcess),
      m_rbgBitmask (o.m_rbgBitmask)
  {
  }

  const uint16_t m_rnti       {0};
  const DciFormat m_format    {DL};
  const uint8_t m_symStart    {0};   // starting symbol index for flexible TTI scheme
  const uint8_t m_numSym      {0};   // number of symbols for flexible TTI scheme
  const uint8_t m_mcs         {0};
  const uint32_t m_tbSize     {0};
  const uint8_t m_ndi         {0};   // By default is retransmission
  const uint8_t m_rv          {0};   // not used for UL DCI
  const VarTtiType m_type     {CTRL_DATA};
  uint8_t m_harqProcess {0};
  std::vector<uint8_t> m_rbgBitmask  {};   //!< RBG mask: 0 if the RBG is not used, 1 otherwise
};

struct SidelinkTbAllocInfo
{
  SidelinkTbAllocInfo () :
    m_rnti (0)
  {

  }
  //struct
  SidelinkSfnSf m_SidelinkSfnSf;
  uint16_t m_rnti;
  std::vector<unsigned> m_rbMap;
  SidelinkTbInfoElement m_tbInfo;
};

struct VarTtiAllocInfo
{
  VarTtiAllocInfo (const VarTtiAllocInfo &o) = default;

  VarTtiAllocInfo (const std::shared_ptr<SidelinkDciInfoElementTdma> &dci)
    : m_dci (dci)
  {
  }

  bool m_isOmni           {false};
  std::shared_ptr<SidelinkDciInfoElementTdma> m_dci;

  bool operator < (const VarTtiAllocInfo& o) const
  {
    NS_ASSERT (m_dci != nullptr);
    return (m_dci->m_symStart < o.m_dci->m_symStart);
  }
};

struct SidelinkSlotAllocInfo
{

  SidelinkSlotAllocInfo ()
  {
  }

  SidelinkSlotAllocInfo (SidelinkSfnSf sfn)
    : m_sidelinkSfnSf (sfn)
  {
  }

  /**
   * \brief Enum which indicates the allocations that are inside the allocation info
   */
  enum AllocationType
  {
    NONE = 0, //!< No allocations
    DL   = 1,  //!< DL Allocations
    UL   = 2,  //!< UL Allocations
    BOTH = 3 //!< DL and UL allocations
  };

  /**
   * \brief Merge the input parameter to this SidelinkSlotAllocInfo
   * \param other SidelinkSlotAllocInfo to merge in this allocation
   *
   * After the merge, order the allocation by symStart in DCI
   */
  void Merge (const SidelinkSlotAllocInfo & other);

  SidelinkSfnSf m_sidelinkSfnSf          {};     //!< SidelinkSfnSf of this allocation
  uint32_t m_numSymAlloc {0};    //!< Number of allocated symbols
  std::deque<VarTtiAllocInfo> m_varTtiAllocInfo; //!< queue of allocations
  AllocationType m_type {NONE}; //!< Allocations type

  /**
   * \brief operator < (less than)
   * \param rhs other SidelinkSlotAllocInfo to compare
   * \return true if this SidelinkSlotAllocInfo is less than rhs
   *
   * The comparison is done on SidelinkSfnSf
   */
  bool operator < (const SidelinkSlotAllocInfo& rhs) const;
};

class MmWaveSidelinkPhyMacCommon : public Object
{
public:
  MmWaveSidelinkPhyMacCommon (void);

  ~MmWaveSidelinkPhyMacCommon (void);

  // inherited from Object
  virtual void DoInitialize (void);

  virtual void DoDispose (void);

  static TypeId GetTypeId (void);

  Time GetSymbolPeriod (void) const;

  uint8_t GetSymbolsPerSlot (void) const;

  Time GetSlotPeriod () const;

  uint32_t GetVarTtisPerSlot (void) const;

  uint32_t GetSubframesPerFrame (void) const;

  uint32_t GetSlotsPerSubframe (void) const;

  uint32_t GetNumReferenceSymbols (void);

  uint32_t GetNumScsPerRb (void) const;

  double GetSubcarrierSpacing (void) const;

  uint32_t GetNumRefScPerRb (void) const;

  uint32_t GetBandwidthInRbg () const;

  // for TDMA, number of reference subcarriers across entire bandwidth (default to 1/4th of SCs)
  uint32_t GetNumRefScPerSym (void) const;

  uint32_t GetNumRbPerRbg (void) const;

  uint32_t GetNumerology (void) const;

  double GetBandwidth (void) const;

  /*
   * brief: bandwidth in number of RBs
   */
  uint32_t GetBandwidthInRbs () const;

  double GetCenterFrequency (void) const;

  uint32_t GetTbDecodeLatency (void) const;

  uint32_t GetMaxTbSize (void) const;

  void SetSymbolPeriod (double prdSym);

  void SetSymbolsPerSlot (uint8_t numSym);

  void SetSlotPeriod (double period);

  void SetVarTtiPerSlot (uint32_t numVarTti);

  void SetSubframePerFrame (uint32_t numSf);

  void SetNumReferenceSymbols (uint32_t refSym);

  void SetNumScsPrRb (uint32_t numScs);

  void SetNumRefScPerRb (uint32_t numRefSc);

  void SetRbNum (uint32_t numRB);

  /*
   * brief
   * rbgSize size of RBG in number of resource blocks
   */
  void SetNumRbPerRbg (uint32_t rbgSize);

  void SetNumerology (uint32_t numerology);

  /*
   * brief Set bandwidth value in Hz
   * param bandwidth the bandwidth value in Hz
   */
  void SetBandwidth (double bandwidth);

  void SetCentreFrequency (double fc);

  void SetTbDecodeLatency (uint32_t us);

  void SetMaxTbSize (uint32_t bytes);

private:
  Time m_symbolPeriod;
  uint8_t m_symbolsPerSlot;
  Time m_slotPeriod;
  uint32_t m_fixedTtisPerSlot;   // TODO: check if this is obsolete attribute
  uint32_t m_slotsPerSubframe;   // TODO: perform parameter cleanup, leave only mandatory ones, many redundant settings
  uint32_t m_subframesPerFrame;
  uint32_t m_numRefSymbols;
  uint32_t m_numRbPerRbg;
  uint16_t m_numerology;
  double m_subcarrierSpacing;
  uint32_t m_rbNum;   // replaced nyu chunk
  uint32_t m_numRefScPerRb;
  uint32_t m_numSubCarriersPerRb;
  double m_centerFrequency;
  double m_bandwidth;
  bool m_bandwidthConfigured;
  uint32_t m_tbDecodeLatencyUs;
  uint32_t m_maxTbSizeBytes;
  std::string m_staticTddPattern;
};

}

}

#endif /* SRC_MMWAVE_MODEL_SIDELINK_PHY_MAC_COMMON_H_ */
