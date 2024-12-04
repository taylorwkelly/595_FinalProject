/*
 * Copyright (c) 2016 University of Washington
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
 * Authors: Tom Henderson <tomhend@u.washington.edu>
 *          Matías Richart <mrichart@fing.edu.uy>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Juan Leon <juanleon@uw.edu>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config-store-module.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/gnuplot.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/ssid.h"
#include "ns3/tuple.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iomanip>
#include <string>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("unit-test");

// 290K @ 20 MHz
double noiseDbm = -174.0; //!< Value for noise.

// Simulation Parameters
int g_seed = 0;
double g_test;
double g_SNR = 0;
std::string g_MCS;
uint16_t serverNss = 1;
uint16_t clientNss = 1;
uint16_t serverChannelWidth = 0; // use default for standard and band
uint16_t clientChannelWidth = 0; // use default for standard and band
std::string standard("802.11a");
uint16_t serverShortGuardInterval = 800;
uint16_t clientShortGuardInterval = 800;
int g_packetSize = 0;
bool converged = false;
double m_initialSnr = 0;

WifiTxVector m_txVector;
std::string wifiManager("None");
WifiHelper wifi;
Ptr<WifiPhy> m_phy;


double g_intervalBytes = 0;  //!< Bytes received in an interval.
uint64_t g_intervalRate = 0; //!< Rate in an interval.
uint64_t g_highestRate = 0;
std::vector<int> selectedMCSlist;
std::set<ns3::WifiMode> mcsUsed;
std::vector<std::tuple<double, std::string>> time_mcs;
std::vector<std::tuple<double, double>> startTime_startBytes;
std::vector<std::tuple<double, double>> endTime_endBytes;
std::vector<std::pair<double, WifiTxVector>> m_thresholds;
double m_ber = 1e-6;
std::vector<std::string> modes = {"VhtMcs0",
                                  "VhtMcs1",
                                  "VhtMcs2",
                                  "VhtMcs3",
                                  "VhtMcs4",
                                  "VhtMcs5",
                                  "VhtMcs6",
                                  "VhtMcs7",
                                  "VhtMcs8"};

void
AddSnrThreshold(WifiTxVector txVector, double snr)
{
    m_thresholds.emplace_back(snr, txVector);
}

double
linearToDb(double snr)
{
    return 10.0 * log10(snr);
}

void
BuildSnrThresholds()
{
    m_thresholds.clear();
    WifiMode mode;
    WifiTxVector txVector;

    if (standard == "802.11a" || standard == "802.11g")
    {
        std::vector<std::string> modes = {
            "OfdmRate6Mbps", "OfdmRate9Mbps", "OfdmRate12Mbps", "OfdmRate18Mbps",
            "OfdmRate24Mbps", "OfdmRate36Mbps", "OfdmRate48Mbps", "OfdmRate54Mbps"
        };

        for (const auto& modeName : modes)
        {
            mode = WifiMode(modeName);
            txVector.SetMode(mode);
            txVector.SetChannelWidth(20); // Standard 802.11a uses 20 MHz
            txVector.SetNss(1);
            txVector.SetGuardInterval(800); // Default GI for 802.11a
            double snr = linearToDb(m_phy->CalculateSnr(txVector, m_ber));
            AddSnrThreshold(txVector, snr);
        }
    }
    else
    {
        for (const auto& mode : modes) // Handles other standards like 802.11ac
        {
            txVector.SetChannelWidth(20);
            txVector.SetNss(1);
            txVector.SetGuardInterval(800);
            txVector.SetMode(mode);
            double snr = linearToDb(m_phy->CalculateSnr(txVector, m_ber));
            AddSnrThreshold(txVector, snr);
        }
    }
}

/**
 * Packet received.
 *
 * \param pkt The packet.
 * \param addr The sender address.
 */
void
PacketRx(Ptr<const Packet> pkt, const Address& addr)
{
    g_intervalBytes += pkt->GetSize();
}

/**
 * Convert MCS to string
 */
std::string
findMcs()
{
    std::string mcs = std::to_string(m_txVector.GetMode().GetMcsValue());
    if (m_txVector.GetGuardInterval() == 400)
    {
        mcs += "s";
    }

    return mcs;
}

/**
 * Function called to store the amount of received bytes after an interval
 */
void
SaveBytes()
{
    double currentTime = Simulator::Now().GetSeconds();
    endTime_endBytes.push_back(std::tie(currentTime, g_intervalBytes));
}

/**
 * Restart byte count
 */
void
ResetBytes()
{
    g_intervalBytes = 0;
}

/**
 * Report Rate changed.
 *
 * \param oldVal Old value.
 * \param newVal New value.
 */
void
RateChange(uint64_t oldVal, uint64_t newVal)
{
    g_intervalRate = newVal;
    if (g_intervalRate > g_highestRate)
    {
        g_highestRate = g_intervalRate;
    };

    std::string mcs = findMcs();
    double currentTime = Simulator::Now().GetSeconds();

    if ((currentTime > 10.5) && (currentTime < 17))
    {
        startTime_startBytes.push_back(std::tie(currentTime, g_intervalBytes));
        Simulator::Schedule(Seconds(3.5), &SaveBytes);
    }
    time_mcs.push_back(std::tie(currentTime, mcs));
    selectedMCSlist.push_back(std::stoi(mcs));
}

/**
 * Report Rate changed.
 *
 * \param txVector Vector to be used for next transmission
 */
void
TxVectorTrace(std::string context, WifiTxVector txVector)
{
    m_txVector = txVector;
    if ((mcsUsed.find(m_txVector.GetMode()) == mcsUsed.end()))
    {
        mcsUsed.insert(m_txVector.GetMode());
    }
}

/**
 *  Set the receive signal strength
 */
void
SetRss(Ptr<FixedRssLossModel> rssModel, double rss)
{
    rssModel->SetRss(rss);
}

double
round_to(double value, double precision = 1.0)
{
    return std::round(value / precision) * precision;
}

void
outputTXT(const std::string& fileName, const std::vector<std::string>& data)
{
    std::ofstream file(fileName, std::ios::app);
    for (const auto& line : data)
    {
        file << line << "\n";
    }
    file.close();
}

void
DisplayThroughput()
{
    double t2converge = 0;
    bool converged = false;
    std::string f_mcs;
    double f_time;
    double lTime = 0;
    int min_mcs = 9;
    bool drop = false;
    if (m_initialSnr == 1)
    {
        drop = false;
    }
    else
    {
        drop = true;
    }

    if (wifiManager == "MinstrelHt" || wifiManager == "CARA" || wifiManager == "AARF")
    {
        for (auto it : time_mcs)
        {
            std::tie(f_time, f_mcs) = it;
            if (f_time > 10.5)
            {
                t2converge = f_time - 10.5;
                converged = true;
            }
        }
    }
    else
    {
        for (auto it : time_mcs)
        {
            std::tie(f_time, f_mcs) = it;
            if (f_time > 10.5)
            {
                converged = true;
            }
        }
        if (converged)
        {
            t2converge = lTime - 10.5;
        }
    }

    std::string fileName = "figure-7-" + wifiManager + ".txt";
    std::vector<std::string> data;

    if (converged)
    {
        data.push_back(std::to_string(g_test) + " " + wifiManager + " " + std::to_string(t2converge));
        std::cout << wifiManager << " Converged on SNR: " << g_test << " Latency: " << t2converge
                  << std::endl;
    }
    else
    {
        data.push_back(std::to_string(g_test) + " " + wifiManager + " " + "N/A");
        std::cout << wifiManager << " Did not converge." << std::endl;
    }

    outputTXT(fileName, data);
}


/// Step structure
struct Step
{
    double stepSize; ///< step size in dBm
    double stepTime; ///< step size in seconds
};

/// StandardInfo structure
struct StandardInfo
{
    StandardInfo()
    {
        m_name = "none";
    }

    /**
     * Constructor
     *
     * \param name reference name
     * \param standard wifi standard
     * \param band PHY band
     * \param width channel width
     * \param snrLow SNR low
     * \param snrHigh SNR high
     * \param xMin x minimum
     * \param xMax x maximum
     * \param yMax y maximum
     */
    StandardInfo(std::string name,
                 WifiStandard standard,
                 WifiPhyBand band,
                 uint16_t width,
                 double snrLow,
                 double snrHigh,
                 double xMin,
                 double xMax,
                 double yMax)
        : m_name(name),
          m_standard(standard),
          m_band(band),
          m_width(width),
          m_snrLow(snrLow),
          m_snrHigh(snrHigh),
          m_xMin(xMin),
          m_xMax(xMax),
          m_yMax(yMax)
    {
    }

    std::string m_name;      ///< name
    WifiStandard m_standard; ///< standard
    WifiPhyBand m_band;      ///< PHY band
    uint16_t m_width;        ///< channel width
    double m_snrLow;         ///< lowest SNR
    double m_snrHigh;        ///< highest SNR
    double m_xMin;           ///< X minimum
    double m_xMax;           ///< X maximum
    double m_yMax;           ///< Y maximum
};

int
main(int argc, char* argv[])
{
    std::vector<StandardInfo> serverStandards;
    std::vector<StandardInfo> clientStandards;
    uint32_t steps;
    uint32_t rtsThreshold = 999999; // disabled even for large A-MPDU
    uint32_t maxAmpduSize = 65535;
    double stepSize = 1; // dBm
    double stepTime = 1; // seconds

    bool broadcast = 0;
    int ap1_x = 0;
    int ap1_y = 0;
    int sta1_x = 5;
    int sta1_y = 0;

    StandardInfo serverSelectedStandard;
    StandardInfo clientSelectedStandard;
    bool infrastructure = false;
    uint32_t maxSlrc = 7;
    uint32_t maxSsrc = 7;

    double g_time = 0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("time", "Duration of simulation", g_time);
    cmd.AddValue("seed", "Duration of simulation", g_seed);
    cmd.AddValue("test", "Test type (1 = static SNR ; 2 = SNR drop at given time)", g_test);
    cmd.AddValue("packetSize", "Size of frames to send", g_packetSize);
    cmd.AddValue("mcs", "MCS to use for control frames", g_MCS);
    cmd.AddValue("maxSsrc",
                 "The maximum number of retransmission attempts for a RTS packet",
                 maxSsrc);
    cmd.AddValue("maxSlrc",
                 "The maximum number of retransmission attempts for a Data packet",
                 maxSlrc);
    cmd.AddValue("rtsThreshold", "RTS threshold", rtsThreshold);
    cmd.AddValue("maxAmpduSize", "Max A-MPDU size", maxAmpduSize);
    cmd.AddValue("stepSize", "Power between steps (dBm)", stepSize);
    cmd.AddValue("stepTime", "Time on each step (seconds)", stepTime);
    cmd.AddValue("broadcast", "Send broadcast instead of unicast", broadcast);

    cmd.AddValue("serverChannelWidth",
                 "Set channel width of the server (valid only for 802.11n or ac)",
                 serverChannelWidth);
    cmd.AddValue("clientChannelWidth",
                 "Set channel width of the client (valid only for 802.11n or ac)",
                 clientChannelWidth);
    cmd.AddValue("serverNss", "Set nss of the server (valid only for 802.11n or ac)", serverNss);
    cmd.AddValue("clientNss", "Set nss of the client (valid only for 802.11n or ac)", clientNss);
    cmd.AddValue("serverShortGuardInterval",
                 "Set short guard interval of the server (802.11n/ac/ax) in nanoseconds",
                 serverShortGuardInterval);
    cmd.AddValue("clientShortGuardInterval",
                 "Set short guard interval of the client (802.11n/ac/ax) in nanoseconds",
                 clientShortGuardInterval);
    cmd.AddValue(
        "standard",
        "Set standard (802.11a, 802.11b, 802.11g, 802.11p-10MHz, 802.11p-5MHz, 802.11n-5GHz, "
        "802.11n-2.4GHz, 802.11ac, 802.11ax-6GHz, 802.11ax-5GHz, 802.11ax-2.4GHz)",
        standard);
    cmd.AddValue("wifiManager",
                 "Set wifi rate manager (Aarf, Aarfcd, Amrr, Arf, Cara, Ideal, Minstrel, "
                 "MinstrelHt, Onoe, Rraa, ThompsonSampling)",
                 wifiManager);
    cmd.AddValue("infrastructure", "Use infrastructure instead of adhoc", infrastructure);
    cmd.AddValue("snr", "Initial SNR", m_initialSnr);
    cmd.Parse(argc, argv);

    if (infrastructure == false)
    {
        NS_ABORT_MSG_IF(serverNss != clientNss,
                        "In ad hoc mode, we assume sender and receiver are similarly configured");
    }

    if (standard == "802.11b")
    {
        if (serverChannelWidth == 0)
        {
            serverChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211b, WIFI_PHY_BAND_2_4GHZ);
        }
        NS_ABORT_MSG_IF(serverChannelWidth != 22 && serverChannelWidth != 22,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(serverNss != 1, "Invalid nss for standard " << standard);
        if (clientChannelWidth == 0)
        {
            clientChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211b, WIFI_PHY_BAND_2_4GHZ);
        }
        NS_ABORT_MSG_IF(clientChannelWidth != 22 && clientChannelWidth != 22,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(clientNss != 1, "Invalid nss for standard " << standard);
    }
    else if (standard == "802.11a" || standard == "802.11g")
    {
        if (serverChannelWidth == 0)
        {
            serverChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211g, WIFI_PHY_BAND_2_4GHZ);
        }
        NS_ABORT_MSG_IF(serverChannelWidth != 20,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(serverNss != 1, "Invalid nss for standard " << standard);
        if (clientChannelWidth == 0)
        {
            clientChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211g, WIFI_PHY_BAND_2_4GHZ);
        }
        NS_ABORT_MSG_IF(clientChannelWidth != 20,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(clientNss != 1, "Invalid nss for standard " << standard);
    }
    else if (standard == "802.11n-5GHz" || standard == "802.11n-2.4GHz")
    {
        WifiPhyBand band =
            (standard == "802.11n-2.4GHz" ? WIFI_PHY_BAND_2_4GHZ : WIFI_PHY_BAND_5GHZ);
        if (serverChannelWidth == 0)
        {
            serverChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211n, band);
        }
        NS_ABORT_MSG_IF(serverChannelWidth != 20 && serverChannelWidth != 40,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(serverNss == 0 || serverNss > 4,
                        "Invalid nss " << serverNss << " for standard " << standard);
        if (clientChannelWidth == 0)
        {
            clientChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211n, band);
        }
        NS_ABORT_MSG_IF(clientChannelWidth != 20 && clientChannelWidth != 40,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(clientNss == 0 || clientNss > 4,
                        "Invalid nss " << clientNss << " for standard " << standard);
    }
    else if (standard == "802.11ac")
    {
        if (serverChannelWidth == 0)
        {
            serverChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211ac, WIFI_PHY_BAND_5GHZ);
        }
        NS_ABORT_MSG_IF(serverChannelWidth != 20 && serverChannelWidth != 40 &&
                            serverChannelWidth != 80 && serverChannelWidth != 160,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(serverNss == 0 || serverNss > 4,
                        "Invalid nss " << serverNss << " for standard " << standard);
        if (clientChannelWidth == 0)
        {
            clientChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211ac, WIFI_PHY_BAND_5GHZ);
        }
        NS_ABORT_MSG_IF(clientChannelWidth != 20 && clientChannelWidth != 40 &&
                            clientChannelWidth != 80 && clientChannelWidth != 160,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(clientNss == 0 || clientNss > 4,
                        "Invalid nss " << clientNss << " for standard " << standard);
    }
    else if (standard == "802.11ax-6GHz" || standard == "802.11ax-5GHz" ||
             standard == "802.11ax-2.4GHz")
    {
        WifiPhyBand band = (standard == "802.11ax-2.4GHz" ? WIFI_PHY_BAND_2_4GHZ
                            : standard == "802.11ax-6GHz" ? WIFI_PHY_BAND_6GHZ
                                                          : WIFI_PHY_BAND_5GHZ);
        if (serverChannelWidth == 0)
        {
            serverChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211ax, band);
        }
        NS_ABORT_MSG_IF(serverChannelWidth != 20 && serverChannelWidth != 40 &&
                            serverChannelWidth != 80 && serverChannelWidth != 160,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(serverNss == 0 || serverNss > 4,
                        "Invalid nss " << serverNss << " for standard " << standard);
        if (clientChannelWidth == 0)
        {
            clientChannelWidth = GetDefaultChannelWidth(WIFI_STANDARD_80211ax, band);
        }
        NS_ABORT_MSG_IF(clientChannelWidth != 20 && clientChannelWidth != 40 &&
                            clientChannelWidth != 80 && clientChannelWidth != 160,
                        "Invalid channel width for standard " << standard);
        NS_ABORT_MSG_IF(clientNss == 0 || clientNss > 4,
                        "Invalid nss " << clientNss << " for standard " << standard);
    }

    // As channel width increases, scale up plot's yRange value
    uint32_t channelRateFactor = std::max(clientChannelWidth, serverChannelWidth) / 20;
    channelRateFactor = channelRateFactor * std::max(clientNss, serverNss);

    // The first number is channel width, second is minimum SNR, third is maximum
    // SNR, fourth and fifth provide xrange axis limits, and sixth the yaxis
    // maximum
    serverStandards = {
        StandardInfo("802.11a", WIFI_STANDARD_80211a, WIFI_PHY_BAND_5GHZ, 20, 3, 27, 0, 30, 60),
        StandardInfo("802.11b", WIFI_STANDARD_80211b, WIFI_PHY_BAND_2_4GHZ, 22, -5, 11, -6, 15, 15),
        StandardInfo("802.11g", WIFI_STANDARD_80211g, WIFI_PHY_BAND_2_4GHZ, 20, -5, 27, -6, 30, 60),
        StandardInfo("802.11n-5GHz",
                     WIFI_STANDARD_80211n,
                     WIFI_PHY_BAND_5GHZ,
                     serverChannelWidth,
                     3,
                     30,
                     0,
                     35,
                     80 * channelRateFactor),
        StandardInfo("802.11n-2.4GHz",
                     WIFI_STANDARD_80211n,
                     WIFI_PHY_BAND_2_4GHZ,
                     serverChannelWidth,
                     3,
                     30,
                     0,
                     35,
                     80 * channelRateFactor),
        StandardInfo("802.11ac",
                     WIFI_STANDARD_80211ac,
                     WIFI_PHY_BAND_5GHZ,
                     serverChannelWidth,
                     5,
                     50,
                     0,
                     55,
                     120 * channelRateFactor),
        StandardInfo("802.11p-10MHz",
                     WIFI_STANDARD_80211p,
                     WIFI_PHY_BAND_5GHZ,
                     10,
                     3,
                     27,
                     0,
                     30,
                     60),
        StandardInfo("802.11p-5MHz", WIFI_STANDARD_80211p, WIFI_PHY_BAND_5GHZ, 5, 3, 27, 0, 30, 60),
        StandardInfo("802.11ax-6GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_6GHZ,
                     serverChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     120 * channelRateFactor),
        StandardInfo("802.11ax-5GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_5GHZ,
                     serverChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     120 * channelRateFactor),
        StandardInfo("802.11ax-2.4GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_2_4GHZ,
                     serverChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     120 * channelRateFactor),
    };

    clientStandards = {
        StandardInfo("802.11a", WIFI_STANDARD_80211a, WIFI_PHY_BAND_5GHZ, 20, 3, 27, 0, 30, 60),
        StandardInfo("802.11b", WIFI_STANDARD_80211b, WIFI_PHY_BAND_2_4GHZ, 22, -5, 11, -6, 15, 15),
        StandardInfo("802.11g", WIFI_STANDARD_80211g, WIFI_PHY_BAND_2_4GHZ, 20, -5, 27, -6, 30, 60),
        StandardInfo("802.11n-5GHz",
                     WIFI_STANDARD_80211n,
                     WIFI_PHY_BAND_5GHZ,
                     clientChannelWidth,
                     3,
                     30,
                     0,
                     35,
                     80 * channelRateFactor),
        StandardInfo("802.11n-2.4GHz",
                     WIFI_STANDARD_80211n,
                     WIFI_PHY_BAND_2_4GHZ,
                     clientChannelWidth,
                     3,
                     30,
                     0,
                     35,
                     80 * channelRateFactor),
        StandardInfo("802.11ac",
                     WIFI_STANDARD_80211ac,
                     WIFI_PHY_BAND_5GHZ,
                     clientChannelWidth,
                     5,
                     50,
                     0,
                     55,
                     120 * channelRateFactor),
        StandardInfo("802.11p-10MHz",
                     WIFI_STANDARD_80211p,
                     WIFI_PHY_BAND_5GHZ,
                     10,
                     3,
                     27,
                     0,
                     30,
                     60),
        StandardInfo("802.11p-5MHz", WIFI_STANDARD_80211p, WIFI_PHY_BAND_5GHZ, 5, 3, 27, 0, 30, 60),
        StandardInfo("802.11ax-6GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_6GHZ,
                     clientChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     160 * channelRateFactor),
        StandardInfo("802.11ax-5GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_5GHZ,
                     clientChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     160 * channelRateFactor),
        StandardInfo("802.11ax-2.4GHz",
                     WIFI_STANDARD_80211ax,
                     WIFI_PHY_BAND_2_4GHZ,
                     clientChannelWidth,
                     5,
                     55,
                     0,
                     60,
                     160 * channelRateFactor),
    };

    for (std::vector<StandardInfo>::size_type i = 0; i != serverStandards.size(); i++)
    {
        if (standard == serverStandards[i].m_name)
        {
            serverSelectedStandard = serverStandards[i];
        }
    }
    for (std::vector<StandardInfo>::size_type i = 0; i != clientStandards.size(); i++)
    {
        if (standard == clientStandards[i].m_name)
        {
            clientSelectedStandard = clientStandards[i];
        }
    }

    NS_ABORT_MSG_IF(serverSelectedStandard.m_name == "none",
                    "Standard " << standard << " not found");
    NS_ABORT_MSG_IF(clientSelectedStandard.m_name == "none",
                    "Standard " << standard << " not found");

    NS_ABORT_MSG_IF(clientSelectedStandard.m_snrLow >= clientSelectedStandard.m_snrHigh,
                    "SNR values in wrong order");
    steps = static_cast<uint32_t>(std::abs(static_cast<double>(clientSelectedStandard.m_snrHigh -
                                                               clientSelectedStandard.m_snrLow) /
                                           stepSize) +
                                  1);
    NS_LOG_DEBUG("Using " << steps << " steps for SNR range " << clientSelectedStandard.m_snrLow
                          << ":" << clientSelectedStandard.m_snrHigh);
    Ptr<Node> clientNode = CreateObject<Node>();
    Ptr<Node> serverNode = CreateObject<Node>();

    // std::cout << "mode,time,fsr" << std::endl;

    Config::SetDefault("ns3::WifiRemoteStationManager::MaxSlrc", UintegerValue(maxSlrc));
    Config::SetDefault("ns3::WifiRemoteStationManager::MaxSsrc", UintegerValue(maxSsrc));
    Config::SetDefault("ns3::MinstrelWifiManager::PrintStats", BooleanValue(false));
    Config::SetDefault("ns3::MinstrelWifiManager::PrintSamples", BooleanValue(false));
    Config::SetDefault("ns3::MinstrelHtWifiManager::PrintStats", BooleanValue(false));

    // Extend the time a packet can sit in the MAC queue so when using lower MCS packets do not
    // expire
    Config::SetDefault("ns3::WifiMacQueue::MaxDelay", TimeValue(Seconds(1.5)));

    // Configure the BER threshold for ideal manager
    // Config::SetDefault("ns3::IdealWifiManager::BerThreshold", DoubleValue(1e-6));

    // Disable the default noise figure of 7 dBm in WifiPhy; the calculations of
    // SNR below assume that the only noise is thermal noise
    Config::SetDefault("ns3::WifiPhy::RxNoiseFigure", DoubleValue(0));

    // By default, the CCA sensitivity is -82 dBm, meaning if the RSS is
    // below this value, the receiver will reject the Wi-Fi frame.
    // However, we want to probe the error model down to low SNR values,
    // and we have disabled the noise figure, so the noise level in 20 MHz
    // will be about -101 dBm.  Therefore, lower the CCA sensitivity to a
    // value that disables it (e.g. -110 dBm)
    Config::SetDefault("ns3::WifiPhy::CcaSensitivity", DoubleValue(-110));

    wifi.SetStandard(serverSelectedStandard.m_standard);
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    // Disable the preamble detection model for the same reason that we disabled
    // CCA sensitivity above-- we want to enable reception at low SNR
    wifiPhy.DisablePreambleDetectionModel();

    Ptr<YansWifiChannel> wifiChannel = CreateObject<YansWifiChannel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    wifiChannel->SetPropagationDelayModel(delayModel);
    Ptr<FixedRssLossModel> rssLossModel = CreateObject<FixedRssLossModel>();
    wifiChannel->SetPropagationLossModel(rssLossModel);
    wifiPhy.SetChannel(wifiChannel);
    NS_LOG_INFO("Using WiFi Manager: " << wifiManager);
    if (wifiManager == "MinstrelHt" || wifiManager == "CARA" || wifiManager == "AARF")
    {
        wifi.SetRemoteStationManager("ns3::" + wifiManager + "WifiManager",
                                 "RtsCtsThreshold", UintegerValue(rtsThreshold),
                                 "MaxSlrc", UintegerValue(maxSlrc),
                                 "MaxSsrc", UintegerValue(maxSsrc));
    }
    else if (wifiManager == "Ideal" || wifiManager == "ConstantRate")
    {
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                    "DataMode",
                                    StringValue("OfdmRate6Mbps"),
                                    "ControlMode",
                                    StringValue("OfdmRate6Mbps"));
    }
    else if (wifiManager == "None")
    {
        NS_ABORT_MSG("WiFi manager not set. Specify a valid manager using --wifiManager.");
    }
    else
    {
    NS_ABORT_MSG("Unsupported WiFi Manager: " + wifiManager);
    }


    NetDeviceContainer serverDevice;
    NetDeviceContainer clientDevice;

    TupleValue<UintegerValue, UintegerValue, EnumValue, UintegerValue> channelValue;

    WifiMacHelper wifiMac;
    if (infrastructure)
    {
        Ssid ssid = Ssid("ns-3-ssid");
        wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
        channelValue.Set(WifiPhy::ChannelTuple{0,
                                               serverSelectedStandard.m_width,
                                               serverSelectedStandard.m_band,
                                               0});
        wifiPhy.Set("ChannelSettings", channelValue);
        serverDevice = wifi.Install(wifiPhy, wifiMac, serverNode);

        wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
        channelValue.Set(WifiPhy::ChannelTuple{0,
                                               clientSelectedStandard.m_width,
                                               clientSelectedStandard.m_band,
                                               0});
        clientDevice = wifi.Install(wifiPhy, wifiMac, clientNode);
    }
    else
    {
        wifiMac.SetType("ns3::AdhocWifiMac");
        channelValue.Set(WifiPhy::ChannelTuple{0,
                                               serverSelectedStandard.m_width,
                                               serverSelectedStandard.m_band,
                                               0});
        wifiPhy.Set("ChannelSettings", channelValue);
        serverDevice = wifi.Install(wifiPhy, wifiMac, serverNode);

        channelValue.Set(WifiPhy::ChannelTuple{0,
                                               clientSelectedStandard.m_width,
                                               clientSelectedStandard.m_band,
                                               0});
        clientDevice = wifi.Install(wifiPhy, wifiMac, clientNode);
    }

    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_MaxAmpduSize",
                UintegerValue(maxAmpduSize));
    if (wifiManager != "ConstantRate")
    {
        Config::ConnectWithoutContext(
            "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/$ns3::" +
                wifiManager + "WifiManager/Rate",
            MakeCallback(&RateChange));
        if (wifiManager == "MinstrelHt") 
        {
        Config::Connect("/NodeList/0/DeviceList/*/RemoteStationManager/TxVectorChange",
                        MakeCallback(&TxVectorTrace));
        }
    }

    // Configure the mobility.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    // Initial position of AP and STA
    positionAlloc->Add(Vector(ap1_x, ap1_y, 0.0));
    NS_LOG_INFO("Setting initial AP position to " << Vector(ap1_x, ap1_y, 0.0));
    positionAlloc->Add(Vector(sta1_x, sta1_y, 0.0));
    NS_LOG_INFO("Setting initial STA position to " << Vector(sta1_x, sta1_y, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(clientNode);
    mobility.Install(serverNode);

    // Perform post-install configuration from defaults for channel width,
    // guard interval, and nss, if necessary
    // Obtain pointer to the WifiPhy
    Ptr<NetDevice> ndClient = clientDevice.Get(0);
    Ptr<NetDevice> ndServer = serverDevice.Get(0);

    Ptr<WifiNetDevice> wndClient = ndClient->GetObject<WifiNetDevice>();
    Ptr<WifiNetDevice> wndServer = ndServer->GetObject<WifiNetDevice>();

    Ptr<WifiPhy> wifiPhyPtrClient = wndClient->GetPhy();
    m_phy = wifiPhyPtrClient;
    // m_phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    Ptr<WifiPhy> wifiPhyPtrServer = wndServer->GetPhy();
    if (standard == "802.11g" || standard == "802.11a")
    {
        m_txVector.SetMode(WifiMode("OfdmRate6Mbps")); // Default mode for 802.11a
        m_txVector.SetChannelWidth(20);               // 802.11a uses 20 MHz
        m_txVector.SetNss(1);                         // Single spatial stream
        m_txVector.SetGuardInterval(800);             // Default guard interval
    }

    uint8_t t_clientNss = static_cast<uint8_t>(clientNss);
    uint8_t t_serverNss = static_cast<uint8_t>(serverNss);
    wifiPhyPtrClient->SetNumberOfAntennas(t_clientNss);
    wifiPhyPtrClient->SetMaxSupportedTxSpatialStreams(t_clientNss);
    wifiPhyPtrClient->SetMaxSupportedRxSpatialStreams(t_clientNss);
    wifiPhyPtrServer->SetNumberOfAntennas(t_serverNss);
    wifiPhyPtrServer->SetMaxSupportedTxSpatialStreams(t_serverNss);
    wifiPhyPtrServer->SetMaxSupportedRxSpatialStreams(t_serverNss);
    // Only set the guard interval for HT and VHT modes
    if (serverSelectedStandard.m_name == "802.11n-5GHz" ||
        serverSelectedStandard.m_name == "802.11n-2.4GHz" ||
        serverSelectedStandard.m_name == "802.11ac")
    {
        Ptr<HtConfiguration> clientHtConfiguration = wndClient->GetHtConfiguration();
        clientHtConfiguration->SetShortGuardIntervalSupported(clientShortGuardInterval == 400);
        Ptr<HtConfiguration> serverHtConfiguration = wndServer->GetHtConfiguration();
        serverHtConfiguration->SetShortGuardIntervalSupported(serverShortGuardInterval == 400);
    }
    else if (serverSelectedStandard.m_name == "802.11ax-6GHz" ||
             serverSelectedStandard.m_name == "802.11ax-5GHz" ||
             serverSelectedStandard.m_name == "802.11ax-2.4GHz")
    {
        wndServer->GetHeConfiguration()->SetGuardInterval(NanoSeconds(serverShortGuardInterval));
        wndClient->GetHeConfiguration()->SetGuardInterval(NanoSeconds(clientShortGuardInterval));
    }
    NS_LOG_DEBUG("Channel width " << wifiPhyPtrClient->GetChannelWidth() << " noiseDbm "
                                  << noiseDbm);
    NS_LOG_DEBUG("NSS " << wifiPhyPtrClient->GetMaxSupportedTxSpatialStreams());

    // Configure signal and noise, and schedule first iteration
    noiseDbm += 10 * log10(clientSelectedStandard.m_width * 1000000);

    double rssCurrent = (m_initialSnr + noiseDbm);
    double rssAfter = (g_test + noiseDbm);
    g_SNR = g_test;
    std::cout << "Initial SNR: " << m_initialSnr << std::endl;
    std::cout << "Resulting SNR: " << g_SNR << std::endl;
    rssLossModel->SetRss(rssCurrent);
    Simulator::Schedule(Seconds(10.5), &SetRss, rssLossModel, (rssAfter));

    Simulator::Schedule(Seconds(10.5), &ResetBytes);

    PacketSocketHelper packetSocketHelper;
    packetSocketHelper.Install(serverNode);
    packetSocketHelper.Install(clientNode);

    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(serverDevice.Get(0)->GetIfIndex());
    if (broadcast)
    {
        socketAddr.SetPhysicalAddress(serverDevice.Get(0)->GetBroadcast());
    }
    else
    {
        socketAddr.SetPhysicalAddress(serverDevice.Get(0)->GetAddress());
    }
    // Arbitrary protocol type.
    // Note: PacketSocket doesn't have any L4 multiplexing or demultiplexing
    //       The only mux/demux is based on the protocol field
    socketAddr.SetProtocol(1);

    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetRemote(socketAddr);
    client->SetStartTime(Seconds(0.5));                   // allow simulation warmup
    client->SetAttribute("MaxPackets", UintegerValue(0)); // unlimited
    client->SetAttribute("PacketSize", UintegerValue(g_packetSize));

    // Set a maximum rate 10% above the yMax specified for the selected standard
    double rate = clientSelectedStandard.m_yMax * 1e6 * 1.10;
    double clientInterval = static_cast<double>(g_packetSize) * 8 / rate;

    client->SetAttribute("Interval", TimeValue(Seconds(clientInterval)));

    clientNode->AddApplication(client);

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->SetLocal(socketAddr);
    server->TraceConnectWithoutContext("Rx", MakeCallback(&PacketRx));
    serverNode->AddApplication(server);
    Simulator::ScheduleNow(&BuildSnrThresholds);
    // wifiPhy.EnablePcap("STA-DROP", ndClient);
    // wifiPhy.EnablePcap("AP-DROP", ndServer);

    Simulator::Stop(Seconds(g_time));
    Simulator::Run();
    Simulator::Destroy();
    DisplayThroughput();

    return 0;
}
