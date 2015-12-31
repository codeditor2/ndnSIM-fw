/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "deviation2rtt.h"

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

//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.DeviationStrategy2RTT");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (DeviationStrategy2RTT);

LogComponent DeviationStrategy2RTT::g_log = LogComponent (DeviationStrategy2RTT::GetLogName ().c_str ());


//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

struct FaceMetricWithEntroyByStatus
{
  typedef FaceMetricWithEntroyContainer::type::index<i_status>::type 
  type;
};


//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.DeviationStrategy2RTT".
//When we want to see the log of this model, use NS_LOG=ndn.fw.DeviationStrategy2RTT please.
std::string
DeviationStrategy2RTT::GetLogName ()
{
  return super::GetLogName ()+".DeviationStrategy2RTT";
}


TypeId
DeviationStrategy2RTT::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::DeviationStrategy2RTT")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <DeviationStrategy2RTT> ()
    ;
  return tid;
}

DeviationStrategy2RTT::DeviationStrategy2RTT ()
{
//  NS_LOG_INFO ("intiate DeviationStrategy2RTT");
}

bool
DeviationStrategy2RTT::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  FaceMetricWithEntroyContainer::type m_faces_withEntroy;
  
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
  {
    m_faces_withEntroy.insert( FaceMetricWithEntroy( metricFace.GetFace(),metricFace.GetStatus(),metricFace.GetSRtt(),metricFace.GetRoutingCost()));
     
  }
  
  //data standard pre-process
  if( m_faces_withEntroy.size() > 1 )
  {
    int status_min = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().begin()->GetStatus () );
    int status_max = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().rbegin()->GetStatus () );
    
    int64_t srtt_min = m_faces_withEntroy.get<i_srtt>().begin()->GetSRtt().ToInteger(Time::NS);
    int64_t srtt_max = m_faces_withEntroy.get<i_srtt>().rbegin()->GetSRtt().ToInteger(Time::NS);
    
//     double pi_min = m_faces_withEntroy.get<i_pi>().begin()->GetPI();
//     double pi_max = m_faces_withEntroy.get<i_pi>().rbegin()->GetPI();
    
    int status_gap = (status_min == status_max) ? 1 : (status_max - status_min);
    int64_t srtt_gap = (srtt_min == srtt_max) ? 1 : (srtt_max - srtt_min);
//     double pi_gap = (pi_min == pi_max) ? 1.0 : (pi_max - pi_min);
    
    double efficacy_coefficient = 0.95;
    
    double status_sum = 0.0;
    double m_sRtt_sum = 0.0;
//     double m_pi_sum = 0.0;
    
    
// (3)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
      double status_formal = ( status_max -  boost::lexical_cast<int>( metricFacewithEntroy->GetStatus ()) ) * efficacy_coefficient / 
	status_gap + (1.0-efficacy_coefficient);
      
      double m_sRtt_formal = ( srtt_max -   metricFacewithEntroy->GetSRtt ().ToInteger(Time::NS) ) * efficacy_coefficient / 
	srtt_gap + (1.0-efficacy_coefficient);

//       double m_pi_formal = (pi_max - metricFacewithEntroy->GetPI()) * efficacy_coefficient / pi_gap + (1.0-efficacy_coefficient);
	
      status_sum += status_formal;
      m_sRtt_sum += m_sRtt_formal;
//       m_pi_sum += m_pi_formal;
	
      m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetStatusFormal, ll::_1, status_formal));
      m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetSRttFormal, ll::_1, m_sRtt_formal));
//       m_faces_withEntroy.modify (metricFacewithEntroy, ll::bind (&FaceMetricWithEntroy::SetPIFormal, ll::_1, m_pi_formal));
    }
    
 
    double v1_sum = 0.0;
    double v2_sum = 0.0;
//     double v3_sum = 0.0;
	
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
	//
	double v_i1 = fabs(m_faces_withEntroy.size()*(metricFacewithEntroy->GetStatusFormal()) - status_sum);
	v1_sum += v_i1;
	  
	double v_i2 = fabs(m_faces_withEntroy.size()*(metricFacewithEntroy->GetSRttFormal()) - m_sRtt_sum);
	v2_sum += v_i2;
	
// 	double v_i3 = fabs(m_faces_withEntroy.size()*(metricFacewithEntroy->GetPIFormal()) - m_pi_sum);
// 	v3_sum += v_i3;
    }   
    
    double status_weight = 0.0;
    double m_sRtt_weight = 0.0;
//     double m_pi_weight = 0.0;
    
    if(v1_sum + v2_sum != 0) {
	status_weight = v1_sum / (v1_sum + v2_sum);
	m_sRtt_weight = v2_sum / (v1_sum + v2_sum );
// 	m_pi_weight = v3_sum / (v1_sum + v2_sum);
    } else {
	status_weight = 1.0;
	m_sRtt_weight = 0.0;
// 	m_pi_weight = 0.0;
    }
    NS_LOG_INFO("v1=" << status_weight << ",v2=" << m_sRtt_weight);

// (5)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
//       double score = status_weight * metricFacewithEntroy->GetStatusFormal() + m_sRtt_weight * metricFacewithEntroy->GetSRttFormal() + m_pi_weight * metricFacewithEntroy->GetPIFormal();
      double score = status_weight * metricFacewithEntroy->GetStatusFormal() + m_sRtt_weight * metricFacewithEntroy->GetSRttFormal();
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
//     NS_LOG_INFO("error =======total_score====="<<total_score<<"=============error");
    BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
    {
      //NS_LOG_DEBUG ("Trying " << boost::cref(metricFacewithEntroy));
      //because we use score as first choice to select face, so this may lead to the fact that red face rank high in some special case.
      if (metricFacewithEntroy.GetStatus () == fib::FaceMetric::NDN_FIB_RED)
	  continue;
      if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
	  {
	    continue;
	  }

	propagatedCount++;
//         std::cout << metricFacewithEntroy.GetFace()->GetId() << "\n";
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

//       NS_LOG_INFO ("SeqNum\t" << interest->GetName ().get (-1).toSeqNum () << "\tFace\t" << metricFacewithEntroy.GetFace()->GetId());
//       std::cout << "probability\t" << p << "\tFaceId\t" << metricFacewithEntroy.GetFace()->GetId() << "\n";
//       std::cout << metricFacewithEntroy.GetFace()->GetId() << "\n";
      propagatedCount++;
      break; // do only once;
    }
    
  }


  
  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
