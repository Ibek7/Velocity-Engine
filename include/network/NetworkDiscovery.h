#ifndef JJM_NETWORK_DISCOVERY_H
#define JJM_NETWORK_DISCOVERY_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace JJM {
namespace Network {

struct ServerInfo {
    std::string name;
    std::string address;
    int port;
    int playerCount;
    int maxPlayers;
    std::string gameMode;
    float ping;
    std::chrono::system_clock::time_point lastSeen;
};

class NetworkDiscovery {
public:
    static NetworkDiscovery& getInstance();
    
    void startBroadcast(int port, const ServerInfo& serverInfo);
    void stopBroadcast();
    
    void startDiscovery(int port);
    void stopDiscovery();
    
    void update();
    
    std::vector<ServerInfo> getDiscoveredServers() const;
    void clearDiscoveredServers();
    
    using ServerFoundCallback = std::function<void(const ServerInfo&)>;
    void setServerFoundCallback(ServerFoundCallback callback);

private:
    NetworkDiscovery();
    ~NetworkDiscovery();
    
    bool broadcasting;
    bool discovering;
    int broadcastPort;
    ServerInfo myServerInfo;
    std::vector<ServerInfo> discoveredServers;
    ServerFoundCallback serverFoundCallback;
};

} // namespace Network
} // namespace JJM

#endif
