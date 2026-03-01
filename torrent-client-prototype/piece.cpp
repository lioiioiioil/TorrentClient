#include "byte_tools.h"
#include "piece.h"

constexpr size_t BLOCK_SIZE = 1 << 14;

/*
 * index -- номер части файла, нумерация начинается с 0
 * length -- длина части файла. Все части, кроме последней, имеют длину, равную `torrentFile.pieceLength`
 * hash -- хеш-сумма части файла, взятая из `torrentFile.pieceHashes`
 */
Piece::Piece(size_t index, size_t length, std::string hash)
    : index_(index)
    , length_(length)
    , hash_(hash)
    , blocks_()
{
    Block block;
    block.piece = index_;
    block.status = Block::Status::Missing;
    block.data = "";
    size_t offset = 0;
    while (offset < length_) {
        block.offset = offset;
        block.length = (block.offset + BLOCK_SIZE <= length_ ? BLOCK_SIZE :
                        length_ - block.offset);
        blocks_.push_back(block);
        offset += BLOCK_SIZE;
    }
}

/*
 * Совпадает ли хеш скачанных данных с ожидаемым
 */
bool Piece::HashMatches() const {
    return GetDataHash() == GetHash();
}

/*
 * Дать указатель на отсутствующий (еще не скачанный и не запрошенный) блок
 */
Block* Piece::FirstMissingBlock() {
    for(auto& x : blocks_) {
        if (x.status == Block::Status::Missing) {
            x.status = Block::Status::Pending;
            return &x;
        }
    }
    return nullptr;
}
/*
 * Получить порядковый номер части файла
 */
size_t Piece::GetIndex() const {
    return index_;
}

/*
 * Сохранить скачанные данные для какого-то блока
 */
void Piece::SaveBlock(size_t blockOffset, std::string data) {
    for(auto& x : blocks_) {
        if (x.offset == blockOffset) {
            x.data = data;
            x.status = Block::Status::Retrieved;
            return;
        }
    }
}

/*
 * Скачали ли уже все блоки
 */
bool Piece::AllBlocksRetrieved() const {
    bool flag = true;
    for(auto& x : blocks_) {
        if (x.status != Block::Status::Retrieved) flag = false;
    }
    return flag && HashMatches();
}

/*
 * Получить скачанные данные для части файла
 */
std::string Piece::GetData() const {
    std::string big_data = "";
    for(auto& x : blocks_) {
        big_data += x.data;
    }
    return big_data;
}

/*
 * Посчитать хеш по скачанным данным
 */
std::string Piece::GetDataHash() const {
    std::string big_data = GetData();
    return CalculateSHA1(big_data);
}

/*
 * Получить хеш для части из .torrent файла
 */
const std::string& Piece::GetHash() const {
    return hash_;
}

/*
 * Удалить все скачанные данные и отметить все блоки как Missing
 */
void Piece::Reset() {
    for(auto& x : blocks_) {
        x.data = "";
        x.status = Block::Status::Missing;
    }
}
