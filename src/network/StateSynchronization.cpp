#include "../../include/network/StateSynchronization.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace JJM {
namespace Network {

// ============================================================================
// SyncVar Template Specializations
// ============================================================================

template<>
void SyncVar<float>::serialize(std::vector<uint8_t>& buffer) const {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&m_value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(float));
}

template<>
void SyncVar<float>::deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + sizeof(float) <= buffer.size()) {
        std::memcpy(&m_value, &buffer[offset], sizeof(float));
        offset += sizeof(float);
    }
}

template<>
void SyncVar<int>::serialize(std::vector<uint8_t>& buffer) const {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&m_value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(int));
}

template<>
void SyncVar<int>::deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + sizeof(int) <= buffer.size()) {
        std::memcpy(&m_value, &buffer[offset], sizeof(int));
        offset += sizeof(int);
    }
}

// ============================================================================
// NetworkTransform Implementation
// ============================================================================

NetworkTransform::NetworkTransform() {
    position[0] = position[1] = position[2] = 0;
    rotation[0] = rotation[1] = rotation[2] = 0;
    rotation[3] = 1;  // w component
    scale[0] = scale[1] = scale[2] = 1;
}

void NetworkTransform::serialize(std::vector<uint8_t>& buffer) const {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(this);
    buffer.insert(buffer.end(), data, data + sizeof(NetworkTransform));
}

void NetworkTransform::deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + sizeof(NetworkTransform) <= buffer.size()) {
        std::memcpy(this, &buffer[offset], sizeof(NetworkTransform));
        offset += sizeof(NetworkTransform));
    }
}

NetworkTransform NetworkTransform::interpolate(const NetworkTransform& a,
                                              const NetworkTransform& b, float t) {
    NetworkTransform result;
    
    // Linear interpolation for position
    for (int i = 0; i < 3; i++) {
        result.position[i] = a.position[i] + (b.position[i] - a.position[i]) * t;
        result.scale[i] = a.scale[i] + (b.scale[i] - a.scale[i]) * t;
    }
    
    // Slerp for rotation (quaternion)
    float dot = 0;
    for (int i = 0; i < 4; i++) {
        dot += a.rotation[i] * b.rotation[i];
    }
    
    if (dot < 0) {
        dot = -dot;
        for (int i = 0; i < 4; i++) {
            result.rotation[i] = a.rotation[i] + (-b.rotation[i] - a.rotation[i]) * t;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            result.rotation[i] = a.rotation[i] + (b.rotation[i] - a.rotation[i]) * t;
        }
    }
    
    // Normalize quaternion
    float len = 0;
    for (int i = 0; i < 4; i++) {
        len += result.rotation[i] * result.rotation[i];
    }
    len = std::sqrt(len);
    if (len > 0) {
        for (int i = 0; i < 4; i++) {
            result.rotation[i] /= len;
        }
    }
    
    return result;
}

// ============================================================================
// SnapshotBuffer Implementation
// ============================================================================

SnapshotBuffer::SnapshotBuffer(size_t maxSnapshots) 
    : m_maxSnapshots(maxSnapshots), m_nextSnapshotId(0) {
}

void SnapshotBuffer::addSnapshot(const StateSnapshot& snapshot) {
    m_snapshots.push_back(snapshot);
    
    // Keep only recent snapshots
    if (m_snapshots.size() > m_maxSnapshots) {
        m_snapshots.erase(m_snapshots.begin());
    }
}

StateSnapshot SnapshotBuffer::getSnapshot(uint32_t snapshotId) const {
    for (const auto& snapshot : m_snapshots) {
        if (snapshot.snapshotId == snapshotId) {
            return snapshot;
        }
    }
    return StateSnapshot();
}

StateSnapshot SnapshotBuffer::getSnapshotAt(double timestamp) const {
    if (m_snapshots.empty()) return StateSnapshot();
    
    // Find closest snapshot
    const StateSnapshot* closest = &m_snapshots[0];
    double minDiff = std::abs(closest->timestamp - timestamp);
    
    for (const auto& snapshot : m_snapshots) {
        double diff = std::abs(snapshot.timestamp - timestamp);
        if (diff < minDiff) {
            minDiff = diff;
            closest = &snapshot;
        }
    }
    
    return *closest;
}

bool SnapshotBuffer::getSnapshotsForInterpolation(double targetTime,
                                                  StateSnapshot& older,
                                                  StateSnapshot& newer,
                                                  float& t) const {
    if (m_snapshots.size() < 2) return false;
    
    // Find two snapshots that bracket the target time
    for (size_t i = 0; i < m_snapshots.size() - 1; i++) {
        if (m_snapshots[i].timestamp <= targetTime && 
            m_snapshots[i + 1].timestamp >= targetTime) {
            older = m_snapshots[i];
            newer = m_snapshots[i + 1];
            
            double duration = newer.timestamp - older.timestamp;
            if (duration > 0) {
                t = static_cast<float>((targetTime - older.timestamp) / duration);
            } else {
                t = 0;
            }
            return true;
        }
    }
    
    return false;
}

void SnapshotBuffer::clear() {
    m_snapshots.clear();
}

// ============================================================================
// NetworkedObject Implementation
// ============================================================================

NetworkedObject::NetworkedObject(uint32_t networkId)
    : m_networkId(networkId)
    , m_authority(Authority::SERVER)
    , m_syncTransform(true)
    , m_updateRate(10.0f)
    , m_lastUpdateTime(0) {
}

NetworkedObject::~NetworkedObject() {
}

void NetworkedObject::registerSyncVar(const std::string& name, SyncVarBase* syncVar) {
    m_syncVars[name] = syncVar;
}

void NetworkedObject::unregisterSyncVar(const std::string& name) {
    m_syncVars.erase(name);
}

SyncVarBase* NetworkedObject::getSyncVar(const std::string& name) {
    auto it = m_syncVars.find(name);
    return (it != m_syncVars.end()) ? it->second : nullptr;
}

void NetworkedObject::serialize(std::vector<uint8_t>& buffer) const {
    // Serialize network ID
    const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&m_networkId);
    buffer.insert(buffer.end(), idBytes, idBytes + sizeof(uint32_t));
    
    // Serialize transform if enabled
    if (m_syncTransform) {
        m_transform.serialize(buffer);
    }
    
    // Serialize sync vars
    for (const auto& pair : m_syncVars) {
        pair.second->serialize(buffer);
    }
}

void NetworkedObject::deserialize(const std::vector<uint8_t>& buffer) {
    size_t offset = 0;
    
    // Deserialize network ID
    if (offset + sizeof(uint32_t) <= buffer.size()) {
        std::memcpy(&m_networkId, &buffer[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    
    // Deserialize transform
    if (m_syncTransform) {
        m_transform.deserialize(buffer, offset);
    }
    
    // Deserialize sync vars
    for (auto& pair : m_syncVars) {
        pair.second->deserialize(buffer, offset);
    }
}

bool NetworkedObject::hasDirtyState() const {
    for (const auto& pair : m_syncVars) {
        if (pair.second->isDirty()) {
            return true;
        }
    }
    return false;
}

void NetworkedObject::clearDirtyState() {
    for (auto& pair : m_syncVars) {
        pair.second->clearDirty();
    }
}

// ============================================================================
// RPCCall Implementation
// ============================================================================

void RPCCall::serialize(std::vector<uint8_t>& buffer) const {
    // Serialize object ID
    const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&objectId);
    buffer.insert(buffer.end(), idBytes, idBytes + sizeof(uint32_t));
    
    // Serialize function name length and string
    uint32_t nameLen = functionName.length();
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&nameLen);
    buffer.insert(buffer.end(), lenBytes, lenBytes + sizeof(uint32_t));
    buffer.insert(buffer.end(), functionName.begin(), functionName.end());
    
    // Serialize parameters
    uint32_t paramSize = parameters.size();
    const uint8_t* sizeBytes = reinterpret_cast<const uint8_t*>(&paramSize);
    buffer.insert(buffer.end(), sizeBytes, sizeBytes + sizeof(uint32_t));
    buffer.insert(buffer.end(), parameters.begin(), parameters.end());
}

void RPCCall::deserialize(const std::vector<uint8_t>& buffer) {
    size_t offset = 0;
    
    // Deserialize object ID
    if (offset + sizeof(uint32_t) <= buffer.size()) {
        std::memcpy(&objectId, &buffer[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    
    // Deserialize function name
    uint32_t nameLen = 0;
    if (offset + sizeof(uint32_t) <= buffer.size()) {
        std::memcpy(&nameLen, &buffer[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    if (offset + nameLen <= buffer.size()) {
        functionName = std::string(buffer.begin() + offset, buffer.begin() + offset + nameLen);
        offset += nameLen;
    }
    
    // Deserialize parameters
    uint32_t paramSize = 0;
    if (offset + sizeof(uint32_t) <= buffer.size()) {
        std::memcpy(&paramSize, &buffer[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    if (offset + paramSize <= buffer.size()) {
        parameters = std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + paramSize);
    }
}

// ============================================================================
// StateSynchronizer Implementation
// ============================================================================

StateSynchronizer::StateSynchronizer()
    : m_isServer(false)
    , m_nextSnapshotId(0)
    , m_lastSnapshotTime(0)
    , m_snapshotRate(20.0f)
    , m_interpolationEnabled(true)
    , m_interpolationDelay(0.1f)
    , m_interpolationMethod(InterpolationMethod::LINEAR)
    , m_interpolationTime(0)
    , m_predictionEnabled(false)
    , m_bandwidthLimit(1024 * 1024)
    , m_bandwidthUsage(0) {
    resetStats();
}

StateSynchronizer::~StateSynchronizer() {
    shutdown();
}

void StateSynchronizer::initialize(bool isServer) {
    m_isServer = isServer;
    m_snapshotBuffer.clear();
    resetStats();
}

void StateSynchronizer::shutdown() {
    m_objects.clear();
    m_snapshotBuffer.clear();
    m_rpcHandlers.clear();
    m_pendingRPCs.clear();
}

void StateSynchronizer::registerObject(NetworkedObject* object) {
    m_objects[object->getNetworkId()] = object;
    m_stats.objectCount = m_objects.size();
}

void StateSynchronizer::unregisterObject(uint32_t networkId) {
    m_objects.erase(networkId);
    m_stats.objectCount = m_objects.size();
}

NetworkedObject* StateSynchronizer::getObject(uint32_t networkId) {
    auto it = m_objects.find(networkId);
    return (it != m_objects.end()) ? it->second : nullptr;
}

void StateSynchronizer::update(float deltaTime) {
    double currentTime = getCurrentTime();
    
    if (m_isServer) {
        // Server: Generate snapshots at regular intervals
        if (currentTime - m_lastSnapshotTime >= 1.0 / m_snapshotRate) {
            generateSnapshot();
            m_lastSnapshotTime = currentTime;
        }
    } else {
        // Client: Interpolate between snapshots
        if (m_interpolationEnabled) {
            updateInterpolation();
        }
        
        // Client-side prediction
        if (m_predictionEnabled) {
            updatePrediction(deltaTime);
        }
    }
    
    // Process pending RPCs
    processPendingRPCs();
}

void StateSynchronizer::generateSnapshot() {
    StateSnapshot snapshot;
    snapshot.snapshotId = m_nextSnapshotId++;
    snapshot.timestamp = getCurrentTime();
    
    // Serialize all dirty objects
    for (auto& pair : m_objects) {
        NetworkedObject* obj = pair.second;
        if (obj->hasDirtyState()) {
            std::vector<uint8_t> state;
            obj->serialize(state);
            snapshot.objectStates[obj->getNetworkId()] = state;
            obj->clearDirtyState();
        }
    }
    
    m_snapshotBuffer.addSnapshot(snapshot);
    m_stats.snapshotsSent++;
}

StateSnapshot StateSynchronizer::getLatestSnapshot() const {
    if (m_snapshotBuffer.size() > 0) {
        // Return most recent snapshot
        return m_snapshotBuffer.getSnapshotAt(getCurrentTime());
    }
    return StateSnapshot();
}

void StateSynchronizer::receiveSnapshot(const StateSnapshot& snapshot) {
    m_snapshotBuffer.addSnapshot(snapshot);
    m_stats.snapshotsReceived++;
}

void StateSynchronizer::applySnapshot(const StateSnapshot& snapshot) {
    for (const auto& pair : snapshot.objectStates) {
        NetworkedObject* obj = getObject(pair.first);
        if (obj) {
            obj->deserialize(pair.second);
        }
    }
}

void StateSynchronizer::updateInterpolation() {
    double targetTime = getCurrentTime() - m_interpolationDelay;
    
    StateSnapshot older, newer;
    float t;
    
    if (m_snapshotBuffer.getSnapshotsForInterpolation(targetTime, older, newer, t)) {
        // Interpolate between snapshots
        for (auto& pair : m_objects) {
            NetworkedObject* obj = pair.second;
            uint32_t netId = obj->getNetworkId();
            
            if (older.objectStates.count(netId) && newer.objectStates.count(netId)) {
                // Would deserialize and interpolate here
                // For transform, use NetworkTransform::interpolate()
            }
        }
    }
}

void StateSynchronizer::updatePrediction(float deltaTime) {
    // Apply local input/physics to predict state
    // Store predicted state for reconciliation
}

void StateSynchronizer::reconcileState(uint32_t objectId, uint32_t acknowledgedSnapshot) {
    // Server acknowledged a snapshot, reconcile predicted vs actual
}

void StateSynchronizer::registerRPC(uint32_t objectId, const std::string& name, RPCHandler handler) {
    m_rpcHandlers[objectId][name] = handler;
}

void StateSynchronizer::callRPC(uint32_t objectId, const std::string& name,
                               const std::vector<uint8_t>& params, SyncMode mode) {
    RPCCall rpc;
    rpc.objectId = objectId;
    rpc.functionName = name;
    rpc.parameters = params;
    rpc.mode = mode;
    
    m_pendingRPCs.push_back(rpc);
}

void StateSynchronizer::receiveRPC(const RPCCall& rpc) {
    auto objIt = m_rpcHandlers.find(rpc.objectId);
    if (objIt != m_rpcHandlers.end()) {
        auto funcIt = objIt->second.find(rpc.functionName);
        if (funcIt != objIt->second.end()) {
            funcIt->second(rpc.parameters);
        }
    }
}

void StateSynchronizer::processPendingRPCs() {
    // Send/process RPCs (would integrate with network layer)
    m_pendingRPCs.clear();
}

void StateSynchronizer::setPriority(uint32_t objectId, int priority) {
    m_priorities[objectId] = priority;
}

int StateSynchronizer::getPriority(uint32_t objectId) const {
    auto it = m_priorities.find(objectId);
    return (it != m_priorities.end()) ? it->second : 0;
}

std::vector<uint32_t> StateSynchronizer::getSortedObjectsByPriority() {
    std::vector<uint32_t> sorted;
    for (const auto& pair : m_objects) {
        sorted.push_back(pair.first);
    }
    
    std::sort(sorted.begin(), sorted.end(), [this](uint32_t a, uint32_t b) {
        return getPriority(a) > getPriority(b);
    });
    
    return sorted;
}

void StateSynchronizer::resetStats() {
    m_stats = Stats{};
    m_stats.objectCount = m_objects.size();
}

double StateSynchronizer::getCurrentTime() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

// ============================================================================
// DeltaCompressor Implementation
// ============================================================================

std::vector<uint8_t> DeltaCompressor::compress(const std::vector<uint8_t>& current,
                                              const std::vector<uint8_t>& baseline) {
    std::vector<uint8_t> delta;
    
    // Simple XOR delta compression
    size_t minSize = std::min(current.size(), baseline.size());
    for (size_t i = 0; i < minSize; i++) {
        delta.push_back(current[i] ^ baseline[i]);
    }
    
    // Append any extra bytes
    if (current.size() > baseline.size()) {
        delta.insert(delta.end(), current.begin() + baseline.size(), current.end());
    }
    
    return delta;
}

std::vector<uint8_t> DeltaCompressor::decompress(const std::vector<uint8_t>& delta,
                                                const std::vector<uint8_t>& baseline) {
    std::vector<uint8_t> result;
    
    size_t minSize = std::min(delta.size(), baseline.size());
    for (size_t i = 0; i < minSize; i++) {
        result.push_back(delta[i] ^ baseline[i]);
    }
    
    // Append any extra bytes
    if (delta.size() > baseline.size()) {
        result.insert(result.end(), delta.begin() + baseline.size(), delta.end());
    }
    
    return result;
}

float DeltaCompressor::calculateSimilarity(const std::vector<uint8_t>& a,
                                          const std::vector<uint8_t>& b) {
    if (a.empty() || b.empty()) return 0.0f;
    
    size_t matches = 0;
    size_t minSize = std::min(a.size(), b.size());
    
    for (size_t i = 0; i < minSize; i++) {
        if (a[i] == b[i]) matches++;
    }
    
    return static_cast<float>(matches) / std::max(a.size(), b.size());
}

// ============================================================================
// InterestManager Implementation
// ============================================================================

InterestManager::InterestManager() {
}

void InterestManager::setClientInterest(uint32_t clientId, const Region& region) {
    m_clientInterests[clientId] = region;
}

void InterestManager::removeClient(uint32_t clientId) {
    m_clientInterests.erase(clientId);
}

bool InterestManager::isRelevant(uint32_t clientId, uint32_t objectId,
                                const float position[3]) const {
    auto it = m_clientInterests.find(clientId);
    if (it == m_clientInterests.end()) return false;
    
    return isInRegion(it->second, position);
}

std::vector<uint32_t> InterestManager::getRelevantObjects(
    uint32_t clientId,
    const std::unordered_map<uint32_t, float[3]>& objectPositions) const {
    
    std::vector<uint32_t> relevant;
    auto it = m_clientInterests.find(clientId);
    if (it == m_clientInterests.end()) return relevant;
    
    for (const auto& pair : objectPositions) {
        if (isInRegion(it->second, pair.second)) {
            relevant.push_back(pair.first);
        }
    }
    
    return relevant;
}

bool InterestManager::isInRegion(const Region& region, const float position[3]) const {
    float dx = position[0] - region.center[0];
    float dy = position[1] - region.center[1];
    float dz = position[2] - region.center[2];
    float distSq = dx*dx + dy*dy + dz*dz;
    return distSq <= region.radius * region.radius;
}

// ============================================================================
// LagCompensator Implementation
// ============================================================================

LagCompensator::LagCompensator() : m_maxHistoryTime(1.0) {
}

void LagCompensator::recordState(uint32_t objectId, double timestamp,
                                const NetworkTransform& transform) {
    HistoricalState state;
    state.timestamp = timestamp;
    state.transform = transform;
    
    m_history[objectId].push_back(state);
}

NetworkTransform LagCompensator::rewind(uint32_t objectId, double timestamp) const {
    auto it = m_history.find(objectId);
    if (it == m_history.end() || it->second.empty()) {
        return NetworkTransform();
    }
    
    // Find closest historical state
    const HistoricalState* closest = &it->second[0];
    double minDiff = std::abs(closest->timestamp - timestamp);
    
    for (const auto& state : it->second) {
        double diff = std::abs(state.timestamp - timestamp);
        if (diff < minDiff) {
            minDiff = diff;
            closest = &state;
        }
    }
    
    return closest->transform;
}

NetworkTransform LagCompensator::compensate(uint32_t objectId, float clientLatency) const {
    double targetTime = getCurrentTime() - clientLatency;
    return rewind(objectId, targetTime);
}

void LagCompensator::clearOldHistory(double olderThan) {
    for (auto& pair : m_history) {
        auto& states = pair.second;
        states.erase(
            std::remove_if(states.begin(), states.end(),
                [olderThan](const HistoricalState& state) {
                    return state.timestamp < olderThan;
                }),
            states.end()
        );
    }
}

} // namespace Network
} // namespace JJM
