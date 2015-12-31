/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "optimal-strategy.h"

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

namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (OptimalRoute);

LogComponent OptimalRoute::g_log = LogComponent (OptimalRoute::GetLogName ().c_str ());

std::string
OptimalRoute::GetLogName ()
{
  return super::GetLogName ()+".OptimalRoute";
}


TypeId
OptimalRoute::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::OptimalRoute")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <OptimalRoute> ()
    ;
  return tid;
}

OptimalRoute::OptimalRoute ()
{
}

bool
OptimalRoute::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << interest->GetName ());

  
  UniformVariable x (0.0,1.0);
  double p = x.GetValue ();
  double weightSum = 0.0;
  double per_probability=  0.0;

  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      weightSum += 1.0/metricFace.GetWeight();
    }
    

  int propagatedCount = 0;

  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      per_probability += 1.0/(weightSum*metricFace.GetWeight());
      if(per_probability < p)
      {
	continue;
      }

      if (TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
        {	  
          pitEntry->GetFibEntry ()->IncreaseFacePI(metricFace.GetFace());
	  pitEntry->GetFibEntry ()->UpdateFaceWeight(metricFace.GetFace());
	  NS_LOG_INFO ("SeqNum\t" << interest->GetName ().get (-1).toSeqNum () << "\tFace\t" << metricFace.GetFace()->GetId());
// 	  std::cout << "SeqNum\t" << interest->GetName ().get (-1).toSeqNum () << "\tFace\t" << metricFace.GetFace()->GetId() << "\n";
//           std::cout << metricFace.GetFace()->GetId() << "\n";
        } else {
	   continue;
	}
      
      propagatedCount++;
      break; // do only once
    }

  return propagatedCount > 0;
}

} // namespace fw
} // namespace ndn
} // namespace ns3
