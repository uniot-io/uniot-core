#pragma once

#include <Logger.h>
#include <WiFiUdp.h>
#include <time.h>

namespace uniot {
class SimpleNTP {
 public:
  typedef void (*SyncTimeCallback)(time_t epoch);
  static constexpr const char *servers[] = {"time.google.com", "time.nist.gov", "pool.ntp.org"};

  SimpleNTP() : mSyncTimeCallback(nullptr) {}

  void setSyncTimeCallback(SyncTimeCallback callback) {
    mSyncTimeCallback = callback;
  }

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

  SyncTimeCallback mSyncTimeCallback;
};
}  // namespace uniot