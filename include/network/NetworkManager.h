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
#include <deque>
#include <chrono>
#include <unordered_map>
#include <cmath>

namespace JJM {
namespace Network {

// =============================================================================
// Network Prediction and Lag Compensation System
// =============================================================================

// Sequence number for ordering packets
using SequenceNumber = uint32_t;
using Timestamp = uint64_t;  // Milliseconds

// Network tick/frame number
using NetworkTick = uint32_t;

// Forward declarations
class NetworkPrediction;
class LagCompensation;
class ClientInterpolation;

// State snapshot for rollback
template<typename T>
struct StateSnapshot {
    NetworkTick tick;
    Timestamp timestamp;
    T state;
    
    StateSnapshot() : tick(0), timestamp(0) {}
    StateSnapshot(NetworkTick t, Timestamp ts, const T& s) 
        : tick(t), timestamp(ts), state(s) {}
};

// Input with timestamp for prediction
struct TimestampedInput {
    NetworkTick tick;
    Timestamp clientTimestamp;
    Timestamp serverTimestamp;
    SequenceNumber sequence;
    std::vector<uint8_t> inputData;
    bool acknowledged;
    
    TimestampedInput()
        : tick(0)
        , clientTimestamp(0)
        , serverTimestamp(0)
        , sequence(0)
        , acknowledged(false)
    {}
};

// Network timing information
struct NetworkTiming {
    Timestamp rtt;              // Round-trip time
    Timestamp rttVariance;      // RTT variance (jitter)
    Timestamp clockOffset;      // Server-client clock difference
    Timestamp oneWayLatency;    // Estimated one-way latency
    
    float packetLoss;           // Packet loss percentage
    float bandwidth;            // Estimated bandwidth (bytes/sec)
    
    NetworkTiming()
        : rtt(0)
        , rttVariance(0)
        , clockOffset(0)
        , oneWayLatency(0)
        , packetLoss(0)
        , bandwidth(0)
    {}
};

// =============================================================================
// Client-Side Prediction
// =============================================================================

class ClientPrediction {
public:
    struct PredictionConfig {
        size_t maxInputHistory;         // Max inputs to store
        size_t maxSnapshotHistory;      // Max snapshots for rollback
        bool enableReconciliation;      // Server reconciliation
        float reconciliationThreshold;  // Threshold for correction
        float smoothingFactor;          // Visual smoothing
        
        PredictionConfig()
            : maxInputHistory(128)
            , maxSnapshotHistory(64)
            , enableReconciliation(true)
            , reconciliationThreshold(0.01f)
            , smoothingFactor(0.2f)
        {}
    };
    
private:
    PredictionConfig config;
    
    // Input history for reconciliation
    std::deque<TimestampedInput> pendingInputs;
    SequenceNumber lastAcknowledgedInput;
    
    // State history for rollback
    std::deque<std::pair<NetworkTick, std::vector<uint8_t>>> stateHistory;
    
    // Visual smoothing
    std::vector<uint8_t> visualState;
    std::vector<uint8_t> predictedState;
    float smoothingProgress;
    
    // Callback for applying inputs
    std::function<void(const std::vector<uint8_t>&, std::vector<uint8_t>&)> applyInputCallback;
    
public:
    ClientPrediction() 
        : lastAcknowledgedInput(0)
        , smoothingProgress(0)
    {}
    
    void setConfig(const PredictionConfig& cfg) { config = cfg; }
    
    // Register input callback: (input, state) -> applies input to state
    void setApplyInputCallback(std::function<void(const std::vector<uint8_t>&, std::vector<uint8_t>&)> cb) {
        applyInputCallback = cb;
    }
    
    // Record local input
    void recordInput(const TimestampedInput& input) {
        pendingInputs.push_back(input);
        
        // Apply input to predicted state
        if (applyInputCallback && !predictedState.empty()) {
            applyInputCallback(input.inputData, predictedState);
        }
        
        // Limit history size
        while (pendingInputs.size() > config.maxInputHistory) {
            pendingInputs.pop_front();
        }
    }
    
    // Handle server acknowledgment
    void acknowledgeInput(SequenceNumber sequence) {
        lastAcknowledgedInput = sequence;
        
        // Remove acknowledged inputs
        while (!pendingInputs.empty() && 
               pendingInputs.front().sequence <= sequence) {
            pendingInputs.front().acknowledged = true;
            pendingInputs.pop_front();
        }
    }
    
    // Reconcile with server state
    void reconcile(NetworkTick serverTick, const std::vector<uint8_t>& serverState) {
        if (!config.enableReconciliation) {
            predictedState = serverState;
            return;
        }
        
        // Start from server state
        predictedState = serverState;
        
        // Re-apply unacknowledged inputs
        for (const auto& input : pendingInputs) {
            if (!input.acknowledged && applyInputCallback) {
                applyInputCallback(input.inputData, predictedState);
            }
        }
        
        // Save state for history
        stateHistory.push_back({serverTick, serverState});
        while (stateHistory.size() > config.maxSnapshotHistory) {
            stateHistory.pop_front();
        }
    }
    
    // Get state to render (with smoothing)
    const std::vector<uint8_t>& getVisualState() { return visualState; }
    const std::vector<uint8_t>& getPredictedState() { return predictedState; }
    
    // Update smoothing
    void updateSmoothing(float deltaTime) {
        if (visualState.empty()) {
            visualState = predictedState;
            return;
        }
        
        smoothingProgress += deltaTime / config.smoothingFactor;
        if (smoothingProgress >= 1.0f) {
            visualState = predictedState;
            smoothingProgress = 0;
        }
        // Actual interpolation would be done by the game-specific code
    }
    
    size_t getPendingInputCount() const { return pendingInputs.size(); }
    SequenceNumber getLastAcknowledgedInput() const { return lastAcknowledgedInput; }
};

// =============================================================================
// Server-Side Lag Compensation
// =============================================================================

class LagCompensation {
public:
    struct LagCompConfig {
        Timestamp maxCompensationMs;    // Max lag to compensate
        size_t maxHistorySnapshots;     // Max snapshots to store
        bool enableInterpolation;       // Interpolate between snapshots
        float interpolationFactor;      // Interpolation amount
        
        LagCompConfig()
            : maxCompensationMs(250)
            , maxHistorySnapshots(128)
            , enableInterpolation(true)
            , interpolationFactor(0.5f)
        {}
    };
    
private:
    LagCompConfig config;
    
    // World state history for rewinding
    std::deque<std::pair<Timestamp, std::vector<uint8_t>>> worldHistory;
    
    // Per-client timing
    std::unordered_map<int, NetworkTiming> clientTimings;
    
public:
    LagCompensation() = default;
    
    void setConfig(const LagCompConfig& cfg) { config = cfg; }
    
    // Record world state
    void recordWorldState(Timestamp timestamp, const std::vector<uint8_t>& state) {
        worldHistory.push_back({timestamp, state});
        
        while (worldHistory.size() > config.maxHistorySnapshots) {
            worldHistory.pop_front();
        }
    }
    
    // Get world state at specific time (for hit detection, etc.)
    std::vector<uint8_t> getWorldStateAt(Timestamp targetTime) const {
        if (worldHistory.empty()) {
            return {};
        }
        
        // Find surrounding snapshots
        const auto* before = &worldHistory.front();
        const auto* after = &worldHistory.front();
        
        for (size_t i = 0; i < worldHistory.size(); ++i) {
            if (worldHistory[i].first <= targetTime) {
                before = &worldHistory[i];
            }
            if (worldHistory[i].first >= targetTime && after->first < targetTime) {
                after = &worldHistory[i];
            }
        }
        
        // If interpolation disabled or exact match, return closest
        if (!config.enableInterpolation || before->first == after->first) {
            return before->second;
        }
        
        // Return the "before" state (interpolation would be game-specific)
        return before->second;
    }
    
    // Calculate compensated timestamp for a client action
    Timestamp getCompensatedTimestamp(int clientId, Timestamp clientTimestamp) const {
        auto it = clientTimings.find(clientId);
        if (it == clientTimings.end()) {
            return clientTimestamp;
        }
        
        const auto& timing = it->second;
        Timestamp serverTime = clientTimestamp + timing.clockOffset;
        Timestamp compensatedTime = serverTime - std::min(timing.oneWayLatency, config.maxCompensationMs);
        
        return compensatedTime;
    }
    
    // Update client timing
    void updateClientTiming(int clientId, const NetworkTiming& timing) {
        clientTimings[clientId] = timing;
    }
    
    // Remove client
    void removeClient(int clientId) {
        clientTimings.erase(clientId);
    }
    
    const NetworkTiming* getClientTiming(int clientId) const {
        auto it = clientTimings.find(clientId);
        return it != clientTimings.end() ? &it->second : nullptr;
    }
};

// =============================================================================
// Entity Interpolation (Client-side smoothing of server updates)
// =============================================================================

template<typename T>
class EntityInterpolation {
public:
    struct InterpolationConfig {
        Timestamp interpolationDelayMs; // Fixed delay for smoothing
        size_t maxSnapshots;            // Max snapshots to buffer
        bool enableExtrapolation;       // Predict beyond last snapshot
        Timestamp maxExtrapolationMs;   // Max extrapolation time
        
        InterpolationConfig()
            : interpolationDelayMs(100)
            , maxSnapshots(32)
            , enableExtrapolation(true)
            , maxExtrapolationMs(200)
        {}
    };
    
private:
    InterpolationConfig config;
    std::deque<StateSnapshot<T>> snapshots;
    
    // Interpolation function
    std::function<T(const T&, const T&, float)> interpolateFunc;
    
public:
    EntityInterpolation() = default;
    
    void setConfig(const InterpolationConfig& cfg) { config = cfg; }
    
    // Set custom interpolation function
    void setInterpolateFunction(std::function<T(const T&, const T&, float)> func) {
        interpolateFunc = func;
    }
    
    // Add snapshot from server
    void addSnapshot(NetworkTick tick, Timestamp timestamp, const T& state) {
        snapshots.push_back(StateSnapshot<T>(tick, timestamp, state));
        
        // Keep sorted by timestamp
        std::sort(snapshots.begin(), snapshots.end(),
            [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
        
        // Limit size
        while (snapshots.size() > config.maxSnapshots) {
            snapshots.pop_front();
        }
    }
    
    // Get interpolated state
    T getInterpolatedState(Timestamp currentTime) const {
        if (snapshots.empty()) {
            return T();
        }
        
        if (snapshots.size() == 1) {
            return snapshots.front().state;
        }
        
        // Render time is behind real time
        Timestamp renderTime = currentTime - config.interpolationDelayMs;
        
        // Find surrounding snapshots
        const StateSnapshot<T>* before = nullptr;
        const StateSnapshot<T>* after = nullptr;
        
        for (size_t i = 0; i < snapshots.size(); ++i) {
            if (snapshots[i].timestamp <= renderTime) {
                before = &snapshots[i];
            }
            if (snapshots[i].timestamp > renderTime && after == nullptr) {
                after = &snapshots[i];
            }
        }
        
        // No previous snapshot - use earliest
        if (!before) {
            return snapshots.front().state;
        }
        
        // No future snapshot - extrapolate or use latest
        if (!after) {
            if (config.enableExtrapolation && snapshots.size() >= 2) {
                Timestamp timeSinceLast = renderTime - before->timestamp;
                if (timeSinceLast <= config.maxExtrapolationMs) {
                    // Simple extrapolation using last two snapshots
                    const auto& prev = snapshots[snapshots.size() - 2];
                    const auto& last = snapshots.back();
                    
                    if (interpolateFunc && last.timestamp > prev.timestamp) {
                        Timestamp delta = last.timestamp - prev.timestamp;
                        float t = static_cast<float>(timeSinceLast) / delta;
                        return interpolateFunc(prev.state, last.state, 1.0f + t);
                    }
                }
            }
            return snapshots.back().state;
        }
        
        // Interpolate between snapshots
        if (interpolateFunc && after->timestamp > before->timestamp) {
            Timestamp delta = after->timestamp - before->timestamp;
            Timestamp elapsed = renderTime - before->timestamp;
            float t = static_cast<float>(elapsed) / delta;
            return interpolateFunc(before->state, after->state, t);
        }
        
        return before->state;
    }
    
    void clear() { snapshots.clear(); }
    size_t getSnapshotCount() const { return snapshots.size(); }
};

// =============================================================================
// Network Time Synchronization
// =============================================================================

class NetworkTimeSync {
public:
    struct SyncConfig {
        Timestamp syncIntervalMs;
        size_t sampleCount;             // Samples for averaging
        float outlierThreshold;         // RTT threshold for outlier rejection
        
        SyncConfig()
            : syncIntervalMs(1000)
            , sampleCount(10)
            , outlierThreshold(2.0f)
        {}
    };
    
private:
    SyncConfig config;
    
    struct TimeSample {
        Timestamp localSendTime;
        Timestamp serverTime;
        Timestamp localReceiveTime;
        Timestamp rtt;
    };
    
    std::deque<TimeSample> samples;
    
    Timestamp estimatedOffset;
    Timestamp estimatedRtt;
    Timestamp rttVariance;
    
    bool isSynchronized;
    
public:
    NetworkTimeSync()
        : estimatedOffset(0)
        , estimatedRtt(100)
        , rttVariance(0)
        , isSynchronized(false)
    {}
    
    void setConfig(const SyncConfig& cfg) { config = cfg; }
    
    // Create sync request timestamp
    Timestamp createSyncRequest() {
        return getCurrentLocalTime();
    }
    
    // Process sync response
    void processSyncResponse(Timestamp localSendTime, Timestamp serverTime, Timestamp localReceiveTime) {
        TimeSample sample;
        sample.localSendTime = localSendTime;
        sample.serverTime = serverTime;
        sample.localReceiveTime = localReceiveTime;
        sample.rtt = localReceiveTime - localSendTime;
        
        // Reject outliers if we have enough samples
        if (samples.size() >= 3) {
            float avgRtt = 0;
            for (const auto& s : samples) avgRtt += s.rtt;
            avgRtt /= samples.size();
            
            if (sample.rtt > avgRtt * config.outlierThreshold) {
                return;  // Reject outlier
            }
        }
        
        samples.push_back(sample);
        while (samples.size() > config.sampleCount) {
            samples.pop_front();
        }
        
        // Calculate offset using median RTT
        std::vector<Timestamp> rtts;
        for (const auto& s : samples) rtts.push_back(s.rtt);
        std::sort(rtts.begin(), rtts.end());
        
        size_t medianIdx = rtts.size() / 2;
        estimatedRtt = rtts[medianIdx];
        
        // Calculate variance
        Timestamp sum = 0;
        for (auto rtt : rtts) {
            Timestamp diff = rtt > estimatedRtt ? rtt - estimatedRtt : estimatedRtt - rtt;
            sum += diff * diff;
        }
        rttVariance = static_cast<Timestamp>(std::sqrt(sum / rtts.size()));
        
        // Calculate offset: serverTime = localTime + offset
        // offset = serverTime - (localSendTime + RTT/2)
        Timestamp halfRtt = estimatedRtt / 2;
        Timestamp localTimeAtServer = localSendTime + halfRtt;
        estimatedOffset = serverTime - localTimeAtServer;
        
        isSynchronized = samples.size() >= 3;
    }
    
    // Convert local time to server time
    Timestamp localToServerTime(Timestamp localTime) const {
        return localTime + estimatedOffset;
    }
    
    // Convert server time to local time
    Timestamp serverToLocalTime(Timestamp serverTime) const {
        return serverTime - estimatedOffset;
    }
    
    Timestamp getEstimatedRtt() const { return estimatedRtt; }
    Timestamp getRttVariance() const { return rttVariance; }
    Timestamp getOffset() const { return estimatedOffset; }
    bool isSynced() const { return isSynchronized; }
    
    NetworkTiming getNetworkTiming() const {
        NetworkTiming timing;
        timing.rtt = estimatedRtt;
        timing.rttVariance = rttVariance;
        timing.clockOffset = estimatedOffset;
        timing.oneWayLatency = estimatedRtt / 2;
        return timing;
    }
    
private:
    Timestamp getCurrentLocalTime() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

// =============================================================================
// Input Buffer for Server (stores client inputs for processing)
// =============================================================================

class ServerInputBuffer {
private:
    struct ClientInputBuffer {
        std::deque<TimestampedInput> inputs;
        SequenceNumber lastProcessedSequence;
        NetworkTick lastProcessedTick;
        
        ClientInputBuffer() : lastProcessedSequence(0), lastProcessedTick(0) {}
    };
    
    std::unordered_map<int, ClientInputBuffer> clientBuffers;
    size_t maxInputsPerClient;
    
public:
    ServerInputBuffer(size_t maxInputs = 64) : maxInputsPerClient(maxInputs) {}
    
    void addInput(int clientId, const TimestampedInput& input) {
        auto& buffer = clientBuffers[clientId];
        
        // Ignore old/duplicate inputs
        if (input.sequence <= buffer.lastProcessedSequence) {
            return;
        }
        
        // Insert in order
        auto it = buffer.inputs.begin();
        while (it != buffer.inputs.end() && it->sequence < input.sequence) {
            ++it;
        }
        buffer.inputs.insert(it, input);
        
        // Limit size
        while (buffer.inputs.size() > maxInputsPerClient) {
            buffer.inputs.pop_front();
        }
    }
    
    // Get inputs for a specific tick
    std::vector<std::pair<int, TimestampedInput>> getInputsForTick(NetworkTick tick) {
        std::vector<std::pair<int, TimestampedInput>> result;
        
        for (auto& [clientId, buffer] : clientBuffers) {
            while (!buffer.inputs.empty() && buffer.inputs.front().tick <= tick) {
                if (buffer.inputs.front().tick == tick) {
                    result.push_back({clientId, buffer.inputs.front()});
                }
                buffer.lastProcessedSequence = buffer.inputs.front().sequence;
                buffer.lastProcessedTick = buffer.inputs.front().tick;
                buffer.inputs.pop_front();
            }
        }
        
        return result;
    }
    
    // Get last processed info for acknowledgment
    SequenceNumber getLastProcessedSequence(int clientId) const {
        auto it = clientBuffers.find(clientId);
        return it != clientBuffers.end() ? it->second.lastProcessedSequence : 0;
    }
    
    void removeClient(int clientId) {
        clientBuffers.erase(clientId);
    }
    
    void clear() {
        clientBuffers.clear();
    }
};

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