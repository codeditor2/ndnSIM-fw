/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "random-strategy.h"

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



#define EPSILON 0.000001 
namespace ll = boost::lambda;

//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.RandomStrategy");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (RandomStrategy);

LogComponent RandomStrategy::g_log = LogComponent (RandomStrategy::GetLogName ().c_str ());



//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.DeviationStrategy".
//When we want to see the log of this model, use NS_LOG=ndn.fw.DeviationStrategy please.
std::string
RandomStrategy::GetLogName ()
{
  return super::GetLogName ()+".RandomStrategy";
}


TypeId
RandomStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::RandomStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <RandomStrategy> ()
    ;
  return tid;
}

RandomStrategy::RandomStrategy ()
{
  NS_LOG_INFO ("intiate RandomStrategy");
}

bool
RandomStrategy::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  typedef fib::FaceMetricContainer::type::index<fib::i_metric>::type FacesByMetric;
  FacesByMetric &faces = pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ();
  FacesByMetric::iterator faceIterator = faces.begin ();

  int propagatedCount = 0;
  int faceCount = 1;
  int selectFace = 1;
  int i;

  // calculate the alternative faces number
  while (faceIterator != faces.end ()) {
      faceCount ++;
      faceIterator ++;
  }

  // generate a random number between 1 and faceCount
  // srand((unsigned)time(NULL));
//  SeedManager::SetSeed(time(NULL));
  UniformVariable x (1, faceCount);
  selectFace = x.GetValue();
  
  // forward
  faceIterator = faces.begin ();
  for(i=1; i <= faceCount && faceIterator != faces.end (); i++) {
      if ( i == selectFace ) {
	  if (TrySendOutInterest (inFace, faceIterator->GetFace (), interest, pitEntry))
	  {
	    propagatedCount ++;
// 	    std::cout << "selectFace\t" << selectFace << "\tFaceId\t" << faceIterator->GetFace()->GetId() << "\n";
	  }
      }
      faceIterator ++;
  }
  
  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
