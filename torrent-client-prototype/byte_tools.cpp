#include "byte_tools.h"

int BytesToInt(std::string_view bytes) {
    int ans = 0;
    uint8_t val;
    for(int i = 0; i < bytes.size(); ++i) {
        val = static_cast<uint8_t>(bytes[i]);
        ans <<= 8;
        ans += val;
    }
    return ans;
}

std::string IntToBytes(int val) {
    std::string st = "";
    for(int i = 0; i < 4; ++i) {
        st = static_cast<char>(val % 256) + st;
        val >>= 8;
    }
    return st;
}

void sha1(char* arr, size_t len, unsigned char* ans)
{
    SHA1( (const unsigned char*) arr, len, ans);
}

std::string CalculateSHA1(const std::string& msg) {
    char msg_mas[msg.size()];
    unsigned char msg_hash[20];
    for (size_t i = 0; i < msg.size(); ++i) {
        msg_mas[i] = msg[i];
    }
    sha1(msg_mas, msg.size(), msg_hash);
    std::string ans_hash = "";
    for (int i = 0; i < 20; ++i) {
        ans_hash += msg_hash[i];
    }
    return ans_hash;
}

/*
 * Представить массив байтов в виде строки, содержащей только символы, соответствующие цифрам в шестнадцатеричном исчислении.
 * Конкретный формат выходной строки не важен. Важно то, чтобы выходная строка не содержала символов, которые нельзя
 * было бы представить в кодировке utf-8. Данная функция будет использована для вывода SHA1 хеш-суммы в лог.
 */
std::string HexEncode(const std::string& input) {
    std::string ans = "";
    for(int i = 0; i < input.size(); ++i) {
        uint8_t ch = static_cast<uint8_t> (input[i]);
        ans += (ch / 16 < 10 ? '0' + ch / 16 : 'a' + (ch / 16 - 10));
        ans += (ch % 16 < 10 ? '0' + ch % 16 : 'a' + (ch % 16 - 10));
    }
    return ans;
}
