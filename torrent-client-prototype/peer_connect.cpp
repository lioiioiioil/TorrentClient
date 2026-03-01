#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"

using namespace std::chrono_literals;

PeerPiecesAvailability::PeerPiecesAvailability()
        : bitfield_()
{}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield)
        : bitfield_(bitfield)
{}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    size_t ind_str = pieceIndex / 8, ind_ch = 7 - pieceIndex % 8;
    uint8_t ch = static_cast<uint8_t> (bitfield_[ind_str]);
    return (ch & (1 << ind_ch));
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    size_t ind_str = pieceIndex / 8, ind_ch = 7 - pieceIndex % 8;
    uint8_t ch = static_cast<uint8_t> (bitfield_[ind_str]);
    ch |= (1 << ind_ch);
    bitfield_ = bitfield_.substr(0, ind_str) + static_cast<char> (ch) +
                bitfield_.substr(ind_str + 1, bitfield_.size() - (ind_str + 1));
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * 8;
}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage& storage)
        : tf_(tf)
        , socket_(peer.ip, peer.port, std::chrono::milliseconds(1000), std::chrono::milliseconds(1000))
        , selfPeerId_(selfPeerId)
        , peerId_(peer.ip)
        , piecesAvailability_()
        , terminated_(false)
        , choked_(true)
        , pieceInProgress_(nullptr)
        , pieceStorage_(storage)
        , pendingBlock_(false)
        , failed_(false)
{}

void PeerConnect::Run() {
    terminated_.store(false);
    failed_.store(false);
    if (EstablishConnection()) {
        std::cout << "Connection established to peer" << std::endl;
        MainLoop();
        socket_.CloseConnection();
    } else {
        std::cerr << "Cannot establish connection to peer" << std::endl;
        failed_.store(true);
        socket_.CloseConnection();
    }
}

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();
    std::string message = "";
    message += static_cast<char>(19);
    message += "BitTorrent protocol";
    for(int i = 0; i < 8; ++i) message += static_cast<char>(0);
    message += tf_.infoHash + selfPeerId_;
    socket_.SendData(message);
    uint8_t len = static_cast<uint8_t> (socket_.ReceiveData(1)[0]);
    socket_.ReceiveData(len + 8);
    std::string info_hash = socket_.ReceiveData(20);
    if (info_hash != tf_.infoHash) throw std::runtime_error("wrong info_hash");
    socket_.ReceiveData(20); // peerId
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
                  socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    std::string st = socket_.ReceiveData();
    Message msg = Message::Parse(st);
    if (msg.id == MessageId::Unchoke) choked_ = false;
    else if (msg.id == MessageId::BitField) {
        piecesAvailability_ = PeerPiecesAvailability(msg.payload);
    }
    else throw std::runtime_error("No field and no unchoke");
}

void PeerConnect::SendInterested() {
    Message msg = Message::Init(MessageId::Interested, "");
    socket_.SendData(msg.ToString());
}

void PeerConnect::RequestPiece() {
    if (pieceInProgress_ == nullptr) {
        auto piece = pieceStorage_.GetNextPieceToDownload();
        std::unordered_set<size_t> bannedPieces;
        while (piece != nullptr && !piecesAvailability_.IsPieceAvailable(piece->GetIndex()) &&
            bannedPieces.count(piece->GetIndex()) == 0 ) {
            bannedPieces.insert(piece->GetIndex());
            pieceStorage_.AddPieceToDownload(piece);
            piece = pieceStorage_.GetNextPieceToDownload();
        }
        if (piece && bannedPieces.count(piece->GetIndex()) == 1) {
            pieceStorage_.AddPieceToDownload(piece);
        }
        pieceInProgress_ = piece;
        if (pieceInProgress_ == nullptr) {
            Terminate();
            return;
        }
    }

    Block* ptr = pieceInProgress_ -> FirstMissingBlock();
    if (ptr == nullptr) {
        pieceInProgress_ = nullptr;
        return;
    }
    pendingBlock_ = true;
    auto block = *ptr;
    Message msg = Message::Init(MessageId::Request,
                                IntToBytes(block.piece) +
                                IntToBytes(block.offset) + IntToBytes(block.length));
    socket_.SendData(msg.ToString());
}

void PeerConnect::Terminate() {
    std::cout << "Terminate " << peerId_ << std::endl;
    terminated_.store(true);
}

bool PeerConnect::Failed() const {
    return failed_.load();
}

const std::string& PeerConnect::GetPeerId() const {
    return peerId_;
}

void PeerConnect::MainLoop() {
    std::chrono::time_point last_action(std::chrono::steady_clock::now());
    while (!terminated_.load()) {
        try {
            std::string ans = socket_.ReceiveData();
            Message message = Message::Parse(ans);
            switch (message.id) {
                case MessageId::Choke : {
                    choked_ = true;
                    break;
                }

                case MessageId::Unchoke : {
                    choked_ = false;
                    break;
                }

                case MessageId::Have : {
                    piecesAvailability_.SetPieceAvailability(BytesToInt(message.payload));
                    break;
                }

                case MessageId::Piece : {
                    size_t indexOfPiece = BytesToInt(message.payload.substr(0, 4)),
                            offset = BytesToInt(message.payload.substr(4, 4));

                    std::string data = message.payload.substr(8, message.payload.size() - 8);
                    if (pieceInProgress_ && pieceInProgress_ -> GetIndex() == indexOfPiece) {
                        pieceInProgress_ -> SaveBlock(offset, data);
                        if (pieceInProgress_ -> AllBlocksRetrieved()) {
                            pieceStorage_.PieceProcessed(pieceInProgress_);
                            pieceInProgress_ = nullptr;
                        }
                    }

                    pendingBlock_ = false;
                    break;
                }

                default:
                {
                    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_action).count() > 3) {
                        throw std::runtime_error("time out(");
                    }
                    break;
                }

            }
            if (!choked_ && !pendingBlock_) {
                last_action = std::chrono::steady_clock::now();
                RequestPiece();
            }
        }
        catch (const std::exception& e) {
            std::cout << "error in mainloop " << e.what() << std::endl;
            if (pieceInProgress_ != nullptr)
                pieceStorage_.AddPieceToDownload(pieceInProgress_);
            pendingBlock_ = false;
            failed_.store(true);
            Terminate();
        }
    }
}
