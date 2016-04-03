//
// Created by wastl on 03.04.16.
//

#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <glog/logging.h>

#include "Snowflake.h"

namespace wikidata {
namespace primarysources {
namespace {

const int64_t kDatacenterIdBits = 10L;
const int64_t kMaxDatacenterId = -1L ^(-1L << kDatacenterIdBits);
const int64_t kSequenceBits = 12L;

const int64_t kDatacenterIdShift = kSequenceBits;
const int64_t kTimestampLeftShift = kSequenceBits + kDatacenterIdBits;
const int64_t kSequenceMask = -1L ^(-1L << kSequenceBits);

int64_t lastTimestamp = 0L;
int64_t sequence = 0L;

std::mutex mutex_;

int64_t getMachineId() {
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    bool success = false;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        LOG(ERROR) << "Error opening socket: " << strerror(errno);
        return 0;
    };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        LOG(ERROR) << "Error reading hardware address: " << strerror(errno);
        return 0;
    }

    struct ifreq *it = ifc.ifc_req;
    const struct ifreq *const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        std::strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = true;
                    break;
                }
            }
        } else {
            LOG(ERROR) << "Error reading hardware address: " << strerror(errno);
            return 0;
        }
    }

    if (!success) {
        LOG(ERROR) << "No hardware address found";
        return 0;
    }

    int rnd = rand() & 0x000000FF;

    return ((0x000000FF & (long)ifr.ifr_hwaddr.sa_data[5]) | (0x0000FF00 & (((long)rnd)<<8)))>>6;
}

inline int64_t getTimeStamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds> (
            std::chrono::steady_clock::now().time_since_epoch()).count();
}

int64_t tillNextMillis() {
    int64_t millis = getTimeStamp();

    while (millis <= lastTimestamp) {
        millis = getTimeStamp();
    }

    return millis;
}
}  // namespace



int64_t Snowflake(int64_t datacenterId) {
    if (datacenterId == 0 || datacenterId > kMaxDatacenterId) {
        datacenterId = getMachineId();
    }

    int64_t timestamp = getTimeStamp();
    if (timestamp < lastTimestamp) {
        LOG(WARNING) << "Clock moved backwards. Refusing to generate id for "
                     << (lastTimestamp - timestamp) << " milliseconds.";
        std::this_thread::sleep_for(std::chrono::milliseconds(lastTimestamp - timestamp));
    }

    // guard modification of lastTimestamp and sequence
    std::lock_guard<std::mutex> lock(mutex_);
    if (timestamp == lastTimestamp) {
        sequence = (sequence + 1) & kSequenceMask;
        if (sequence == 0) {
            timestamp = tillNextMillis();
        }
    } else {
        sequence = 0;
    }
    lastTimestamp = timestamp;

    return (timestamp << kTimestampLeftShift) | (datacenterId << kDatacenterIdShift) | sequence;
}


}  // namespace primarysources
}  // namespace wikidata


