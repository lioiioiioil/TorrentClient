#include "torrent_file.h"
#include "bencode.h"
#include "byte_tools.h"

TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    std::stringstream buffer;
    buffer << file.rdbuf();

    file.close();

    std::string toDecode_ = buffer.str();


    TorrentFile torrent_file;

    std::shared_ptr<ben_code> root = decode(toDecode_);

    std::map <std::string, std::shared_ptr<ben_code>> &dictionary = root->get_dict();
    std::string info_for_hash;


    for (auto &x: dictionary) {
        if (x.first == "announce") torrent_file.announce = x.second->get_string();
        else if (x.first == "comment") torrent_file.comment = x.second->get_string();
        else if (x.first == "info") {
            get_bencode(x.second, info_for_hash);
            std::map <std::string, std::shared_ptr<ben_code>> &dict = x.second->get_dict();
            for (auto &y: dict) {
                if (y.first == "piece length") torrent_file.pieceLength = y.second->get_int();
                if (y.first == "name") torrent_file.name = y.second->get_string();
                if (y.first == "files") {
                    std::map <std::string, std::shared_ptr<ben_code>> &d = y.second->get_list().front()->get_dict();
                    for (auto &z : d) {
                        if (z.first == "length") {
                            torrent_file.length = z.second->get_int();
                        }
                    }
                }
                if (y.first == "pieces") {
                    std::string pieces = y.second->get_string();
                    for (int j = 0; j < pieces.size() / 20; ++j) {
                        torrent_file.pieceHashes.push_back(pieces.substr(j * 20, 20));
                    }
                }
            }
        }
    }
    torrent_file.infoHash = CalculateSHA1(info_for_hash);
    return torrent_file;
}
