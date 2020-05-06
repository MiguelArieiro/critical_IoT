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
//                STA1          STA2
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
//    Throughput for BSS 2: 5.9364 Mbit/s
//
// This difference between those results is because OBSS_PD spatial
// enables to ignore transmissions from another BSS when the received power
// is below the configured threshold, and therefore either defer during ongoing
// transmission or transmit at the same time.
//

#include <iomanip>
#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/internet-module.h"

using namespace ns3;
uint32_t num_sta = 5;       //number os stations
uint32_t num_ap = 2;       //number os stations
std::vector<uint32_t> bytesReceived(num_sta*num_ap+num_ap);

// Global variables for use in callbacks.
std::vector<double> g_signalDbmAvg(num_sta*num_ap+num_ap);
std::vector<double> g_noiseDbmAvg(num_sta*num_ap+num_ap);
std::vector<uint32_t> g_samples(num_sta*num_ap+num_ap);
std::vector <uint64_t>  totalPacketsThrough  (num_ap);
std::vector <double> throughput (num_ap);

uint32_t ContextToNodeId(std::string context)
{
  std::string sub = context.substr(10);
  uint32_t pos = sub.find("/Device");
  return atoi(sub.substr(0, pos).c_str());
}



void MonitorSniffRx (std::string context,
                    Ptr<const Packet> packet,
                    uint16_t channelFreqMhz,
                    WifiTxVector txVector,
                    MpduInfo aMpdu,
                    SignalNoiseDbm signalNoise)

{
  uint32_t nodeId = ContextToNodeId(context);
  g_samples[nodeId]++;
  g_signalDbmAvg[nodeId] += ((signalNoise.signal - g_signalDbmAvg[nodeId]) / g_samples[nodeId]);
  g_noiseDbmAvg[nodeId] += ((signalNoise.noise - g_noiseDbmAvg[nodeId]) / g_samples[nodeId]);
}
void SocketRx(std::string context, Ptr<const Packet> p, const Address &addr)
{
  uint32_t nodeId = ContextToNodeId(context);
  bytesReceived[nodeId] += p->GetSize();
}

int main(int argc, char *argv[])
{
  double duration = 10.0;      // seconds
  double d1 = 30.0;            // meters
  double d2 = 30.0;            // meters
  double d3 = 150.0;           // meters
  double powSta = 10.0;       // dBm
  double powAp = 21.0;        // dBm
  double ccaEdTrSta = -62;    // dBm
  double ccaEdTrAp = -62;     // dBm
  uint32_t payloadSize = 1500; // bytes
  uint32_t mcs = 0;            // MCS value
  double interval = 0.001;     // seconds
  double obssPdThreshold = -72.0; // dBm
  bool enableObssPd = true;
  bool udp = true;            //udp/tcp

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
  cmd.Parse(argc, argv);

  //criar containers de access points e STA
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(num_sta*num_ap);

  NodeContainer wifiApNodes;
  wifiApNodes.Create(num_ap);

  //spectrum definition
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

  //creating wifi helper
  WifiHelper wifi;                                  //helps to create WifiNetDevice objects
  wifi.SetStandard(WIFI_PHY_STANDARD_80211ax_5GHZ); //define standard como 802.11ax
  if (enableObssPd)
  {
    wifi.SetObssPdAlgorithm("ns3::ConstantObssPdAlgorithm",
                            "ObssPdLevel", DoubleValue(obssPdThreshold));
  }

  WifiMacHelper mac; //base class for all MAC-level wifi objects.
  std::ostringstream oss;
  oss << "HeMcs" << mcs;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue(oss.str()),
                               "ControlMode", StringValue(oss.str()));
  
  std::vector <Ssid> ssid (num_ap); //The IEEE 802.11 SSID Information Element.
  std::vector <NetDeviceContainer > staDevice (num_ap);
  std::vector <NetDeviceContainer > apDevice (num_ap);
  std::vector <Ptr<WifiNetDevice> > ap2Device (num_ap);
  //Ptr<ApWifiMac> apWifiMac;
  
  for (uint32_t i = 0; i < num_ap; i++){
    

    spectrumPhy.Set("TxPowerStart", DoubleValue(powSta));
    spectrumPhy.Set("TxPowerEnd", DoubleValue(powSta));
    spectrumPhy.Set("CcaEdThreshold", DoubleValue(ccaEdTrSta));
    spectrumPhy.Set("RxSensitivity", DoubleValue(-92.0));

    //SSID creation
    ssid[i] = Ssid(std::to_string(i));//The IEEE 802.11 SSID Information Element.
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid[i]));

    //creating STA net devices container
    staDevice[i] = NetDeviceContainer();
    for (uint32_t j = 0; j < num_sta; j++)
    {
      staDevice[i].Add(wifi.Install(spectrumPhy, mac, wifiStaNodes.Get(i*num_sta+j)));
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
    if (enableObssPd)
    {
      ap2Device[i]->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(i+1));
    }
}


  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP1
  positionAlloc->Add(Vector(d3, 0.0, 0.0));  // AP2

  positionAlloc->Add(Vector(0.0, d1, 0.0));      // STA1a
  positionAlloc->Add(Vector(-d1, d1, 0.0));      // STA1b
  positionAlloc->Add(Vector(0.0, -d1, 0.0));     // STA1c
  positionAlloc->Add(Vector(-d1, -d1, 0.0));     // STA1d
  positionAlloc->Add(Vector(-d1, 0.0, 0.0));     // STA1e
  positionAlloc->Add(Vector(d3, d2, 0.0));       // STA2a
  positionAlloc->Add(Vector(d3 + d2, d2, 0.0));  // STA2b
  positionAlloc->Add(Vector(d3, -d2, 0.0));      // STA2c
  positionAlloc->Add(Vector(d3 + d2, -d2, 0.0)); // STA2d
  positionAlloc->Add(Vector(d3 + d2, 0.0, 0.0)); // STA2e
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(wifiApNodes);
  mobility.Install(wifiStaNodes);

/*   PacketSocketHelper packetSocket;
  packetSocket.Install(wifiApNodes);
  packetSocket.Install(wifiStaNodes);
  ApplicationContainer apps;
  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

  //BSS
  for (uint32_t i = 0; i < num_ap; i++)
  {
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(staDevice[i].Get(0)->GetIfIndex());
    socketAddr.SetPhysicalAddress(apDevice[i].Get(0)->GetAddress());
    socketAddr.SetProtocol(1);
    for (uint32_t j = 0; j < num_sta; j++)
    {
      Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
      client->SetRemote(socketAddr);
      wifiStaNodes.Get(i*num_sta+j)->AddApplication(client);
      client->SetAttribute("PacketSize", UintegerValue(payloadSize));
      client->SetAttribute("MaxPackets", UintegerValue(0));
      client->SetAttribute("Interval", TimeValue(Seconds(interval)));
    }

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->SetLocal(socketAddr);
    wifiApNodes.Get(i)->AddApplication(server);
  } */
  

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNodes);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer staNodeInterface;
  Ipv4InterfaceContainer apNodeInterface;

  /* Setting applications */
  std::vector <ApplicationContainer> serverApp(num_ap);
  for (uint32_t i = 0; i < num_ap; i++){
    apNodeInterface.Add(address.Assign (apDevice[i]));
     staNodeInterface = address.Assign (staDevice[i]);
    if (udp)
      {
        //UDP flow
        uint16_t port = 9;
        UdpServerHelper server (port);
        serverApp[i] = server.Install (wifiApNodes.Get (i));
        serverApp[i].Start (Seconds (0.0));
        serverApp[i].Stop (Seconds (duration + 1));

        UdpClientHelper client (apNodeInterface.GetAddress (i), port);
        client.SetAttribute ("MaxPackets", UintegerValue (0));
        client.SetAttribute ("Interval", TimeValue(Seconds(interval))); //packets/s
        client.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        ApplicationContainer clientApp = client.Install (wifiStaNodes);
        clientApp.Start (Seconds (1.0));
        clientApp.Stop (Seconds (duration + 1));
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

        OnOffHelper onoff ("ns3::TcpSocketFactory", Ipv4Address::GetAny ());
        onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        onoff.SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s
        AddressValue remoteAddress (InetSocketAddress (apNodeInterface.GetAddress (i), port));
        onoff.SetAttribute ("Remote", remoteAddress);
        ApplicationContainer clientApp = onoff.Install (wifiStaNodes);
        clientApp.Start (Seconds (1.0));
        clientApp.Stop (Seconds (duration+ 1));
      }
  }

  //Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx", MakeCallback(&SocketRx));
  Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&MonitorSniffRx));

  Simulator::Stop(Seconds(duration));
  Simulator::Run();

  Simulator::Destroy();


  std::cout << std::setw (5) << "index" <<
    std::setw (12) << "Tput (Mb/s)" <<
    std::setw (12) << "Signal (dBm)" <<
    std::setw (12) << "Noise (dBm)" <<
    std::setw (12) << "SNR (dB)" <<
    std::endl;
  for (uint32_t i = 0; i < num_ap; i++)
  {
    //double throughput = static_cast<double>(bytesReceived[num_sta*num_ap + i]) * 8 / 1000 / 1000 / duration;

    if (udp)
    {
      //UDP
      totalPacketsThrough[i] = DynamicCast<UdpServer> (serverApp[i].Get (0))->GetReceived ();
      throughput[i] = totalPacketsThrough[i] * payloadSize * 8 / (duration * 1000000.0); //Mbit/s
    }
    else
    {
      //TCP
      uint64_t totalBytesRx = DynamicCast<PacketSink> (serverApp[i].Get (0))->GetTotalRx ();
      totalPacketsThrough[i] = totalBytesRx / payloadSize;
      throughput[i] = totalBytesRx * 8 / (duration* 1000000.0); //Mbit/s
    }

    std::cout << std::setw (5) << i+1 <<
    std::setw (12) << throughput[i] <<
    std::setw (12) << g_signalDbmAvg[num_sta*num_ap + i] <<
    std::setw (12) << g_noiseDbmAvg[num_sta*num_ap + i] <<
    std::setw (12) << (g_signalDbmAvg[num_sta*num_ap + i] - g_noiseDbmAvg[num_sta*num_ap + i]) <<
    std::endl;

  }

  return 0;
}
