/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "mdpf3.h"

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

//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.MDPF3Strategy");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (MDPF3Strategy);

LogComponent MDPF3Strategy::g_log = LogComponent (MDPF3Strategy::GetLogName ().c_str ());


struct FaceMetricWithPIByStatus
{
  typedef FaceMetricWithPIContainer::type::index<i_status>::type type;
};


//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.MDPF3Strategy".
//When we want to see the log of this model, use NS_LOG=ndn.fw.MDPF3Strategy please.
std::string
MDPF3Strategy::GetLogName ()
{
  return super::GetLogName ()+".MDPF3Strategy";
}


TypeId
MDPF3Strategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::MDPF3Strategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <MDPF3Strategy> ()
    ;
  return tid;
}

MDPF3Strategy::MDPF3Strategy ()
{
//  NS_LOG_INFO ("intiate MDPF3Strategy");
}

bool
MDPF3Strategy::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  FaceMetricWithPIContainer::type FacesContainer;
  int propagatedCount = 0;
  UniformVariable x (0.0,1.0);
  double p = x.GetValue ();
  double per_probability=  0.0;
  double total_score = 0.0;
  
//  std::cout << "receive from " << inFace->GetId() << "\n";
  
//  std::cout << "alternative faces are:\n";
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
  {
    if(metricFace.GetFace() != inFace)
    {
 //       std::cout << metricFace.GetFace()->GetId() << "\t";
        FacesContainer.insert(FaceMetricWithPI( metricFace.GetFace(),metricFace.GetStatus(),metricFace.GetSRtt(),metricFace.GetRoutingCost(), metricFace.GetWeight() ) ); 
    }
  }
//  std::cout << "\n";
  
  //data standard pre-process
  if(FacesContainer.size() >= 1 )
  {
    int status_min = boost::lexical_cast<int>( FacesContainer.get<i_status>().begin()->GetStatus () );
    int status_max = boost::lexical_cast<int>( FacesContainer.get<i_status>().rbegin()->GetStatus () );
    
    int64_t srtt_min = FacesContainer.get<i_srtt>().begin()->GetSRtt().ToInteger(Time::NS);
    int64_t srtt_max = FacesContainer.get<i_srtt>().rbegin()->GetSRtt().ToInteger(Time::NS);
    
    double pi_min = FacesContainer.get<i_pi>().begin()->GetPI();
    double pi_max = FacesContainer.get<i_pi>().rbegin()->GetPI();
    
    int status_gap = (status_min == status_max) ? 1 : (status_max - status_min);
    int64_t srtt_gap = (srtt_min == srtt_max) ? 1 : (srtt_max - srtt_min);
    double pi_gap = (pi_min == pi_max) ? 1.0 : (pi_max - pi_min);
    
    double efficacy_coefficient = 0.95;
    
    int FaceNum = 0;   
    double status_vector[10];
    double sRtt_vector[10];
    double PI_vector[10];
    
// (3)
    for (FaceMetricWithPIByStatus::type::iterator FaceIteratorByStatus = FacesContainer.get<i_status> ().begin();
       FaceIteratorByStatus !=  FacesContainer.get<i_status> ().end (); FaceIteratorByStatus ++)
    {
      double status_formal = ( status_max -  boost::lexical_cast<int>( FaceIteratorByStatus->GetStatus ()) ) * efficacy_coefficient / 
	status_gap + (1.0-efficacy_coefficient);
      
      double m_sRtt_formal = ( srtt_max -   FaceIteratorByStatus->GetSRtt ().ToInteger(Time::NS) ) * efficacy_coefficient / 
	srtt_gap + (1.0-efficacy_coefficient);

      double m_pi_formal = (pi_max - FaceIteratorByStatus->GetPI()) * efficacy_coefficient / pi_gap + (1.0-efficacy_coefficient);
	
      status_vector[FaceNum] = status_formal;
      sRtt_vector[FaceNum] = m_sRtt_formal;
      PI_vector[FaceNum] = m_pi_formal;
      FaceNum ++;
	
      FacesContainer.modify (FaceIteratorByStatus, ll::bind (&FaceMetricWithPI::SetStatusFormal, ll::_1, status_formal));
      FacesContainer.modify (FaceIteratorByStatus, ll::bind (&FaceMetricWithPI::SetSRttFormal, ll::_1, m_sRtt_formal));
      FacesContainer.modify (FaceIteratorByStatus, ll::bind (&FaceMetricWithPI::SetPIFormal, ll::_1, m_pi_formal));
    }
    
    double v1 = 0.0;
    for(int i = 0; i < FaceNum-1; i ++)
    {
        for(int j = i+1; j < FaceNum; j ++)
	{
	    v1 += fabs(status_vector[i] - status_vector[j]);
	}
    }
    
    double v2 = 0.0;
    for(int i = 0; i < FaceNum-1; i ++)
    {
        for(int j = i+1; j < FaceNum; j ++)
	{
	    v2 += fabs(sRtt_vector[i] - sRtt_vector[j]);
	}
    }
    
    double v3 = 0.0;
    for(int i = 0; i < FaceNum-1; i ++)
    {
        for(int j = i+1; j < FaceNum; j ++)
	{
	    v3 += fabs(PI_vector[i] - PI_vector[j]);
	}
    }
    
    double status_weight = 0.0;
    double m_sRtt_weight = 0.0;
    double m_pi_weight = 0.0;
    double v_sum = v1 + v2 + v3;
    
    if(v_sum != 0) {
	status_weight = v1 / v_sum;
	m_sRtt_weight = v2 / v_sum;
	m_pi_weight = v3 / v_sum;
    } else {
	status_weight = 1.0;
    }
 
//    std::cout << "weight1\t" << status_weight << "\tweight2\t" << m_sRtt_weight << "\tweight3\t" << m_pi_weight << "\n";
    
// (5)
    Ptr<Face> outFace = NULL;
    
    for (FaceMetricWithPIByStatus::type::iterator FaceIteratorByStatus = FacesContainer.get<i_status> ().begin();
       FaceIteratorByStatus !=  FacesContainer.get<i_status> ().end (); FaceIteratorByStatus ++)
    {
      double score = status_weight * FaceIteratorByStatus->GetStatusFormal() + m_sRtt_weight * FaceIteratorByStatus->GetSRttFormal() + m_pi_weight * FaceIteratorByStatus->GetPIFormal();
      total_score += score;
      FacesContainer.modify (FaceIteratorByStatus, ll::bind (&FaceMetricWithPI::SetScore, ll::_1, score)); 
//      std::cout << FaceIteratorByStatus->GetFace()->GetId() << " score \t" << score << "\t";
    } 
//    std::cout << "\n";
    
    BOOST_FOREACH (const FaceMetricWithPI &FaceIteratorByScore, FacesContainer.get<i_score>())
    {
      per_probability += FaceIteratorByScore.GetScore() / total_score; //modified
      if(per_probability < p)
      {
	continue;
      }
      outFace = FaceIteratorByScore.GetFace();
      if (TrySendOutInterest (inFace, outFace, interest, pitEntry))
      {
	  pitEntry->GetFibEntry ()->IncreaseFacePI(outFace);
	  pitEntry->GetFibEntry ()->UpdateFaceWeight(outFace);
//	  std::cout << "forward by " << outFace->GetId() << "\n";
	  propagatedCount++;
	  break; // do only once;
      }
    }     
    
  }
//  std::cout << "\n\n";
  return propagatedCount > 0;
}


} // namespace fw
} // namespace ndn
} // namespace ns3
