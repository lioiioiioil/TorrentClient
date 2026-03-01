#pragma once

#include "torrent_file.h"
#include "peer.h"

class TorrentTracker {
public:

    TorrentTracker(const std::string& url);

    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port);

    const std::vector<Peer>& GetPeers() const;

private:
    std::string url_;
    std::vector<Peer> peers_;
};
