/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Washington
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

//
//  This example program can be used to experiment with spatial
//  reuse mechanisms of 802.11ax.
//
//  The geometry is as follows:
//
//                STA1          STA1
//                 |              |
//              d1 |              |d2
//                 |       d3     |
//                AP1 -----------AP2

//
//  STA1 and AP1 are in one BSS (with color set to 1), while STA2 and AP2 are in
//  another BSS (with color set to 2). The distances are configurable (d1 through d3).
//
//  STA1 is continously transmitting data to AP1, while STA2 is continuously sending data to AP2.
//  Each STA has configurable traffic loads (inter packet interval and packet size).
//  It is also possible to configure TX power per node as well as their CCA-ED tresholds.
//  OBSS_PD spatial reuse feature can be enabled (default) or disabled, and the OBSS_PD
//  threshold can be set as well (default: -72 dBm).
//  A simple Friis path loss model is used and a constant PHY rate is considered.
//
//  In general, the program can be configured at run-time by passing command-line arguments.
//  The following command will display all of the available run-time help options:
//    ./waf --run "wifi-spatial-reuse --help"
//
//  By default, the script shows the benefit of the OBSS_PD spatial reuse script:
//    ./waf --run wifi-spatial-reuse
//    Throughput for BSS 1: 6.6468 Mbit/s
//    Throughput for BSS 2: 6.6672 Mbit/s
//
// If one disables the OBSS_PD feature, a lower throughput is obtained per BSS:
//    ./waf --run "wifi-spatial-reuse --enableObssPd=0"
//    Throughput for BSS 1: 5.8692 Mbit/s
//    Throughput for BSS 2: 5.9364 Mbit/
//
// This difference between those results is because OBSS_PD spatial
// enables to ignore transmissions from another BSS when the received power
// is below the configured threshold, and therefore either defer during ongoing
// transmission or transmit at the same time.
//

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/application-container.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/he-configuration.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/energy-source-container.h"
#include "ns3/li-ion-energy-source-helper.h"
#include "ns3/li-ion-energy-source.h"

#include <iomanip>
#include <iostream>

#include <vector>
#include <cmath>
#include <regex>

#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/internet-module.h"

#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"

#include "ns3/rng-seed-manager.h"


using namespace ns3;

#define MAX_NODES 2048

bool verbose = false;

std::vector<uint64_t> bytesReceived(MAX_NODES);

void MonitorSniffRx(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId)
{
  std::string sub = context.substr(10);
  uint32_t pos = sub.find("/Device");
  uint32_t nodeId = atoi(sub.substr(0, pos).c_str());
  bytesReceived[nodeId] += packet->GetSize();
}

// std::vector<uint64_t> bytesReceivedPS(MAX_NODES);

// void PacketSinkRx (std::string context, Ptr< const Packet > packet, const Address &address)
// {
//   std::string sub = context.substr(10);
//   uint32_t pos = sub.find("/Application");
//   uint32_t nodeId = atoi(sub.substr(0, pos).c_str());
//   bytesReceivedPS[nodeId] += packet->GetSize();
// }

// Trace function for remaining energy at node.
void RemainingEnergy(std::string context, double oldValue, double remainingEnergy)
{

  int32_t nodeId = std::stoi(context);
  if (verbose)
  {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s " << nodeId << " Current remaining energy = " << remainingEnergy << "J");
  }
  //fprintf("%lf,%lf\n", Simulator::Now().GetSeconds(), remainingEnergy);
}

// Trace function for total energy consumption at node.
void TotalEnergy(std::string context, double oldValue, double totalEnergy)
{
  int32_t nodeId = std::stoi(context);
  if (verbose)
  {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s " << nodeId << " Total energy consumed by radio = " << totalEnergy << "J");
  }
  //fprintf(energyConsumedFile[nodeId], "%lf,%lf\n", Simulator::Now().GetSeconds(), totalEnergy);
}
/***************************************************************************/

int main(int argc, char *argv[])
{
  // double d1 = 30.0; // meters
  // double d2 = 30.0; // meters
  // double d3 = 150.0; // meters
  uint32_t seed = 1;
  uint32_t runs = 3;
  bool tracing = true;
  double duration = 10.0;           // seconds
  double powAp = 21.0;              // dBm
  double powSta = 10.0;             // dBm
  double ccaEdTrAp = -62;           // dBm
  double ccaEdTrSta = -62;          // dBm
  double rxSensAp = -92;            // dBm
  double rxSensSta = -92;           // dBm
  uint32_t tcpPayloadSize = 60;      // bytes
  uint32_t udpPayloadSize = 40;      // bytes
  int32_t mcs = 9;                 // MCS value
  uint64_t dataRate = 10000;         // bits/s
  double distance = 25;             // mobility model side = 2 * distance
  int technology = 0;               // technology to be used 802.11ax = 0, 5G = 1;
  int frequency = 5;                // frequency selection
  int channelWidth = 20;            // channel number
  int numApAntennas = 4;            // number of AP Antenas
  int numApRxSpatialStreams = 4;    // number of AP Rx Spatial Streams
  int numApTxSpatialStreams = 4;    // number of AP Tx Spatial Streams
  int numStaAntennas = 4;           // number of STA Antenas
  int numStaRxSpatialStreams = 4;   // number of STA Rx Spatial Streams
  int numStaTxSpatialStreams = 4;   // number of STA Tx Spatial Streams
  bool enableObssPd = true;         // spatial reuse
  double obssPdThreshold = -82.0;   // dBm
  bool useUdp = false;              // udp or tcp
  bool useRts = false;              // enable RTS/CTS
  bool useExtendedBlockAck = false; // enable Extended Block Ack
  int guardInterval = 3200;         // guard interval
  double batteryLevel = 2000000;      // initial battery energy

  //default ns3 energy values 802.11n (2.4GHz)
  double TxCurrentA = 0.38;
  double RxCurrentA = 0.313;
  double IdleCurrentA = 0.273;
  double SleepCurrentA = 0.033;
  double CcaBusyCurrentA = 0.273;
  double SwitchingCurrentA = 0.273;

  uint32_t timeStartServerApps = 1000;
  uint32_t timeStartClientApps = 2000;

  int32_t numAp = 2;
  int32_t numSta = 1;

  CommandLine cmd(__FILE__);
  cmd.AddValue("verbose", "Activate logging", verbose);
  cmd.AddValue("runs", "Sets number of runs", runs);
  cmd.AddValue("seed", "Sets RNG seed number", seed);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  
  cmd.AddValue("ccaEdTrAp", "CCA-ED Threshold of AP (dBm)", ccaEdTrAp);
  cmd.AddValue("ccaEdTrSta", "CCA-ED Threshold of STA (dBm)", ccaEdTrSta);
  cmd.AddValue("channelWidth", "Defines wifi channel number", channelWidth);
  cmd.AddValue("dataRate", "Data rate (bits/s)", dataRate);
  cmd.AddValue("distance", "Distance between networks", distance);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  cmd.AddValue("enableObssPd", "Enable/disable OBSS_PD", enableObssPd);
  cmd.AddValue("frequency", "Wifi device frequency. 2 - 2.4GHz, 5 - 5GHz, 6 - 6GHz", frequency);
  cmd.AddValue("guardInterval", "Set guard interval (ns)", guardInterval);
  cmd.AddValue("mcs", "The constant MCS value to transmit HE PPDUs", mcs);
  cmd.AddValue("useUdp", "UDP if set to 1, TCP otherwise", useUdp);
  cmd.AddValue("batteryLevel", "Initial energy level (J)", batteryLevel);
  cmd.AddValue("numAp", "Number of Wifi APs", numAp);
  cmd.AddValue("numApAntennas", "Number of AP Antennas", numApAntennas);
  cmd.AddValue("numApRxSpatialStreams", "Number of AP Rx Spatial Streams", numApRxSpatialStreams);
  cmd.AddValue("numApTxSpatialStreams", "Number of AP Tx Spatial Streams", numApTxSpatialStreams);
  cmd.AddValue("numSta", "Number of Wifi Stations per AP", numSta);
  cmd.AddValue("numStaAntennas", "Number of STA Antennas", numStaAntennas);
  cmd.AddValue("numStaRxSpatialStreams", "Number of STA Rx Spatial Streams", numStaRxSpatialStreams);
  cmd.AddValue("numStaTxSpatialStreams", "Number of STA Tx Spatial Streams", numStaTxSpatialStreams);
  cmd.AddValue("powAp", "Power of AP (dBm)", powAp);
  cmd.AddValue("powSta", "Power of STA (dBm)", powSta);
  cmd.AddValue("rxSensAp", "RX Sensitivity of AP (dBm)", rxSensAp);
  cmd.AddValue("rxSensSta", "RX Sensitivity of STA (dBm)", rxSensSta);
  cmd.AddValue("tcpPayloadSize", "TCP packet size", tcpPayloadSize);
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 802.11n, 3 = 5G", technology);
  cmd.AddValue("udpPayloadSize", "UDP packet size", udpPayloadSize);
  cmd.AddValue("useExtendedBlockAck", "Enable/disable use of Extended Block Ack", useExtendedBlockAck);
  cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
  // cmd.AddValue("TxCurrentA", "Transmission current (A)", TxCurrentA);
  // cmd.AddValue("RxCurrentA", "Reception current (A)", RxCurrentA);
  // cmd.AddValue("SleepCurrentA", "Sleep current (A)", SleepCurrentA);
  // cmd.AddValue("IdleCurrentA", "Iddle current (A)", IdleCurrentA);
  cmd.Parse(argc, argv);

  ns3::RngSeedManager::SetSeed(seed);

  if ((numAp * (numSta + 1) + 1) > MAX_NODES)
  {
    std::cout << "Error: Limit number of nodes: " << MAX_NODES << std::endl;
  }

  if (useRts)
  {
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
  }

  double avg_energy = 0.0;
  uint64_t bytesRx = 0;

  for(uint32_t r=1; r <= runs; r++){
    SeedManager::SetRun (r);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(numSta * numAp);

    NodeContainer wifiApNodes;
    wifiApNodes.Create(numAp);

    //spectrum definition
    /***************************************************************************/
    SpectrumWifiPhyHelper spectrumPhy;
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    spectrumPhy.SetChannel(spectrumChannel);
    spectrumPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    spectrumPhy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");

    WifiHelper wifi;
    //IEEE 802.11ax
    if (technology == 0)
    {
      switch (frequency)
      {
      case 2:
        wifi.SetStandard(WIFI_STANDARD_80211ax_2_4GHZ);
        Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40));
        
        TxCurrentA = 0.333;
        RxCurrentA = 0.200;
        IdleCurrentA = 0.100;
        SleepCurrentA = 0.05;
        CcaBusyCurrentA = 0.100;
        SwitchingCurrentA = 0.100;
        break;
      case 5:
        wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);

        TxCurrentA = 0.333;
        RxCurrentA = 0.200;
        IdleCurrentA = 0.100;
        SleepCurrentA = 0.05;
        CcaBusyCurrentA = 0.100;
        SwitchingCurrentA = 0.100;
        break;
      case 6:
        wifi.SetStandard(WIFI_STANDARD_80211ax_6GHZ);
        Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (48));

        TxCurrentA = 0.333;
        RxCurrentA = 0.200;
        IdleCurrentA = 0.100;
        SleepCurrentA = 0.05;
        CcaBusyCurrentA = 0.100;
        SwitchingCurrentA = 0.100;
        break;
      default:
        std::cout << "Wrong frequency." << std::endl;
        return 0;
      }

      if (enableObssPd)
      {
        wifi.SetObssPdAlgorithm("ns3::ConstantObssPdAlgorithm",
                                "ObssPdLevel", DoubleValue(obssPdThreshold));
      }

      std::ostringstream oss;
      oss << "HeMcs" << mcs;
      wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                  "DataMode", StringValue(oss.str()),
                                  "ControlMode", StringValue(oss.str()));
    }

    //IEEE 802.11n
    else if (technology == 1)
    {
      if (guardInterval != 1)
      {
        guardInterval = 0;
      }

      if (numApAntennas>4){
        numApAntennas=4;
      }
      if (numApRxSpatialStreams>numApAntennas){
        numApRxSpatialStreams=numApAntennas;
      }
      if (numApTxSpatialStreams>numApAntennas){
        numApTxSpatialStreams=numApAntennas;
      }


      switch (frequency)
      {
      case 2:
        wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
        Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
        
        TxCurrentA = 0.38;
        RxCurrentA = 0.313;
        IdleCurrentA = 0.273;
        SleepCurrentA = 0.033;
        CcaBusyCurrentA = 0.273;
        SwitchingCurrentA = 0.273;
        break;
      case 5:
        wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);

        //val aproximados
        TxCurrentA = 0.52364;
        RxCurrentA = 0.417229;
        IdleCurrentA = 0.374283;
        SleepCurrentA = 0.035211;
        CcaBusyCurrentA = 0.374283;
        SwitchingCurrentA = 0.374283;
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

    spectrumPhy.Set("ChannelWidth", UintegerValue(channelWidth));

    WifiMacHelper mac;
    Ssid ssid;
    std::vector<NetDeviceContainer> staDevices(numAp);
    NetDeviceContainer apDevices = NetDeviceContainer();

    Ptr<WifiNetDevice> apDevice;
    for (int32_t i = 0; i < numAp; i++)
    {
      ssid = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.

      staDevices[i] = NetDeviceContainer();

      //STA creation
      spectrumPhy.Set("Antennas", UintegerValue(numStaAntennas));
      spectrumPhy.Set("MaxSupportedTxSpatialStreams", UintegerValue(numStaTxSpatialStreams));
      spectrumPhy.Set("MaxSupportedRxSpatialStreams", UintegerValue(numStaRxSpatialStreams));

      spectrumPhy.Set("TxPowerStart", DoubleValue(powSta));
      spectrumPhy.Set("TxPowerEnd", DoubleValue(powSta));
      spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
      spectrumPhy.Set("RxSensitivity", DoubleValue(rxSensSta));

      mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

      for (int32_t j = 0; j < numSta; j++)
      {
        staDevices[i].Add(wifi.Install(spectrumPhy, mac, wifiStaNodes.Get(i * numSta + j)));
      }

      //AP creation
      spectrumPhy.Set("Antennas", UintegerValue(numApAntennas));
      spectrumPhy.Set("MaxSupportedTxSpatialStreams", UintegerValue(numApTxSpatialStreams));
      spectrumPhy.Set("MaxSupportedRxSpatialStreams", UintegerValue(numApRxSpatialStreams));

      spectrumPhy.Set("TxPowerStart", DoubleValue(powAp));
      spectrumPhy.Set("TxPowerEnd", DoubleValue(powAp));
      spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
      spectrumPhy.Set("RxSensitivity", DoubleValue(rxSensAp));

      mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

      apDevices.Add(wifi.Install(spectrumPhy, mac, wifiApNodes.Get(i)));

      //Sets BSS color
      if ((technology == 0) && enableObssPd)
      {
        apDevice = apDevices.Get(i)->GetObject<WifiNetDevice>();
        apDevice->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
      }
    }

    //Set guard interval and MPDU buffer size
  
    if (technology == 0)
    { //802.11ax
      
      Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/MpduBufferSize", UintegerValue(useExtendedBlockAck ? 256 : 64));
      Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue(NanoSeconds(guardInterval)));;
    }
    else
    { //802.11n
      Config::Set("/NodeList//DeviceList//$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue(guardInterval));
    }

    /** Mobility Model **/
    /***************************************************************************/
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

        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        for (int32_t j = 0; j < numSta; j++)
        {
          //std::cout << "x:" << x << " y:" << y << " distance:" << distance << "\n";
          mobility.Install(wifiStaNodes.Get(counter * numSta + j));
        }
      }
    }

    // AP pos
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0),
                                  "MinY", DoubleValue(0),
                                  "DeltaX", DoubleValue(distance),
                                  "DeltaY", DoubleValue(distance),
                                  "GridWidth", UintegerValue(edge_size),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNodes);
    //std::cout << "Mobility model configured\n";

    // Routing
    InternetStackHelper stack;

    Ipv4StaticRoutingHelper staticRoutingHelper;
    stack.Install(wifiApNodes);
    stack.SetRoutingHelper(staticRoutingHelper);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    // address.SetBase("10.1.1.0", "255.255.255.0");

    // Ipv4InterfaceContainer csmaInterfaces;
    // csmaInterfaces = address.Assign(csmaDevices);
    address.SetBase("172.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer apInterfaces;
    for (int32_t i = 0; i < numAp; i++)
    {
      apInterfaces.Add(address.Assign(apDevices.Get(i))); //BS_
      address.Assign(staDevices[i]);                      //BS_
      address.NewNetwork();
    }

    for (int32_t i = 0; i < numAp; i++)
    {
      Ptr<Ipv4StaticRouting> staticRouting;
      std::string wifiApIP = "172." + std::to_string(i + 1) + ".0.1";
      // std::string csmaApIP = "10.1.1." + std::to_string(i + 1);
      for (int32_t j = 0; j < numSta; j++)
      {
        staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(wifiStaNodes.Get(i * numSta + j)->GetObject<Ipv4>()->GetRoutingProtocol());
        staticRouting->SetDefaultRoute(wifiApIP.c_str(), 1);
      }
      // staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(csmaNodes.Get(numAp)->GetObject<Ipv4>()->GetRoutingProtocol());
      // staticRouting->AddNetworkRouteTo(wifiApIP.c_str(), "255.255.0.0", csmaApIP.c_str(), 1);
    }
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    if (useUdp)
    {
      PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
      serverApps = packetSinkHelper.Install(wifiApNodes);

      //client
      for (int32_t i = 0; i < numAp; i++)
      {
        OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(apInterfaces.GetAddress(i), 9)));
        onoff.SetConstantRate(DataRate(dataRate), udpPayloadSize);

        for (int32_t j = 0; j < numSta; j++)
        {
          clientApps = onoff.Install(wifiStaNodes.Get(i * numSta + j));
        }
      }
    }
    else
    {
      PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
      serverApps = packetSinkHelper.Install(wifiApNodes);

      //client
      for (int32_t i = 0; i < numAp; i++)
      {
        OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(apInterfaces.GetAddress(i), 5000)));
        onoff.SetConstantRate(DataRate(dataRate), tcpPayloadSize);

        for (int32_t j = 0; j < numSta; j++)
        {
          clientApps = onoff.Install(wifiStaNodes.Get(i * numSta + j));
        }
      }
    }

    serverApps.Start(MilliSeconds(timeStartServerApps)); 
    serverApps.Stop(Seconds(duration + 3));
    clientApps.Start(MilliSeconds(timeStartClientApps)); //2.0
    clientApps.Stop(Seconds(duration + 2));

    /** Energy Model **/
    /***************************************************************************/
    /* energy source */
    LiIonEnergySourceHelper liIonSourceHelper;
    // configure energy source
    liIonSourceHelper.Set("LiIonEnergySourceInitialEnergyJ", DoubleValue(batteryLevel));
    // install source
    EnergySourceContainer sources = liIonSourceHelper.Install(wifiStaNodes);
    /* device energy model */
    WifiRadioEnergyModelHelper radioEnergyHelper;
    // configure radio energy model
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(TxCurrentA));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(RxCurrentA));
    radioEnergyHelper.Set("IdleCurrentA", DoubleValue(IdleCurrentA));
    radioEnergyHelper.Set("SleepCurrentA", DoubleValue(SleepCurrentA));
    radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(CcaBusyCurrentA));
    radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(SwitchingCurrentA));

    // install device model
    DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();

    for (int32_t i = 0; i < numAp; i++)
    {
      for (int32_t j = 0; j < numSta; j++)
      {
        deviceModels.Add(radioEnergyHelper.Install(staDevices[i].Get(j), sources.Get(j + i * numSta)));
      }
    }

    /** connect trace sources **/
    /***************************************************************************/
    //energy source
    
    if (tracing && verbose){
      for (int32_t i = 0; i < numAp * numSta; i++)
      {
        Ptr<LiIonEnergySource> basicSourcePtr = DynamicCast<LiIonEnergySource>(sources.Get(i));

        basicSourcePtr->TraceConnect("RemainingEnergy", std::to_string(i), MakeCallback(&RemainingEnergy));

        // device energy model
        Ptr<DeviceEnergyModel> basicRadioModelPtr = basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
        NS_ASSERT(basicRadioModelPtr != NULL);

        basicRadioModelPtr->TraceConnect("TotalEnergyConsumption", std::to_string(i), MakeCallback(&TotalEnergy));
      }
    }


    //Config::Connect("/NodeList/*/DeviceList/*/Phy/WifiRadioEnergyModel", MakeCallback(&TotalEnergy));
    //Config::Connect("/NodeList/*/DeviceList/*/Phy/LiIonEnergySource", MakeCallback(&RemainingEnergy));
    //Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRx));

    if (verbose) Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));
    

    //Flow monitor logging
    /**************************************************************************/
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowMonHelper;
    flowMonitor = flowMonHelper.InstallAll();
    //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(duration + 3));

    if (tracing == true)
    {
      spectrumPhy.EnablePcap("critical_iot", apDevices);
    }

    Simulator::Run();

    if (verbose){
      for (int32_t i = 0; i < numAp; i++)
      {
        double throughput = static_cast<double>(bytesReceived[numSta*numAp + i]) * 8 / 1000 / 1000 / duration;
        std::cout << "Physical throughput for BSS " << i + 1 << ": " << throughput << " Mbit/s" << std::endl;
      }
    }


    std::string outputDir = "";
    std::string simTag = "wifi_spatial_reuse";
    std::string file = outputDir + "testflow.xml";
    flowMonitor->SerializeToXmlFile(file.c_str(), false, true);
    // Print per-flow statistics
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::regex server_regex ("^172.*.0.1$");

    if (verbose){

      std::ofstream outFile;

      std::string filename = simTag;
      outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
      if (!outFile.is_open())
      {
        std::cerr << "Can't open file " << filename << std::endl;
        return 1;
      }

      outFile.setf(std::ios_base::fixed);

      outFile << "Flow;source;src_port;destiny;dst_port;proto;service;direction;tx_packets;tx_bytes;tx_offered_raw;tx_offered_mbps;rx_bytes;rx_throughput_raw;rx_throughput_mbps;mean_delay(ms);mean_jitter(ms);rx_packets;lost_packets;packet_loss_ratio \n";
      for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
      {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
          protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
          protoStream.str("UDP");
        }
        outFile << i->first << ";";
        outFile << t.sourceAddress << ";" << t.sourcePort << ";" << t.destinationAddress << ";" << t.destinationPort << ";";
        outFile << protoStream.str() << ";";
        // if (t.sourcePort <= VideoPortServer){
        //   //Service , direction
        //   outFile << get_service(t.sourcePort) << ";";
        // }else{
        //     outFile << get_service(t.destinationPort) << ";";
        // }

        std::stringstream ss;
        ss<<t.sourceAddress;
        if (std::regex_match (ss.str(), server_regex))
        {
          outFile << "download"
                  << ";";
        }
        else
        {
          outFile << "upload"
                  << ";";
        }

        outFile << i->second.txPackets << ";";
        outFile << i->second.txBytes << ";";
        outFile << i->second.txBytes * 8.0 / duration<< ";";
        outFile << static_cast<double>(i->second.txBytes) * 8 / duration / 1000 / 1000 << ";";
        outFile << i->second.rxBytes << ";";
        if (i->second.rxPackets > 0)
        {
          // double rxDuration = (timeStartServerApps - timeStartClientApps) / 1000.0;
          double rxDuration = duration;
          averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
          averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

          outFile << i->second.rxBytes * 8.0 / rxDuration << ";";
          outFile << static_cast<double>(i->second.rxBytes) * 8 / rxDuration / 1000 / 1000 << ";";
          outFile << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << ";";
          outFile << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << ";";
        }
        else
        {
          outFile << "0;";
          outFile << "0;";
          outFile << "0;";
          outFile << "0;";
        }
        outFile << i->second.rxPackets << ";";
        int lost_packets = (i->second.txPackets - i->second.rxPackets);
        outFile << lost_packets << ";";
        outFile << (lost_packets * 1.0 / i->second.txPackets) * 100.0 << "\n";
      }

      outFile.close();
      std::ifstream f(filename.c_str());
      if (f.is_open())
      {
        std::cout << f.rdbuf();
      }
    }
    //End Simulator

    //get total energy consumed
    for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++)
    {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      avg_energy += static_cast<double>(energyConsumed)/duration;
    }

    for (int32_t i = 0; i < numAp; i++)
    {   
      bytesRx = static_cast<double>(DynamicCast<PacketSink>(serverApps.Get(i)) -> GetTotalRx())/duration;
    }

    Simulator::Destroy();


  }



  std::cout << avg_energy/(numAp*numSta*runs) << "\t" << bytesRx*8.0/(numAp*numSta*runs) << std::endl;
  return 0;
}
