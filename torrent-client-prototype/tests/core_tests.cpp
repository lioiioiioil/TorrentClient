#include "bencode.h"
#include "byte_tools.h"
#include "message.h"
#include "piece_storage.h"

#include <iterator>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void TestBencode() {
    const std::string encoded = "d4:infod6:lengthi8e4:name4:testee";
    const auto root = decode(encoded);
    Check(root && root->type_ == "dict", "torrent metadata is a dictionary");
    const auto info = root->get_dict().at("info");
    Check(info->get_dict().at("length")->get_int() == 8, "decoded file length");
    Check(info->get_dict().at("name")->get_string() == "test", "decoded filename");
    std::string serialized;
    get_bencode(root, serialized);
    Check(serialized == encoded, "nested metadata round trip");
    const std::string binary("\0\1\2\3", 4);
    const auto binary_root = decode("d4:data4:" + binary + "e");
    Check(binary_root->get_dict().at("data")->get_string() == binary,
          "bencoded strings preserve binary bytes");
}

void TestWireMessages() {
    const std::string payload("\0\0\0\7", 4);
    const std::string expected("\0\0\0\5\4\0\0\0\7", 9);
    Check(Message::Init(MessageId::Have, payload).ToString() == expected,
          "have message uses a big-endian length and message id");
    // TCP framing strips the four-byte length before Message::Parse.
    const auto parsed = Message::Parse(expected.substr(4));
    Check(parsed.id == MessageId::Have && parsed.messageLength == 4 &&
          parsed.payload == payload, "parse an independently specified wire body");
    Check(Message::Init(MessageId::KeepAlive, "").ToString() == std::string(4, '\0'),
          "keep-alive frame contains only a zero length");
    Check(Message::Parse("").id == MessageId::KeepAlive, "empty body is keep-alive");
    Check(BytesToInt(std::string("\1\2\3\4", 4)) == 16909060, "big-endian integer");
}

void TestHashes() {
    Check(HexEncode(CalculateSHA1("abc")) == "a9993e364706816aba3e25717850c26c9cd0d89d",
          "SHA-1 known-answer vector");
    Check(HexEncode(CalculateSHA1("")) == "da39a3ee5e6b4b0d3255bfef95601890afd80709",
          "SHA-1 empty input");
}

struct TemporaryDirectory {
    std::filesystem::path path;
    TemporaryDirectory() : path(std::filesystem::temp_directory_path() /
        ("torrent-core-tests-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()))) {
        if (!std::filesystem::create_directory(path)) {
            throw std::runtime_error("cannot create isolated test directory");
        }
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

void TestPieceStorage() {
    TemporaryDirectory directory;
    TorrentFile torrent{};
    torrent.name = "test.bin";
    torrent.length = 8;
    torrent.pieceLength = 4;
    torrent.pieceHashes = {CalculateSHA1("abcd"), CalculateSHA1("efgh")};
    PieceStorage storage(torrent, directory.path, 2);
    auto first = storage.GetNextPieceToDownload();
    Check(first && first->GetIndex() < 2, "first requested piece exists");
    const auto first_index = first->GetIndex();
    const std::string chunks[] = {"abcd", "efgh"};
    const auto first_block = first->FirstMissingBlock();
    Check(first_block && first_block->length == 4, "first piece contains a full block");
    first->SaveBlock(0, "xxxx");
    storage.PieceProcessed(first);
    Check(!first, "corrupted piece is released for retry");
    Check(storage.PiecesSavedToDiscCount() == 0 && storage.PiecesInProgressCount() == 0,
          "corrupted piece is not saved and leaves the in-progress count");
    auto other = storage.GetNextPieceToDownload();
    Check(other && other->GetIndex() < 2 && other->GetIndex() != first_index,
          "remaining piece precedes the retried piece");
    const auto block = other->FirstMissingBlock();
    Check(block && block->length == 4, "exact-multiple file has a full final piece");
    other->SaveBlock(0, chunks[other->GetIndex()]);
    storage.PieceProcessed(other);
    auto retry = storage.GetNextPieceToDownload();
    Check(retry && retry->GetIndex() == first_index, "corrupted piece can be requested again");
    retry->SaveBlock(0, chunks[first_index]);
    storage.PieceProcessed(retry);
    Check(storage.PiecesSavedToDiscCount() == 2 && storage.QueueIsEmpty() &&
          storage.PiecesInProgressCount() == 0, "all pieces are saved once");
    storage.CloseOutputFile();
    std::ifstream result(directory.path / torrent.name, std::ios::binary);
    const std::string data((std::istreambuf_iterator<char>(result)),
                           std::istreambuf_iterator<char>());
    Check(data == "abcdefgh", "pieces are written at their original offsets");
}
} // namespace

int main() {
    try {
        TestBencode();
        TestWireMessages();
        TestHashes();
        TestPieceStorage();
        std::cout << "All core tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Core test failure: " << error.what() << '\n';
        return 1;
    }
}
