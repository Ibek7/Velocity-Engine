#include "network/NetworkSystem.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace JJM {
namespace Network {

// Connection implementation
Connection::Connection()
    : address(""), port(0), connected(false), ping(0.0f),
      nextSequenceNumber(0), lastReceiveTime(0.0) {}

Connection::Connection(const std::string& address, uint16_t port)
    : address(address), port(port), connected(false), ping(0.0f),
      nextSequenceNumber(0), lastReceiveTime(0.0) {}

Connection::~Connection() {}

double Connection::getTimeSinceLastReceived() const {
    return getCurrentTime() - lastReceiveTime;
}

void Connection::addPendingAck(uint32_t sequenceNumber) {
    pendingAcks.push(sequenceNumber);
}

bool Connection::shouldAck(uint32_t sequenceNumber) const {
    // Check if this sequence number needs to be acked
    return true;
}

double Connection::getCurrentTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

// NetworkClient implementation
NetworkClient::NetworkClient()
    : lastPingTime(0.0) {}

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connect(const std::string& address, uint16_t port) {
    connection = std::make_unique<Connection>(address, port);
    
    // Send connection packet
    Packet connectPacket;
    connectPacket.type = PacketType::Connect;
    sendQueue.push(connectPacket);
    
    connection->setConnected(true);
    
    if (onConnected) {
        onConnected();
    }
    
    return true;
}

void NetworkClient::disconnect() {
    if (!connection || !connection->isConnected()) {
        return;
    }
    
    Packet disconnectPacket;
    disconnectPacket.type = PacketType::Disconnect;
    send(disconnectPacket);
    
    connection->setConnected(false);
    
    if (onDisconnected) {
        onDisconnected();
    }
}

bool NetworkClient::send(const Packet& packet) {
    if (!connection || !connection->isConnected()) {
        return false;
    }
    
    sendQueue.push(packet);
    return true;
}

bool NetworkClient::send(const uint8_t* data, size_t size, bool reliable) {
    if (!connection || !connection->isConnected()) {
        return false;
    }
    
    Packet packet;
    packet.type = reliable ? PacketType::Reliable : PacketType::Unreliable;
    packet.sequenceNumber = connection->getNextSequenceNumber();
    packet.data.assign(data, data + size);
    
    return send(packet);
}

void NetworkClient::update(double deltaTime) {
    if (!connection || !connection->isConnected()) {
        return;
    }
    
    // Send queued packets
    while (!sendQueue.empty()) {
        Packet& packet = sendQueue.front();
        // Actually send the packet over the network
        sendQueue.pop();
    }
    
    // Receive packets
    // In a real implementation, this would read from socket
    while (!receiveQueue.empty()) {
        Packet& packet = receiveQueue.front();
        processPacket(packet);
        receiveQueue.pop();
    }
    
    // Send ping
    if (lastPingTime + 1.0 < deltaTime) {
        sendPing();
        lastPingTime = deltaTime;
    }
}

float NetworkClient::getPing() const {
    return connection ? connection->getPing() : 0.0f;
}

void NetworkClient::processPacket(const Packet& packet) {
    connection->updateLastReceived();
    
    switch (packet.type) {
        case PacketType::Disconnect:
            disconnect();
            break;
            
        case PacketType::Pong:
            handlePong(packet);
            break;
            
        case PacketType::Data:
        case PacketType::Reliable:
        case PacketType::Unreliable:
            if (onDataReceived && !packet.data.empty()) {
                onDataReceived(packet.data.data(), packet.data.size());
            }
            break;
            
        default:
            break;
    }
}

void NetworkClient::sendPing() {
    Packet pingPacket;
    pingPacket.type = PacketType::Ping;
    send(pingPacket);
}

void NetworkClient::handlePong(const Packet& packet) {
    // Calculate ping from round trip time
    if (connection) {
        connection->setPing(static_cast<float>(connection->getTimeSinceLastReceived()));
    }
}

// NetworkServer implementation
NetworkServer::NetworkServer()
    : running(false), port(0), nextClientId(1) {}

NetworkServer::~NetworkServer() {
    stop();
}

bool NetworkServer::start(uint16_t port) {
    this->port = port;
    running = true;
    
    // Initialize server socket
    // Bind to port
    // Listen for connections
    
    return true;
}

void NetworkServer::stop() {
    if (!running) {
        return;
    }
    
    // Disconnect all clients
    for (auto& pair : clients) {
        if (onClientDisconnected) {
            onClientDisconnected(pair.first);
        }
    }
    
    clients.clear();
    running = false;
}

void NetworkServer::update(double deltaTime) {
    if (!running) {
        return;
    }
    
    acceptNewClients();
    processClientPackets();
    checkClientTimeouts();
}

void NetworkServer::broadcast(const uint8_t* data, size_t size, bool reliable) {
    for (auto& pair : clients) {
        sendTo(pair.first, data, size, reliable);
    }
}

void NetworkServer::sendTo(uint32_t clientId, const uint8_t* data, size_t size, bool reliable) {
    auto it = clients.find(clientId);
    if (it == clients.end()) {
        return;
    }
    
    Packet packet;
    packet.type = reliable ? PacketType::Reliable : PacketType::Unreliable;
    packet.sequenceNumber = it->second->connection->getNextSequenceNumber();
    packet.data.assign(data, data + size);
    
    // Send packet to client
}

void NetworkServer::kickClient(uint32_t clientId) {
    handleClientDisconnect(clientId);
}

void NetworkServer::acceptNewClients() {
    // Accept new connections from socket
    // For each new connection, call handleClientConnect
}

void NetworkServer::processClientPackets() {
    for (auto& pair : clients) {
        // Read packets from client socket
        // Call handleClientData for each packet
    }
}

void NetworkServer::checkClientTimeouts() {
    std::vector<uint32_t> timedOutClients;
    
    for (auto& pair : clients) {
        if (pair.second->connection->getTimeSinceLastReceived() > 30.0) {
            timedOutClients.push_back(pair.first);
        }
    }
    
    for (uint32_t clientId : timedOutClients) {
        handleClientDisconnect(clientId);
    }
}

void NetworkServer::handleClientConnect(const std::string& address, uint16_t port) {
    uint32_t clientId = nextClientId++;
    
    auto clientConn = std::make_unique<ClientConnection>();
    clientConn->clientId = clientId;
    clientConn->connection = std::make_unique<Connection>(address, port);
    clientConn->connection->setConnected(true);
    clientConn->lastActivityTime = clientConn->connection->getTimeSinceLastReceived();
    
    clients[clientId] = std::move(clientConn);
    
    if (onClientConnected) {
        onClientConnected(clientId);
    }
}

void NetworkServer::handleClientDisconnect(uint32_t clientId) {
    auto it = clients.find(clientId);
    if (it == clients.end()) {
        return;
    }
    
    if (onClientDisconnected) {
        onClientDisconnected(clientId);
    }
    
    clients.erase(it);
}

void NetworkServer::handleClientData(uint32_t clientId, const Packet& packet) {
    auto it = clients.find(clientId);
    if (it == clients.end()) {
        return;
    }
    
    it->second->connection->updateLastReceived();
    
    if (onDataReceived && !packet.data.empty()) {
        onDataReceived(clientId, packet.data.data(), packet.data.size());
    }
}

// ReliableChannel implementation
ReliableChannel::ReliableChannel()
    : nextSequenceNumber(0), expectedSequenceNumber(0) {}

ReliableChannel::~ReliableChannel() {}

void ReliableChannel::sendReliable(const uint8_t* data, size_t size) {
    std::vector<uint8_t> dataVec(data, data + size);
    sendQueue.push(dataVec);
}

void ReliableChannel::receivePacket(const Packet& packet) {
    uint32_t seq = packet.sequenceNumber;
    
    if (receivedPackets.find(seq) != receivedPackets.end()) {
        return; // Duplicate packet
    }
    
    processReceivedPacket(seq, packet.data);
}

void ReliableChannel::update(double deltaTime) {
    resendUnacked(deltaTime);
}

Packet ReliableChannel::getNextPacket() {
    Packet packet;
    
    if (!sendQueue.empty()) {
        packet.sequenceNumber = nextSequenceNumber++;
        packet.type = PacketType::Reliable;
        packet.data = sendQueue.front();
        sendQueue.pop();
        
        ReliablePacket reliablePacket;
        reliablePacket.packet = packet;
        reliablePacket.sendTime = 0.0; // Current time
        pendingAcks[packet.sequenceNumber] = reliablePacket;
    }
    
    return packet;
}

void ReliableChannel::resendUnacked(double currentTime) {
    for (auto& pair : pendingAcks) {
        ReliablePacket& rp = pair.second;
        if (!rp.acked && (currentTime - rp.sendTime > 1.0)) {
            rp.sendTime = currentTime;
            rp.retryCount++;
        }
    }
}

void ReliableChannel::processReceivedPacket(uint32_t sequenceNumber, const std::vector<uint8_t>& data) {
    receivedPackets[sequenceNumber] = data;
    
    // Deliver packets in order
    while (receivedPackets.find(expectedSequenceNumber) != receivedPackets.end()) {
        const auto& packetData = receivedPackets[expectedSequenceNumber];
        
        if (onDataReceived) {
            onDataReceived(packetData.data(), packetData.size());
        }
        
        receivedPackets.erase(expectedSequenceNumber);
        expectedSequenceNumber++;
    }
}

// NetworkSerializer implementation
NetworkSerializer::NetworkSerializer()
    : writePos(0), readPos(0) {
    buffer.reserve(1024);
}

NetworkSerializer::~NetworkSerializer() {}

void NetworkSerializer::writeBool(bool value) {
    writeUInt8(value ? 1 : 0);
}

void NetworkSerializer::writeInt8(int8_t value) {
    ensureCapacity(1);
    buffer[writePos++] = static_cast<uint8_t>(value);
}

void NetworkSerializer::writeInt16(int16_t value) {
    writeUInt16(static_cast<uint16_t>(value));
}

void NetworkSerializer::writeInt32(int32_t value) {
    writeUInt32(static_cast<uint32_t>(value));
}

void NetworkSerializer::writeInt64(int64_t value) {
    writeUInt64(static_cast<uint64_t>(value));
}

void NetworkSerializer::writeUInt8(uint8_t value) {
    ensureCapacity(1);
    buffer[writePos++] = value;
}

void NetworkSerializer::writeUInt16(uint16_t value) {
    ensureCapacity(2);
    buffer[writePos++] = (value >> 8) & 0xFF;
    buffer[writePos++] = value & 0xFF;
}

void NetworkSerializer::writeUInt32(uint32_t value) {
    ensureCapacity(4);
    buffer[writePos++] = (value >> 24) & 0xFF;
    buffer[writePos++] = (value >> 16) & 0xFF;
    buffer[writePos++] = (value >> 8) & 0xFF;
    buffer[writePos++] = value & 0xFF;
}

void NetworkSerializer::writeUInt64(uint64_t value) {
    ensureCapacity(8);
    for (int i = 7; i >= 0; --i) {
        buffer[writePos++] = (value >> (i * 8)) & 0xFF;
    }
}

void NetworkSerializer::writeFloat(float value) {
    uint32_t intValue;
    std::memcpy(&intValue, &value, sizeof(float));
    writeUInt32(intValue);
}

void NetworkSerializer::writeDouble(double value) {
    uint64_t intValue;
    std::memcpy(&intValue, &value, sizeof(double));
    writeUInt64(intValue);
}

void NetworkSerializer::writeString(const std::string& value) {
    writeUInt32(static_cast<uint32_t>(value.length()));
    writeBytes(reinterpret_cast<const uint8_t*>(value.data()), value.length());
}

void NetworkSerializer::writeBytes(const uint8_t* data, size_t size) {
    ensureCapacity(size);
    std::memcpy(&buffer[writePos], data, size);
    writePos += size;
}

bool NetworkSerializer::readBool() {
    return readUInt8() != 0;
}

int8_t NetworkSerializer::readInt8() {
    return static_cast<int8_t>(readUInt8());
}

int16_t NetworkSerializer::readInt16() {
    return static_cast<int16_t>(readUInt16());
}

int32_t NetworkSerializer::readInt32() {
    return static_cast<int32_t>(readUInt32());
}

int64_t NetworkSerializer::readInt64() {
    return static_cast<int64_t>(readUInt64());
}

uint8_t NetworkSerializer::readUInt8() {
    if (readPos >= buffer.size()) return 0;
    return buffer[readPos++];
}

uint16_t NetworkSerializer::readUInt16() {
    if (readPos + 2 > buffer.size()) return 0;
    uint16_t value = (static_cast<uint16_t>(buffer[readPos]) << 8) |
                     static_cast<uint16_t>(buffer[readPos + 1]);
    readPos += 2;
    return value;
}

uint32_t NetworkSerializer::readUInt32() {
    if (readPos + 4 > buffer.size()) return 0;
    uint32_t value = (static_cast<uint32_t>(buffer[readPos]) << 24) |
                     (static_cast<uint32_t>(buffer[readPos + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[readPos + 2]) << 8) |
                     static_cast<uint32_t>(buffer[readPos + 3]);
    readPos += 4;
    return value;
}

uint64_t NetworkSerializer::readUInt64() {
    if (readPos + 8 > buffer.size()) return 0;
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<uint64_t>(buffer[readPos++]);
    }
    return value;
}

float NetworkSerializer::readFloat() {
    uint32_t intValue = readUInt32();
    float value;
    std::memcpy(&value, &intValue, sizeof(float));
    return value;
}

double NetworkSerializer::readDouble() {
    uint64_t intValue = readUInt64();
    double value;
    std::memcpy(&value, &intValue, sizeof(double));
    return value;
}

std::string NetworkSerializer::readString() {
    uint32_t length = readUInt32();
    if (readPos + length > buffer.size()) return "";
    
    std::string value(reinterpret_cast<const char*>(&buffer[readPos]), length);
    readPos += length;
    return value;
}

void NetworkSerializer::readBytes(uint8_t* outBuffer, size_t size) {
    if (readPos + size > buffer.size()) return;
    std::memcpy(outBuffer, &buffer[readPos], size);
    readPos += size;
}

void NetworkSerializer::reset() {
    buffer.clear();
    writePos = 0;
    readPos = 0;
}

void NetworkSerializer::setData(const uint8_t* data, size_t size) {
    buffer.assign(data, data + size);
    writePos = size;
    readPos = 0;
}

void NetworkSerializer::ensureCapacity(size_t additional) {
    if (writePos + additional > buffer.size()) {
        buffer.resize(writePos + additional);
    }
}

} // namespace Network
} // namespace JJM
