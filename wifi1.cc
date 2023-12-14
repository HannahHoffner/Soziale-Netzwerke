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

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/stats-module.h"

#include <fstream>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WIFINetworkAndLAN");

// tracing time and byte count
static void
   CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
   {
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
   }

int
main(int argc, char* argv[])
{
    // for saving traces 
    std:: string probeName = "ns3::Ipv4PacketProbe";
    std::string probeTrace  = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";

    // logging
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    
    
    // connection between the 2 routers
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // configures the download and upload speed between the routers
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);
    p2pDevices.Get(0)->SetAttribute("DataRate", StringValue("50Mbps"));
    p2pDevices.Get(1)->SetAttribute("DataRate", StringValue("1Gb/s"));


    // stores the 2 server and 1 ISP Router in the LAN network
    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1)); // ISP Router
    csmaNodes.Create(2); // Server

    // attributes for LAN
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);


    // stores the 5 clients in WIFI Network
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(5);
	
    // stores the WIFI Router
    NodeContainer wifiApNode = p2pNodes.Get(0);


    // configuration of WIFI
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;

    // net device for clients
    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);


    // net device for WIFI router
    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiApNode);

    // configures mobility
    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));

    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
  
    // configures the internet stack for whole network
    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    // configures IP Addresses for each network and the 2 routers
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiInterfaces;
    wifiInterfaces = address.Assign(staDevices);
    wifiInterfaces.Add(address.Assign(apDevices));

    // configures UDP in LAN
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(2));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(2), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    
    // Create a BulkSendApplication and install it on client 0 and 1
    uint16_t bulkPort = 5001;
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(wifiInterfaces.GetAddress(5), bulkPort));
    source.SetAttribute("MaxBytes", UintegerValue(0)); // 0 is unlimited
    ApplicationContainer bulkApps = source.Install(wifiStaNodes.Get(0));
    bulkApps.Add(source.Install(wifiStaNodes.Get(1)));
    bulkApps.Start(Seconds(0.0));
    bulkApps.Stop(Seconds(10.0));
 
    // Create a PacketSinkApplication and install it on wifi router
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), bulkPort));
    ApplicationContainer sinkApps = sink.Install(wifiApNode.Get(0));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    // tcp sockets for tracing for bulk clients
    Ptr<Socket> ns3TcpSocket0 = Socket::CreateSocket (wifiStaNodes.Get (0), TcpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (wifiStaNodes.Get (1), TcpSocketFactory::GetTypeId ());

    // Create an OnOffApplication as IOT Device and install in on clients 2, 3 and 4
    uint16_t iotport = 5002;
    OnOffHelper onoff ("ns3::TcpSocketFactory", InetSocketAddress(wifiInterfaces.GetAddress (5), iotport));
    onoff.SetAttribute ("DataRate", StringValue ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1000));
    onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]")); // On for 2 seconds
    onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]")); // Off for 2 seconds

    ApplicationContainer iotApps = onoff.Install (wifiStaNodes.Get (2));
    iotApps.Add(onoff.Install (wifiStaNodes.Get(3)));
    iotApps.Add(onoff.Install (wifiStaNodes.Get(4)));
    iotApps.Start (Seconds (0.0));
    iotApps.Stop (Seconds (10.0)); 

    // Create a PacketSinkApplication for the iot clients and install it on wifi router
    PacketSinkHelper sinkIOT("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), iotport));
    ApplicationContainer sinkIOTApp = sinkIOT.Install(wifiApNode.Get(0));
    sinkIOTApp.Start(Seconds(0.0));
    sinkIOTApp.Stop(Seconds(10.0));

    // tcp sockets for tracing for iot clients
    Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (wifiStaNodes.Get (2), TcpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (wifiStaNodes.Get (3), TcpSocketFactory::GetTypeId ());
    Ptr<Socket> ns3TcpSocket4 = Socket::CreateSocket (wifiStaNodes.Get (4), TcpSocketFactory::GetTypeId ());

  
    // populates routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    // runs simulation for 10 seconds
    Simulator::Stop(Seconds(10.0));

    // tracing
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("seventh.cwnd");
    ns3TcpSocket0->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    ns3TcpSocket3->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    ns3TcpSocket4->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

 
    // write to txt files
    FileHelper fileHelper;
    fileHelper.ConfigureFile ("seventh-packet-byte-count", FileAggregator::FORMATTED);
    fileHelper.Set2dFormat ("Time (Seconds) = %.3e\tPacket Byte Count = %.0f"); 
    fileHelper.WriteProbe (probeName, probeTrace, "OutputBytes");

    NS_LOG_INFO("Run Simulation");
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Done.");
 
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Bulk Received: " << sink1->GetTotalRx() << std::endl;

    Ptr<PacketSink> sink2 = DynamicCast<PacketSink>(sinkIOTApp.Get(0));
    std::cout << "Total Bytes IOT Received: " << sink2->GetTotalRx() << std::endl;


    return 0;
}