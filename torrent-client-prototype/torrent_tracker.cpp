#include "torrent_tracker.h"
#include "bencode.h"

TorrentTracker::TorrentTracker(const std::string& url)
        : url_(url)
        , peers_()
{}
const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    cpr::Response res = cpr::Get(
            cpr::Url{url_},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(port)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
            },
            cpr::Timeout{20000}
    );
    peers_.resize(0);
    std::shared_ptr<ben_code> root = decode(res.text);
    std::map <std::string, std::shared_ptr<ben_code>> &dictionary = root->get_dict();
    for (auto &x: dictionary) {
        if (x.first == "peers") {
            for(int i = 0; i < x.second->get_string().size() / 6; ++i) {
                uint8_t p1 = static_cast<uint8_t > (x.second->get_string()[i * 6 + 4]),
                        p2 = static_cast<uint8_t> (x.second->get_string()[i * 6 + 5]);
                std::string cur = x.second->get_string(), st;
                st = std::to_string(static_cast<uint8_t>(cur[i * 6 + 0])) + '.' + std::to_string(static_cast<uint8_t>(cur[i * 6 + 1])) + '.' +
                     std::to_string(static_cast<uint8_t>(cur[i * 6 + 2])) + '.' + std::to_string(static_cast<uint8_t>(cur[i * 6 + 3]));
                peers_.emplace_back(st, p1 * 256 + p2);
            }
        }
    }
}
