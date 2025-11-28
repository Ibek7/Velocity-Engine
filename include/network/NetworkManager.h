#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <memory>

namespace JJM {
namespace Network {

enum class SocketType {
    TCP,
    UDP
};

enum class PacketType : uint16_t {
    CONNECT = 1,
    DISCONNECT = 2,
    PLAYER_INPUT = 3,
    GAME_STATE = 4,
    CHAT_MESSAGE = 5,
    CUSTOM = 100
};

struct Packet {
    PacketType type;
    uint32_t size;
    std::vector<uint8_t> data;
    
    Packet() : type(PacketType::CUSTOM), size(0) {}
    Packet(PacketType t, const std::vector<uint8_t>& d) : type(t), size(d.size()), data(d) {}
    
    void serialize(std::vector<uint8_t>& buffer) const;
    bool deserialize(const std::vector<uint8_t>& buffer, size_t& offset);
};

struct ClientInfo {
    int id;
    std::string address;
    uint16_t port;
    bool connected;
    std::chrono::steady_clock::time_point lastPing;
    
    ClientInfo() : id(-1), port(0), connected(false) {}
};

class NetworkManager {
public:
    using PacketHandler = std::function<void(int clientId, const Packet&)>;
    using ClientConnectedHandler = std::function<void(int clientId, const std::string& address)>;
    using ClientDisconnectedHandler = std::function<void(int clientId)>;
    
private:
    bool isServer;
    bool isRunning;
    int socketFd;
    SocketType socketType;
    
    std::thread networkThread;
    std::mutex packetMutex;
    std::queue<std::pair<int, Packet>> incomingPackets;
    std::queue<std::pair<int, Packet>> outgoingPackets;
    
    std::map<int, ClientInfo> clients;
    std::map<PacketType, PacketHandler> packetHandlers;
    
    ClientConnectedHandler onClientConnected;
    ClientDisconnectedHandler onClientDisconnected;
    
    int nextClientId;
    std::atomic<bool> shouldStop;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool startServer(uint16_t port, SocketType type = SocketType::TCP);
    bool connectToServer(const std::string& address, uint16_t port, SocketType type = SocketType::TCP);
    void stop();
    
    void sendPacket(const Packet& packet, int clientId = -1);
    void broadcastPacket(const Packet& packet);
    
    void registerPacketHandler(PacketType type, PacketHandler handler);
    void setClientConnectedHandler(ClientConnectedHandler handler);
    void setClientDisconnectedHandler(ClientDisconnectedHandler handler);
    
    void update();
    
    bool isServerMode() const { return isServer; }
    bool isClientMode() const { return !isServer; }
    bool isConnected() const { return isRunning; }
    
    int getClientCount() const;
    std::vector<int> getConnectedClients() const;
    
private:
    void networkThreadFunc();
    void handleTCPServer();
    void handleTCPClient();
    void handleUDPServer();
    void handleUDPClient();
    
    int acceptNewClient();
    void disconnectClient(int clientId);
    
    bool createSocket();
    void closeSocket();
    
    static std::vector<uint8_t> serializeString(const std::string& str);
    static std::string deserializeString(const std::vector<uint8_t>& data, size_t& offset);
    static void writeUint32(std::vector<uint8_t>& buffer, uint32_t value);
    static uint32_t readUint32(const std::vector<uint8_t>& buffer, size_t& offset);
};

} // namespace Network
} // namespace JJM

#endif // NETWORK_MANAGER_H