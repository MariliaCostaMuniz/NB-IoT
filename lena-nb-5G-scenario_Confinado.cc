/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC), 
 * Copyright (c) 2022 Communication Networks Institute at TU Dortmund University
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
 * Authors: Jaume Nin <jaume.nin@cttc.cat>
 *          Manuel Requena <manuel.requena@cttc.es>
 *          Tim Gebauer <tim.gebauer@tu-dortmund.de> (NB-IoT Extension)
 *          Pascal Jörke <pascal.joerke@tu-dortmund.de>
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/lte-module.h"
#include <ns3/winner-plus-propagation-loss-model.h>
//#include "ns3/gtk-config-store.h"
#include <chrono>
#include <iomanip>
#include <stdlib.h>
#include <ctime>    
#include <fstream>
# define M_PIl          3.141592653589793238462643383279502884L /* pi */
using namespace ns3;




/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeBs,
 * attaches one UE per eNodeB starts a flow for each UE to and from a remote host.
 * It also starts another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("LenaNb5G");

int
main (int argc, char *argv[])
{
  Time simTime = Minutes(5); // 
  uint64_t ues_to_consider = 0;

  uint8_t worker = 0;
  int seed = 1;
  int apv = 5;// area por vaca
  std::string simName = "test";
  double cellsize = 6000; // in meters
  int num_ues_app_a = 21;
  //int num_ues_app_b = 2;
  //int num_ues_app_c = 3;
  int packetsize_app_a = 20; // 32 Bytes 5G mMTC payload + 4 Bytes CoAP Header + 13 Bytes DTLS Header // UDP Header and IP Header  are added by NS-3 20
  //int packetsize_app_b = 49;
  //int packetsize_app_c = 49;
  Time packetinterval_app_a = Seconds(37);
  //Time packetinterval_app_b = Days(1);
  //Time packetinterval_app_c = Days(1);
  int A_tot = apv * num_ues_app_a; // Area total= area por vaca* n_de_vacas
  int radius = ceil(sqrt(A_tot/M_PIl));// raio da celula
  bool ciot = false;
  bool edt = false;
  // Command line arguments
  CommandLine cmd (__FILE__);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("simName", "Total duration of the simulation", simName);
  cmd.AddValue ("worker", "worker id when using multithreading to not confuse logging", worker);
  cmd.AddValue ("randomSeed", "randomSeed",seed);
  cmd.AddValue ("numUeAppA", "Number of UEs for Application A",num_ues_app_a);
 // cmd.AddValue ("numUeAppB", "Number of UEs for Application B",num_ues_app_b);
  //cmd.AddValue ("numUeAppC", "Number of UEs for Application C",num_ues_app_c);
  cmd.AddValue ("ciot", "Cellular IoT Optimization",ciot);
  cmd.AddValue ("edt", "Early Data Transmission",edt);
  cmd.Parse (argc, argv);
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->EnableRrcLogging ();
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetUeAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::WinnerPlusPropagationLossModel")); // Note that the Winner+ pathloss model isn't available in the current release of ns3. It can be downloaded at https://github.com/tudo-cni/ns3-propagation-winner-plus
  lteHelper->SetPathlossModelAttribute ("HeightBasestation", DoubleValue (15));//altura da enb;
  lteHelper->SetPathlossModelAttribute ("Environment", EnumValue (UMaEnvironment));//RMA
  lteHelper->SetPathlossModelAttribute ("LineOfSight", BooleanValue (true));// com linha de visada;
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));


  // Calculate UES to consider
  ues_to_consider = num_ues_app_a;
  //std::cout << "UEs to consider: " << ues_to_consider<< std::endl;

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));// tem que calcular
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));// perguntar
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer enbNodes;
  enbNodes.Create (1);
  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 15.0)); // Place our single eNb right in the center of the cell

  MobilityHelper mobilityEnb;
  mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityEnb.SetPositionAllocator(positionAlloc);
  mobilityEnb.Install(enbNodes);



  /*
  For all scenarios, 3*X minutes of simulation time are simulated, but only the intermediate X minutes are evaluated.
  The first X minutes produce no significant results since devices at the beginning are scheduled in an empty cell and experience
  very good transmission conditions. After X minutes, new devices will find ongoing transmissions of previous devices, which enables
  a more realistic situation and produces significant results. Since devices that have started transmissions within the intermediate X
  minutes of the simulation may not complete their transmissions in this intermediate time slot, additional X minutes are simulated
  with more new transmissions to keep the channels busy and let the intermediate devices complete their transmissions.
  */
  NodeContainer ueNodes;
  ueNodes.Create (ues_to_consider*3); // Pre-Run, Run, Post-Run.
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();

  for (uint32_t j = 0; j<3; j++){ // Pre-Run, Run, Post-Run.
    // Instalando mobilidade 
    ObjectFactory pos_a;
    pos_a.SetTypeId ("ns3::UniformDiscPositionAllocator");
    pos_a.Set ("X", DoubleValue (0.0));
    pos_a.Set ("Y", DoubleValue (0.0));
    pos_a.Set ("Z", DoubleValue (1.5));
    pos_a.Set ("rho", DoubleValue (radius));
    Ptr<PositionAllocator> m_position = pos_a.Create ()->GetObject<PositionAllocator> ();
    for (uint32_t i = 0; i < num_ues_app_a; ++i){
      Vector position = m_position->GetNext ();
      positionAllocUe->Add (position);
      //std::cout << "a," << position.x << "," << position.y << "," << position.z << std::endl;
    }
    
  }
  
  MobilityHelper mobilityUe;
  mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe.SetPositionAllocator(positionAllocUe);
  mobilityUe.Install (ueNodes);



  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  RngSeedManager::SetSeed (seed);
  Ptr<UniformRandomVariable> RaUeUniformVariable = CreateObject<UniformRandomVariable> ();


  // Install and start applications on UEs and remote host
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  
  
  // Set up the data transmission for the Pre-Run
  for (uint16_t i = 0; i < ues_to_consider; i++)
    {
      int access = RaUeUniformVariable->GetInteger (50, simTime.GetMilliSeconds());
      lteHelper->AttachSuspendedNb(ueLteDevs.Get(i), enbLteDevs.Get(0));

      Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();
      Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();
      if(ciot == true){
        //std::cout << "ciot" << std::endl;
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(false));
      }
      if(edt == true){
        //std::cout << "EDT" << std::endl;
        ueRrc->SetAttribute("EDT", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("EDT", BooleanValue(false));
      }

      ++ulPort;
      UdpEchoServerHelper server (ulPort);
      serverApps.Add(server.Install (remoteHost));
      //
      // Create a UdpEchoClient application to send UDP datagrams from node zero to
      // node one.
      //
      if (i < num_ues_app_a){
        uint packetsize = packetsize_app_a;
        UdpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (packetinterval_app_a));
        ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));

        serverApps.Get(i)->SetStartTime (MilliSeconds (access));
        clientApps.Get(i)->SetStartTime (MilliSeconds (access));
      }
     
    }


  // Set up the data transmission for the UEs to be considered in the results
  for (uint16_t i = ues_to_consider; i < ues_to_consider*2; i++)
    {
      int access = RaUeUniformVariable->GetInteger (simTime.GetMilliSeconds(), 2*simTime.GetMilliSeconds());
      lteHelper->AttachSuspendedNb(ueLteDevs.Get(i), enbLteDevs.Get(0));

      Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();
      Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();
      ueRrc->EnableLogging();
      if(ciot == true){
        //std::cout << "ciot" << std::endl;
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(false));
      }
      if(edt == true){
        //std::cout << "EDT" << std::endl;
        ueRrc->SetAttribute("EDT", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("EDT", BooleanValue(false));
      }

      ++ulPort;
      UdpEchoServerHelper server (ulPort);
      serverApps.Add(server.Install (remoteHost));
      //
      // Create a UdpEchoClient application to send UDP datagrams from node zero to
      // node one.
      //

      if (i < ues_to_consider+num_ues_app_a){
        uint packetsize = packetsize_app_a;
        UdpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (packetinterval_app_a));
        ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));

        serverApps.Get(i)->SetStartTime (MilliSeconds (access));
        clientApps.Get(i)->SetStartTime (MilliSeconds (access));
      }
     
    }
  


  // Set up the data transmission for the Post-Run
  for (uint16_t i = ues_to_consider*2; i < ues_to_consider*3; i++)
    {
      int access = RaUeUniformVariable->GetInteger (simTime.GetMilliSeconds()*2, simTime.GetMilliSeconds()*3);
      lteHelper->AttachSuspendedNb(ueLteDevs.Get(i), enbLteDevs.Get(0));

      Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();
      Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();
      if(ciot == true){
        //std::cout << "ciot" << std::endl;
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(false));
      }
      if(edt == true){
        //std::cout << "EDT" << std::endl;
        ueRrc->SetAttribute("EDT", BooleanValue(true));
      }
      else{
        ueRrc->SetAttribute("EDT", BooleanValue(false));
      }

      ++ulPort;
      UdpEchoServerHelper server (ulPort);
      serverApps.Add(server.Install (remoteHost));
      //
      // Create a UdpEchoClient application to send UDP datagrams from node zero to
      // node one.
      //
      if (i < ues_to_consider*2 + num_ues_app_a){
        uint packetsize = packetsize_app_a;
        UdpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (packetinterval_app_a));
        ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));

        serverApps.Get(i)->SetStartTime (MilliSeconds (access));
        clientApps.Get(i)->SetStartTime (MilliSeconds (access));
      }
      
    }




  auto start = std::chrono::system_clock::now(); 
  std::time_t start_time = std::chrono::system_clock::to_time_t(start);
  std::cout << "started computation at " << std::ctime(&start_time);
     std::string logdir = "logs/";
  std::string makedir = "mkdir -p ";
  std::string techdir = makedir;

  techdir += logdir;
  int z = std::system(techdir.c_str());
  std::cout << z;
  techdir += "/";
  techdir += simName;
  techdir += "/";
  z = std::system(techdir.c_str());
  std::cout << z;
  logdir += simName;
  logdir += "/";
  logdir += std::to_string(ueNodes.GetN());
  logdir += "_";
  logdir += std::to_string(simTime.GetInteger());
  logdir += "_";
  logdir += std::to_string(ciot);
  logdir += "_";
  logdir += std::to_string(edt);
  
  std::string top_dirmakedir = makedir+logdir; 
  int a = std::system(top_dirmakedir.c_str());
  std::cout << a << std::endl;
  logdir += "/";
  
  auto tm = *std::localtime(&start_time);
  std::stringstream ss;
  ss << std::put_time(&tm, "%d_%m_%Y_%H_%M_%S");
  logdir += ss.str();
  logdir += "_";
  logdir += std::to_string(worker);
  logdir += "_";
  logdir += std::to_string(seed);
  logdir += "_";  

  for (uint16_t i = 0; i < ueNodes.GetN(); i++){

    Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();
    Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();
    Ptr<LteUeMac> ueMac = ueLteDevice->GetMac();
    ueRrc->SetLogDir(logdir); // Will be changed to real ns3 traces later on. For now this logging is easier
    ueMac->SetLogDir(logdir); // Will be changed to real ns3 traces later on. For now this logging is easier

      }
  Ptr<LteEnbNetDevice> enbLteDevice = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>();
  Ptr<LteEnbRrc> enbRrc = enbLteDevice->GetRrc();
  enbRrc->SetLogDir(logdir);
  
  
  //lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-simple-epc");

  Simulator::Stop (3*simTime); // Pre-Run, Run, Post-Run
  Simulator::Run ();
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";
  Simulator::Destroy ();
  return 0;
}

