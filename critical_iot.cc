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

NS_LOG_COMPONENT_DEFINE("Critical IoT Scenario");

uint32_t numSta = 2;                            //number of stations per AP
uint32_t numAp = 2;                             //number of stations
uint32_t numNodes = numSta * numAp + numAp + 1; //number of nodes

// Global variables for use in callbacks.
std::vector<double> signalDbmAvg(numNodes);
std::vector<double> noiseDbmAvg(numNodes);

std::vector<uint64_t> totalPacketsSent(numAp);
std::vector<uint64_t> totalPacketsThrough(numAp);
std::vector<double> throughput(numAp);
std::vector<uint64_t> packetsReceived(numNodes);
std::vector<uint64_t> packetsSent(numNodes);
/***************************************************************************/

// Trace functions
/***************************************************************************/
// Trace function for node id extraction
uint32_t ContextToNodeId(std::string context)
{
  std::string sub = context.substr(10);
  uint32_t pos = sub.find("/Device");
  return atoi(sub.substr(0, pos).c_str());
}

// Trace function for received packets and SNR
void MonitorSniffRx(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  uint32_t nodeId = ContextToNodeId(context);
  packetsReceived[nodeId]++;
  signalDbmAvg[nodeId] += ((signalNoise.signal - signalDbmAvg[nodeId]) / packetsReceived[nodeId]);
  noiseDbmAvg[nodeId] += ((signalNoise.noise - noiseDbmAvg[nodeId]) / packetsReceived[nodeId]);
}

// Trace function for sent packets
void MonitorSniffTx(std::string context, const Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu)
{
  uint32_t nodeId = ContextToNodeId(context);
  packetsSent[nodeId]++;
}

// Trace function for remaining energy at node.
void RemainingEnergy(double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                << "s Current remaining energy = " << remainingEnergy << "J");
}

// Trace function for total energy consumption at node.
void TotalEnergy(double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                << "s Total energy consumed by radio = " << totalEnergy << "J");
}
/***************************************************************************/

int main(int argc, char *argv[])
{
  bool verbose = true;
  bool tracing = false;
  double duration = 10.0;       // seconds
  double powSta = 10.0;         // dBm
  double powAp = 21.0;          // dBm
  double ccaEdTrSta = -62;      // dBm
  double ccaEdTrAp = -62;       // dBm
  uint32_t TCPpayloadSize = 60; // bytes
  uint32_t UDPpayloadSize = 40;   // bytes
  uint32_t mcs = 0; // MCS value
  // double interval = 0.001;        // seconds
  double obssPdThreshold = -82.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0; // technology to be used 802.11ax = 0, 5G = 1;

  CommandLine cmd(__FILE__);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  // cmd.AddValue("interval", "Inter packet interval (s)", interval);
  cmd.AddValue("enableObssPd", "Enable/disable OBSS_PD", enableObssPd);
  cmd.AddValue("powSta", "Power of STA (dBm)", powSta);
  cmd.AddValue("powAp", "Power of AP (dBm)", powAp);
  cmd.AddValue("ccaEdTrSta", "CCA-ED Threshold of STA (dBm)", ccaEdTrSta);
  cmd.AddValue("ccaEdTrAp", "CCA-ED Threshold of AP (dBm)", ccaEdTrAp);
  cmd.AddValue("mcs", "The constant MCS value to transmit HE PPDUs", mcs);
  cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.AddValue("batteryLevel", "Initial energy level (J)", batteryLevel);
  cmd.AddValue("numAp", "Number of Wifi Access Points", numAp);
  cmd.AddValue("numSta", "Number of Wifi Stations per AP", numSta);
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 5G, 2 = 802.11n 2.4GHz, 3 = 802.11n 5GHz", technology);
  cmd.Parse(argc, argv);

  cmd.Parse(argc, argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if ((numAp * numSta) > 18)
  {
    std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
    return 1;
  }

  if (verbose)
  {
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(numSta * numAp);

  NodeContainer wifiApNodes;
  wifiApNodes.Create(numAp);

  NodeContainer csmaNodes;
  csmaNodes.Add(wifiApNodes);
  csmaNodes.Create(1);

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install(csmaNodes);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  phy.SetErrorRateModel("ns3::YansErrorRateModel");
  phy.Set("Frequency", UintegerValue(5180)); // channel 36 at 20 MHz
  phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");

  WifiHelper wifi;
  //wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  if (technology == 0)
  {
    wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ); //define standard como 802.11ax 5GHz
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
  else if (technology == 2)
  {
    wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
  }
  else if (technology == 3)
  {
    wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);
  }
  else
  {
    // if no supported technology is selected
    return 0;
  }

  WifiMacHelper mac;
  Ssid ssid;
  std::vector<NetDeviceContainer> StaDevices(numAp);
  NetDeviceContainer ApDevices = NetDeviceContainer();

  std::vector<Ptr<WifiNetDevice>> ap2Device(numAp);
  for (uint32_t i = 0; i < numAp; i++)
  {
    ssid = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.

    StaDevices[i] = NetDeviceContainer();

    //STA creation
    phy.Set("TxPowerStart", DoubleValue(powSta));
    phy.Set("TxPowerEnd", DoubleValue(powSta));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    phy.Set("RxSensitivity", DoubleValue(-92.0));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    for (uint32_t j = 0; j < numSta; j++)
    {
      StaDevices[i].Add(wifi.Install(phy, mac, wifiStaNodes.Get(i * numSta + j)));
    }

    //AP creation
    phy.Set("TxPowerStart", DoubleValue(powAp));
    phy.Set("TxPowerEnd", DoubleValue(powAp));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
    phy.Set("RxSensitivity", DoubleValue(-92.0));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    ApDevices.Add(wifi.Install(phy, mac, wifiApNodes.Get(i)));

    //Sets BSS color
    if ((technology == 0) && enableObssPd)
    {
      ap2Device[i] = ApDevices.Get(i)->GetObject<WifiNetDevice>(); //TOFIX
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
    }
  }

  /** Configure mobility model **/
  MobilityHelper mobility;

  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(5.0),
                                "DeltaY", DoubleValue(10.0),
                                "GridWidth", UintegerValue(3),
                                "LayoutType", StringValue("RowFirst"));

  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
  mobility.Install(wifiStaNodes);

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNodes);

  // Routing
  InternetStackHelper stack;
  

  Ipv4StaticRoutingHelper staticRoutingHelper;
  stack.Install(csmaNodes);
  stack.SetRoutingHelper (staticRoutingHelper);
  stack.Install(wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign(csmaDevices);

  address.SetBase("172.1.1.0", "255.255.255.0");
  for (uint32_t i = 0; i < numAp; i++)
  {
    address.Assign(ApDevices.Get(i));
    address.Assign(StaDevices[i]);
    address.NewNetwork();
  }

//TODO fix this 
// static routing
  NodeContainer::Iterator iter;
  
  for (uint32_t i = 0; i < numAp; i++)
  {
    Ptr<Ipv4StaticRouting> staticRouting;
    std::string wifiApIP = "172.1." + std::to_string(i+1) + ".1";
    std::string csmaApIP = "10.1.1." + std::to_string(i+1);
    for (uint32_t j = 0; j < numSta; j++)
      {
        staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (wifiStaNodes.Get(i * numSta + j)->GetObject<Ipv4> ()->GetRoutingProtocol ());
        staticRouting->SetDefaultRoute (wifiApIP.c_str(), 1);
      }

      staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (csmaNodes.Get(numAp)->GetObject<Ipv4> ()->GetRoutingProtocol ());
      //staticRouting->SetDefaultRoute ("10.1.1.2", 1);
      staticRouting->AddNetworkRouteTo(wifiApIP.c_str(), "255.255.255.0", csmaApIP.c_str(), 1);
  }

  //server
  PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
  ApplicationContainer serverApps = packetSinkHelper.Install(csmaNodes.Get(numAp));
  serverApps.Start(Seconds(0));
  serverApps.Stop(Seconds(duration + 1));

  //client
  OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), 5000)));
  onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute("PacketSize", UintegerValue(TCPpayloadSize));
  onoff.SetAttribute("DataRate", DataRateValue(100000)); //bit/s

  ApplicationContainer clientApps = onoff.Install(wifiStaNodes);
  clientApps.Start(Seconds(1.0));
  clientApps.Stop(Seconds(duration + 1));


  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(batteryLevel));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install(wifiStaNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174)); //TODO set proper value
  // install device model
  DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();

  for (uint32_t i = 0; i < numAp; i++){
    for (uint32_t j = 0; j < numSta; j++){
      deviceModels.Add(radioEnergyHelper.Install(StaDevices[i].Get(j), sources.Get(j+i*numSta)));
    }
  }
  
  /***************************************************************************/

  /** connect trace sources **/
  /***************************************************************************/
  //energy source
  for (uint32_t i = 0; i < numAp * numSta; i++)
  {
    Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource>(sources.Get(i));

    if (tracing)
    {
      basicSourcePtr->TraceConnectWithoutContext("RemainingEnergy", MakeCallback(&RemainingEnergy));
    }

    // device energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
    NS_ASSERT(basicRadioModelPtr != NULL);

    if (tracing)
    {
      basicRadioModelPtr->TraceConnectWithoutContext("TotalEnergyConsumption", MakeCallback(&TotalEnergy));
    }
  }

  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferTx", MakeCallback(&MonitorSniffTx));
  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));

  //flow monitor logging
  /**************************************************************************/
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(duration + 1));

  if (tracing == true)
  {
    //pointToPoint.EnablePcapAll ("third");
    phy.EnablePcap("third", ApDevices);
    csma.EnablePcap("third", csmaDevices, true);
  }

  Simulator::Run();

  flowMonitor->SerializeToXmlFile("testflow.xml", true, true);

   std::cout << std::setw(10) << "index" << std::setw(15) << "Tput (Mb/s)" << std::setw(15) << "Signal (dBm)" << std::setw(15) << "Noise (dBm)" << std::setw(15) << "SNR (dB)" << std::setw(15) << "Packet loss" << std::setw(15) << "Packets through" << std::setw(15) << "Packets sent" << std::endl;

  for (uint32_t i = 0; i < numAp; i++)
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
      //   uint64_t totalBytesRx = DynamicCast<PacketSink>(apDevice[i].Get(0))->GetTotalRx();
      //   totalPacketsThrough[i] = totalBytesRx / TCPpayloadSize;
      //   throughput[i] = totalBytesRx * 8 / (duration * 1000000.0); //Mbit/s

      throughput[i] = totalPacketsThrough[i] * TCPpayloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }

    for (uint32_t j = i * numSta; j < (1 + i) * numSta; j++)
    {
      totalPacketsSent[i] += packetsSent[j];
    }



    std::cout << std::setw(10) << i + 1 << std::setw(15) << throughput[i] << std::setw(15) << signalDbmAvg[numSta * numAp + i] << std::setw(15) << noiseDbmAvg[numSta * numAp + i] << std::setw(15) << (signalDbmAvg[numSta * numAp + i] - noiseDbmAvg[numSta * numAp + i]) << std::setw(15) << (1 - (float)totalPacketsThrough[i] / totalPacketsSent[i]) << std::setw(15) << totalPacketsThrough[i] << std::setw(15) << totalPacketsSent[i] << std::endl;
  }


  Simulator::Destroy();
  return 0;
}
