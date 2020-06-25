//  STA# and AP# are in one BSS (with color set to #).
//
//  STAs are continuously sending data to APs.
//  Each STA has configurable traffic loads (inter packet interval and packet size).
//  It is also possible to configure TX power per node as well as their CCA-ED tresholds.
//  OBSS_PD spatial reuse feature can be enabled (default) or disabled, and the OBSS_PD
//  threshold can be set as well (default: -82 dBm).
//  A simple Friis path loss model is used and a constant PHY rate is considered.
//
//  In general, the program can be configured at run-time by passing command-line arguments.
//  The following command will display all of the available run-time help options:
//    ./waf --run "wifi-spatial-reuse --help"
//
//  By default, the script shows the benefit of the OBSS_PD spatial reuse script
//    ./waf --run wifi-spatial-reuse-var
//
// If one disables the OBSS_PD feature, a lower throughput is obtained per BSS:
//    ./waf --run "wifi-spatial-reuse --enableObssPd=0"
//
// This difference between those results is because OBSS_PD spatial
// enables to ignore transmissions from another BSS when the received power
// is below the configured threshold, and therefore either defer during ongoing
// transmission or transmit at the same time.
//

#include <iomanip>
#include <iostream>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/gnuplot-helper.h"
#include "ns3/file-helper.h"

using namespace ns3;
uint32_t numSta = 5;       //number of stations
uint32_t numAp = 2;       //number of stations
uint32_t numNodes = numSta*numAp+numAp; //number of nodes

// Global variables for use in callbacks.
std::vector<double> signalDbmAvg(numNodes);
std::vector<double> noiseDbmAvg(numNodes);

std::vector<uint64_t>  totalPacketsSent (numAp);
std::vector<uint64_t>  totalPacketsThrough  (numAp);
std::vector<double> throughput (numAp);
std::vector<uint64_t> packetsReceived(numNodes);
std::vector<uint64_t> packetsSent (numNodes);
/***************************************************************************/


// Trace functions
/***************************************************************************/
// Trace functions for node id extraction
uint32_t ContextToNodeId(std::string context)
{
  std::string sub = context.substr(10);
  uint32_t pos = sub.find("/Device");
  return atoi(sub.substr(0, pos).c_str());
}

// Trace function for received packets and SNR
void MonitorSniffRx (std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  uint32_t nodeId = ContextToNodeId(context);
  packetsReceived[nodeId]++;
  signalDbmAvg[nodeId] += ((signalNoise.signal - signalDbmAvg[nodeId]) / packetsReceived[nodeId]);
  noiseDbmAvg[nodeId] += ((signalNoise.noise - noiseDbmAvg[nodeId]) / packetsReceived[nodeId]);
}

// Trace function for sent packets
void MonitorSniffTx(std::string context, const Ptr< const Packet > packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu)
{
  uint32_t nodeId = ContextToNodeId(context);
  packetsSent[nodeId]++;
}

// Trace function for remaining energy at node.
void RemainingEnergy (double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
}

// Trace function for total energy consumption at node.
void TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}
/***************************************************************************/

int main(int argc, char *argv[])
{
  double duration = 10.0;         // seconds
  double d1 = 30.0;               // meters
  double d2 = 30.0;               // meters
  double d3 = 150.0;              // meters
  double powSta = 10.0;           // dBm
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -62;        // dBm
  double ccaEdTrAp = -62;         // dBm
  uint32_t payloadSize = 1500;    // bytes
  uint32_t mcs = 0;               // MCS value
  double interval = 0.001;        // seconds
  double obssPdThreshold = -82.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0

  CommandLine cmd;
  cmd.AddValue("duration", "Duration of simulation (s)", duration);
  cmd.AddValue("interval", "Inter packet interval (s)", interval);
  cmd.AddValue("enableObssPd", "Enable/disable OBSS_PD", enableObssPd);
  cmd.AddValue("d1", "Distance between STA and AP (m)", d1);
  cmd.AddValue("d2", "Distance between STA and AP (m)", d2);
  cmd.AddValue("d3", "Distance between AP and AP (m)", d3);
  cmd.AddValue("powSta", "Power of STA (dBm)", powSta);
  cmd.AddValue("powAp", "Power of AP (dBm)", powAp);
  cmd.AddValue("ccaEdTrSta", "CCA-ED Threshold of STA (dBm)", ccaEdTrSta);
  cmd.AddValue("ccaEdTrAp", "CCA-ED Threshold of AP (dBm)", ccaEdTrAp);
  cmd.AddValue("mcs", "The constant MCS value to transmit HE PPDUs", mcs);
  cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.AddValue("batteryLevel", "Initial energy level (J)", batteryLevel);
  cmd.AddValue("numAp", "Number of Wifi Access Points", numAp);
  cmd.AddValue("numSta", "Number of Wifi Stations per AP", numSta);
  cmd.AddValue("technology", "Select technology to be used. 802.11ax = 0, 5G = 1", technology);
  cmd.Parse(argc, argv);


  //criar containers de access points e STA
  /***************************************************************************/
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(numSta*numAp);

  NodeContainer wifiApNodes;
  wifiApNodes.Create(numAp);
  /***************************************************************************/


  //spectrum definition
  /***************************************************************************/
  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default();
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
  spectrumChannel->AddPropagationLossModel(lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
  spectrumChannel->SetPropagationDelayModel(delayModel);

  spectrumPhy.SetChannel(spectrumChannel);
  spectrumPhy.SetErrorRateModel("ns3::YansErrorRateModel");
  spectrumPhy.Set("Frequency", UintegerValue(5180)); // channel 36 at 20 MHz
  spectrumPhy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");
  /***************************************************************************/


  //creating wifi helper
  WifiHelper wifi;                                  //helps to create WifiNetDevice objects
  wifi.SetStandard(WIFI_PHY_STANDARD_80211ax_5GHZ); //define standard como 802.11ax 5GHz
  std::ostringstream oss;
  oss << "HeMcs" << mcs;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue(oss.str()),
                               "ControlMode", StringValue(oss.str()));

  if (enableObssPd){
    wifi.SetObssPdAlgorithm("ns3::ConstantObssPdAlgorithm",
                            "ObssPdLevel", DoubleValue(obssPdThreshold));
  }
  
  WifiMacHelper mac; //base class for all MAC-level wifi objects.
  std::vector <Ssid> ssid (numAp); //The IEEE 802.11 SSID Information Element.
  NetDeviceContainer staDevice = NetDeviceContainer();
  std::vector <NetDeviceContainer > apDevice (numAp);
  std::vector <Ptr<WifiNetDevice> > ap2Device (numAp);
  //Ptr<ApWifiMac> apWifiMac;
  
  for (uint32_t i = 0; i < numAp; i++){
    spectrumPhy.Set("TxPowerStart", DoubleValue(powSta));
    spectrumPhy.Set("TxPowerEnd", DoubleValue(powSta));
    spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    spectrumPhy.Set("RxSensitivity", DoubleValue(-92.0));

    //SSID creation
    ssid[i] = Ssid(std::to_string(i));//The IEEE 802.11 SSID Information Element.
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid[i]));

    
    for (uint32_t j = 0; j < numSta; j++){
      staDevice.Add(wifi.Install(spectrumPhy, mac, wifiStaNodes.Get(i*numSta+j)));
    }

    //AP creation
    spectrumPhy.Set("TxPowerStart", DoubleValue(powAp));
    spectrumPhy.Set("TxPowerEnd", DoubleValue(powAp));
    spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrAp));
    spectrumPhy.Set("RxSensitivity", DoubleValue(-92.0));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid[i]));
    apDevice[i] = wifi.Install(spectrumPhy, mac, wifiApNodes.Get(i));

    ap2Device[i] = apDevice[i].Get(0)->GetObject<WifiNetDevice>();
    //apWifiMac = ap2Device[i]->GetMac()->GetObject<ApWifiMac>();

    //Sets BSS color 
    if (enableObssPd){
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i+1));
    }
}


  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (batteryLevel));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (wifiStaNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = DeviceEnergyModelContainer();
  
  deviceModels.Add(radioEnergyHelper.Install (staDevice, sources));
  /***************************************************************************/


  //mobility model
  /***************************************************************************/
  MobilityHelper mobility;
  
/*
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(-d3/2, 0.0, 0.0)); // AP1
  positionAlloc->Add(Vector(d3/2, 0.0, 0.0));  // AP2

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(wifiApNodes);
*/
  
  // Position allocator for the start of the simulation
  // mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
  //                                "X", StringValue ("150.0"),
  //                                "Y", StringValue ("150.0"),
  //                                "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));

  // // The position allocator that will be used by the RandomWaypointMobilityModel
  // GridPositionAllocator posAllocator;
  // posAllocator.SetMinX(-d3);
  // posAllocator.SetMinY(-d3);
  // posAllocator.SetDeltaX(5.0);
  // posAllocator.SetDeltaY(5.0);
  // posAllocator.SetLayoutType(ns3::GridPositionAllocator::ROW_FIRST);
  
  // mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
  //                            "PositionAllocator", PointerValue(&posAllocator));

  
  mobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel","MaxX",DoubleValue(d3), "MaxY",DoubleValue(d3));
  mobility.Install (wifiStaNodes);
  mobility.InstallAll();
  /***************************************************************************/


  /* Internet stack*/
  /***************************************************************************/
  InternetStackHelper stack;
  stack.Install (wifiApNodes);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("192.168.0.0", "255.255.0.0");
  Ipv4InterfaceContainer staNodeInterface;
  Ipv4InterfaceContainer apNodeInterface;
  /***************************************************************************/

  /* Setting packet generator and sink applications */
  /***************************************************************************/
  std::vector <ApplicationContainer> serverApp(numAp);
  for (uint32_t i = 0; i < numAp; i++){
    apNodeInterface.Add (address.Assign (apDevice[i]));
    
    if (udp)
      {
        //UDP flow
        uint16_t port = 9;
        UdpServerHelper server (port);
        serverApp[i] = server.Install (wifiApNodes.Get (i));
        serverApp[i].Start (Seconds (0.0));
        serverApp[i].Stop (Seconds (duration + 1));

        for (uint32_t j = 0; j< numSta; j++){
          staNodeInterface.Add (address.Assign (staDevice.Get(i*numSta+j)));

          UdpClientHelper client (apNodeInterface.GetAddress (i), port);
          client.SetAttribute ("MaxPackets", UintegerValue (0));
          client.SetAttribute ("Interval", TimeValue(Seconds(interval))); //packets/s
          client.SetAttribute ("PacketSize", UintegerValue (payloadSize));
          ApplicationContainer clientApp = client.Install (wifiStaNodes.Get(i*numSta+j));
          clientApp.Start (Seconds (1.0));
          clientApp.Stop (Seconds (duration + 1));
        }
      }
    else
      {
        //TCP flow
        uint16_t port = 50000;
        Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
        serverApp[i] = packetSinkHelper.Install (wifiApNodes.Get (i));
        serverApp[i].Start (Seconds (0.0));
        serverApp[i].Stop (Seconds (duration + 1));

        for (uint32_t j = 0; j< numSta; j++){
          staNodeInterface.Add (address.Assign (staDevice.Get(i*numSta+j)));

          OnOffHelper onoff ("ns3::TcpSocketFactory", Ipv4Address::GetAny ());
          onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
          onoff.SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s
          AddressValue remoteAddress (InetSocketAddress (apNodeInterface.GetAddress (i), port));
          onoff.SetAttribute ("Remote", remoteAddress);
          ApplicationContainer clientApp = onoff.Install (wifiStaNodes.Get(i*numSta+j));
          clientApp.Start (Seconds (1.0));
          clientApp.Stop (Seconds (duration + 1));
        }
      }
  }
  /***************************************************************************/


  /** connect trace sources **/
  /***************************************************************************/
  // energy source
  for (uint32_t i = 0; i < numAp*numSta; i++){
    Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (i));
    basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
    // device energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr =
      basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
    NS_ASSERT (basicRadioModelPtr != NULL);
    basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
  } 
  /***************************************************************************/


  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferTx", MakeCallback (&MonitorSniffTx));
  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&MonitorSniffRx));


  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Stop(Seconds(duration+1));
  Simulator::Run();

  //energy logging
  /***************************************************************************/
  for (uint32_t i = 0; i < numSta*numAp; i++){
      double energyConsumed = deviceModels.Get(i)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                     << "s) - Node " << i << " - Total energy consumed by radio = " << energyConsumed << "J");
    }
  /***************************************************************************/


  //packet loss and snr logging
  /***************************************************************************/
  std::cout << std::setw (5) << "index" <<
  std::setw (12) << "Tput (Mb/s)" <<
  std::setw (12) << "Signal (dBm)" <<
  std::setw (12) << "Noise (dBm)" <<
  std::setw (12) << "SNR (dB)" <<
  std::setw (12) << "Packet loss" <<
  std::endl;

  for (uint32_t i = 0; i < numAp; i++){
    if (udp){
      //UDP
      totalPacketsThrough[i] = DynamicCast<UdpServer> (serverApp[i].Get (0))->GetReceived ();
      throughput[i] = totalPacketsThrough[i] * payloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }else{
      //TCP
      uint64_t totalBytesRx = DynamicCast<PacketSink> (serverApp[i].Get (0))->GetTotalRx ();
      totalPacketsThrough[i] = totalBytesRx / payloadSize;
      throughput[i] = totalBytesRx * 8 / (duration* 1000000.0); //Mbit/s
    }

    for (uint32_t j = i*numSta; j < (1+i)*numSta; j++){
      totalPacketsSent[i]+=packetsSent[j];
    }

    std::cout << std::setw (5) << i+1 <<
    std::setw (12) << throughput[i] <<
    std::setw (12) << signalDbmAvg[numSta*numAp + i] <<
    std::setw (12) << noiseDbmAvg[numSta*numAp + i] <<
    std::setw (12) << (signalDbmAvg[numSta*numAp + i] - noiseDbmAvg[numSta*numAp + i]) <<
    std::setw (12) << (1-(float)totalPacketsThrough[i]/totalPacketsSent[i]) <<
    std::setw (12) << totalPacketsThrough[i] <<
    std::setw (12) << totalPacketsSent[i] <<
    std::endl;
  }
  /***************************************************************************/

//   std::string probeType = "ns3::Ipv4PacketProbe";
//   std::string tracePath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";

// //output bytes logging
//   GnuplotHelper plotHelper;
//   plotHelper.PlotProbe (probeType,
//                         tracePath,
//                         "OutputBytes",
//                         "Packet Byte Count",
//                         GnuplotAggregator::KEY_BELOW);

//   FileHelper fileHelper;
//   fileHelper.ConfigureFile ("seventh-packet-byte-count",
//                              FileAggregator::FORMATTED);
//   fileHelper.Set2dFormat ("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
//   fileHelper.WriteProbe (probeType,
//                           tracePath,
//                           "OutputBytes");


/*
  std::cout << std::setw (5) << "Node index" <<
  std::setw (20) << "Packet loss" <<
  std::setw (20) << "Energy" <<
  std::setw (20) << "Test" <<
  std::endl;

  for (uint32_t i = 0; i < numNodes; i++)
  {
  std::cout << std::setw (5) << i <<
  std::setw (20) << (packetsReceived[i]/packetsSent[i]) <<
  std::setw (20) << 0 <<
  std::setw (20) << (packetsReceived[i])  <<
  std::endl;
  }
  */
  
  Simulator::Destroy();
  return 0;
}
