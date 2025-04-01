#pragma once

#include <Logger.h>
#include <WiFiUdp.h>
#include <time.h>

namespace uniot {
/**
 * @brief Simple Network Time Protocol client for obtaining time from NTP servers
 * @defgroup date-time-ntp Simple NTP
 * @ingroup date-time
 * @{
 *
 * SimpleNTP provides functionality to synchronize device time with NTP servers.
 * It connects to one of several predefined NTP servers, retrieves the current time,
 * and converts it to Unix epoch format.
 */
class SimpleNTP {
 public:
  /**
   * @brief Callback function type for time synchronization events
   *
   * This callback is triggered when time is successfully synchronized with an NTP server.
   *
   * @param epoch The current time as Unix timestamp (seconds since Jan 1, 1970)
   */
  typedef void (*SyncTimeCallback)(time_t epoch);

  /**
   * @brief Array of NTP server hostnames to try connecting to
   *
   * One server is randomly selected during each sync attempt
   */
  static constexpr const char *servers[] = {"time.google.com", "time.nist.gov", "pool.ntp.org"};

  /**
   * @brief Default constructor
   *
   * Initializes a SimpleNTP instance with no callback
   */
  SimpleNTP() : mSyncTimeCallback(nullptr) {}

  /**
   * @brief Sets the callback function for time synchronization events
   *
   * @param callback Function to be called when time is successfully synchronized
   */
  void setSyncTimeCallback(SyncTimeCallback callback) {
    mSyncTimeCallback = callback;
  }

  /**
   * @brief Requests and retrieves current time from NTP server
   *
   * Connects to a randomly selected NTP server, sends a request packet,
   * waits for and processes the response to extract the current time.
   *
   * @retval time_t Current time in seconds since Jan 1, 1970
   * @retval 0 if time synchronization failed
   */
  time_t getNtpTime() {
    WiFiUDP udp;
    const unsigned short localUdpPort = 1234;
    if (!udp.begin(localUdpPort)) {
      UNIOT_LOG_ERROR("Failed to initialize UDP on port %d.", localUdpPort);
      return 0;
    }

    while (udp.parsePacket() != 0) {
      udp.flush();
    }

    if (!_sendNTPPacket(udp)) {
      udp.stop();
      return 0;
    }

    int maxRetries = 200;
    bool responseReceived = _waitForResponse(udp, 1500, maxRetries, 10);
    if (!responseReceived) {
      UNIOT_LOG_ERROR("No UDP response received from NTP server after %d attempts.", maxRetries);
      udp.stop();
      return 0;
    }

    time_t currentEpoch = _processNTPResponse(udp);
    if (!currentEpoch) {
      udp.stop();
      return 0;
    }

    if (mSyncTimeCallback) {
      mSyncTimeCallback(currentEpoch);
    }
    udp.stop();

    return currentEpoch;
  }

 private:
  /**
   * @brief Prepares and sends an NTP request packet
   *
   * Creates a standard NTP request packet and sends it to a randomly selected
   * server from the predefined server list.
   *
   * @param udp WiFiUDP instance to use for sending the packet
   * @retval true Packet sent successfully
   * @retval false Failed to send packet
   */
  bool _sendNTPPacket(WiFiUDP &udp) {
    uint8_t packet[48];
    memset(packet, 0, sizeof(packet));
    packet[0] = 0b11100011;  // LI, Version, Mode
    packet[1] = 0;           // Stratum, or type of clock
    packet[2] = 6;           // Polling Interval
    packet[3] = 0xEC;        // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packet[12] = 49;
    packet[13] = 0x4E;
    packet[14] = 49;
    packet[15] = 52;

    const char *selectedServer = servers[random(0, sizeof(servers) / sizeof(servers[0]))];

    const unsigned short ntpUdpPort = 123;
    udp.beginPacket(selectedServer, ntpUdpPort);
    size_t bytesWritten = udp.write(packet, sizeof(packet));
    if (bytesWritten != sizeof(packet)) {
      UNIOT_LOG_ERROR("Failed to write NTP packet to UDP");
      return false;
    }
    udp.endPacket();
    UNIOT_LOG_TRACE("NTP packet sent to %s:%d.", selectedServer, ntpUdpPort);
    return true;
  }

  /**
   * @brief Waits for a response from the NTP server
   *
   * Attempts to receive a response packet multiple times with delays between attempts.
   * Returns true as soon as a valid-sized packet is received or false if max retries reached.
   *
   * @param udp WiFiUDP instance to receive the response
   * @param timeout Maximum time to wait for a response (in milliseconds)
   * @param maxRetries Maximum number of retry attempts
   * @param retryDelay Delay between retry attempts (in milliseconds)
   * @retval true Response received successfully
   * @retval false Response wait timed out
   */
  bool _waitForResponse(WiFiUDP &udp, unsigned long timeout, int maxRetries, unsigned long retryDelay) {
    UNIOT_LOG_TRACE("Waiting for NTP response with timeout %lu ms and max %d retries.", timeout, maxRetries);
    unsigned long startTime = millis();

    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
      unsigned long currentTime = millis();
      if (currentTime - startTime >= timeout) {
        UNIOT_LOG_WARN("NTP response wait timed out on attempt %d.", attempt);
        return false;
      }

      int packetSize = udp.parsePacket();
      if (packetSize >= 48) {
        UNIOT_LOG_TRACE("NTP response received on attempt %d.", attempt);
        return true;
      }

      delay(retryDelay);
    }

    return false;
  }

  /**
   * @brief Processes the NTP response to extract the timestamp
   *
   * Reads the NTP response packet, extracts the timestamp fields,
   * and converts from NTP time format to Unix epoch time.
   *
   * @param udp WiFiUDP instance with the received response
   * @retval time_t Current time in seconds since Jan 1, 1970
   * @retval 0 Processing failed
   */
  time_t _processNTPResponse(WiFiUDP &udp) {
    uint8_t packet[48];
    int len = udp.read(packet, sizeof(packet));
    if (len < 48) {
      UNIOT_LOG_ERROR("Incomplete NTP packet received. Expected 48 bytes, got %d bytes.", len);
      return 0;
    }

    // Extract the timestamp from the NTP packet
    unsigned long highWord = word(packet[40], packet[41]);
    unsigned long lowWord = word(packet[42], packet[43]);
    unsigned long secsSince1900 = (highWord << 16) | lowWord;

    // Convert NTP time to Unix epoch time
    time_t currentEpoch = secsSince1900 - 2208988800UL;
    UNIOT_LOG_TRACE("NTP time (epoch): %ld", currentEpoch);

    return currentEpoch;
  }

  /**
   * @brief Callback function to be called when time is successfully synced
   */
  SyncTimeCallback mSyncTimeCallback;
};
/** @} */
}  // namespace uniot
