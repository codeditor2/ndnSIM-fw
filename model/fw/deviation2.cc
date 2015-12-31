/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "deviation2.h"

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

//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.DeviationStrategy2");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (DeviationStrategy2);

LogComponent DeviationStrategy2::g_log = LogComponent (DeviationStrategy2::GetLogName ().c_str ());


//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

struct FaceMetricWithEntroyByStatus
{
  typedef FaceMetricWithEntroyContainer::type::index<i_status>::type 
  type;
};


//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.DeviationStrategy2".
//When we want to see the log of this model, use NS_LOG=ndn.fw.DeviationStrategy2 please.
std::string
DeviationStrategy2::GetLogName ()
{
  return super::GetLogName ()+".DeviationStrategy2";
}


TypeId
DeviationStrategy2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::DeviationStrategy2")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <DeviationStrategy2> ()
    ;
  return tid;
}

DeviationStrategy2::DeviationStrategy2 ()
{
//  NS_LOG_INFO ("intiate DeviationStrategy2");
}

bool
DeviationStrategy2::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
//   NS_LOG_INFO ("Begin solving forwarding");
  FaceMetricWithEntroyContainer::type m_faces_withEntroy;
  
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
  {
    m_faces_withEntroy.insert( FaceMetricWithEntroy( metricFace.GetFace(),metricFace.GetStatus(),metricFace.GetSRtt(),metricFace.GetRoutingCost(), metricFace.GetWeight() ) );    
  }
  
//   NS_LOG_DEBUG ("the interest is " << *interest<<" the size of fib_with_entroy = "<<m_faces_withEntroy.size());
//   NS_LOG_DEBUG ("the interest is " << *interest<<" the size of fib = "<<pitEntry->GetFibEntry ()->m_faces.size());
  
  //data standard pre-process
  if( m_faces_withEntroy.size() > 1 )
  {
    int status_min = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().begin()->GetStatus () );
    int status_max = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().rbegin()->GetStatus () );
    
    double pi_min = m_faces_withEntroy.get<i_pi>().begin()->GetPI();
    double pi_max = m_faces_withEntroy.get<i_pi>().rbegin()->GetPI();
    
    int status_gap = (status_min == status_max) ? 1 : (status_max - status_min);
    double pi_gap = (pi_min == pi_max) ? 1.0 : (pi_max - pi_min);
    
    double efficacy_coefficient = 0.95;
    
    double status_sum = 0.0;
    double m_pi_sum = 0.0;   
    
// (3)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
      double status_formal = ( status_max -  boost::lexical_cast<int>( metricFacewithEntroy->GetStatus ()) ) * efficacy_coefficient / 
	status_gap + (1.0-efficacy_coefficient);

      double m_pi_formal = (pi_max - metricFacewithEntroy->GetPI()) * efficacy_coefficient / pi_gap + (1.0-efficacy_coefficient);
	
      status_sum += status_formal;
      m_pi_sum += m_pi_formal;
	
      m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetStatusFormal, ll::_1, status_formal));
      m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetPIFormal, ll::_1, m_pi_formal));
    }
    
 
    double v1_sum = 0.0;
    double v3_sum = 0.0;
	
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
	//
	double v_i1 = fabs(m_faces_withEntroy.size()*(metricFacewithEntroy->GetStatusFormal()) - status_sum);
	v1_sum += v_i1;
	
	double v_i3 = fabs(m_faces_withEntroy.size()*(metricFacewithEntroy->GetPIFormal()) - m_pi_sum);
	v3_sum += v_i3;
    }
    
    double status_weight = 0.0;
//     double m_sRtt_weight = 0.0;
    double m_pi_weight = 0.0;
    
    if(v1_sum + v3_sum != 0) {
        status_weight = v1_sum / (v1_sum + v3_sum);
	m_pi_weight = v3_sum / (v1_sum + v3_sum);
    } else {
	status_weight = 1.0;
	m_pi_weight = 0.0;
    }

// (5)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
      double score = status_weight * metricFacewithEntroy->GetStatusFormal() + m_pi_weight * metricFacewithEntroy->GetPIFormal();
     
      m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetScore, ll::_1, score));
    } 
    
  }
  
  int propagatedCount = 0;
  
  UniformVariable x (0.0,1.0);
  double p =x.GetValue ();
  double total_score = 0.0;
  double per_probability=  0.0;
  
  double gamma = 3;
  double delta = 1; 
  
  // 
  BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
  {  
    total_score+= pow(metricFacewithEntroy.GetScore (),gamma) * pow(1.0/(metricFacewithEntroy.GetRoutingCost ()+1),delta) ; //modified  
  }
  
  if(fabs( total_score - 0.0) < EPSILON)
  {
    BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
    {
      if (metricFacewithEntroy.GetStatus () == fib::FaceMetric::NDN_FIB_RED)
	  continue;
      if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
	  {
	    continue;
	  }

	propagatedCount++;
//        std::cout << metricFacewithEntroy.GetFace()->GetId() << "\n";
	break; // do only once
      
    }
    
  }
  
  else
  {
    
    BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
    {

      per_probability += pow(metricFacewithEntroy.GetScore (),gamma) * pow(1.0/(metricFacewithEntroy.GetRoutingCost ()+1),delta) / total_score; //modified
      if(per_probability < p)
      {
	continue;
      }
      
      if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
      {
	continue;
      }
      pitEntry->GetFibEntry ()->IncreaseFacePI(metricFacewithEntroy.GetFace());
      pitEntry->GetFibEntry ()->UpdateFaceWeight(metricFacewithEntroy.GetFace());
      NS_LOG_INFO ("SeqNum\t" << interest->GetName ().get (-1).toSeqNum () << "\tFace\t" << metricFacewithEntroy.GetFace()->GetId());
//       std::cout << metricFacewithEntroy.GetFace()->GetId() << "\n";
//       std::cout << "SeqNum\t" << interest->GetName ().get (-1).toSeqNum () << "\tFace\t" << metricFacewithEntroy.GetFace()->GetId() << "\n";
//       ndn::Name::const_iterator iterator = interest->GetName ().begin();
//       iterator ++;
//       while (iterator != interest->GetName ().end() )
//       {
// 	NS_LOG_INFO("name\t" << iterator->toSeqNum());
// 	iterator ++;
//       }
      
      propagatedCount++;
      break; // do only once;
    }
    
  }

  
  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
