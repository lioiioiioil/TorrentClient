#include "tcp_connect.h"
#include "byte_tools.h"

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
        : ip_(ip)
        , port_(port)
        , connectTimeout_(connectTimeout)
        , readTimeout_(readTimeout)
        , sock_(socket(AF_INET, SOCK_STREAM, 0))
{}
void TcpConnect::EstablishConnection() {
    int flags = fcntl(sock_, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(sock_, F_SETFL, flags);
    struct sockaddr_in Addr;
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port_);
    Addr.sin_addr.s_addr = inet_addr(ip_.c_str());
    connect(sock_, (struct sockaddr *)&Addr, sizeof(Addr));

    struct pollfd fds[1];
    fds[0].fd = sock_;
    fds[0].events = POLLOUT;

    int result = poll(fds, 1, connectTimeout_.count());
    if (result <= 0) throw std::runtime_error("error_connection");
}

void TcpConnect::SendData(const std::string& data) const {
    int len = data.size();
    auto ptr = data.c_str();
    while (len > 0) {
        int sent = send(sock_, ptr, len, 0);
        if (sent <= 0) throw std::runtime_error("Failed to send data");
        len -= sent;
        ptr += sent;
    }

}
std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    struct pollfd fds[1];
    fds[0].fd = sock_;
    fds[0].events = POLLIN;
    std::string st = "";
    size_t length = bufferSize;
    if (bufferSize == 0)
    {
        std::string val;
        length = 4;
        char buf[length];
        int len = length;
        auto ptr = &buf[0];
        while (len > 0) {
            int ret = poll(fds, 1, readTimeout_.count());
            if (ret <= 0) throw std::runtime_error("error_receive");
            int receive = recv(sock_, ptr, len, 0);
            if (receive <= 0) throw std::runtime_error("error_receive");
            len -= receive;
            ptr += receive;
        }
        for(int i = 0; i < 4; ++i) {
            val += buf[i];
        }
        length = BytesToInt(val);
    }
    if (length > 0) {
        char buffer[length];
        int len = length;
        auto ptr = &buffer[0];
        while (len > 0) {
            int ret = poll(fds, 1, readTimeout_.count());
            if (ret <= 0) throw std::runtime_error("error_receive");
            int receive = recv(sock_, ptr, len, 0);
            if (receive <= 0) throw std::runtime_error("error_receive 2");
            len -= receive;
            ptr += receive;
        }
        for (int i = 0; i < length; ++i) {
            st += buffer[i];
        }
    }
    return st;
}
void TcpConnect::CloseConnection() {
    close(sock_);
}
const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}

