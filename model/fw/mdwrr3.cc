/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "mdwrr3.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/core-module.h"

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <math.h>

#include <stdlib.h>
#include <time.h>



//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.MDWRR3Strategy");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (MDWRR3Strategy);

LogComponent MDWRR3Strategy::g_log = LogComponent (MDWRR3Strategy::GetLogName ().c_str ());

//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.MDWRR3Strategy".
//When we want to see the log of this model, use NS_LOG=ndn.fw.MDWRR3Strategy please.
std::string
MDWRR3Strategy::GetLogName ()
{
  return super::GetLogName ()+".MDWRR3Strategy";
}


TypeId
MDWRR3Strategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::MDWRR3Strategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <MDWRR3Strategy> ()
    ;
  return tid;
}

MDWRR3Strategy::MDWRR3Strategy ()
{
//  NS_LOG_INFO ("intiate MDWRR3Strategy");
}

bool
MDWRR3Strategy::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  double tmp;
  int propagatedCount = 0;
  double max_weight = 0.0;	// ????
  Ptr<Face> outFace = NULL;
  Ptr<fib::Entry> fibEntry = pitEntry->GetFibEntry();
  
  fibEntry->UpdateAllCurrentWeight();
  std::cout << "Update all current weight\n";
  
  BOOST_FOREACH (const fib::FaceMetric &faceMetric, fibEntry->m_faces.get<fib::i_metric> ())
  {
    if(faceMetric.GetFace() != inFace)
    {
      tmp = faceMetric.GetCurrentWeight();
      std::cout << tmp << "\n";
      if(max_weight < tmp)
      {
	max_weight = tmp;
	outFace = faceMetric.GetFace();
      }
    }
  }
  
  std::cout << "Trying to send a Interest\n";
  if(outFace)
  {
    std::cout << outFace->GetId() << "\n";
    if(TrySendOutInterest(inFace, outFace, interest, pitEntry))
    {
      fibEntry->UpdateCurrentWeight(outFace);
      propagatedCount ++;
    }
    std::cout << "forward by " << outFace->GetId() << "\n";
  }

  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
