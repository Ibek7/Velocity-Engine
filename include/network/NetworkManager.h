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

/**
 * @brief Compression algorithm types
 */
enum class CompressionType {
    None,
    LZ4,        // Fast compression, moderate ratio
    ZSTD,       // Good balance of speed and ratio
    Deflate,    // Standard zlib compression
    LZO         // Ultra-fast compression
};

/**
 * @brief Packet compression statistics
 */
struct CompressionStats {
    size_t totalBytesIn;
    size_t totalBytesOut;
    size_t packetsCompressed;
    size_t packetsDecompressed;
    float avgCompressionRatio;
    float avgCompressionTimeMs;
    float avgDecompressionTimeMs;
    
    CompressionStats()
        : totalBytesIn(0)
        , totalBytesOut(0)
        , packetsCompressed(0)
        , packetsDecompressed(0)
        , avgCompressionRatio(1.0f)
        , avgCompressionTimeMs(0.0f)
        , avgDecompressionTimeMs(0.0f)
    {}
    
    void reset() {
        totalBytesIn = 0;
        totalBytesOut = 0;
        packetsCompressed = 0;
        packetsDecompressed = 0;
        avgCompressionRatio = 1.0f;
        avgCompressionTimeMs = 0.0f;
        avgDecompressionTimeMs = 0.0f;
    }
};

/**
 * @brief Packet compression configuration
 */
struct CompressionConfig {
    CompressionType type;
    int compressionLevel;       // 1-9 for most algorithms
    size_t minSizeToCompress;   // Don't compress packets smaller than this
    bool adaptiveCompression;   // Adjust based on network conditions
    
    CompressionConfig()
        : type(CompressionType::LZ4)
        , compressionLevel(3)
        , minSizeToCompress(64)
        , adaptiveCompression(true)
    {}
};

/**
 * @brief Packet compressor for network data
 */
class PacketCompressor {
public:
    PacketCompressor();
    ~PacketCompressor();
    
    // Configuration
    void setConfig(const CompressionConfig& config);
    const CompressionConfig& getConfig() const { return config; }
    void setCompressionType(CompressionType type);
    void setCompressionLevel(int level);
    void setMinSizeToCompress(size_t minSize);
    
    // Compression operations
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData);
    
    // Packet-level operations
    Packet compressPacket(const Packet& packet);
    Packet decompressPacket(const Packet& packet);
    
    // Batch operations
    std::vector<Packet> compressPackets(const std::vector<Packet>& packets);
    std::vector<Packet> decompressPackets(const std::vector<Packet>& packets);
    
    // Statistics
    const CompressionStats& getStats() const { return stats; }
    void resetStats() { stats.reset(); }
    float getCompressionRatio() const;
    
    // Utility
    static bool isCompressed(const std::vector<uint8_t>& data);
    static size_t estimateCompressedSize(size_t originalSize, CompressionType type);
    
private:
    CompressionConfig config;
    CompressionStats stats;
    
    std::vector<uint8_t> compressLZ4(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressLZ4(const std::vector<uint8_t>& data);
    std::vector<uint8_t> compressZSTD(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressZSTD(const std::vector<uint8_t>& data);
    std::vector<uint8_t> compressDeflate(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressDeflate(const std::vector<uint8_t>& data);
    
    void updateStats(size_t originalSize, size_t compressedSize, float timeMs, bool isCompression);
};

/**
 * @brief Network bandwidth limiter
 */
class BandwidthLimiter {
public:
    BandwidthLimiter();
    ~BandwidthLimiter();
    
    // Configuration
    void setMaxBytesPerSecond(size_t maxBytes);
    void setMaxPacketsPerSecond(size_t maxPackets);
    void setBurstAllowance(float burstMultiplier);
    
    // Rate limiting
    bool canSend(size_t packetSize);
    void recordSent(size_t packetSize);
    void update(float deltaTime);
    
    // Statistics
    size_t getCurrentBytesPerSecond() const { return currentBytesPerSecond; }
    size_t getCurrentPacketsPerSecond() const { return currentPacketsPerSecond; }
    float getUtilization() const;
    
private:
    size_t maxBytesPerSecond;
    size_t maxPacketsPerSecond;
    float burstMultiplier;
    
    size_t currentBytesPerSecond;
    size_t currentPacketsPerSecond;
    size_t bytesSentThisSecond;
    size_t packetsSentThisSecond;
    float timeAccumulator;
};

/**
 * @brief Network quality of service manager
 */
class NetworkQoS {
public:
    enum class Priority {
        Low,
        Normal,
        High,
        Critical
    };
    
    NetworkQoS();
    ~NetworkQoS();
    
    // Priority queuing
    void queuePacket(const Packet& packet, Priority priority);
    Packet dequeuePacket();
    bool hasPackets() const;
    
    // Packet type priorities
    void setPacketTypePriority(PacketType type, Priority priority);
    Priority getPacketTypePriority(PacketType type) const;
    
    // Statistics
    size_t getQueuedPacketCount() const;
    size_t getQueuedPacketCount(Priority priority) const;

private:
    std::map<Priority, std::queue<Packet>> priorityQueues;
    std::map<PacketType, Priority> packetPriorities;
};

} // namespace Network
} // namespace JJM

#endif // NETWORK_MANAGER_H