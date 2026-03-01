#include "message.h"
#include "byte_tools.h"

Message::Message(MessageId id, size_t messageLength, std::string payload)
        : id(id)
        , messageLength(messageLength)
        , payload(payload)
{}

/*
 * Выделяем тип сообщения и длину и создаем объект типа Message.
 * Подразумевается, что здесь в качестве `messageString` будет приниматься строка, прочитанная из TCP-сокета
 */
Message Message::Parse(const std::string& messageString) {
    if (messageString.size() == 0) return Message(MessageId::KeepAlive, 0, "");
    return Message(static_cast<MessageId> (messageString[0])
            , messageString.size() - 1
            , messageString.substr(1, messageString.size() - 1));
}

/*
 * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
 */
Message Message::Init(MessageId id, const std::string& payload) {
    return Message(id, payload.size(), payload);
}

/*
 * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
 * Получается строка вида "<1 + payload length><message id><payload>"
 * Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
 * id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
 */
std::string Message::ToString() const {
    if (id == MessageId::KeepAlive) return IntToBytes(0);
    std::string st;
    st = IntToBytes(1 + messageLength) + static_cast<char>(id) + payload;
    return st;
}