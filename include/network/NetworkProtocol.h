#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

namespace JJM {
namespace Network {

enum class SocketType {
    TCP,
    UDP
};

enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    Failed
};

struct Packet {
    uint32_t id;
    uint32_t size;
    std::vector<uint8_t> data;
    
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& value);
    void writeBytes(const uint8_t* bytes, size_t count);
    
    int8_t readInt8(size_t& offset) const;
    int16_t readInt16(size_t& offset) const;
    int32_t readInt32(size_t& offset) const;
    int64_t readInt64(size_t& offset) const;
    float readFloat(size_t& offset) const;
    double readDouble(size_t& offset) const;
    std::string readString(size_t& offset) const;
    void readBytes(uint8_t* buffer, size_t count, size_t& offset) const;
    
    void clear();
};

class Socket {
public:
    Socket(SocketType type);
    ~Socket();
    
    bool bind(const std::string& address, uint16_t port);
    bool listen(int backlog = 5);
    Socket* accept();
    
    bool connect(const std::string& address, uint16_t port);
    void disconnect();
    
    int send(const uint8_t* data, size_t size);
    int receive(uint8_t* buffer, size_t size);
    
    bool isValid() const { return socketHandle != -1; }
    SocketType getType() const { return type; }
    
    void setBlocking(bool blocking);
    void setReuseAddress(bool reuse);
    
private:
    int socketHandle;
    SocketType type;
    
    void close();
};

class Connection {
public:
    Connection(Socket* socket);
    ~Connection();
    
    bool send(const Packet& packet);
    bool receive(Packet& packet);
    
    void setReliable(bool reliable) { this->reliable = reliable; }
    bool isReliable() const { return reliable; }
    
    ConnectionStatus getStatus() const { return status; }
    void setStatus(ConnectionStatus status) { this->status = status; }
    
    uint32_t getPing() const { return ping; }
    void updatePing(uint32_t newPing) { ping = newPing; }

private:
    std::unique_ptr<Socket> socket;
    ConnectionStatus status;
    bool reliable;
    uint32_t ping;
    
    std::vector<Packet> sendQueue;
    std::vector<Packet> receiveQueue;
    
    bool sendRaw(const uint8_t* data, size_t size);
    bool receiveRaw(uint8_t* buffer, size_t size);
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    bool startServer(uint16_t port, SocketType type = SocketType::TCP);
    void stopServer();
    
    Connection* connect(const std::string& address, uint16_t port, SocketType type = SocketType::TCP);
    void disconnect(Connection* connection);
    
    void update();
    
    void setPacketHandler(std::function<void(Connection*, const Packet&)> handler) {
        packetHandler = handler;
    }
    
    void setConnectionHandler(std::function<void(Connection*, bool)> handler) {
        connectionHandler = handler;
    }
    
    const std::vector<Connection*>& getConnections() const { return connections; }

private:
    std::unique_ptr<Socket> serverSocket;
    std::vector<Connection*> connections;
    
    std::function<void(Connection*, const Packet&)> packetHandler;
    std::function<void(Connection*, bool)> connectionHandler;
    
    void acceptConnections();
    void processConnections();
};

} // namespace Network
} // namespace JJM
