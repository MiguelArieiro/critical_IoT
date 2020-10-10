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
#include <string>
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
#include "ns3/csma-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-address-helper.h"

using namespace ns3;
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
  bool logging = false;           // enable logging
  double duration = 10.0;         // seconds
  double d1 = 30.0;               // meters
  double d2 = 30.0;               // meters
  double d3 = 150.0;              // meters
  double powSta = 10.0;           // dBm
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -62;        // dBm
  double ccaEdTrAp = -62;         // dBm
  uint32_t TCPpayloadSize = 60;   // bytes
  uint32_t UDPpayloadSize = 40;   // bytes
  uint32_t mcs = 0;               // MCS value
  double interval = 0.001;        // seconds
  double obssPdThreshold = -82.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 20;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 5G = 1;

  CommandLine cmd;
  cmd.AddValue("logging", "Set to 1 to turn on logging", logging);
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
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 5G, 2 = 802.11n 2.4GHz, 3 = 802.11n 5GHz", technology);
  cmd.Parse(argc, argv);

  //criar containers de access points e STA
  /***************************************************************************/
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(numSta * numAp);

  NodeContainer wifiApNodes;
  wifiApNodes.Create(numAp);

  NodeContainer serverNode;
  serverNode.Create(1);
  /***************************************************************************/

  //CSMA channel
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("1000Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install(wifiApNodes);
  csmaDevices.Add(csma.Install(serverNode));

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
  WifiHelper wifi; //helps to create WifiNetDevice objects

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

  WifiMacHelper mac;             //base class for all MAC-level wifi objects.
  std::vector<Ssid> ssid(numAp); //The IEEE 802.11 SSID Information Element.
  NetDeviceContainer staDevice = NetDeviceContainer();
  std::vector<NetDeviceContainer> apDevice(numAp);
  std::vector<Ptr<WifiNetDevice>> ap2Device(numAp);
  //Ptr<ApWifiMac> apWifiMac;

  for (uint32_t i = 0; i < numAp; i++)
  {
    spectrumPhy.Set("TxPowerStart", DoubleValue(powSta));
    spectrumPhy.Set("TxPowerEnd", DoubleValue(powSta));
    spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    spectrumPhy.Set("RxSensitivity", DoubleValue(-92.0));

    //SSID creation
    ssid[i] = Ssid(std::to_string(i)); //The IEEE 802.11 SSID Information Element.
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid[i]));

    for (uint32_t j = 0; j < numSta; j++)
    {
      staDevice.Add(wifi.Install(spectrumPhy, mac, wifiStaNodes.Get(i * numSta + j)));
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
    if ((technology == 0) && enableObssPd)
    {
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
    }
  }

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

  deviceModels.Add(radioEnergyHelper.Install(staDevice, sources));
  /***************************************************************************/

  //mobility model
  /***************************************************************************/
  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(serverNode);

  mobility.SetMobilityModel("ns3::SteadyStateRandomWaypointMobilityModel", "MaxX", DoubleValue(d3), "MaxY", DoubleValue(d3));
  mobility.Install(wifiStaNodes);
  mobility.Install(wifiApNodes);
  /***************************************************************************/

  /* Internet stack*/
  /***************************************************************************/
  InternetStackHelper stack;
  OlsrHelper olsr;

  stack.Install(serverNode);
  stack.SetRoutingHelper(olsr);
  stack.Install(wifiStaNodes);
  //stack.SetRoutingHelper(olsr);
  stack.Install(wifiApNodes);
  //stack.SetRoutingHelper(olsr);

  Ipv4AddressHelper address;

  Ipv4InterfaceContainer csmaNodeInterface;
  address.SetBase("10.168.0.0", "255.255.255.0");
  csmaNodeInterface = address.Assign(csmaDevices);

  Ipv4InterfaceContainer apNodeInterface;
  // address.SetBase("192.168.254.0", "255.255.255.0");

  // for (uint32_t i = 0; i < numAp; i++)
  // {
  //   apNodeInterface.Add(address.Assign(apDevice[i]));
  // }

  /***************************************************************************/

  std::vector<Ipv4InterfaceContainer> staNodeInterface(numAp);

  /* Setting packet generator and sink applications */
  /***************************************************************************/

  ApplicationContainer serverApp;

  uint16_t port;
  //sets base STA+AP lan address
  address.SetBase("192.168.0.0", "255.255.255.0");

  if (udp)
  {
    //UDP flow
    port = 9;
    UdpServerHelper server(port);
    serverApp = server.Install(serverNode.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(duration + 1));
  }
  else
  {
    //TCP flow
    port = 50000;
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    serverApp = packetSinkHelper.Install(serverNode.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(duration + 1));
  }

  for (uint32_t i = 0; i < numAp; i++)
  {
    apNodeInterface.Add(address.Assign(apDevice[i]));

    if (udp)
    {
      for (uint32_t j = 0; j < numSta; j++)
      {
        staNodeInterface[i].Add(address.Assign(staDevice.Get(i * numSta + j)));

        UdpClientHelper client(csmaNodeInterface.GetAddress(numAp), port);
        client.SetAttribute("MaxPackets", UintegerValue(0));
        client.SetAttribute("Interval", TimeValue(Seconds(interval))); //packets/s
        client.SetAttribute("PacketSize", UintegerValue(UDPpayloadSize));
        ApplicationContainer clientApp = client.Install(wifiStaNodes.Get(i * numSta + j));
        clientApp.Start(Seconds(0));
        clientApp.Stop(Seconds(duration));
      }
    }
    else
    {
      for (uint32_t j = 0; j < numSta; j++)
      {
        staNodeInterface[i].Add(address.Assign(staDevice.Get(i * numSta + j)));

        std::cout << staNodeInterface[i].GetAddress(j) << std::endl;
        std::cout << csmaNodeInterface.GetAddress(1) << std::endl;

        OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaNodeInterface.GetAddress(numAp), port)));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute("PacketSize", UintegerValue(TCPpayloadSize));
        onoff.SetAttribute("DataRate", DataRateValue(TCPpayloadSize / interval * 8)); //bit/s

        ApplicationContainer clientApp = onoff.Install(wifiStaNodes.Get(i * numSta + j));
        clientApp.Start(Seconds(0));
        clientApp.Stop(Seconds(duration));
      }
    }

    // Assign a new network prefix for the next LAN, according to the
    // network mask initialized above
    address.NewNetwork();
  }
  /***************************************************************************/

  /** connect trace sources **/
  /***************************************************************************/
  //energy source
  for (uint32_t i = 0; i < numAp * numSta; i++)
  {
    Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource>(sources.Get(i));

    if (logging)
    {
      basicSourcePtr->TraceConnectWithoutContext("RemainingEnergy", MakeCallback(&RemainingEnergy));
    }

    // device energy model
    Ptr<DeviceEnergyModel> basicRadioModelPtr =
        basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
    NS_ASSERT(basicRadioModelPtr != NULL);

    if (logging)
    {
      basicRadioModelPtr->TraceConnectWithoutContext("TotalEnergyConsumption", MakeCallback(&TotalEnergy));
    }
  }
  /***************************************************************************/

  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferTx", MakeCallback(&MonitorSniffTx));
  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&MonitorSniffRx));

  //flow monitor logging
  /**************************************************************************/
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(duration + 1));
  Simulator::Run();
  flowMonitor->SerializeToXmlFile("testflow.xml", true, true);

  //energy logging
  /***************************************************************************/
  for (uint32_t i = 0; i < numSta * numAp; i++)
  {
    double energyConsumed = deviceModels.Get(i)->GetTotalEnergyConsumption();
    NS_LOG_UNCOND("End of simulation (" << Simulator::Now().GetSeconds()
                                        << "s) - Node " << i << " - Total energy consumed by radio = " << energyConsumed << "J");
  }
  /***************************************************************************/

  //TODO fix logging
  //packet loss and snr logging
  /***************************************************************************/
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
  std::cout << numNodes << " " << numAp << " " << numSta << std::endl;
  std::cout << totalPacketsThrough[0] << " " << packetsReceived[numNodes - 1] << " " << totalPacketsSent[0] << std::endl;

  // uint64_t totalBytesRx = DynamicCast<PacketSink>(csmaDevices.Get(0))->GetTotalRx();
  // std::cout << totalBytesRx << std::endl;

  Simulator::Destroy();
  return 0;
}
