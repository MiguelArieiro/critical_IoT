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

NS_LOG_COMPONENT_DEFINE ("Critical IoT Scenario");

uint32_t numSTA = 2;                           //number of stations per AP
uint32_t numAP = 2;                             //number of stations
uint32_t numNodes = numSTA * numAP + numAP + 1; //number of nodes

// Global variables for use in callbacks.
std::vector<double> signalDbmAvg(numNodes);
std::vector<double> noiseDbmAvg(numNodes);

std::vector<uint64_t> totalPacketsSent(numAP);
std::vector<uint64_t> totalPacketsThrough(numAP);
std::vector<double> throughput(numAP);
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

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 0;
  uint32_t nWifi = 3;
  bool tracing = false;

  // bool logging = false;           // enable logging
  double duration = 10.0;         // seconds
  double powSta = 10.0;           // dBm
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -62;        // dBm
  double ccaEdTrAp = -62;         // dBm
  uint32_t TCPpayloadSize = 60;   // bytes
  // uint32_t UDPpayloadSize = 40;   // bytes
  uint32_t mcs = 0;               // MCS value
  // double interval = 0.001;        // seconds
  double obssPdThreshold = -82.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  // bool udp = false;               // udp or tcp
  // double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 5G = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    }

  NodeContainer wifiAPNodes;
  wifiAPNodes.Create(numAP);

  NodeContainer csmaNodes;
  csmaNodes.Add (wifiAPNodes);
  csmaNodes.Create (1);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiSTANodes;
  wifiSTANodes.Create (numSTA*numAP);
  


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  phy.SetErrorRateModel("ns3::YansErrorRateModel");
  phy.Set("Frequency", UintegerValue(5180)); // channel 36 at 20 MHz
  phy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");

  WifiHelper wifi;
  //wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  if (technology == 0)
  {
    wifi.SetStandard(WIFI_PHY_STANDARD_80211ax_5GHZ); //define standard como 802.11ax 5GHz
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
    else if (technology == 2)
    {
      wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    }
    else if (technology == 3)
    {
      wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
    }
    else
    {
      // if no supported technology is selected
      return 0;
    }


  
  WifiMacHelper mac;
  Ssid ssid;
  std::vector<NetDeviceContainer> STADevices(numAP);
  NetDeviceContainer APDevices;

  std::vector<Ptr<WifiNetDevice>> ap2Device(numAP);
  for (uint32_t i = 0; i < numAP; i++)
  {


    ssid = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.

    //STA creation
    phy.Set("TxPowerStart", DoubleValue(powSta));
    phy.Set("TxPowerEnd", DoubleValue(powSta));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    phy.Set("RxSensitivity", DoubleValue(-92.0));

    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));

    for (uint32_t j = 0; j < numSTA; j++)
    {
      STADevices[i].Add(wifi.Install (phy, mac, wifiSTANodes.Get(i*numSTA+j)));
    }
  
    //AP creation
    phy.Set("TxPowerStart", DoubleValue(powAp));
    phy.Set("TxPowerEnd", DoubleValue(powAp));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
    phy.Set("RxSensitivity", DoubleValue(-92.0));

    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

    APDevices.Add (wifi.Install (phy, mac, wifiAPNodes.Get(i)));

    ap2Device[i] = APDevices.Get(i)->GetObject<WifiNetDevice>(); //TOFIX
    //Sets BSS color
    if ((technology == 0) && enableObssPd)
    {
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
    }
  }
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiSTANodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiAPNodes);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiSTANodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("172.1.1.0", "255.255.255.0");
  for (uint32_t i = 0; i < numAP; i++)
  {
    address.Assign (STADevices[i]);
    address.Assign (APDevices.Get(i));
    address.NewNetwork();
  }

  //server
  PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5000));
  ApplicationContainer serverApps = packetSinkHelper.Install (csmaNodes.Get (numAP));
  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (duration+1));

  OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(nCsma), 5000)));
  onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute("PacketSize", UintegerValue(TCPpayloadSize));
  onoff.SetAttribute("DataRate", DataRateValue(100000)); //bit/s

  ApplicationContainer clientApps = onoff.Install (wifiSTANodes);
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (duration+1));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (duration+1));

  if (tracing == true)
    {
      //pointToPoint.EnablePcapAll ("third");
      phy.EnablePcap ("third", APDevices);
      csma.EnablePcap ("third", csmaDevices, true);
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
