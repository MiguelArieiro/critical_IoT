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
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/energy-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Critical MCS Scenario");

bool verbose;

int32_t numSta = 300;                          //number of stations per AP
int32_t numAp = 10;                            //number of stations
int32_t numNodes = numSta * numAp + numAp + 1; //number of nodes

// Global variables for use in callbacks.
std::vector<double> signalDbmAvg(numNodes);
std::vector<double> noiseDbmAvg(numNodes);

std::vector<uint64_t> totalPacketsSent(numAp);
std::vector<uint64_t> totalPacketsThrough(numAp);
std::vector<double> throughput(numAp);
std::vector<uint64_t> packetsReceived(numNodes);
std::vector<uint64_t> packetsSent(numNodes);

std::vector<FILE *> signalDbmFile(numNodes);
std::vector<FILE *> noiseDbmFile(numNodes);
std::vector<FILE *> energyRemainingFile(numNodes);
std::vector<FILE *> energyConsumedFile(numNodes);

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

// Trace function for received packets and SNR
void MonitorSniffRx(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  int32_t nodeId = ContextToNodeId(context);
  packetsReceived[nodeId]++;
  signalDbmAvg[nodeId] += ((signalNoise.signal - signalDbmAvg[nodeId]) / packetsReceived[nodeId]);
  noiseDbmAvg[nodeId] += ((signalNoise.noise - noiseDbmAvg[nodeId]) / packetsReceived[nodeId]);

  fprintf(signalDbmFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), signalNoise.signal);
  fprintf(noiseDbmFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), signalNoise.noise);
}

// Trace function for sent packets
void MonitorSniffTx(std::string context, const Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu)
{
  int32_t nodeId = ContextToNodeId(context);
  packetsSent[nodeId]++;
}

// Trace function for remaining energy at node.
void RemainingEnergy(std::string context, double oldValue, double remainingEnergy)
{

  int32_t nodeId = std::stoi(context);
  /*/if (verbose)
  {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s " << nodeId << " Current remaining energy = " << remainingEnergy << "J");
  }*/
  fprintf(energyRemainingFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), remainingEnergy);
}

// Trace function for total energy consumption at node.
void TotalEnergy(std::string context, double oldValue, double totalEnergy)
{
  int32_t nodeId = std::stoi(context);
  /*if (verbose)
  {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s " << nodeId << " Total energy consumed by radio = " << totalEnergy << "J");
  }*/
  fprintf(energyConsumedFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), totalEnergy);
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
  double rxSensSta = -92;         // dBm
  double rxSensAp = -92;          // dBm
  int32_t TCPpayloadSize = 60;    // bytes
  int32_t UDPpayloadSize = 40;    // bytes
  int32_t mcs = 7;                // MCS value
  int64_t dataRate = 1000000;     // bits/s
  double obssPdThreshold = -82.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 5G = 1;
  double distance = 10;           // mobility model quadrant size
  int frequency = 5;              // frequency selection
  int channelWidth = 20;          // channel number
  int numRxSpatialStreams = 2;    // number of Rx Spatial Streams
  int numTxSpatialStreams = 2;    // number of Tx Spatial Streams
  int numAntennas = 2;            // number of Antenas
  bool voice = true;    //Do Voice

  // double TxCurrentA = 0.144;    // Transmission current (A)
  // double RxCurrentA = 0.088;    // Reception current (A)
  // double SleepCurrentA = 0.017; // Sleep current (A)
  // double IdleCurrentA = 0.020;  // Iddle current (A)

  CommandLine cmd(__FILE__);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  cmd.AddValue("dataRate", "Data rate (bits/s)", dataRate);
  cmd.AddValue("enableObssPd", "Enable/disable OBSS_PD", enableObssPd);
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
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 802.11n, 3 = 5G", technology);
  cmd.AddValue("distance", "Distance between networks", distance);
  cmd.AddValue("TCPpayloadSize", "TCP packet size", TCPpayloadSize);
  cmd.AddValue("UDPpayloadSize", "UDP packet size", UDPpayloadSize);
  cmd.AddValue("frequency", "Wifi device frequency. 2 - 2.4GHz, 5 - 5GHz, 6 - 6GHz", frequency);
  cmd.AddValue("channelWidth", "Defines wifi channel number", channelWidth);
  cmd.AddValue("numTxSpatialStreams", "Number of Tx Spatial Streams", numTxSpatialStreams);
  cmd.AddValue("numRxSpatialStreams", "Number of Rx Spatial Streams", numRxSpatialStreams);
  cmd.AddValue("numAntennas", "Number of Rx Spatial Streams", numAntennas);
  cmd.AddValue("voice", "Use Voice PTT", voice);
  // cmd.AddValue("TxCurrentA", "Transmission current (A)", TxCurrentA);
  // cmd.AddValue("RxCurrentA", "Reception current (A)", RxCurrentA);
  // cmd.AddValue("SleepCurrentA", "Sleep current (A)", SleepCurrentA);
  // cmd.AddValue("IdleCurrentA", "Iddle current (A)", IdleCurrentA);
  cmd.Parse(argc, argv);

  numNodes = numSta * numAp + numAp + 1; //updating numNodes

  // opening files
  FILE* nodeIDFile = fopen ("NodeID.txt","w+");
  std::string filename;
  for (int i = 0; i < numSta * numAp; i++)
  {
    filename = "STA_signalDBM_Node_" + std::to_string(i) + ".txt";
    signalDbmFile[i] = fopen(filename.c_str(), "w+");
    filename = "STA_signalDBM_Node_" + std::to_string(i) + ".txt";
    noiseDbmFile[i] = fopen(filename.c_str(), "w+");
    filename = "STA_energyRemaining_Node_" + std::to_string(i) + ".txt";
    energyRemainingFile[i] = fopen(filename.c_str(), "w+");
    filename = "STA_energyConsumed_Node_" + std::to_string(i) + ".txt";
    energyConsumedFile[i] = fopen(filename.c_str(), "w+");
  }
  for (int i = numSta * numAp; i < numNodes - 1; i++)
  {
    filename = "AP_signalDBM_Node_" + std::to_string(i) + ".txt";
    signalDbmFile[i] = fopen(filename.c_str(), "w+");
    filename = "AP_signalDBM_Node_" + std::to_string(i) + ".txt";
    noiseDbmFile[i] = fopen(filename.c_str(), "w+");
    filename = "AP_energyRemaining_Node_" + std::to_string(i) + ".txt";
    energyRemainingFile[i] = fopen(filename.c_str(), "w+");
    filename = "AP_energyConsumed_Node_" + std::to_string(i) + ".txt";
    energyConsumedFile[i] = fopen(filename.c_str(), "w+");
  }

  if (verbose)
  {
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(numSta * numAp);

  NodeContainer wifiApNodes;
  wifiApNodes.Create(numAp);

  NodeContainer csmaNodes;
  csmaNodes.Add(wifiApNodes);
  csmaNodes.Create(1); //BS: Isto cria o servidor ?

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("10000Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

  NetDeviceContainer csmaDevices;

  //spectrum definition
  /***************************************************************************/
  SpectrumWifiPhyHelper phy = SpectrumWifiPhyHelper::Default();
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
  spectrumChannel->AddPropagationLossModel(lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
  spectrumChannel->SetPropagationDelayModel(delayModel);

  phy.SetChannel(spectrumChannel);
  phy.SetErrorRateModel("ns3::YansErrorRateModel");
  phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");
  /***************************************************************************/

  phy.Set("Antennas", UintegerValue(numAntennas));
  phy.Set("MaxSupportedTxSpatialStreams", UintegerValue(numTxSpatialStreams));
  phy.Set("MaxSupportedRxSpatialStreams", UintegerValue(numRxSpatialStreams));

  WifiHelper wifi;
  //wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  //IEEE 802.11ax
  if (technology == 0)
  {
    switch (frequency)
    {
    case 2:
      wifi.SetStandard(WIFI_STANDARD_80211ax_2_4GHZ);
      break;
    case 5:
      wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);
      break;
    case 6:
      wifi.SetStandard(WIFI_STANDARD_80211ax_6GHZ);
      break;
    default:
      std::cout << "Wrong frequency." << std::endl;
      return 0;
    }

    std::ostringstream oss;
    oss << "HeMcs" << mcs;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(oss.str()),
                                 "ControlMode", StringValue(oss.str()));

    if (enableObssPd)
    {
      wifi.SetObssPdAlgorithm("ns3::ConstantObssPdAlgorithm",
                              "ObssPdLevel", DoubleValue(obssPdThreshold));
    }
  }

  //IEEE 802.11n
  else if (technology == 1)
  {
    switch (frequency)
    {
    case 2:
      wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
      break;
    case 5:
      wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);
      break;
    default:
      std::cout << "Wrong frequency." << std::endl;
      return 0;
    }

    std::ostringstream oss;
    oss << "HtMcs" << mcs;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(oss.str()),
                                 "ControlMode", StringValue(oss.str()));
  }
  else
  {
    // if no supported technology is selected
    return 0;
  }

  phy.Set("ChannelWidth", UintegerValue(channelWidth));

  WifiMacHelper mac;
  Ssid ssid;
  std::vector<NetDeviceContainer> StaDevices(numAp);
  NetDeviceContainer ApDevices = NetDeviceContainer();

  std::vector<Ptr<WifiNetDevice>> ap2Device(numAp);
  for (int32_t i = 0; i < numAp; i++)
  {
    ssid = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.

    StaDevices[i] = NetDeviceContainer();

    //STA creation
    phy.Set("TxPowerStart", DoubleValue(powSta));
    phy.Set("TxPowerEnd", DoubleValue(powSta));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    phy.Set("RxSensitivity", DoubleValue(rxSensSta));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    for (int32_t j = 0; j < numSta; j++)
    {
      StaDevices[i].Add(wifi.Install(phy, mac, wifiStaNodes.Get(i * numSta + j)));
    }

    //AP creation
    phy.Set("TxPowerStart", DoubleValue(powAp));
    phy.Set("TxPowerEnd", DoubleValue(powAp));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
    phy.Set("RxSensitivity", DoubleValue(rxSensAp));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    ApDevices.Add(wifi.Install(phy, mac, wifiApNodes.Get(i)));

    //Sets BSS color
    if ((technology == 0) && enableObssPd)
    {
      ap2Device[i] = ApDevices.Get(i)->GetObject<WifiNetDevice>();
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
    }
  }

  csmaDevices = csma.Install(csmaNodes);

  /** Configure mobility model **/
  MobilityHelper mobility;

  int32_t edge_size = (ceil(sqrt(numAp)));
  int32_t sta_edge_size = (ceil(sqrt(numSta)));
  int32_t counter = 0;
  for (int32_t y = 0; (y < edge_size) && (counter < numAp); y++)
  {
    for (int32_t x = 0; (x < edge_size) && (counter < numAp); x++, counter++)
    {
      //positionAlloc->Add(Vector((double)x*distance, (double)y*distance, 0.0));
      mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue((x - 1) * distance),
                                    "MinY", DoubleValue((y - 1) * distance),
                                    "DeltaX", DoubleValue(2 * distance / sta_edge_size),
                                    "DeltaY", DoubleValue(2 * distance / sta_edge_size),
                                    "GridWidth", UintegerValue(sta_edge_size),
                                    "LayoutType", StringValue("RowFirst"));

      mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                "Bounds", RectangleValue(Rectangle((x - 1) * distance - 1, (x + 1) * distance + 1, (y - 1) * distance - 1, (y + 1) * distance + 1)));
      for (int32_t j = 0; j < numSta; j++)
      {
        //std::cout << "x:" << x << " y:" << y << " distance:" << distance << "\n";
        mobility.Install(wifiStaNodes.Get(counter * numSta + j));
      }
    }
  }

  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0),
                                "MinY", DoubleValue(0),
                                "DeltaX", DoubleValue(2 * distance),
                                "DeltaY", DoubleValue(2 * distance),
                                "GridWidth", UintegerValue(edge_size),
                                "LayoutType", StringValue("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNodes);
  std::cout << "Mobility model configured\n";
  // Routing
  InternetStackHelper stack;

  Ipv4StaticRoutingHelper staticRoutingHelper;
  stack.Install(csmaNodes);
  stack.SetRoutingHelper(staticRoutingHelper);
  stack.Install(wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign(csmaDevices);

  address.SetBase("172.1.0.0", "255.255.0.0");
  for (int32_t i = 0; i < numAp; i++)
  {
    address.Assign(ApDevices.Get(i));
    address.Assign(StaDevices[i]);
    address.NewNetwork();
  }

  // static routing
  NodeContainer::Iterator iter;

  for (int32_t i = 0; i < numAp; i++)
  {
    Ptr<Ipv4StaticRouting> staticRouting;
    std::string wifiApIP = "172." + std::to_string(i + 1) + ".0.1";
    std::string csmaApIP = "10.1.1." + std::to_string(i + 1);
    for (int32_t j = 0; j < numSta; j++)
    {
      staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(wifiStaNodes.Get(i * numSta + j)->GetObject<Ipv4>()->GetRoutingProtocol());
      staticRouting->SetDefaultRoute(wifiApIP.c_str(), 1);
    }

    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(csmaNodes.Get(numAp)->GetObject<Ipv4>()->GetRoutingProtocol());
    //staticRouting->SetDefaultRoute ("10.1.1.2", 1);
    staticRouting->AddNetworkRouteTo(wifiApIP.c_str(), "255.255.0.0", csmaApIP.c_str(), 1);
  }

  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferTx", MakeCallback(&MonitorSniffTx));
  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));

  uint16_t pttPortServer = 2;
  uint16_t pttPortCli = 19;

  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  StringValue AtrOnTime, AtrOffTime;

  if (voice){
      std::cout << "Using voice PTT \n";
      PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), pttPortServer));
      serverApps.Add( packetSinkHelper.Install(wifiStaNodes) );


     for (int32_t i = 0; i < numAp; i++){
      for (int32_t j = 0; j < numSta; j++){
        //Station 0 sends the PTT
        ++pttPortCli;

        //PTT client on server
        Ptr<Ipv4> ipv4 = wifiStaNodes.Get (i*numSta + j)->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback
        std::cout <<"Install app in WiFi node " << (i*numSta + j) << " ip:" << iaddr.GetLocal() << " \n" ;

        OnOffHelper pttOnOff("ns3::UdpSocketFactory", Address(InetSocketAddress(iaddr.GetLocal(), pttPortServer)));
        AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
        AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
        pttOnOff.SetAttribute("OnTime", AtrOnTime); //2 em 2s ?
        pttOnOff.SetAttribute("OffTime", AtrOffTime );
        pttOnOff.SetAttribute("PacketSize", UintegerValue(200)); //200 bytes as per Fog evaluation
        pttOnOff.SetAttribute("DataRate", StringValue("56kbps")); //bit/s
        clientApps.Add( pttOnOff.Install(csmaNodes.Get(0)) );

      }
     }
  } 
  //server
  if (udp)
  {
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    serverApps.Add( packetSinkHelper.Install(csmaNodes.Get(numAp)) );
    
    //client
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), 9)));
    AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
    onoff.SetAttribute("OnTime", AtrOnTime);
    onoff.SetAttribute("OffTime", AtrOffTime);
    onoff.SetAttribute("PacketSize", UintegerValue(UDPpayloadSize));
    onoff.SetAttribute("DataRate", DataRateValue(dataRate)); //bit/s

    clientApps.Add( onoff.Install(wifiStaNodes) );
   
  }
  else
  {
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
    serverApps.Add ( packetSinkHelper.Install(csmaNodes.Get(numAp)) );

    //client
    OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), 5000)));
    AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=2]");
    onoff.SetAttribute("OnTime", AtrOnTime);
    onoff.SetAttribute("OffTime", AtrOffTime);
    onoff.SetAttribute("PacketSize", UintegerValue(TCPpayloadSize));
    onoff.SetAttribute("DataRate", DataRateValue(dataRate)); //bit/s

    clientApps.Add (onoff.Install(wifiStaNodes) );

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
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install(wifiStaNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.144));
  radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.088));
  radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.017));
  radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.00000426));
  radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(0.0017));
  radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(0.00000426));

  // install device model
  DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();

  for (int32_t i = 0; i < numAp; i++)
  {
    for (int32_t j = 0; j < numSta; j++)
    {
      deviceModels.Add(radioEnergyHelper.Install(StaDevices[i].Get(j), sources.Get(j + i * numSta)));
    }
  }

  /***************************************************************************/

  /** connect trace sources **/
  /***************************************************************************/
  //energy source
  for (int32_t i = 0; i < numAp * numSta; i++)
  {
    Ptr<LiIonEnergySource> basicSourcePtr = DynamicCast<LiIonEnergySource>(sources.Get(i));

      basicSourcePtr->TraceConnect("RemainingEnergy", std::to_string(i), MakeCallback(&RemainingEnergy));

    // device energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
    NS_ASSERT(basicRadioModelPtr != NULL);

      basicRadioModelPtr->TraceConnect("TotalEnergyConsumption", std::to_string(i), MakeCallback(&TotalEnergy));
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

  if (tracing == true)
  {
    phy.EnablePcap("critical_iot", ApDevices);
    csma.EnablePcap("critical_iot", csmaDevices, true);
  }

  std::cout << "code???\n";
  Simulator::Run();

  flowMonitor->SerializeToXmlFile("testflow.xml", true, true);

  std::cout << std::setw(10) << "index" << std::setw(15) << "Tput (Mb/s)" << std::setw(15) << "Signal (dBm)" << std::setw(15) << "Noise (dBm)" << std::setw(15) << "SNR (dB)" << std::setw(15) << "Packet loss" << std::setw(15) << "Packets through" << std::setw(15) << "Packets sent" << std::endl;

  for (int32_t i = 0; i < numAp; i++)
  {
    totalPacketsThrough[i] = packetsReceived[numNodes - numAp - 1 + i];

    if (udp)
    {
      //UDP
      //   totalPacketsThrough[i] = DynamicCast<UdpServer>(apDevice[i].Get(0))->GetReceived();
      throughput[i] = totalPacketsThrough[i] * UDPpayloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }
    else
    {
      //TCP
      // uint64_t totalBytesRx = DynamicCast<PacketSink>(apDevice[i].Get(0))->GetTotalRx();
      // totalPacketsThrough[i] = totalBytesRx / TCPpayloadSize;
      // throughput[i] = totalBytesRx * 8 / (duration * 1000000.0); //Mbit/s

      throughput[i] = totalPacketsThrough[i] * TCPpayloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }

    for (int32_t j = i * numSta; j < (1 + i) * numSta; j++)
    {
      totalPacketsSent[i] += packetsSent[j];
      //std::cout<<totalPacketsSent[i]<<std::endl;
    }
    std::cout << std::setw(10) << i + 1 << std::setw(15) << throughput[i] << std::setw(15) << signalDbmAvg[numSta * numAp + i] << std::setw(15) << noiseDbmAvg[numSta * numAp + i] << std::setw(15) << (signalDbmAvg[numSta * numAp + i] - noiseDbmAvg[numSta * numAp + i]) << std::setw(15) << (1 - (float)totalPacketsThrough[i] / (float)totalPacketsSent[i]) << std::setw(15) << totalPacketsThrough[i] << std::setw(15) << totalPacketsSent[i] << std::endl;
  }

  // for (int32_t j = 0; j < numNodes; j++)
  //   {

  //     std::cout<<packetsSent[j]<<"\t"<<packetsReceived[j]<<std::endl;
  //   }



  Simulator::Destroy();

  for (int32_t i = 0; i < numAp; i++)
  {
    Ptr<Ipv4StaticRouting> staticRouting;
    std::string wifiApIP = "172." + std::to_string(i + 1) + ".0.1";
    std::string csmaApIP = "10.1.1." + std::to_string(i + 1);
    fprintf (nodeIDFile, "AP, %d, %s\n", i*numSta, wifiApIP.c_str());
    fprintf (nodeIDFile, "AP, %d, %s\n", i*numSta, csmaApIP.c_str());
    for (int32_t j = 0; j < numSta; j++){
      std::string wifiApIP = "172." + std::to_string(i + 1) + ".0." + std::to_string(j + 2);
      fprintf (nodeIDFile, "STA, %d, %s\n", i*numSta+j, wifiApIP.c_str());
    }

  }
  std::string csmaApIP = "10.1.1." + std::to_string(numAp);
  fprintf (nodeIDFile, "Server, %d, %s\n", numAp*numSta, csmaApIP.c_str());

  for (int i = 0; i < numNodes - 1; i++)
  {
    fclose(signalDbmFile[i]);
    fclose(noiseDbmFile[i]);
    fclose(energyRemainingFile[i]);
    fclose(energyConsumedFile[i]);
  }
  fclose(nodeIDFile);
  return 0;
}
