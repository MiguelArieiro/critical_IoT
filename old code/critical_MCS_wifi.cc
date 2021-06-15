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
#include "ns3/internet-apps-module.h" //added
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-helper.h"
//#include "ns3/flow-monitor-module.h" //Added
#include "ns3/energy-module.h"
#include "ns3/nr-module.h" //For NR 5G
#include "ns3/lte-module.h" // For LTE

//#include "ns3/config-store-module.h" //Added
//#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"

#include "ns3/rng-seed-manager.h"

/*
Based on the following examples:
 Wifi-he-network (802.11ax)
 Wifi-spatial-reuse (802.11ax)

 cttc-nr-demo (NR 5G)
*/

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

NS_LOG_COMPONENT_DEFINE("CriticalMCSScenario_WiFi");

#include "common_MCS.h"

int64_t lastTimeRemaining = 0;
int64_t lastTimeTotal = 0;

// Trace function for received packets and SNR
void MonitorSniffRx(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  int32_t nodeId = ContextToNodeId(context);
  packetsReceived[nodeId]++;
  signalDbmAvg[nodeId] += ((signalNoise.signal - signalDbmAvg[nodeId]) / packetsReceived[nodeId]);
  noiseDbmAvg[nodeId] += ((signalNoise.noise - noiseDbmAvg[nodeId]) / packetsReceived[nodeId]);

  if (verbose)
    fprintf(FIsignal, "%lf,%d,%lf,%lf,%hhu,%d\n", Simulator::Now().GetSeconds(), nodeId, signalNoise.signal, signalNoise.noise, txVector.GetTxPowerLevel(),txVector.GetMode().GetMcsValue()  );
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
  if ((Simulator::Now().GetMilliSeconds() - lastTimeRemaining) >= 50 ){
    fprintf(FIenergyRemaining, "%lf,%d,%lf\n", Simulator::Now().GetSeconds(), nodeId, remainingEnergy);
    lastTimeRemaining = Simulator::Now().GetMilliSeconds();
  }
}

// Trace function for total energy consumption at node.
void TotalEnergy(std::string context, double oldValue, double totalEnergy)
{
  int32_t nodeId = std::stoi(context);

  if ((Simulator::Now().GetMilliSeconds() - lastTimeTotal) >= 50 ){
    fprintf(FIenergyConsumed, "%lf,%d,%lf\n", Simulator::Now().GetSeconds(),nodeId, totalEnergy);
    lastTimeTotal = Simulator::Now().GetMilliSeconds() ;
  }
}
/***************************************************************************/

int main(int argc, char *argv[])
{
  bool tracing = false;
  //double duration = 10.0;         // seconds
  double powSta = 10.0;           // dBm
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -62;        // dBm
  double ccaEdTrAp = -62;         // dBm
  double rxSensSta = -84;         // dBm
  double rxSensAp = -102;          // dBm
  int32_t TCPpayloadSize = 60;    // bytes
  int32_t UDPpayloadSize = 40;    // bytes
  int32_t mcs = 7;                // MCS value
  //int64_t dataRate = 1000000;     // bits/s
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */
  std::string dataRateVoice = "160kbps";                  /* Application layer datarate. */
  std::string speed="0";

  double obssPdThreshold = -72.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  //bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 802.11n =1, 5G = 2;
  double distance = 10;           // mobility model quadrant size
  int frequency = 5;              // frequency selection
  int channelWidth = 20;          // channel number
  int numRxSpatialStreams = 2;    // number of Rx Spatial Streams
  int numTxSpatialStreams = 2;    // number of Tx Spatial Streams
  int numAntennas = 2;            // number of Antenas
  //bool voice = true;              //Do Voice
  int powerMode = 1;              //Configure Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )

  //default ns3 energy values 802.11n (2.4GHz)
  double TxCurrentA = 0.38;
  double RxCurrentA = 0.313;
  double IdleCurrentA = 0.273;
  double SleepCurrentA = 0.033;
  double CcaBusyCurrentA = 0.273;
  double SwitchingCurrentA = 0.273;

  bool useRts = false;
  bool useExtendedBlockAck = false;

  CommandLine cmd(__FILE__);
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  cmd.AddValue("dataRate", "Data rate (bits/s)", dataRate);
  cmd.AddValue("dataRateVoice", "Voice Data rate (bits/s)", dataRateVoice); //new
  cmd.AddValue("speed", "Velocity ", speed); //new m/s

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
  cmd.AddValue("obssPdThreshold", "Use PD level for OBSS threshold", obssPdThreshold); //Default to -72
  cmd.AddValue("powerMode", "Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )", powerMode);

  cmd.AddValue ("useRts", "Enable/disable RTS/CTS", useRts);
  cmd.AddValue ("useExtendedBlockAck", "Enable/disable use of extended BACK", useExtendedBlockAck);
  cmd.AddValue ("seed", "Sets RNG seed number", seed);

  ns3::RngSeedManager::SetSeed(seed);
  
  //enable RTS
  if (useRts)
  {
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  }

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  cmd.Parse(argc, argv);

  numNodes = numSta * numAp + numAp + 1; //updating numNodes

  if (verbose) FIsignal = fopen("FIsignal.csv", "w+");
  FIenergyConsumed = fopen("FIenergyConsumed.csv", "w+");
  FIenergyRemaining = fopen("FIenergyRemaining.csv", "w+");

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
  csmaNodes.Create(1); //Idealmente 2 Servidores (ccc_to_cloud e gw_mc)

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("600Mbps")); //WiFi Maximum theoretical
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));

  NetDeviceContainer csmaDevices;

  //spectrum definition
  /***************************************************************************/
  SpectrumWifiPhyHelper phy ;
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
      //val aproximados
      TxCurrentA = 0.52364;
      RxCurrentA = 0.417229;
      IdleCurrentA = 0.374283;
      SleepCurrentA = 0.035211;
      CcaBusyCurrentA = 0.374283;
      SwitchingCurrentA = 0.374283;
      break;
    case 6:
      wifi.SetStandard(WIFI_STANDARD_80211ax_6GHZ);
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
    oss << "HeMcs" << mcs;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(oss.str()),
                                 "ControlMode", StringValue(oss.str()));
    //Other that could be used is the IdealWiFiManager which adjust according to the SINR.
    //This requires BerThreshold (default to 10^-6)

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
  else if (technology == 2)
  {
    // if no supported technology is selected
    return 0;
  }

  phy.Set("ChannelWidth", UintegerValue(channelWidth));

  WifiMacHelper mac;
  Ssid ssid;

  NetDeviceContainer ApDevices;
  NetDeviceContainer StaDevices;

  //std::vector<Ptr<WifiNetDevice>> ap2Device(numAp); //BS_
  for (int32_t i = 0; i < numAp; i++)
  {
    ssid = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.

    //STA creation
    phy.Set("TxPowerStart", DoubleValue(powSta));
    phy.Set("TxPowerEnd", DoubleValue(powSta));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    phy.Set("RxSensitivity", DoubleValue(rxSensSta));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    for (int32_t j = 0; j < numSta; j++)
    {
      StaDevices.Add(wifi.Install(phy, mac, wifiStaNodes.Get(i * numSta + j))); //BS_
    }

    //AP creation
    phy.Set("TxPowerStart", DoubleValue(powAp));
    phy.Set("TxPowerEnd", DoubleValue(powAp));
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
    phy.Set("RxSensitivity", DoubleValue(rxSensAp));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    //ApDevices.Add(wifi.Install(phy, mac, wifiApNodes.Get(i)));  //BS_
    NetDeviceContainer BSaPdev = wifi.Install(phy, mac, wifiApNodes.Get(i));
    ApDevices.Add(BSaPdev);  //BS_

    //Sets BSS color
    if ((technology == 0) && enableObssPd)
    {
      Ptr<WifiNetDevice> wnd = BSaPdev.Get(0)->GetObject<WifiNetDevice>();
      wnd->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
    }
  }

  csmaDevices = csma.Install(csmaNodes);

  //enable extended block akn
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/MpduBufferSize", UintegerValue (useExtendedBlockAck ? 256 : 64));

  speed = std::string("ns3::ConstantRandomVariable[Constant=")+ speed + std::string("]");

  /** Configure mobility model **/
  MobilityHelper mobility;

  int32_t edge_size = (ceil(sqrt(numAp)));
  int32_t sta_edge_size = (ceil(sqrt(numSta)));
  int32_t counter = 0;
  for (int32_t y = 0; (y < edge_size) && (counter < numAp); y++)
  {
    for (int32_t x = 0; (x < edge_size) && (counter < numAp); x++, counter++)
    {
      mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue((x - 1) * distance),
                                    "MinY", DoubleValue((y - 1) * distance),
                                    "DeltaX", DoubleValue(2 * distance / sta_edge_size),
                                    "DeltaY", DoubleValue(2 * distance / sta_edge_size),
                                    "GridWidth", UintegerValue(sta_edge_size),
                                    "LayoutType", StringValue("RowFirst"));

      mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                "Bounds", RectangleValue(Rectangle((x - 1) * distance - 1, (x + 1) * distance + 1, (y - 1) * distance - 1, (y + 1) * distance + 1)),
                                "Speed", StringValue(speed)); //0.2m/s

      for (int32_t j = 0; j < numSta; j++)
      {
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
    address.Assign(ApDevices.Get(i)); //BS_

    for (int32_t j = 0; j < numSta; j++)
    {
      address.Assign(StaDevices.Get(i*numSta + j)); //BS_
    }
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
    staticRouting->AddNetworkRouteTo(wifiApIP.c_str(), "255.255.0.0", csmaApIP.c_str(), 1);
  }

  uint16_t pttPortServer = 2;
  uint16_t pttPortCli = 19;

  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  StringValue AtrOnTime, AtrOffTime;


  expDistVoicePTT->SetAttribute("Mean", DoubleValue(betaIntervalVoicePTT )); // should be 1.0/ betaIntervalVoicePTT
  expDistIoT->SetAttribute("Mean", DoubleValue(betaIntervalIoT )); // should be 1.0/ betaIntervalVoicePTT

  if (voice){ //As download
      //std::cout << "Using voice PTT \n";
      PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), pttPortServer));
      serverApps.Add( packetSinkHelper.Install(wifiStaNodes) );
      sinkVoicePTT = StaticCast<PacketSink> (serverApps.Get (0));

     for (int32_t i = 0; i < numAp; i++){
      for (int32_t j = 0; j < numSta; j++){
        //Station 0 sends the PTT
        ++pttPortCli;

        //PTT client on server
        Ptr<Ipv4> ipv4 = wifiStaNodes.Get (i*numSta + j)->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback
        //std::cout <<"Install app in WiFi node " << (i*numSta + j) << " ip:" << iaddr.GetLocal() << " \n" ;
        /*
        OnOffHelper voicePtt("ns3::UdpSocketFactory", Address(InetSocketAddress(iaddr.GetLocal(), pttPortServer)));
        AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
        AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=0]");
        voicePtt.SetAttribute("OnTime", AtrOnTime); //2 em 2s ?
        voicePtt.SetAttribute("OffTime", AtrOffTime );
        voicePtt.SetAttribute("PacketSize", UintegerValue(200)); //200 bytes as per Fog evaluation
        voicePtt.SetAttribute("DataRate", DataRateValue(DataRate(dataRateVoice))); //StringValue("1600kbps")); //bit/s
        //clientApps.Add( pttOnOff.Install(csmaNodes.Get(0)) ); //BS_
        */
        UdpClientHelper voicePtt;
        setTruncatedParetoParms(minSizeVoicePTT, maxSizeVoicePTT, shapeSizeVoicePTT, scaleSizeVoicePTT);
        uint32_t pktSize = getTruncatedSize();
        double aint = expDistVoicePTT->GetValue();

        std::cout << " Size = " << pktSize << " intervale = " << aint << std::endl;

        voicePtt.SetAttribute("RemoteAddress", AddressValue(iaddr.GetLocal()) );
        voicePtt.SetAttribute("RemotePort", UintegerValue ( pttPortServer) );
        voicePtt.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
        voicePtt.SetAttribute("PacketSize", UintegerValue ( pktSize) );
        voicePtt.SetAttribute("Interval", TimeValue (Seconds( aint ) ));
        clientApps.Add( voicePtt.Install(csmaNodes.Get(numAp)));
      }
     }
  }
  //server
  if (udp) //As uplink
  {
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    serverApps.Add( packetSinkHelper.Install(csmaNodes.Get(numAp)) );
    if (!voice) sinkCoAP = StaticCast<PacketSink> (serverApps.Get (0));
    else sinkCoAP = StaticCast<PacketSink> (serverApps.Get (1));

    //client
    /*
    OnOffHelper coapApp("ns3::UdpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), 9)));
    AtrOnTime = StringValue("ns3::ConstantRandomVariable[Constant=1]");
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=0]");
    coapApp.SetAttribute("OnTime", AtrOnTime);
    coapApp.SetAttribute("OffTime", AtrOffTime);
    coapApp.SetAttribute("PacketSize", UintegerValue(UDPpayloadSize));
    coapApp.SetAttribute("DataRate", DataRateValue(DataRate(dataRate))); //bit/s
    */
    UdpClientHelper coapApp;
    setTruncatedParetoParms(minSizeCoAP, maxSizeCoAP, shapeSizeCoAP, scaleSizeCoAP);
    uint32_t pktSize = getTruncatedSize();
    double aint = expDistIoT->GetValue();

    std::cout << " CoAP Size = " << pktSize << " intervale = " << aint << std::endl;

    coapApp.SetAttribute("RemoteAddress", AddressValue(csmaInterfaces.GetAddress(numAp)) );
    coapApp.SetAttribute("RemotePort", UintegerValue ( 9) );
    coapApp.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
    coapApp.SetAttribute("PacketSize", UintegerValue ( pktSize) );
    coapApp.SetAttribute("Interval", TimeValue (Seconds( aint ) ));

    clientApps.Add( coapApp.Install(wifiStaNodes) );

  }
  else //As uplink
  {
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
    serverApps.Add ( packetSinkHelper.Install(csmaNodes.Get(numAp)) );
    if (!voice)sinkMQTT = StaticCast<PacketSink> (serverApps.Get (0));
    else sinkMQTT = StaticCast<PacketSink> (serverApps.Get (1));

    setTruncatedParetoParms(minSizeCoAP, maxSizeCoAP, shapeSizeCoAP, scaleSizeCoAP);
    uint32_t pktSize = getTruncatedSize();
    double aint = expDistIoT->GetValue();

    std::cout << " MQTT Size = " << pktSize << " intervale = " << aint << std::endl;

    //client
    OnOffHelper mqtt("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), 5000)));
    AtrOnTime = StringValue("ns3::ExponentialRandomVariable[Mean=0.001666667]"); //TODO Change
    AtrOffTime = StringValue("ns3::ConstantRandomVariable[Constant=0]");
    mqtt.SetAttribute("OnTime", AtrOnTime);
    mqtt.SetAttribute("OffTime", AtrOffTime);
    mqtt.SetAttribute("PacketSize", UintegerValue(pktSize));
    mqtt.SetAttribute("DataRate", DataRateValue(DataRate(dataRate))); //bit/s

    clientApps.Add (mqtt.Install(wifiStaNodes) );

  }
  //Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback (&AppRxInfo));
  //Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/TxWithAddresses", MakeCallback (&AppTxInfo));

  //Start APPs
  serverApps.Start(Seconds(0));
  serverApps.Stop(Seconds(duration + 1));
  clientApps.Start(Seconds(timeStartClientApps)); //1.0
  clientApps.Stop(Seconds(duration + 1));

  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);


  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  LiIonEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set("LiIonEnergySourceInitialEnergyJ", DoubleValue(batteryLevel));
  basicSourceHelper.Set("InitialCellVoltage", DoubleValue(3.85)); //BS_

  // install source
  EnergySourceContainer sources = basicSourceHelper.Install(wifiStaNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
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

  // install device model
  DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();

  for (int32_t i = 0; i < numAp; i++)
  {
    for (int32_t j = 0; j < numSta; j++)
    {
      deviceModels.Add(radioEnergyHelper.Install(StaDevices.Get(i*numSta + j),sources.Get(j + i * numSta) ));
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

  //flow monitor logging
  /**************************************************************************/
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(duration + 1));

  if (tracing == true)
  {
    //phy.EnablePcap("critical_iot", ApDevices);
    csma.EnablePcap("critical_iot", csmaDevices, true);
  }

  //std::cout << "code???\n";
  Simulator::Run();


  flowMonitor->SerializeToXmlFile("testflow.xml", true, true);
  //flowStatsSim(flowMonitor, flowHelper, "zout-wifi", "res-wifi");


  Simulator::Destroy();

  PrintFinalThroughput(duration);
  //statsPackets(duration);

  if (verbose) fclose(FIsignal);
  //fclose(FInoise);
  fclose(FIenergyConsumed);
  fclose(FIenergyRemaining);
  return 0;
}
