#include "network/NetworkManager.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>

namespace JJM {
namespace Network {

// Packet implementation
void Packet::serialize(std::vector<uint8_t>& buffer) const {
    buffer.clear();
    
    // Write type (2 bytes)
    uint16_t typeValue = static_cast<uint16_t>(type);
    buffer.push_back(typeValue & 0xFF);
    buffer.push_back((typeValue >> 8) & 0xFF);
    
    // Write size (4 bytes)
    buffer.push_back(size & 0xFF);
    buffer.push_back((size >> 8) & 0xFF);
    buffer.push_back((size >> 16) & 0xFF);
    buffer.push_back((size >> 24) & 0xFF);
    
    // Write data
    buffer.insert(buffer.end(), data.begin(), data.end());
}

bool Packet::deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (buffer.size() < offset + 6) return false;
    
    // Read type
    uint16_t typeValue = buffer[offset] | (buffer[offset + 1] << 8);
    type = static_cast<PacketType>(typeValue);
    offset += 2;
    
    // Read size
    size = buffer[offset] | (buffer[offset + 1] << 8) | 
           (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;
    
    // Read data
    if (buffer.size() < offset + size) return false;
    
    data.clear();
    data.resize(size);
    std::copy(buffer.begin() + offset, buffer.begin() + offset + size, data.begin());
    offset += size;
    
    return true;
}

// NetworkManager implementation
NetworkManager::NetworkManager()
    : isServer(false), isRunning(false), socketFd(-1),
      socketType(SocketType::TCP), nextClientId(1), shouldStop(false) {
}

NetworkManager::~NetworkManager() {
    stop();
}

bool NetworkManager::startServer(uint16_t port, SocketType type) {
    if (isRunning) return false;
    
    socketType = type;
    isServer = true;
    
    if (!createSocket()) {
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        closeSocket();
        return false;
    }
    
    if (socketType == SocketType::TCP) {
        if (listen(socketFd, 10) < 0) {
            std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
            closeSocket();
            return false;
        }
    }
    
    isRunning = true;
    shouldStop = false;
    networkThread = std::thread(&NetworkManager::networkThreadFunc, this);
    
    std::cout << "Server started on port " << port << " (" 
              << (socketType == SocketType::TCP ? "TCP" : "UDP") << ")" << std::endl;
    return true;
}

bool NetworkManager::connectToServer(const std::string& address, uint16_t port, SocketType type) {
    if (isRunning) return false;
    
    socketType = type;
    isServer = false;
    
    if (!createSocket()) {
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << address << std::endl;
        closeSocket();
        return false;
    }
    
    if (socketType == SocketType::TCP) {
        if (connect(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Failed to connect to server: " << strerror(errno) << std::endl;
            closeSocket();
            return false;
        }
    }
    
    isRunning = true;
    shouldStop = false;
    networkThread = std::thread(&NetworkManager::networkThreadFunc, this);
    
    std::cout << "Connected to server " << address << ":" << port << std::endl;
    return true;
}

void NetworkManager::stop() {
    if (!isRunning) return;
    
    shouldStop = true;
    isRunning = false;
    
    if (networkThread.joinable()) {
        networkThread.join();
    }
    
    closeSocket();
    clients.clear();
}

void NetworkManager::sendPacket(const Packet& packet, int clientId) {
    std::lock_guard<std::mutex> lock(packetMutex);
    outgoingPackets.push(std::make_pair(clientId, packet));
}

void NetworkManager::broadcastPacket(const Packet& packet) {
    sendPacket(packet, -1);
}

void NetworkManager::registerPacketHandler(PacketType type, PacketHandler handler) {
    packetHandlers[type] = handler;
}

void NetworkManager::setClientConnectedHandler(ClientConnectedHandler handler) {
    onClientConnected = handler;
}

void NetworkManager::setClientDisconnectedHandler(ClientDisconnectedHandler handler) {
    onClientDisconnected = handler;
}

void NetworkManager::update() {
    std::lock_guard<std::mutex> lock(packetMutex);
    
    while (!incomingPackets.empty()) {
        auto& packetPair = incomingPackets.front();
        
        auto it = packetHandlers.find(packetPair.second.type);
        if (it != packetHandlers.end()) {
            it->second(packetPair.first, packetPair.second);
        }
        
        incomingPackets.pop();
    }
}

int NetworkManager::getClientCount() const {
    return static_cast<int>(clients.size());
}

std::vector<int> NetworkManager::getConnectedClients() const {
    std::vector<int> clientIds;
    for (const auto& pair : clients) {
        if (pair.second.connected) {
            clientIds.push_back(pair.first);
        }
    }
    return clientIds;
}

void NetworkManager::networkThreadFunc() {
    if (isServer) {
        if (socketType == SocketType::TCP) {
            handleTCPServer();
        } else {
            handleUDPServer();
        }
    } else {
        if (socketType == SocketType::TCP) {
            handleTCPClient();
        } else {
            handleUDPClient();
        }
    }
}

void NetworkManager::handleTCPServer() {
    fd_set readfds;
    struct timeval timeout;
    
    while (!shouldStop) {
        FD_ZERO(&readfds);
        FD_SET(socketFd, &readfds);
        
        int maxfd = socketFd;
        for (const auto& pair : clients) {
            if (pair.second.connected) {
                FD_SET(pair.first, &readfds);
                maxfd = std::max(maxfd, pair.first);
            }
        }
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity > 0) {
            if (FD_ISSET(socketFd, &readfds)) {
                int clientSocket = acceptNewClient();
                if (clientSocket > 0) {
                    ClientInfo info;
                    info.id = clientSocket;
                    info.connected = true;
                    clients[clientSocket] = info;
                    
                    if (onClientConnected) {
                        onClientConnected(clientSocket, "unknown");
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void NetworkManager::handleTCPClient() {
    // Simplified TCP client handling
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void NetworkManager::handleUDPServer() {
    // Simplified UDP server handling
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void NetworkManager::handleUDPClient() {
    // Simplified UDP client handling
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

int NetworkManager::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(socketFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        return -1;
    }
    
    return clientSocket;
}

void NetworkManager::disconnectClient(int clientId) {
    auto it = clients.find(clientId);
    if (it != clients.end()) {
        it->second.connected = false;
        close(clientId);
        
        if (onClientDisconnected) {
            onClientDisconnected(clientId);
        }
    }
}

bool NetworkManager::createSocket() {
    int type = (socketType == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;
    int protocol = (socketType == SocketType::TCP) ? IPPROTO_TCP : IPPROTO_UDP;
    
    socketFd = socket(AF_INET, type, protocol);
    if (socketFd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(socketFd);
        socketFd = -1;
        return false;
    }
    
    return true;
}

void NetworkManager::closeSocket() {
    if (socketFd >= 0) {
        close(socketFd);
        socketFd = -1;
    }
}

std::vector<uint8_t> NetworkManager::serializeString(const std::string& str) {
    std::vector<uint8_t> result;
    uint32_t size = str.length();
    
    result.push_back(size & 0xFF);
    result.push_back((size >> 8) & 0xFF);
    result.push_back((size >> 16) & 0xFF);
    result.push_back((size >> 24) & 0xFF);
    
    result.insert(result.end(), str.begin(), str.end());
    return result;
}

std::string NetworkManager::deserializeString(const std::vector<uint8_t>& data, size_t& offset) {
    if (data.size() < offset + 4) return "";
    
    uint32_t size = data[offset] | (data[offset + 1] << 8) | 
                    (data[offset + 2] << 16) | (data[offset + 3] << 24);
    offset += 4;
    
    if (data.size() < offset + size) return "";
    
    std::string result(data.begin() + offset, data.begin() + offset + size);
    offset += size;
    return result;
}

void NetworkManager::writeUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

uint32_t NetworkManager::readUint32(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (buffer.size() < offset + 4) return 0;
    
    uint32_t value = buffer[offset] | (buffer[offset + 1] << 8) | 
                     (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;
    return value;
}

} // namespace Network
} // namespace JJM