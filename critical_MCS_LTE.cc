/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/config-store.h" //added
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
//#include "ns3/internet-apps-module.h" //added
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
//#include "ns3/yans-wifi-helper.h"
//#include "ns3/ssid.h"
#include "ns3/multi-model-spectrum-channel.h"
//#include "ns3/spectrum-wifi-helper.h"
//#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-helper.h"
//#include "ns3/flow-monitor-module.h" //Added
#include "ns3/energy-module.h"
#include "ns3/nr-module.h" //For NR 5G
#include "ns3/lte-module.h" // For LTE
#include "ns3/point-to-point-module.h" //LTE?

//#include "ns3/config-store-module.h" //Added
//#include "ns3/olsr-helper.h"
//#include "ns3/wifi-module.h"

/*
Based on the following examples:
 Wifi-he-network (802.11ax)
 Wifi-spatial-reuse (802.11ax)

 lena-simple-epc (LTE)
 lena-intercell-interference (LTE)

 cttc-nr-demo (NR 5G)
*/

/*
topology

ue
ue    enb1
ue    enbn      PGW   RemoteHost

*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CriticalMCSScenario_LTE");

bool verbose;

int32_t numSta = 25;                          //number of stations per AP
int32_t numAp = 2;                            //number of stations
int32_t numNodes = numSta * numAp + numAp + 1; //number of nodes


// Global variables for use in callbacks.
std::vector<double> signalDbmAvg(numNodes);
std::vector<double> noiseDbmAvg(numNodes);

std::vector<uint64_t> totalPacketsSent(numAp);
std::vector<uint64_t> totalPacketsThrough(numAp);
std::vector<double> throughput(numAp);
std::vector<uint64_t> packetsReceived(numNodes);
std::vector<uint64_t> packetsSent(numNodes);

FILE * FIsignal;
//FILE * FInoise;
FILE * FIenergyRemaining;
FILE * FIenergyConsumed;
/*
std::vector<FILE *> signalDbmFile(numNodes); //BS_
std::vector<FILE *> noiseDbmFile(numNodes);
std::vector<FILE *> energyRemainingFile(numNodes);
std::vector<FILE *> energyConsumedFile(numNodes);
*/

int64_t lastTimeRemaining = 0;
int64_t lastTimeTotal = 0;


/***************************************************************************/
// Trace functions
/***************************************************************************/
// Trace function for node id extraction
int32_t ContextToNodeId(std::string context)
{
  std::string sub = context.substr(10);
  int32_t pos = sub.find("/Device");
  return atoi(sub.substr(0, pos).c_str());
}


// Trace function for remaining energy at node.
void RemainingEnergy(std::string context, double oldValue, double remainingEnergy)
{

  int32_t nodeId = std::stoi(context);

  //fprintf(energyRemainingFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), remainingEnergy);
  if ((Simulator::Now().GetMilliSeconds() - lastTimeRemaining) >= 50 ){
    fprintf(FIenergyRemaining, "%lf,%d,%lf\n", Simulator::Now().GetSeconds(), nodeId, remainingEnergy);
    lastTimeRemaining = Simulator::Now().GetMilliSeconds();
  }
}

// Trace function for total energy consumption at node.
void TotalEnergy(std::string context, double oldValue, double totalEnergy)
{
  int32_t nodeId = std::stoi(context);

  //fprintf(energyConsumedFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), totalEnergy);
  if ((Simulator::Now().GetMilliSeconds() - lastTimeTotal) >= 50 ){
    fprintf(FIenergyConsumed, "%lf,%d,%lf\n", Simulator::Now().GetSeconds(),nodeId, totalEnergy);
    lastTimeTotal = Simulator::Now().GetMilliSeconds() ;
  }
}
/***************************************************************************/

int main(int argc, char *argv[])
{
  bool tracing = false;
  double duration = 10.0;         // seconds
  double powSta = 10.0;           // dBm
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -62;        // dBm
  double ccaEdTrAp = -62;         // dBm
  double rxSensSta = -84;         // dBm
  double rxSensAp = -102;          // dBm
  int32_t TCPpayloadSize = 60;    // bytes
  int32_t UDPpayloadSize = 40;    // bytes
  int32_t mcs = 7;                // MCS value
  int64_t dataRate = 1000000;     // bits/s
  //double obssPdThreshold = -72.0; // dBm
  //bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 802.11n =1, 5G = 2;
  double distance = 10;           // mobility model quadrant size
  int frequency = 5;              // frequency selection
  int channelWidth = 20;          // channel number
  int numRxSpatialStreams = 2;    // number of Rx Spatial Streams
  int numTxSpatialStreams = 2;    // number of Tx Spatial Streams
  int numAntennas = 2;            // number of Antenas
  bool voice = true;              //Do Voice
  int powerMode = 1;              //Configure Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )

  bool useCa = true; //LTE
  double enbDist = 100.0;
  double radius = 50.0;
  double distUE = 10;

  // double TxCurrentA = 0.144;    // Transmission current (A)
  // double RxCurrentA = 0.088;    // Reception current (A)
  // double SleepCurrentA = 0.017; // Sleep current (A)
  // double IdleCurrentA = 0.020;  // Iddle current (A)

  CommandLine cmd(__FILE__);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  cmd.AddValue("dataRate", "Data rate (bits/s)", dataRate);

  cmd.AddValue("powSta", "Power of STA (dBm)", powSta);
  cmd.AddValue("powAp", "Power of AP (dBm)", powAp);
  cmd.AddValue("ccaEdTrSta", "CCA-ED Threshold of STA (dBm)", ccaEdTrSta);
  cmd.AddValue("ccaEdTrAp", "CCA-ED Threshold of AP (dBm)", ccaEdTrAp);
  cmd.AddValue("rxSensSta", "RX Sensitivity of STA (dBm)", rxSensSta);
  cmd.AddValue("rxSensAp", "RX Sensitivity of AP (dBm)", rxSensAp);
  cmd.AddValue("mcs", "The constant MCS value to transmit HE PPDUs", mcs);
  cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.AddValue("batteryLevel", "Initial energy level (J)", batteryLevel);
  cmd.AddValue("numAp", "Number of Wifi Access Points", numAp);
  cmd.AddValue("numSta", "Number of Wifi Stations per AP", numSta);
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 802.11n, 2 = LTE, 3 = 5G", technology);
  cmd.AddValue("distance", "Distance between networks", distance);
  cmd.AddValue("TCPpayloadSize", "TCP packet size", TCPpayloadSize);
  cmd.AddValue("UDPpayloadSize", "UDP packet size", UDPpayloadSize);
  cmd.AddValue("frequency", "Wifi device frequency. 2 - 2.4GHz, 5 - 5GHz, 6 - 6GHz", frequency);
  cmd.AddValue("channelWidth", "Defines wifi channel number", channelWidth);
  cmd.AddValue("numTxSpatialStreams", "Number of Tx Spatial Streams", numTxSpatialStreams);
  cmd.AddValue("numRxSpatialStreams", "Number of Rx Spatial Streams", numRxSpatialStreams);
  cmd.AddValue("numAntennas", "Number of Rx Spatial Streams", numAntennas);
  cmd.AddValue("voice", "Use Voice PTT", voice);
  //cmd.AddValue("obssPdThreshold", "Use PD level for OBSS threshold", obssPdThreshold); //Default to -72
  cmd.AddValue("powerMode", "Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )", powerMode);


  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  cmd.Parse(argc, argv);
  std::cout << "ParseArgs " << std::endl;

  numNodes = numSta * numAp + numAp + 1; //updating numNodes


  if (verbose) FIsignal = fopen("FIsignal.csv", "w+");
  //FInoise = fopen("FInoise.csv", "w+");
  FIenergyConsumed = fopen("FIenergyConsumed.csv", "w+");
  FIenergyRemaining = fopen("FIenergyRemaining.csv", "w+");

  if (verbose)
  {
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  }

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
     Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (160));
   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
  std::cout << "LTE path loss model" << std::endl;
  // Uncomment to enable logging
  //lteHelper->EnableLogComponents ();

  //NodeContainer wifiStaNodes;
  //wifiStaNodes.Create(numSta * numAp);
  NodeContainer ueNodes;
  ueNodes.Create(numSta * numAp);

  //NodeContainer wifiApNodes;
  //wifiApNodes.Create(numAp);
  NodeContainer enbNodes;
  enbNodes.Create(numAp);

  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  std::cout << "packet GW" << std::endl;

  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
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

  //LTE Settings
  /*
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  lteHelper->SetSchedulerAttribute ("CqiTimerThreshold", UintegerValue (3));
  */
  NetDeviceContainer enbDevices;
  NetDeviceContainer ueDevices;
  enbDevices = lteHelper->InstallEnbDevice (enbNodes); //LTE Nodes
  std::cout << "install eNB s" << std::endl;
  ueDevices = lteHelper->InstallUeDevice (ueNodes);

  lteHelper->EnableRlcTraces();
  lteHelper->EnableMacTraces();
  
  // Activate a data radio bearer each UE
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  double auxenBdist =0, auxdistUE=0;
  MobilityHelper ueMobility;

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevices));

  for (int32_t i = 0; i < numAp; i++)
  {
    positionAlloc->Add (Vector (auxenBdist, 0.0, 0.0));

    for (int32_t j = 0; j < numSta; j++)
    {
      int32_t iue = i*numSta + j;
      auxdistUE = auxenBdist-distUE ;
      if (auxdistUE<0)auxdistUE=0;
      ueMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                        "X", DoubleValue (auxenBdist-distUE),
                                        "Y", DoubleValue (0.0),
                                        "rho", DoubleValue (radius));
      ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      ueMobility.Install(ueNodes.Get(iue));

      Ptr<Node> ueNode = ueNodes.Get (iue);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
      //NetDeviceContainer ueDevs1;
      //ueDevs1 = lteHelper->InstallUeDevice (ueNodes.Get(iue));
      //lteHelper->Attach (ueDevs1, enbDevices.Get (i));
      //lteHelper->ActivateDataRadioBearer (ueDevs1, bearer); //Attach data radio bearer to each UE
      //ueDevices.Add(ueDevs1);

    }
    auxenBdist+=enbDist;
  }
  std::cout << "After Position" << std::endl;
  //csmaDevices = csma.Install(csmaNodes);


  for (int32_t i = 0; i < numAp; i++)
  {
    for (int32_t j = 0; j < numSta; j++)
    {
      int32_t iue = i*numSta + j;
      lteHelper->Attach (ueDevices.Get(iue), enbDevices.Get(i));
    }
  }
  std::cout << "After Attaching UE to enB" << std::endl;

  /** Configure mobility model **/
  //MobilityHelper mobility;
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (positionAlloc);
  enbMobility.Install (enbNodes);



  //Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferTx", MakeCallback(&MonitorSniffTx));
  //Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));

  uint16_t pttPortServer = 2;
  uint16_t pttPortCli = 19;

  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  StringValue AtrOnTime, AtrOffTime;

  if (voice){ //As download
      std::cout << "Using voice PTT \n";
      PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), pttPortServer));
      serverApps.Add( packetSinkHelper.Install(ueNodes) );


     for (int32_t i = 0; i < numAp; i++){
      for (int32_t j = 0; j < numSta; j++){
        //Station 0 sends the PTT
        ++pttPortCli;

        //PTT client on server
        Ptr<Ipv4> ipv4 = ueNodes.Get (i*numSta + j)->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback
        std::cout <<"Install app in UE node " << (i*numSta + j) << " ip:" << iaddr.GetLocal() << " \n" ;

        OnOffHelper pttOnOff("ns3::UdpSocketFactory", Address(InetSocketAddress(iaddr.GetLocal(), pttPortServer)));
        AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
        AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
        pttOnOff.SetAttribute("OnTime", AtrOnTime); //2 em 2s ?
        pttOnOff.SetAttribute("OffTime", AtrOffTime );
        pttOnOff.SetAttribute("PacketSize", UintegerValue(200)); //200 bytes as per Fog evaluation
        pttOnOff.SetAttribute("DataRate", StringValue("1600kbps")); //bit/s
        clientApps.Add( pttOnOff.Install(remoteHost));
      }
     }
  }
  //server
  if (udp) //As uplink
  {
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    serverApps.Add( packetSinkHelper.Install(remoteHost) );

    //client
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(remoteHostAddr, 9)));
    AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
    onoff.SetAttribute("OnTime", AtrOnTime);
    onoff.SetAttribute("OffTime", AtrOffTime);
    onoff.SetAttribute("PacketSize", UintegerValue(UDPpayloadSize));
    onoff.SetAttribute("DataRate", DataRateValue(dataRate)); //bit/s

    clientApps.Add( onoff.Install(ueNodes) );

  }
  else //As uplink
  {
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
    serverApps.Add ( packetSinkHelper.Install(remoteHost) );

    //client
    OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(remoteHostAddr, 5000)));
    AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
    onoff.SetAttribute("OnTime", AtrOnTime);
    onoff.SetAttribute("OffTime", AtrOffTime);
    onoff.SetAttribute("PacketSize", UintegerValue(TCPpayloadSize));
    onoff.SetAttribute("DataRate", DataRateValue(dataRate)); //bit/s

    clientApps.Add (onoff.Install(ueNodes) );

  }


  //Start APPs
  serverApps.Start(Seconds(0));
  serverApps.Stop(Seconds(duration + 1));
  clientApps.Start(Seconds(1.0));
  clientApps.Stop(Seconds(duration + 1));

  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  LiIonEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set("LiIonEnergySourceInitialEnergyJ", DoubleValue(batteryLevel));
  basicSourceHelper.Set("InitialCellVoltage", DoubleValue(3.85)); //BS_

  // install source
  EnergySourceContainer sources = basicSourceHelper.Install(ueNodes);
  /* device energy model */
  //WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  /*
  switch (powerMode) { //BS_
    case 1: //apply the defaults of NS3
      radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.380));
      radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.313));
      radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.273));
      radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.033));
      radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(0.273));
      radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(0.273));
      break;
    case 2:
      //uint8_t mulfact = 2;
      radioEnergyHelper.Set("TxCurrentA", DoubleValue(2*0.380));
      radioEnergyHelper.Set("RxCurrentA", DoubleValue(2*0.313));
      radioEnergyHelper.Set("IdleCurrentA", DoubleValue(2*0.273));
      radioEnergyHelper.Set("SleepCurrentA", DoubleValue(2*0.033));
      radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(2*0.273));
      radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(2*0.273));
      break;
    case 0:
    default:
      radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.144));
      radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.088));
      radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.017));
      radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.00000426));
      radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(0.0017));
      radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(0.00000426));
  }
  */
  // install device model
  /* //TODO: Check Energy Model in LTE
  DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();

  for (int32_t i = 0; i < numAp; i++)
  {
    for (int32_t j = 0; j < numSta; j++)
    {

      deviceModels.Add(radioEnergyHelper.Install(ueDevices.Get(i*numSta + j),sources.Get(j + i * numSta) ));
    }
  }
  */
  /***************************************************************************/
  /** connect trace sources **/
  /***************************************************************************/
  //energy source
  for (int32_t i = 0; i < numAp * numSta; i++)
  {
    Ptr<LiIonEnergySource> basicSourcePtr = DynamicCast<LiIonEnergySource>(sources.Get(i));
    basicSourcePtr->TraceConnect("RemainingEnergy", std::to_string(i), MakeCallback(&RemainingEnergy));

    // device energy model
    //TODO:
    //Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
    //NS_ASSERT(basicRadioModelPtr != NULL);
    //basicRadioModelPtr->TraceConnect("TotalEnergyConsumption", std::to_string(i), MakeCallback(&TotalEnergy));
  }

  //Config::Connect("/NodeList/*/DeviceList/*/Phy/WifiRadioEnergyModel", MakeCallback(&TotalEnergy));
  //Config::Connect("/NodeList/*/DeviceList/*/Phy/LiIonEnergySource", MakeCallback(&RemainingEnergy));
  //flow monitor logging
  /**************************************************************************/
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(duration + 1));

  std::cout << "code???\n";
  Simulator::Run();

  flowMonitor->SerializeToXmlFile("testflow.xml", true, true);

  std::cout << std::setw(10) << "index" << std::setw(15) << "Tput (Mb/s)" << std::setw(15) << "Signal (dBm)" << std::setw(15) << "Noise (dBm)" << std::setw(15) << "SNR (dB)" << std::setw(15) << "Packet loss" << std::setw(15) << "Packets through" << std::setw(15) << "Packets sent" << std::endl;

  for (int32_t i = 0; i < numAp; i++)
  {
    totalPacketsThrough[i] = packetsReceived[numNodes - numAp - 1 + i];

    if (udp)
    {
      throughput[i] = totalPacketsThrough[i] * UDPpayloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }
    else
    {
      throughput[i] = totalPacketsThrough[i] * TCPpayloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }

    for (int32_t j = i * numSta; j < (1 + i) * numSta; j++)
    {
      totalPacketsSent[i] += packetsSent[j];
    }
    std::cout << std::setw(10) << i + 1 << std::setw(15) << throughput[i] << std::setw(15) << signalDbmAvg[numSta * numAp + i] << std::setw(15) << noiseDbmAvg[numSta * numAp + i] << std::setw(15) << (signalDbmAvg[numSta * numAp + i] - noiseDbmAvg[numSta * numAp + i]) << std::setw(15) << (1 - (float)totalPacketsThrough[i] / (float)totalPacketsSent[i]) << std::setw(15) << totalPacketsThrough[i] << std::setw(15) << totalPacketsSent[i] << std::endl;
  }


  for (int32_t j = 0; j < numNodes; j++)
  {
    std::cout<<j<<","<<packetsSent[j]<<","<<packetsReceived[j]<<std::endl;
  }

  Simulator::Destroy();

  if (verbose) fclose(FIsignal);
  fclose(FIenergyConsumed);
  fclose(FIenergyRemaining);
  return 0;
}
