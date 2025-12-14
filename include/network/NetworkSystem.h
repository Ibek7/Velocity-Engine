#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <unordered_map>

namespace JJM {
namespace Network {

enum class PacketType : uint8_t {
    Connect,
    Disconnect,
    Ping,
    Pong,
    Data,
    Reliable,
    Unreliable
};

struct Packet {
    PacketType type;
    uint32_t sequenceNumber;
    uint32_t ackNumber;
    uint32_t ackBitfield;
    std::vector<uint8_t> data;
    
    Packet() : type(PacketType::Data), sequenceNumber(0), 
               ackNumber(0), ackBitfield(0) {}
};

class Connection {
public:
    Connection();
    Connection(const std::string& address, uint16_t port);
    ~Connection();
    
    void setAddress(const std::string& address) { this->address = address; }
    const std::string& getAddress() const { return address; }
    
    void setPort(uint16_t port) { this->port = port; }
    uint16_t getPort() const { return port; }
    
    void setConnected(bool connected) { this->connected = connected; }
    bool isConnected() const { return connected; }
    
    void setPing(float ping) { this->ping = ping; }
    float getPing() const { return ping; }
    
    uint32_t getNextSequenceNumber() { return nextSequenceNumber++; }
    
    void updateLastReceived() { lastReceiveTime = getCurrentTime(); }
    double getTimeSinceLastReceived() const;
    
    void addPendingAck(uint32_t sequenceNumber);
    bool shouldAck(uint32_t sequenceNumber) const;

private:
    std::string address;
    uint16_t port;
    bool connected;
    float ping;
    uint32_t nextSequenceNumber;
    double lastReceiveTime;
    std::queue<uint32_t> pendingAcks;
    
    double getCurrentTime() const;
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    
    bool connect(const std::string& address, uint16_t port);
    void disconnect();
    
    bool send(const Packet& packet);
    bool send(const uint8_t* data, size_t size, bool reliable = true);
    
    void update(double deltaTime);
    
    bool isConnected() const { return connection && connection->isConnected(); }
    
    void setOnConnected(std::function<void()> callback) { 
        onConnected = callback; 
    }
    void setOnDisconnected(std::function<void()> callback) { 
        onDisconnected = callback; 
    }
    void setOnDataReceived(std::function<void(const uint8_t*, size_t)> callback) {
        onDataReceived = callback;
    }
    
    float getPing() const;

private:
    std::unique_ptr<Connection> connection;
    std::queue<Packet> sendQueue;
    std::queue<Packet> receiveQueue;
    
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const uint8_t*, size_t)> onDataReceived;
    
    double lastPingTime;
    
    void processPacket(const Packet& packet);
    void sendPing();
    void handlePong(const Packet& packet);
};

class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();
    
    bool start(uint16_t port);
    void stop();
    
    void update(double deltaTime);
    
    void broadcast(const uint8_t* data, size_t size, bool reliable = true);
    void sendTo(uint32_t clientId, const uint8_t* data, size_t size, bool reliable = true);
    
    void kickClient(uint32_t clientId);
    
    bool isRunning() const { return running; }
    
    void setOnClientConnected(std::function<void(uint32_t)> callback) {
        onClientConnected = callback;
    }
    void setOnClientDisconnected(std::function<void(uint32_t)> callback) {
        onClientDisconnected = callback;
    }
    void setOnDataReceived(std::function<void(uint32_t, const uint8_t*, size_t)> callback) {
        onDataReceived = callback;
    }
    
    size_t getClientCount() const { return clients.size(); }

private:
    struct ClientConnection {
        uint32_t clientId;
        std::unique_ptr<Connection> connection;
        double lastActivityTime;
        
        ClientConnection() : clientId(0), lastActivityTime(0.0) {}
    };
    
    bool running;
    uint16_t port;
    std::unordered_map<uint32_t, std::unique_ptr<ClientConnection>> clients;
    uint32_t nextClientId;
    
    std::function<void(uint32_t)> onClientConnected;
    std::function<void(uint32_t)> onClientDisconnected;
    std::function<void(uint32_t, const uint8_t*, size_t)> onDataReceived;
    
    void acceptNewClients();
    void processClientPackets();
    void checkClientTimeouts();
    
    void handleClientConnect(const std::string& address, uint16_t port);
    void handleClientDisconnect(uint32_t clientId);
    void handleClientData(uint32_t clientId, const Packet& packet);
};

class ReliableChannel {
public:
    ReliableChannel();
    ~ReliableChannel();
    
    void sendReliable(const uint8_t* data, size_t size);
    void receivePacket(const Packet& packet);
    
    void update(double deltaTime);
    
    bool hasDataToSend() const { return !sendQueue.empty(); }
    Packet getNextPacket();
    
    void setOnDataReceived(std::function<void(const uint8_t*, size_t)> callback) {
        onDataReceived = callback;
    }

private:
    struct ReliablePacket {
        Packet packet;
        double sendTime;
        int retryCount;
        bool acked;
        
        ReliablePacket() : sendTime(0.0), retryCount(0), acked(false) {}
    };
    
    std::queue<std::vector<uint8_t>> sendQueue;
    std::unordered_map<uint32_t, ReliablePacket> pendingAcks;
    std::unordered_map<uint32_t, std::vector<uint8_t>> receivedPackets;
    
    uint32_t nextSequenceNumber;
    uint32_t expectedSequenceNumber;
    
    std::function<void(const uint8_t*, size_t)> onDataReceived;
    
    void resendUnacked(double currentTime);
    void processReceivedPacket(uint32_t sequenceNumber, const std::vector<uint8_t>& data);
};

class NetworkSerializer {
public:
    NetworkSerializer();
    ~NetworkSerializer();
    
    void writeBool(bool value);
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& value);
    void writeBytes(const uint8_t* data, size_t size);
    
    bool readBool();
    int8_t readInt8();
    int16_t readInt16();
    int32_t readInt32();
    int64_t readInt64();
    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();
    float readFloat();
    double readDouble();
    std::string readString();
    void readBytes(uint8_t* buffer, size_t size);
    
    const uint8_t* getData() const { return buffer.data(); }
    size_t getSize() const { return writePos; }
    
    void reset();
    void setData(const uint8_t* data, size_t size);

private:
    std::vector<uint8_t> buffer;
    size_t writePos;
    size_t readPos;
    
    void ensureCapacity(size_t additional);
};

} // namespace Network
} // namespace JJM
