#include "tracker.h"
#include "crypto/auth.h"

#include <cstdint>
#include <map>
#include <unordered_set>
#include <unordered_map>

std::unordered_set<PeerId512_t> removed_peers;
std::unordered_map<PeerId512_t, TrackerPeerInfo> tracker_peer_info;
std::map<uint32_t, PeerId512_t> IPv4_peer_idx;
std::unordered_multimap<PeerId512_t, ConnForwardInfo> conn_forward_info;

// TODO: add locks

void flush_removed_peers() {
    for (const auto &id : removed_peers) {
        tracker_peer_info.erase(id);
        conn_forward_info.erase(id);
    }
    std::map<uint32_t, PeerId512_t> IPv4_peer_idx_new;
    for (const auto &entry : IPv4_peer_idx) {
        if (tracker_peer_info.find(entry.second) != tracker_peer_info.end()) {
            IPv4_peer_idx_new[entry.first] = entry.second;
        }
    }
    IPv4_peer_idx = IPv4_peer_idx_new;
    removed_peers.clear();
}

std::vector<TrackerPeerInfo> find_peers(
    const std::string &search_str, int limit
) {
    std::vector<TrackerPeerInfo> result;
    for (const auto &entry : tracker_peer_info) {
        if (entry.second.description.find(search_str) != std::string::npos) {
            result.push_back(entry.second);
            if (result.size() >= limit) {
                break;
            }
        }
    }
    return result;
}

// find peers by closest IPv4 addresses
std::vector<TrackerPeerInfo> find_ipv4_closest_peers(
    uint32_t ip, int limit
) {
    std::vector<TrackerPeerInfo> result;
    auto it_prev = IPv4_peer_idx.lower_bound(ip);
    auto it_next = IPv4_peer_idx.upper_bound(ip);
    while (
        result.size() < limit
        && it_prev != IPv4_peer_idx.begin()
        && it_next != IPv4_peer_idx.end()
    ) {
        if (ip - it_prev->first < it_next->first - ip) {
            auto it = tracker_peer_info.find(it_prev->second);
            if (it != tracker_peer_info.end()) {
                result.push_back(it->second);
            }
            --it_prev;
        } else {
            auto it = tracker_peer_info.find(it_next->second);
            if (it != tracker_peer_info.end()) {
                result.push_back(it->second);
            }
            ++it_next;
        }
    }
    while (result.size() < limit && it_prev != IPv4_peer_idx.begin()) {
        auto it = tracker_peer_info.find(it_prev->second);
        if (it != tracker_peer_info.end()) {
            result.push_back(it->second);
        }
        --it_prev;
    }
    while (result.size() < limit && it_next != IPv4_peer_idx.end()) {
        auto it = tracker_peer_info.find(it_next->second);
        if (it != tracker_peer_info.end()) {
            result.push_back(it->second);
        }
        ++it_next;
    }
    return result;
}

std::vector<ConnForwardInfo> find_forwards(
    const std::unordered_set<PeerId512_t> &peer_ids,
    std::string search_str, int limit
) {
    std::vector<ConnForwardInfo> result;
    for (const auto &id : peer_ids) {
        auto range = conn_forward_info.equal_range(id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.description.find(search_str) != std::string::npos) {
                result.push_back(it->second);
                if (result.size() >= limit) {
                    break;
                }
            }
        }
    }
    return result;
}
