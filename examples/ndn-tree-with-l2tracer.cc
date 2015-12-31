// ndn-simple-withl2tracer.cc

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 10);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-tree-25-node.txt");
  topologyReader.Read ();
  
  /****************************************************************************/
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
 // ccnxHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::EntropyStrategy");
  //ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  ccnxHelper.InstallAll ();
  /****************************************************************************/
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ccnxGlobalRoutingHelper;
  ccnxGlobalRoutingHelper.InstallAll ();
  /****************************************************************************/
  // Getting containers for the consumer/producer
  Ptr<Node> consumer1 = Names::Find<Node> ("Src1");
  Ptr<Node> consumer2 = Names::Find<Node> ("Src2");
  Ptr<Node> consumer3 = Names::Find<Node> ("Src3");
  Ptr<Node> consumer4 = Names::Find<Node> ("Src4");
  Ptr<Node> consumer5 = Names::Find<Node> ("Src5");
  Ptr<Node> consumer6 = Names::Find<Node> ("Src6");
  Ptr<Node> consumer7 = Names::Find<Node> ("Src7");
  Ptr<Node> consumer8 = Names::Find<Node> ("Src8");
  Ptr<Node> consumer9 = Names::Find<Node> ("Src9");

  Ptr<Node> producer1 = Names::Find<Node> ("Dst1");
  Ptr<Node> producer2 = Names::Find<Node> ("Dst2");
  Ptr<Node> producer3 = Names::Find<Node> ("Dst3");
  Ptr<Node> producer4 = Names::Find<Node> ("Dst4");
  Ptr<Node> producer5 = Names::Find<Node> ("Dst5");
  Ptr<Node> producer6 = Names::Find<Node> ("Dst6");
  Ptr<Node> producer7 = Names::Find<Node> ("Dst7");
  Ptr<Node> producer8 = Names::Find<Node> ("Dst8");
  Ptr<Node> producer9 = Names::Find<Node> ("Dst9");
  /****************************************************************************/
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1000"));//interests per Second
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  /****************************************************************************/
  // on the first to ninth consumer node install a Consumer application
  // that will express interests in /dst1 to /dst9 namespace
  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer1);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer2);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer3);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer4);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer5);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer6);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer7);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer8);

  consumerHelper.SetPrefix ("/dst");
  consumerHelper.Install (consumer9);
  
  /****************************************************************************/
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  /****************************************************************************/
  // Register /dst1 to /dst9 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 to /dst9 namespace
  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer1);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer1);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer2);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer2);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer3);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer3);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer4);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer4);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer5);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer5);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer6);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer6);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer7);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer7);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer8);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer8);

  ccnxGlobalRoutingHelper.AddOrigins ("/dst", producer9);
  producerHelper.SetPrefix ("/dst");
  producerHelper.Install (producer9);

  /*****************************************************************************/
  // Calculate and install FIBs
  ccnxGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (10.0));

  /****************************************************************************/
  //Tracer:

  L2RateTracer::InstallAll ("/home/wangjiawei/experimentResult/results/drop-trace.txt", Seconds (0.5));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
