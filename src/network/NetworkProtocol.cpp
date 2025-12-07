#include "network/NetworkProtocol.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

namespace JJM {
namespace Network {

void Packet::writeInt8(int8_t value) {
    data.push_back(static_cast<uint8_t>(value));
}

void Packet::writeInt16(int16_t value) {
    uint16_t netValue = htons(static_cast<uint16_t>(value));
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&netValue);
    data.insert(data.end(), bytes, bytes + sizeof(int16_t));
}

void Packet::writeInt32(int32_t value) {
    uint32_t netValue = htonl(static_cast<uint32_t>(value));
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&netValue);
    data.insert(data.end(), bytes, bytes + sizeof(int32_t));
}

void Packet::writeInt64(int64_t value) {
    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

void Packet::writeFloat(float value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(float));
}

void Packet::writeDouble(double value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(double));
}

void Packet::writeString(const std::string& value) {
    writeInt32(static_cast<int32_t>(value.size()));
    data.insert(data.end(), value.begin(), value.end());
}

void Packet::writeBytes(const uint8_t* bytes, size_t count) {
    data.insert(data.end(), bytes, bytes + count);
}

int8_t Packet::readInt8(size_t& offset) const {
    if (offset + sizeof(int8_t) > data.size()) return 0;
    return static_cast<int8_t>(data[offset++]);
}

int16_t Packet::readInt16(size_t& offset) const {
    if (offset + sizeof(int16_t) > data.size()) return 0;
    uint16_t netValue;
    std::memcpy(&netValue, &data[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    return static_cast<int16_t>(ntohs(netValue));
}

int32_t Packet::readInt32(size_t& offset) const {
    if (offset + sizeof(int32_t) > data.size()) return 0;
    uint32_t netValue;
    std::memcpy(&netValue, &data[offset], sizeof(int32_t));
    offset += sizeof(int32_t);
    return static_cast<int32_t>(ntohl(netValue));
}

int64_t Packet::readInt64(size_t& offset) const {
    if (offset + sizeof(int64_t) > data.size()) return 0;
    int64_t value = 0;
    for (size_t i = 0; i < 8; ++i) {
        value = (value << 8) | data[offset++];
    }
    return value;
}

float Packet::readFloat(size_t& offset) const {
    if (offset + sizeof(float) > data.size()) return 0.0f;
    float value;
    std::memcpy(&value, &data[offset], sizeof(float));
    offset += sizeof(float);
    return value;
}

double Packet::readDouble(size_t& offset) const {
    if (offset + sizeof(double) > data.size()) return 0.0;
    double value;
    std::memcpy(&value, &data[offset], sizeof(double));
    offset += sizeof(double);
    return value;
}

std::string Packet::readString(size_t& offset) const {
    int32_t length = readInt32(offset);
    if (length <= 0 || offset + length > data.size()) return "";
    
    std::string result(reinterpret_cast<const char*>(&data[offset]), length);
    offset += length;
    return result;
}

void Packet::readBytes(uint8_t* buffer, size_t count, size_t& offset) const {
    if (offset + count > data.size()) return;
    std::memcpy(buffer, &data[offset], count);
    offset += count;
}

void Packet::clear() {
    data.clear();
    size = 0;
}

Socket::Socket(SocketType type) : socketHandle(-1), type(type) {
    int sockType = (type == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;
    socketHandle = socket(AF_INET, sockType, 0);
}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (socketHandle != -1) {
        ::close(socketHandle);
        socketHandle = -1;
    }
}

bool Socket::bind(const std::string& address, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = address.empty() ? INADDR_ANY : inet_addr(address.c_str());
    
    return ::bind(socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

bool Socket::listen(int backlog) {
    return ::listen(socketHandle, backlog) == 0;
}

Socket* Socket::accept() {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = ::accept(socketHandle, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (clientSocket < 0) return nullptr;
    
    Socket* client = new Socket(type);
    client->socketHandle = clientSocket;
    return client;
}

bool Socket::connect(const std::string& address, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    
    return ::connect(socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

void Socket::disconnect() {
    close();
}

int Socket::send(const uint8_t* data, size_t size) {
    return static_cast<int>(::send(socketHandle, data, size, 0));
}

int Socket::receive(uint8_t* buffer, size_t size) {
    return static_cast<int>(::recv(socketHandle, buffer, size, 0));
}

void Socket::setBlocking(bool blocking) {
    int flags = fcntl(socketHandle, F_GETFL, 0);
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    fcntl(socketHandle, F_SETFL, flags);
}

void Socket::setReuseAddress(bool reuse) {
    int option = reuse ? 1 : 0;
    setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
}

Connection::Connection(Socket* socket)
    : socket(socket), status(ConnectionStatus::Connected), reliable(true), ping(0), nextPacketId(0) {}

Connection::~Connection() {}

bool Connection::send(const Packet& packet) {
    std::vector<uint8_t> buffer;
    
    uint32_t netId = htonl(packet.id);
    uint32_t netSize = htonl(static_cast<uint32_t>(packet.data.size()));
    
    const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&netId);
    const uint8_t* sizeBytes = reinterpret_cast<const uint8_t*>(&netSize);
    
    buffer.insert(buffer.end(), idBytes, idBytes + sizeof(uint32_t));
    buffer.insert(buffer.end(), sizeBytes, sizeBytes + sizeof(uint32_t));
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
    
    return sendRaw(buffer.data(), buffer.size());
}

bool Connection::receive(Packet& packet) {
    uint8_t header[8];
    if (!receiveRaw(header, 8)) return false;
    
    uint32_t netId, netSize;
    std::memcpy(&netId, header, sizeof(uint32_t));
    std::memcpy(&netSize, header + 4, sizeof(uint32_t));
    
    packet.id = ntohl(netId);
    packet.size = ntohl(netSize);
    
    packet.data.resize(packet.size);
    return receiveRaw(packet.data.data(), packet.size);
}

bool Connection::sendRaw(const uint8_t* data, size_t size) {
    return socket->send(data, size) == static_cast<int>(size);
}

bool Connection::receiveRaw(uint8_t* buffer, size_t size) {
    return socket->receive(buffer, size) == static_cast<int>(size);
}

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {
    stopServer();
    for (Connection* conn : connections) {
        delete conn;
    }
}

bool NetworkManager::startServer(uint16_t port, SocketType type) {
    serverSocket = std::make_unique<Socket>(type);
    serverSocket->setReuseAddress(true);
    
    if (!serverSocket->bind("", port)) return false;
    if (type == SocketType::TCP && !serverSocket->listen()) return false;
    
    serverSocket->setBlocking(false);
    return true;
}

void NetworkManager::stopServer() {
    serverSocket.reset();
}

Connection* NetworkManager::connect(const std::string& address, uint16_t port, SocketType type) {
    Socket* socket = new Socket(type);
    
    if (!socket->connect(address, port)) {
        delete socket;
        return nullptr;
    }
    
    Connection* connection = new Connection(socket);
    connections.push_back(connection);
    
    if (connectionHandler) {
        connectionHandler(connection, true);
    }
    
    return connection;
}

void NetworkManager::disconnect(Connection* connection) {
    auto it = std::find(connections.begin(), connections.end(), connection);
    if (it != connections.end()) {
        if (connectionHandler) {
            connectionHandler(connection, false);
        }
        delete connection;
        connections.erase(it);
    }
}

void NetworkManager::update() {
    if (serverSocket) {
        acceptConnections();
    }
    processConnections();
}

void NetworkManager::acceptConnections() {
    Socket* clientSocket = serverSocket->accept();
    if (clientSocket) {
        Connection* connection = new Connection(clientSocket);
        connections.push_back(connection);
        
        if (connectionHandler) {
            connectionHandler(connection, true);
        }
    }
}

void NetworkManager::processConnections() {
    for (Connection* connection : connections) {
        Packet packet;
        while (connection->receive(packet)) {
            if (packetHandler) {
                packetHandler(connection, packet);
            }
        }
    }
}

} // namespace Network
} // namespace JJM
