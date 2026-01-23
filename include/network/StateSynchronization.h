#ifndef STATE_SYNCHRONIZATION_H
#define STATE_SYNCHRONIZATION_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <chrono>

namespace JJM {
namespace Network {

// Forward declarations
class SyncVar;
class NetworkedObject;
class StateSynchronizer;
class SnapshotBuffer;

// Synchronization mode
enum class SyncMode {
    UNRELIABLE,          // Send without guarantee
    RELIABLE,            // Guaranteed delivery
    RELIABLE_ORDERED     // Guaranteed and in order
};

// Interpolation method
enum class InterpolationMethod {
    NONE,
    LINEAR,
    CUBIC,
    HERMITE
};

// Authority mode
enum class Authority {
    SERVER,              // Only server can modify
    CLIENT,              // Client has authority (rare)
    SHARED               // Both can modify (with conflict resolution)
};

// Sync variable base class
class SyncVarBase {
public:
    virtual ~SyncVarBase() = default;
    
    virtual void serialize(std::vector<uint8_t>& buffer) const = 0;
    virtual void deserialize(const std::vector<uint8_t>& buffer, size_t& offset) = 0;
    virtual bool isDirty() const = 0;
    virtual void clearDirty() = 0;
    virtual size_t getSize() const = 0;
    
    void setOnChanged(std::function<void()> callback) { m_onChanged = callback; }
    
protected:
    std::function<void()> m_onChanged;
    void triggerChanged() { if (m_onChanged) m_onChanged(); }
};

// Templated sync variable
template<typename T>
class SyncVar : public SyncVarBase {
public:
    SyncVar() : m_value(), m_dirty(false) {}
    explicit SyncVar(const T& value) : m_value(value), m_dirty(false) {}
    
    const T& get() const { return m_value; }
    
    void set(const T& value) {
        if (m_value != value) {
            m_value = value;
            m_dirty = true;
            triggerChanged();
        }
    }
    
    operator const T&() const { return m_value; }
    SyncVar& operator=(const T& value) { set(value); return *this; }
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    void deserialize(const std::vector<uint8_t>& buffer, size_t& offset) override;
    bool isDirty() const override { return m_dirty; }
    void clearDirty() override { m_dirty = false; }
    size_t getSize() const override { return sizeof(T); }
    
private:
    T m_value;
    bool m_dirty;
};

// Networked transform (position, rotation, scale)
struct NetworkTransform {
    float position[3];
    float rotation[4];    // Quaternion
    float scale[3];
    
    NetworkTransform();
    void serialize(std::vector<uint8_t>& buffer) const;
    void deserialize(const std::vector<uint8_t>& buffer, size_t& offset);
    
    static NetworkTransform interpolate(const NetworkTransform& a, 
                                       const NetworkTransform& b, float t);
};

// State snapshot
struct StateSnapshot {
    uint32_t snapshotId;
    double timestamp;
    std::unordered_map<uint32_t, std::vector<uint8_t>> objectStates;  // objectId -> state data
    
    StateSnapshot() : snapshotId(0), timestamp(0) {}
};

// Snapshot buffer (for interpolation)
class SnapshotBuffer {
public:
    SnapshotBuffer(size_t maxSnapshots = 32);
    
    void addSnapshot(const StateSnapshot& snapshot);
    StateSnapshot getSnapshot(uint32_t snapshotId) const;
    StateSnapshot getSnapshotAt(double timestamp) const;
    
    // Get two snapshots for interpolation
    bool getSnapshotsForInterpolation(double targetTime, 
                                     StateSnapshot& older, 
                                     StateSnapshot& newer,
                                     float& t) const;
    
    void clear();
    size_t size() const { return m_snapshots.size(); }
    
private:
    std::vector<StateSnapshot> m_snapshots;
    size_t m_maxSnapshots;
    uint32_t m_nextSnapshotId;
};

// Networked object
class NetworkedObject {
public:
    NetworkedObject(uint32_t networkId);
    virtual ~NetworkedObject();
    
    uint32_t getNetworkId() const { return m_networkId; }
    void setNetworkId(uint32_t id) { m_networkId = id; }
    
    // Authority
    void setAuthority(Authority authority) { m_authority = authority; }
    Authority getAuthority() const { return m_authority; }
    bool hasAuthority() const { return m_authority != Authority::SERVER; }
    
    // Sync variables
    void registerSyncVar(const std::string& name, SyncVarBase* syncVar);
    void unregisterSyncVar(const std::string& name);
    SyncVarBase* getSyncVar(const std::string& name);
    const std::unordered_map<std::string, SyncVarBase*>& getSyncVars() const { return m_syncVars; }
    
    // Serialization
    virtual void serialize(std::vector<uint8_t>& buffer) const;
    virtual void deserialize(const std::vector<uint8_t>& buffer);
    
    // State management
    bool hasDirtyState() const;
    void clearDirtyState();
    
    // Transform sync
    void setSyncTransform(bool sync) { m_syncTransform = sync; }
    bool getSyncTransform() const { return m_syncTransform; }
    void setTransform(const NetworkTransform& transform) { m_transform = transform; }
    const NetworkTransform& getTransform() const { return m_transform; }
    
    // Update rate
    void setUpdateRate(float rate) { m_updateRate = rate; }
    float getUpdateRate() const { return m_updateRate; }
    
protected:
    uint32_t m_networkId;
    Authority m_authority;
    std::unordered_map<std::string, SyncVarBase*> m_syncVars;
    NetworkTransform m_transform;
    bool m_syncTransform;
    float m_updateRate;  // Updates per second
    double m_lastUpdateTime;
};

// Remote procedure call (RPC)
struct RPCCall {
    uint32_t objectId;
    std::string functionName;
    std::vector<uint8_t> parameters;
    SyncMode mode;
    
    void serialize(std::vector<uint8_t>& buffer) const;
    void deserialize(const std::vector<uint8_t>& buffer);
};

// RPC handler
using RPCHandler = std::function<void(const std::vector<uint8_t>&)>;

// State synchronizer
class StateSynchronizer {
public:
    StateSynchronizer();
    ~StateSynchronizer();
    
    // Initialization
    void initialize(bool isServer);
    void shutdown();
    
    // Object management
    void registerObject(NetworkedObject* object);
    void unregisterObject(uint32_t networkId);
    NetworkedObject* getObject(uint32_t networkId);
    
    // Update
    void update(float deltaTime);
    
    // Server-side: Generate and broadcast snapshots
    void generateSnapshot();
    StateSnapshot getLatestSnapshot() const;
    
    // Client-side: Receive and apply snapshots
    void receiveSnapshot(const StateSnapshot& snapshot);
    void applySnapshot(const StateSnapshot& snapshot);
    
    // Interpolation
    void enableInterpolation(bool enable) { m_interpolationEnabled = enable; }
    bool isInterpolationEnabled() const { return m_interpolationEnabled; }
    void setInterpolationDelay(float delay) { m_interpolationDelay = delay; }
    float getInterpolationDelay() const { return m_interpolationDelay; }
    void setInterpolationMethod(InterpolationMethod method) { m_interpolationMethod = method; }
    
    // Client-side prediction
    void enablePrediction(bool enable) { m_predictionEnabled = enable; }
    bool isPredictionEnabled() const { return m_predictionEnabled; }
    
    // Server reconciliation
    void reconcileState(uint32_t objectId, uint32_t acknowledgedSnapshot);
    
    // RPCs
    void registerRPC(uint32_t objectId, const std::string& name, RPCHandler handler);
    void callRPC(uint32_t objectId, const std::string& name, 
                const std::vector<uint8_t>& params, SyncMode mode = SyncMode::RELIABLE);
    void receiveRPC(const RPCCall& rpc);
    
    // Priority system
    void setPriority(uint32_t objectId, int priority);
    int getPriority(uint32_t objectId) const;
    
    // Bandwidth management
    void setBandwidthLimit(size_t bytesPerSecond) { m_bandwidthLimit = bytesPerSecond; }
    size_t getBandwidthUsage() const { return m_bandwidthUsage; }
    
    // Statistics
    struct Stats {
        size_t snapshotsSent;
        size_t snapshotsReceived;
        size_t bytesUpstream;
        size_t bytesDownstream;
        float avgSnapshotSize;
        float avgInterpolationDelay;
        int objectCount;
    };
    const Stats& getStats() const { return m_stats; }
    void resetStats();
    
    // Configuration
    void setSnapshotRate(float rate) { m_snapshotRate = rate; }
    float getSnapshotRate() const { return m_snapshotRate; }
    
private:
    bool m_isServer;
    std::unordered_map<uint32_t, NetworkedObject*> m_objects;
    std::unordered_map<uint32_t, int> m_priorities;
    
    // Snapshots
    SnapshotBuffer m_snapshotBuffer;
    uint32_t m_nextSnapshotId;
    double m_lastSnapshotTime;
    float m_snapshotRate;
    
    // Interpolation
    bool m_interpolationEnabled;
    float m_interpolationDelay;
    InterpolationMethod m_interpolationMethod;
    double m_interpolationTime;
    
    // Prediction
    bool m_predictionEnabled;
    std::unordered_map<uint32_t, StateSnapshot> m_predictedStates;
    
    // RPCs
    std::unordered_map<uint32_t, std::unordered_map<std::string, RPCHandler>> m_rpcHandlers;
    std::vector<RPCCall> m_pendingRPCs;
    
    // Bandwidth
    size_t m_bandwidthLimit;
    size_t m_bandwidthUsage;
    
    // Stats
    Stats m_stats;
    
    // Helpers
    void updateInterpolation();
    void updatePrediction(float deltaTime);
    void processPendingRPCs();
    std::vector<uint32_t> getSortedObjectsByPriority();
    double getCurrentTime() const;
};

// Delta compression for state
class DeltaCompressor {
public:
    // Compress state against baseline
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& current,
                                        const std::vector<uint8_t>& baseline);
    
    // Decompress delta with baseline
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& delta,
                                          const std::vector<uint8_t>& baseline);
    
    // Calculate similarity (0-1, higher is more similar)
    static float calculateSimilarity(const std::vector<uint8_t>& a,
                                    const std::vector<uint8_t>& b);
};

// Interest management (relevancy)
class InterestManager {
public:
    struct Region {
        float center[3];
        float radius;
    };
    
    InterestManager();
    
    // Register clients and their areas of interest
    void setClientInterest(uint32_t clientId, const Region& region);
    void removeClient(uint32_t clientId);
    
    // Check if object is relevant to client
    bool isRelevant(uint32_t clientId, uint32_t objectId, const float position[3]) const;
    
    // Get all relevant objects for a client
    std::vector<uint32_t> getRelevantObjects(uint32_t clientId,
                                            const std::unordered_map<uint32_t, float[3]>& objectPositions) const;
    
private:
    std::unordered_map<uint32_t, Region> m_clientInterests;
    
    bool isInRegion(const Region& region, const float position[3]) const;
};

// Lag compensation
class LagCompensator {
public:
    LagCompensator();
    
    // Store historical states
    void recordState(uint32_t objectId, double timestamp, const NetworkTransform& transform);
    
    // Rewind to specific time
    NetworkTransform rewind(uint32_t objectId, double timestamp) const;
    
    // Compensate for client latency
    NetworkTransform compensate(uint32_t objectId, float clientLatency) const;
    
    // Clear old history
    void clearOldHistory(double olderThan);
    
private:
    struct HistoricalState {
        double timestamp;
        NetworkTransform transform;
    };
    
    std::unordered_map<uint32_t, std::vector<HistoricalState>> m_history;
    double m_maxHistoryTime;  // How far back to keep history
};

} // namespace Network
} // namespace JJM

#endif // STATE_SYNCHRONIZATION_H
