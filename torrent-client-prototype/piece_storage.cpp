#include "piece_storage.h"

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, const int count_pieces_to_download)
        : tf_(tf)
        , TotalPiecesCount_(tf.pieceHashes.size())
        , remainPieces_()
        , PiecesInProgressCount_(0)
        , PiecesSavedToDiscIndices_()
        , file_(outputDirectory / tf_.name, std::ios::binary | std::ios::out)
        , mtx_()
{
    file_.seekp(tf.length - 1);
    file_.write("\0", 1);
    if (!file_.is_open()) throw "file can't be opened";
    for(int i = count_pieces_to_download - 1; i >= 0; --i) {
        remainPieces_.emplace(std::make_shared<Piece>(
                i,
                (i == tf.pieceHashes.size() - 1 ? tf.length % tf.pieceLength : tf.pieceLength),
                tf.pieceHashes[i]
                ));
    }
}
PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (remainPieces_.empty()) return nullptr;
    PiecePtr ptr = remainPieces_.front();
    remainPieces_.pop();
    ++PiecesInProgressCount_;
    return ptr;
}

void PieceStorage::AddPieceToDownload(PiecePtr& ptr) {
    std::lock_guard<std::mutex> lock(mtx_);
    ptr->Reset();
    remainPieces_.push(ptr);
    --PiecesInProgressCount_;
    ptr.reset();
}

void PieceStorage::PieceProcessed(PiecePtr& piece) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (!piece->HashMatches()) {
        std::cout << "Piece " << piece->GetIndex() << " hash doesn't match" << std::endl;
        mtx_.unlock();
        AddPieceToDownload(piece);
        return;
    }
    SavePieceToDisk(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return TotalPiecesCount_;
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    if (!file_.is_open()) {
        std::cerr << "Output file is already closed" << std::endl;
        return;
    }
    file_.seekp(piece->GetIndex() * tf_.pieceLength);
    file_.write(piece->GetData().data(), piece->GetData().size());
    --PiecesInProgressCount_;
    PiecesSavedToDiscIndices_.push_back(piece -> GetIndex());
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return PiecesSavedToDiscIndices_.size();
}

void PieceStorage::CloseOutputFile() {
    std::lock_guard<std::mutex> lock(mtx_);
    file_.close();
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return PiecesSavedToDiscIndices_;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return PiecesInProgressCount_;
}