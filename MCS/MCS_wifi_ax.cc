
/*

1) Create dir
mkdir -p res_MCS/expid_25_run__1_tcp
mkdir -p res_MCS/expid_25_run__1_udp

20210221 -- More or less ok with 2 STAs.
 ./waf --run "MCS --dir=expid_25_run__1_tcp --run=1 --duration=10 --technology=0 --numAp=1 --numSta=2 --powSta=18 --powAp=23 --mcs=11 --udp=0 --distance=1 --frequency=5 --channelWidth=20 --numTxSpatialStreams=4 --numRxSpatialStreams=4 --numAntennas=4 --speed=0 --powerMode=0 --guardInterval=800"
 ./waf --run "MCS --dir=expid_25_run__1_udp --run=1 --duration=10 --technology=0 --numAp=1 --numSta=2 --powSta=18 --powAp=23 --mcs=11 --udp=1 --distance=1 --frequency=5 --channelWidth=20 --numTxSpatialStreams=4 --numRxSpatialStreams=4 --numAntennas=4 --speed=0 --powerMode=0 --guardInterval=800"




20210221 -- Not good with with several  STAs and several APs.
./waf --run "MCS --dir=expid_26_run__1_tcp_160 --run=1 --duration=10 --technology=0 --numAp=2 --numSta=2 --powSta=18 --powAp=23 --mcs=11 --udp=1 --distance=1 --frequency=5 --channelWidth=160 --numTxSpatialStreams=4 --numRxSpatialStreams=4 --numAntennas=4 --speed=0 --powerMode=0 --guardInterval=800"

./waf --run "MCS --dir=expid_26_run__1_tcp_160 --run=1 --duration=10 --technology=0 --numAp=1 --numSta=25 --powSta=18 --powAp=23 --mcs=11 --udp=1 --distance=5 --frequency=5 --channelWidth=160 --numTxSpatialStreams=4 --numRxSpatialStreams=4 --numAntennas=4 --speed=0 --powerMode=0 --guardInterval=800"



 Current Issues:
  FIXME: 1- Data Rate of TCP applications is not working as intended.
  FIXME: 2- Multiple APs are not working. For instance using 2APs with a single STA the second STA is not able to send receive traffic
            ./waf --run "MCS --dir=expid_26_run__1_tcp_160 --run=1 --duration=10 --technology=0 --numAp=2 --numSta=1 --powSta=18 --powAp=23 --mcs=11 --udp=1 --distance=50 --frequency=5 --channelWidth=160 --numTxSpatialStreams=4 --numRxSpatialStreams=4 --numAntennas=4 --speed=0 --powerMode=0 --guardInterval=800"

  FIXME: 3-

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

//#include "ns3/flow-monitor-module.h" //Added
#include "ns3/energy-module.h"
#include "ns3/nr-module.h" //For NR 5G
#include "ns3/lte-module.h" // For LTE

//#include "ns3/config-store-module.h" //Added
//#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"
//#include "ParetoDist.h"
#include "ns3/random-variable-stream.h" //Modified
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"


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

NS_LOG_COMPONENT_DEFINE("MCS_Wifi_ax");

//#include "MCS_wifi_ax.h"

int64_t lastTimeRemaining = 0;
int64_t lastTimeTotal = 0;

uint32_t timeStartServerApps = 1000;
uint32_t timeStartClientApps = 2000;

std::string outputDir = "res_MCS/";
std::string simTag = "wifi_ax";



//Voice PTT Application
uint16_t pttPortServer = 1;
uint16_t pttPortCli = 10;
const uint32_t minSizeVoicePTT = 20;
const uint32_t maxSizeVoicePTT = 200;
const double shapeSizeVoicePTT = 0.5;
const double scaleSizeVoicePTT = 1;
const double betaIntervalVoicePTT = 0.03333333; // 1 flow each 30s = 1/30

//Voice FD application
uint16_t FDPortServer = 2;
uint16_t FDPortCli = 200;
const uint32_t minSizeVoiceFD = 20;
const uint32_t maxSizeVoiceFD = 200;
const double shapeSizeVoiceFD = 0.5;
const double scaleSizeVoiceFD = 1;
const double betaIntervalVoiceFD = 0.03333333;

//Location data application
uint16_t LocDataPortServer = 3;
uint16_t LocDataPortCli = 300;
const uint32_t minSizeLocData = 36;
const uint32_t maxSizeLocData = 100;
const double shapeSizeLocData = 0.5;
const double scaleSizeLocData = 1;
const double betaIntervalLocData = 0.03333333;

//IoT data application
uint16_t IoTDataPortServer = 5;
uint16_t IoTDataPortCli = 500;
const uint32_t minSizeIoTData = 8;
const uint32_t maxSizeIoTData = 500;
const double shapeSizeIoTData = 0.5;
const double scaleSizeIoTData = 1;
const double betaIntervalIoTData = 0.03333333;

//Video Streaming Application
uint16_t VideoPortServer = 6;
uint16_t VideoPortCli = 600;
const uint32_t minSizeVideo = 250;
const uint32_t maxSizeVideo = 1300;
const double shapeSizeVideo = 0.5;
const double scaleSizeVideo = 1;
const double betaIntervalVideo = 0.03333333;

//const double div_factor = 1e5;

const uint8_t sizeCoAP_header = 20; //8 (Options UDP) + 4 CoAp + 8 CoAP
//const uint8_t sizeMQTT_header = 44; //40 (Options TCP) + 2 MQTT + 2 MQTT  // FIXME: TCP is not working

FILE * FIenergyRemaining;
FILE * FIenergyConsumed;


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



std::string get_service(int port )
{
  std::string service="";

  if (port == pttPortServer) return "voicePTT";
  if (port == FDPortServer) return "voiceFD";
  if (port == IoTDataPortServer) return "IoTData";
  if (port == LocDataPortServer) return "LocData";
  if (port == VideoPortServer) return "Video";

  return (service);
}

/* ival value in bytes to be convert, mul indicates if it is on kilobytes, megabytes...*/
double byte_to_bit(double ival, int mul){
  if (mul ==0) mul=1;
  return (ival*8.0*mul);
}

// Trace function for node id extraction
int32_t ContextToNodeId(std::string context, std::string str="/Device")
{
  std::string sub = context.substr(10);
  int32_t pos = sub.find(str);
  return atoi(sub.substr(0, pos).c_str());
}




/***************************************************************************/
int main(int argc, char *argv[])
{
  //Debug purposes
  bool verbose = true;
  bool testOn = false;
  bool tracing = false;

  int32_t numSta = 2;                          //number of stations per AP
  int32_t numAp = 1;                            //number of stations
  int32_t numNodes = numSta * numAp + numAp + 1; //number of nodes

  //int32_t TCPpayloadSize = 60;    // bytes
  //int32_t UDPpayloadSize = 40;    // bytes

  double duration = 600.0;         // seconds
  double powSta = 10.0;           // dBm 18
  double powAp = 21.0;            // dBm
  double ccaEdTrSta = -82;        // dBm -62
  double ccaEdTrAp = -82;         // dBm
  double rxSensSta = -84;         // dBm
  double rxSensAp = -102;          // dBm
  int32_t mcs = 7;                // MCS value

  std::string speed="0";
  std::string dir_exp="zz";

  double obssPdThreshold = -72.0; // dBm
  bool enableObssPd = true;       // spatial reuse
  bool udp = false;               // udp or tcp
  double batteryLevel = 59598;       // initial battery energy
  int technology = 0;             // technology to be used 802.11ax = 0, 802.11n =1, 5G = 2;
  double distance = 10;           // mobility model quadrant size
  int frequency = 5;              // frequency selection
  int channelWidth = 20;          // channel number
  int numRxSpatialStreams = 2;    // number of Rx Spatial Streams
  int numTxSpatialStreams = 2;    // number of Tx Spatial Streams
  int numAntennas = 2;            // number of Antenas

  int powerMode = 1;              //Configure Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )
  int gi = 3200; //Guard interval in seconds
  int run = 1;



  CommandLine cmd(__FILE__);

  cmd.AddValue("dir", "Directory of experiment and run", dir_exp);
  cmd.AddValue("run", "Directory of experiment and run", run);
  cmd.AddValue("duration", "Duration of simulation (s)", duration);

  cmd.AddValue("powSta", "Power of STA (dBm)", powSta);
  cmd.AddValue("powAp", "Power of AP (dBm)", powAp);

  cmd.AddValue("mcs", "The constant MCS value to transmit HE PPDUs", mcs);
  cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);

  cmd.AddValue("numAp", "Number of Wifi Access Points", numAp);
  cmd.AddValue("numSta", "Number of Wifi Stations per AP", numSta);
  cmd.AddValue("technology", "Select technology to be used. 0 = 802.11ax, 1 = 802.11n, 2 = LTE, 3 = 5G", technology);
  cmd.AddValue("distance", "Distance between networks", distance);

  cmd.AddValue("frequency", "Wifi device frequency. 2 - 2.4GHz, 5 - 5GHz, 6 - 6GHz", frequency);
  cmd.AddValue("channelWidth", "Defines wifi channel number", channelWidth);
  cmd.AddValue("numTxSpatialStreams", "Number of Tx Spatial Streams", numTxSpatialStreams);
  cmd.AddValue("numRxSpatialStreams", "Number of Rx Spatial Streams", numRxSpatialStreams);
  cmd.AddValue("numAntennas", "Number of Rx Spatial Streams", numAntennas);

  cmd.AddValue("speed", "Velocity ", speed); //new m/s

  cmd.AddValue("powerMode", "Settings for Power Mode (0- min per REF1, 1 - per NS3 defaults, 2-per Double of NS3 defaults )", powerMode);
  cmd.AddValue("guardInterval", "GuardInterval in NanoSeconds (800, 3200)", gi);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  cmd.Parse(argc, argv);

  ns3::RngSeedManager::SetSeed(run);
  ns3::RngSeedManager::SetRun(run);

  /*
   *
   */
  Ptr<PacketSink> sinkVoicePTT;
  Ptr<PacketSink> sinkVoiceFD;
  Ptr<PacketSink> sinkLocData;
  Ptr<PacketSink> sinkIoTData;
  Ptr<PacketSink> sinkVideo;


  /* ----------------------------- Start Distributions -------- * /
  For applications size
  */
  double bound =0.0;
  int streamId=1;

  Ptr<ExponentialRandomVariable> expDistVoicePTT    = CreateObject<ExponentialRandomVariable> ();
  expDistVoicePTT->SetAttribute("Mean", DoubleValue(betaIntervalVoicePTT)); // should be 1.0/ betaIntervalVoicePTT
  expDistVoicePTT->SetAttribute("Bound", DoubleValue(bound ));
  expDistVoicePTT->SetAttribute("Stream", IntegerValue(streamId ));

  Ptr<ExponentialRandomVariable> expDistVoiceFD     = CreateObject<ExponentialRandomVariable> ();
  expDistVoiceFD->SetAttribute("Mean", DoubleValue(betaIntervalVoiceFD )); // should be 1.0/ betaIntervalVoiceFD
  expDistVoiceFD->SetAttribute("Bound", DoubleValue(bound ));
  expDistVoiceFD->SetAttribute("Stream", IntegerValue(streamId ));

  Ptr<ExponentialRandomVariable> expDistLocData     = CreateObject<ExponentialRandomVariable> ();
  expDistLocData->SetAttribute("Mean", DoubleValue(betaIntervalLocData )); //
  expDistLocData->SetAttribute("Bound", DoubleValue(bound ));
  expDistLocData->SetAttribute("Stream", IntegerValue(streamId ));

  Ptr<ExponentialRandomVariable> expDistIoTData     = CreateObject<ExponentialRandomVariable> ();
  expDistIoTData->SetAttribute("Mean", DoubleValue(betaIntervalIoTData )); //
  expDistIoTData->SetAttribute("Bound", DoubleValue(bound));
  expDistIoTData->SetAttribute("Stream", IntegerValue(streamId ));

  Ptr<ExponentialRandomVariable> expDistVideoStream = CreateObject<ExponentialRandomVariable> ();
  expDistVideoStream->SetAttribute("Mean", DoubleValue(betaIntervalVideo )); //
  expDistVideoStream->SetAttribute("Bound", DoubleValue(bound));
  expDistVideoStream->SetAttribute("Stream", IntegerValue(streamId ));

  //Exclude mission analysis

  /* ----------------------------- End Distributions -------- */

  //
  //Environment configuration
  //
  numNodes = numSta * numAp + numAp + 1; //updating numNodes
  outputDir = outputDir + "/" + dir_exp+ "/";



  std::string file="";
  file = outputDir+"FIenergyConsumed.csv";
  FIenergyConsumed = fopen(file.c_str(), "w+");
  file = outputDir+"FIenergyConsumed.csv";
  FIenergyRemaining = fopen(file.c_str(), "w+");

  //
  // Nodes
  //
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
    /*
    // See wifi-phy.cc
    if (i==0)
    	phy.Set("Frequency", UintegerValue (5250)); //channel 50at 160Mhz
    if (i==1)
    	phy.Set("Frequency", UintegerValue (5570)); //channel 114 at 160Mhz
    if (i==2)
    	phy.Set("Frequency", UintegerValue (5815)); //channel 163 at 160Mhz
	*/
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    //ApDevices.Add(wifi.Install(phy, mac, wifiApNodes.Get(i)));  //BS_
    NetDeviceContainer BSaPdev = wifi.Install(phy, mac, wifiApNodes.Get(i));
    ApDevices.Add(BSaPdev);  //BS_

    //Sets BSS color and GI Interval
    if ((technology == 0) && enableObssPd)
    {
      Ptr<WifiNetDevice> wnd = BSaPdev.Get(0)->GetObject<WifiNetDevice>();
      wnd->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i + 1));
      //wnd->GetHeConfiguration()->SetAttribute("GuardInterval", NanoSeconds(gi));
      Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue (NanoSeconds (gi)));
    }
  }

  csmaDevices = csma.Install(csmaNodes);

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

  //print positions,
  if (verbose){
	  int idxAP = 0;
	  for (NodeContainer::Iterator iap = wifiApNodes.Begin() ; iap < wifiApNodes.End(); ++iap, idxAP++){
		  Ptr<Node> obj = *iap;
		  Ptr<MobilityModel> positAp = obj->GetObject<MobilityModel>();
		  NS_ASSERT(positAp!=0);
		  Vector pos = positAp->GetPosition();
		  std::cout <<" AP = " << idxAP << " x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
	  }

	  idxAP = 0;
	  for (NodeContainer::Iterator iap = wifiStaNodes.Begin() ; iap < wifiStaNodes.End(); ++iap, idxAP++){
		  Ptr<Node> obj = *iap;
		  Ptr<MobilityModel> positAp = obj->GetObject<MobilityModel>();
		  NS_ASSERT(positAp!=0);
		  Vector pos = positAp->GetPosition();
		  std::cout <<" STA = " << idxAP << " x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
	  }
  }


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



  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  StringValue AtrOnTime, AtrOffTime;
  int IndApp=0;

  // ------- Ini Voice PTT
  PacketSinkHelper packetSinkHelperPTT("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), pttPortServer));
  serverApps.Add( packetSinkHelperPTT.Install(wifiStaNodes) );
  sinkVoicePTT = StaticCast<PacketSink> (serverApps.Get (IndApp));

   for (int32_t i = 0; i < numAp; i++){
    for (int32_t j = 0; j < numSta; j++){
      //Station 0 sends the PTT
      ++pttPortCli;

      //PTT client on server
      Ptr<Ipv4> ipv4 = wifiStaNodes.Get (i*numSta + j)->GetObject<Ipv4>();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback

      UdpClientHelper voicePtt;
      ParetoDist sizeVoicePtt = ParetoDist(minSizeVoicePTT, maxSizeVoicePTT, shapeSizeVoicePTT, scaleSizeVoicePTT);
      uint32_t pktSize = sizeVoicePtt.getTruncatedSize();
      double aint = expDistVoicePTT->GetValue();

      if (verbose){
    	  std::cout << "VoicePTT Size = " << pktSize << " interval = " << aint << std::endl;
      }

      voicePtt.SetAttribute("RemoteAddress", AddressValue(iaddr.GetLocal()) );
      voicePtt.SetAttribute("RemotePort", UintegerValue ( pttPortServer) );
      voicePtt.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
      voicePtt.SetAttribute("PacketSize", UintegerValue ( pktSize) );
      if (testOn) aint =1.0/2.0; //TO_REMOVE_ON_PROD
      voicePtt.SetAttribute("Interval", TimeValue (Seconds( aint ) ));
      clientApps.Add( voicePtt.Install(csmaNodes.Get(numAp)));
    }
   }
  // ------- END Voice PTT


  // ------- Ini Voice FD
  IndApp++;

  UdpEchoServerHelper voiceFDServerCCC (FDPortServer);
  //voiceFDServerCCC.SetAttribute("Port", UintegerValue(FDPortServer));
  serverApps.Add( voiceFDServerCCC.Install(wifiStaNodes) );
  sinkVoiceFD = StaticCast<PacketSink> (serverApps.Get (IndApp));

   for (int32_t i = 0; i < numAp; i++){
    for (int32_t j = 0; j < numSta; j++){

      ++FDPortCli;

      Ptr<Ipv4> ipv4 = wifiStaNodes.Get (i*numSta + j)->GetObject<Ipv4>();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback

      ParetoDist sizeVoiceFD = ParetoDist(minSizeVoiceFD, maxSizeVoiceFD, shapeSizeVoiceFD, scaleSizeVoiceFD);
      uint32_t pktSizeFD = sizeVoiceFD.getTruncatedSize();
      double aintFD = expDistVoiceFD->GetValue();

      if (verbose)
    	  std::cout << "VoiceFD Size = " << pktSizeFD << " intervale = " << aintFD << std::endl;

      UdpEchoClientHelper voiceFD (iaddr.GetLocal(), FDPortServer);
      voiceFD.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
      voiceFD.SetAttribute("PacketSize", UintegerValue ( pktSizeFD) );
      if (testOn) aintFD =1.0/2.0; //TO_REMOVE_ON_PROD
      voiceFD.SetAttribute("Interval", TimeValue (Seconds( aintFD ) ));
      clientApps.Add( voiceFD.Install(csmaNodes.Get(numAp)));
    }
   }
  // ------- END Voice PTT

  if (udp) //As uplink
  {
    //Location Data
    IndApp++;
    PacketSinkHelper packetSinkHelperLocData("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), LocDataPortServer));
    serverApps.Add( packetSinkHelperLocData.Install(csmaNodes.Get(numAp)) );
    sinkLocData = StaticCast<PacketSink> (serverApps.Get (IndApp));

    UdpClientHelper LocDataApp;
    ParetoDist sizeLocData = ParetoDist(minSizeLocData, maxSizeLocData, shapeSizeLocData, scaleSizeLocData);
    uint32_t pktSizeLocData = sizeLocData.getTruncatedSize();
    double aintLocData = expDistLocData->GetValue();

    if (verbose)
    	std::cout << " LocData Size = " << pktSizeLocData << " intervale = " << aintLocData << std::endl;

    LocDataApp.SetAttribute("RemoteAddress", AddressValue(csmaInterfaces.GetAddress(numAp)) );
    LocDataApp.SetAttribute("RemotePort", UintegerValue ( LocDataPortServer) );
    LocDataApp.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
    LocDataApp.SetAttribute("PacketSize", UintegerValue ( pktSizeLocData + sizeCoAP_header) ); //Count CoAP header
    if (testOn) aintLocData =1.0/2.0; //TO_REMOVE_ON_PROD
    LocDataApp.SetAttribute("Interval", TimeValue (Seconds( aintLocData ) ));
    clientApps.Add( LocDataApp.Install(wifiStaNodes) );


    //IoT Data
    IndApp++;
    PacketSinkHelper packetSinkHelperIoTData("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), IoTDataPortServer));
    serverApps.Add( packetSinkHelperIoTData.Install(csmaNodes.Get(numAp)) );
    sinkIoTData = StaticCast<PacketSink> (serverApps.Get (IndApp));

    UdpClientHelper IoTDataApp;
    ParetoDist sizeIoTData = ParetoDist(minSizeIoTData, maxSizeIoTData, shapeSizeIoTData, scaleSizeIoTData);
    uint32_t pktSizeIoTData = sizeIoTData.getTruncatedSize();
    double aintIoTData = expDistIoTData->GetValue();

    if (verbose)
    	std::cout << " IoTData Size = " << pktSizeIoTData << " intervale = " << aintIoTData << std::endl;

    LocDataApp.SetAttribute("RemoteAddress", AddressValue(csmaInterfaces.GetAddress(numAp)) );
    LocDataApp.SetAttribute("RemotePort", UintegerValue ( IoTDataPortServer) );
    LocDataApp.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
    LocDataApp.SetAttribute("PacketSize", UintegerValue ( pktSizeIoTData + sizeCoAP_header) ); //Count CoAP header
    if (testOn) aintIoTData =1.0/2.0; //TO_REMOVE_ON_PROD
    LocDataApp.SetAttribute("Interval", TimeValue (Seconds( aintIoTData ) ));

    clientApps.Add( LocDataApp.Install(wifiStaNodes) );
  }
  else //As uplink
  {
	/*
	 *
	 * Bruno: As per 20210222 TCP apps are not working. The values of packets sent is really low.
	 *  FIXME: TCP apps
	 *
	 *
    //Location Data
    IndApp++;

    PacketSinkHelper packetSinkHelperLocDataTCP("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), LocDataPortServer));
    serverApps.Add ( packetSinkHelperLocDataTCP.Install(csmaNodes.Get(numAp)) );
    sinkLocData = StaticCast<PacketSink> (serverApps.Get (IndApp));

    ParetoDist sizeLocData = ParetoDist(minSizeLocData, maxSizeLocData, shapeSizeLocData, scaleSizeLocData);
    uint32_t pktSizeLocData = sizeLocData.getTruncatedSize() + sizeMQTT_header;
    double aintLocData = expDistLocData->GetValue();

    OnOffHelper LocDataApp("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), LocDataPortServer)));
    //std::string aux = "ns3::ExponentialRandomVariable[Mean=30.3333333|Bound=0]";
    //LocDataApp.SetAttribute("OnTime", StringValue(aux));
    LocDataApp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    LocDataApp.SetAttribute("PacketSize", UintegerValue(pktSizeLocData ));
    double datarateauxLocData= byte_to_bit(pktSizeLocData, 1) / aintLocData; //TODO: confirm size in bytes
    LocDataApp.SetAttribute("DataRate", DataRateValue(DataRate(datarateauxLocData))); //bit/s
    //LocDataApp.SetAttribute("DataRate", DataRateValue(DataRate("600Mbps"))); //bit/s

    if (verbose)
    	std::cout << " LocData Size = " << pktSizeLocData << " interval = " << aintLocData << " dataRate=" << datarateauxLocData  << std::endl;

    clientApps.Add (LocDataApp.Install(wifiStaNodes) );


    //IoT Data
    IndApp++;

    PacketSinkHelper packetSinkHelperIoTDataTCP("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), IoTDataPortServer));
    serverApps.Add ( packetSinkHelperIoTDataTCP.Install(csmaNodes.Get(numAp)) );
    sinkIoTData = StaticCast<PacketSink> (serverApps.Get (IndApp));

    ParetoDist sizeIoTData = ParetoDist(minSizeIoTData, maxSizeIoTData, shapeSizeIoTData, scaleSizeIoTData);
    uint32_t pktSizeIoTData = sizeIoTData.getTruncatedSize() + sizeMQTT_header;
    double aintIoTData = expDistIoTData->GetValue();

    OnOffHelper IoTDataApp("ns3::TcpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(numAp), IoTDataPortServer)));
    IoTDataApp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    IoTDataApp.SetAttribute("PacketSize", UintegerValue(pktSizeIoTData ));
    double datarateauxIoTData= byte_to_bit(pktSizeIoTData, 1) / aintIoTData; //TODO: confirm size in bytes
    IoTDataApp.SetAttribute("DataRate", DataRateValue(DataRate(datarateauxIoTData))); //bit/s

    if (verbose)
    	std::cout << " IoTData Size = " << pktSizeIoTData << " interval = " << aintIoTData  << " dataRate=" << datarateauxIoTData << std::endl;

    clientApps.Add (IoTDataApp.Install(wifiStaNodes) );
	*/
  }

  //
  // ------- Ini Video Stream App
  IndApp++;


  PacketSinkHelper packetSinkHelperVideo("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), VideoPortServer));
  serverApps.Add( packetSinkHelperVideo.Install(wifiStaNodes) );
  sinkVideo = StaticCast<PacketSink> (serverApps.Get (IndApp));

   for (int32_t i = 0; i < numAp; i++){
    for (int32_t j = 0; j < numSta; j++){
      //Station 0 sends the PTT
      ++pttPortCli;

      //video client on server
      Ptr<Ipv4> ipv4 = wifiStaNodes.Get (i*numSta + j)->GetObject<Ipv4>();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0); //Interface 0 is loopback

      UdpClientHelper videoStream;
      ParetoDist sizeVideo = ParetoDist(minSizeVideo, maxSizeVideo, shapeSizeVideo, scaleSizeVideo);
      uint32_t pktSizeVideo = sizeVideo.getTruncatedSize();
      double aintVideo = expDistVideoStream->GetValue();

      if (verbose)
    	  std::cout << "Video Size = " << pktSizeVideo << " intervale = " << aintVideo << std::endl;

      videoStream.SetAttribute("RemoteAddress", AddressValue(iaddr.GetLocal()) );
      videoStream.SetAttribute("RemotePort", UintegerValue ( VideoPortServer) );
      videoStream.SetAttribute("MaxPackets", UintegerValue ( 0xFFFFFFFF) );
      videoStream.SetAttribute("PacketSize", UintegerValue ( pktSizeVideo) );
      if (testOn) aintVideo =1.0/2.0; //TO_REMOVE_ON_PROD
      videoStream.SetAttribute("Interval", TimeValue (Seconds( aintVideo ) ));
      clientApps.Add( videoStream.Install(csmaNodes.Get(numAp)));
    }
   }
  // ------- END Video Stream App

  //Start APPs
  serverApps.Start(MilliSeconds(timeStartServerApps));
  serverApps.Stop(Seconds(duration + 1));
  clientApps.Start(MilliSeconds(timeStartClientApps)); //2.0
  clientApps.Stop(Seconds(duration + 1));

  //Make stats on screen
  //Simulator::Schedule (Seconds (1.1), &CalculateThroughput);


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
  FlowMonitorHelper flowmonHelper;
  flowMonitor = flowmonHelper.InstallAll();
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(duration + 1));

  if (tracing == true)
  {
    //phy.EnablePcap("critical_iot", ApDevices);
    csma.EnablePcap("critical_iot", csmaDevices, true);
  }

  //PrintRouting tables
  // Trace routing tables
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (2), routingStream);

  //Trace
  //Config::ConnectWithoutContext ("/NodeList/0/$ns3::Ipv4L3Protocol/Tx", MakeCallback (&TxTracer));


  //std::cout << "code???\n";
  Simulator::Run();

  file = outputDir+"testflow.xml";
  flowMonitor->SerializeToXmlFile(file.c_str(), true, true);

  // Print per-flow statistics
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats ();

  double averageFlowThroughput = 0.0;
  double averageFlowDelay = 0.0;

  std::ofstream outFile;
  std::string filename = outputDir + "/" + simTag;
  outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::trunc);
  if (!outFile.is_open ())
    {
      std::cerr << "Can't open file " << filename << std::endl;
      return 1;
    }

  outFile.setf (std::ios_base::fixed);

  outFile << "Flow;source;src_port;destiny;dst_port;proto;service;direction;tx_packets;tx_bytes;tx_offered_raw;tx_offered_mbps;rx_bytes;rx_throughput_raw;rx_throughput_mbps;mean_delay(ms);mean_jitter(ms);rx_packets;lost_packets;packet_loss_ratio \n";
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::stringstream protoStream;
      protoStream << (uint16_t) t.protocol;
      if (t.protocol == 6)
        {
          protoStream.str ("TCP");
        }
      if (t.protocol == 17)
        {
          protoStream.str ("UDP");
        }
      outFile <<  i->first << ";";
      outFile <<  t.sourceAddress << ";" << t.sourcePort << ";" << t.destinationAddress << ";" << t.destinationPort  << ";";
      outFile << protoStream.str() << ";";
      if (t.sourcePort <= VideoPortServer){
        //Service , direction
        outFile << get_service(t.sourcePort) << ";";
      }else{
          outFile << get_service(t.destinationPort) << ";";
      }

      if (t.sourceAddress == "10.1.1.10"){
        outFile << "download"  << ";";
      }else {
        outFile << "upload"  << ";";
      }

      outFile <<  i->second.txPackets << ";";
      outFile <<  i->second.txBytes << ";";
      outFile <<  i->second.txBytes * 8.0 / ((timeStartServerApps - timeStartClientApps) / 1000.0)  << ";";
      outFile << i->second.txBytes * 8.0 / ((timeStartServerApps - timeStartClientApps) / 1000.0) / 1000.0 / 1000.0  << ";";
      outFile << i->second.rxBytes << ";";
      if (i->second.rxPackets > 0)
        {
          double rxDuration = (timeStartServerApps - timeStartClientApps) / 1000.0;
          averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
          averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;

          outFile  << i->second.rxBytes * 8.0 / rxDuration  << ";";
          outFile  << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000  << ";";
          outFile  << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << ";";
          outFile  << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << ";";
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
      outFile << ( lost_packets * 1.0 / i->second.txPackets) * 100.0 << "\n";
    }

  //outFile << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size() << "\n";
  //outFile << "  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";

  outFile.close ();
  std::ifstream f (filename.c_str ());
  if (f.is_open())
    {
      std::cout << f.rdbuf();
    }

  //
  // End Simulator
  //
  Simulator::Destroy();

  fclose(FIenergyConsumed);
  fclose(FIenergyRemaining);
  return 0;
}
