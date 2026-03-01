#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"

namespace fs = std::filesystem;

std::mutex cerrMutex, coutMutex;

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);

void CheckDownloadedPiecesIntegrity(const std::filesystem::path& outputFilename, const TorrentFile& tf, PieceStorage& pieces, const int PiecesToDownload) {
    pieces.CloseOutputFile();

    if (std::filesystem::file_size(outputFilename) != tf.length) {
        throw std::runtime_error("Output file has wrong size");
    }

    if (pieces.GetPiecesSavedToDiscIndices().size() != pieces.PiecesSavedToDiscCount()) {
        throw std::runtime_error("Cannot determine real amount of saved pieces");
    }

    if (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        throw std::runtime_error("Downloaded pieces amount is not enough");
    }

    if (pieces.TotalPiecesCount() != tf.pieceHashes.size() || pieces.TotalPiecesCount() < 200) {
        throw std::runtime_error("Wrong amount of pieces");
    }

    std::vector<size_t> pieceIndices = pieces.GetPiecesSavedToDiscIndices();
    std::sort(pieceIndices.begin(), pieceIndices.end());

    std::ifstream file(outputFilename, std::ios_base::binary);
    for (size_t pieceIndex : pieceIndices) {
        const std::streamoff positionInFile = pieceIndex * tf.pieceLength;
        file.seekg(positionInFile);
        if (!file.good()) {
            throw std::runtime_error("Cannot read from file");
        }
        std::string pieceDataFromFile(tf.pieceLength, '\0');
        file.read(pieceDataFromFile.data(), tf.pieceLength);
        const size_t readBytesCount = file.gcount();
        pieceDataFromFile.resize(readBytesCount);
        const std::string realHash = CalculateSHA1(pieceDataFromFile);

        if (realHash != tf.pieceHashes[pieceIndex]) {
            std::cerr << "File piece with index " << pieceIndex << " started at position " << positionInFile <<
                      " with length " << pieceDataFromFile.length() << " has wrong hash " << HexEncode(realHash) <<
                      ". Expected hash is " << HexEncode(tf.pieceHashes[pieceIndex]) << std::endl;
            throw std::runtime_error("Wrong piece hash");
        }
    }
}

void DeleteDownloadedFile(const std::filesystem::path& outputFilename) {
    std::filesystem::remove(outputFilename);
}


bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker, const int PiecesToDownload) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<std::shared_ptr<PeerConnect>> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(std::make_shared<PeerConnect>(peer, torrentFile, ourId, pieces));
    }

    peerThreads.reserve(peerConnections.size());

    for (auto& peerConnectPtr : peerConnections) {
        peerThreads.emplace_back(
                [peerConnectPtr] () {
                    bool tryAgain = true;
                    int attempts = 0;
                    do {
                        try {
                            ++attempts;
                            std::cout << "try to run peer " << peerConnectPtr->GetPeerId() << " for the " << attempts << " time\n";
                            peerConnectPtr->Run();
                        } catch (const std::runtime_error& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Runtime error: " << e.what() << std::endl;
                        } catch (const std::exception& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Exception: " << e.what() << std::endl;
                        } catch (...) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Unknown error" << std::endl;
                        }
                        tryAgain = peerConnectPtr->Failed() && attempts < 3;
                        std::this_thread::sleep_for(3s);
                    } while (tryAgain);
                    std::cout << "close peer " << peerConnectPtr->GetPeerId() << '\n';
                }
        );
        std::this_thread::sleep_for(2s);
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Started " << peerThreads.size() << " threads for peers" << std::endl;
    }
    std::this_thread::sleep_for(10s);
    std::cout << " keep download file" << '\n';
    while (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        std::cout << "pieces left: " << PiecesToDownload - pieces.PiecesSavedToDiscCount() << '\n';
        std::cout << (pieces.QueueIsEmpty() ? "queue is empty\n": "queue is not empty\n");
        std::cout << "Pieces in progress:" << pieces.PiecesInProgressCount() << '\n';
        if (pieces.PiecesInProgressCount() == 0) {
            {
                std::cout << "NOPIECES" << std::endl;
                std::lock_guard<std::mutex> coutLock(coutMutex);
                std::cout
                        << "Want to download more pieces but all peer connections are not working. Let's request new peers"
                        << std::endl;
            }

            for (auto& peerConnectPtr : peerConnections) {
                peerConnectPtr->Terminate();
            }
            for (std::thread& thread : peerThreads) {
                thread.join();
            }
            return true;
        }
        std::this_thread::sleep_for(3s);
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Terminating all peer connections" << std::endl;
    }
    for (auto& peerConnectPtr : peerConnections) {
        peerConnectPtr->Terminate();
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId, const int PiecesToDownload) {
    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        tracker.UpdatePeers(torrentFile, ourId, 12345);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }
        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker, PiecesToDownload);
    } while (requestMorePeers);
}

int main(int argc, char *argv[]) {
    std::filesystem::path outputDirectory, file;
    int percent;
    if (argc < 6) throw std::runtime_error("not enough arg");
    std::string element;
    for(int i = 1; i < argc; ++i) {
        element = argv[i];
        if (element == "-d") {
            outputDirectory = argv[i + 1];
        }
        else if (element == "-p") {
            percent = std::stoi(argv[i + 1]);
            file = argv[i + 2];
        }
    }

    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(file.string());
        std::cout << "Loaded torrent file " << file << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    std::filesystem::create_directories(outputDirectory);
    int PiecesToDownload = (int) ceil(torrentFile.pieceHashes.size() * ((long double) percent / 100));
    PieceStorage pieces(torrentFile, outputDirectory, PiecesToDownload);
    DownloadTorrentFile(torrentFile, pieces, PeerId, PiecesToDownload);
    CheckDownloadedPiecesIntegrity(outputDirectory / torrentFile.name, torrentFile, pieces, PiecesToDownload);
    std::cout << "file downloaded" << '\n';
    return 0;
}
