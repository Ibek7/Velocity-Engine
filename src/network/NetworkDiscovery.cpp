#include "network/NetworkDiscovery.h"
#include <iostream>
#include <algorithm>

namespace JJM {
namespace Network {

NetworkDiscovery& NetworkDiscovery::getInstance() {
    static NetworkDiscovery instance;
    return instance;
}

NetworkDiscovery::NetworkDiscovery() 
    : broadcasting(false), discovering(false), broadcastPort(0) {
}

NetworkDiscovery::~NetworkDiscovery() {
    stopBroadcast();
    stopDiscovery();
}

void NetworkDiscovery::startBroadcast(int port, const ServerInfo& serverInfo) {
    broadcastPort = port;
    myServerInfo = serverInfo;
    broadcasting = true;
    
    std::cout << "Started broadcasting on port " << port << std::endl;
}

void NetworkDiscovery::stopBroadcast() {
    if (broadcasting) {
        broadcasting = false;
        std::cout << "Stopped broadcasting" << std::endl;
    }
}

void NetworkDiscovery::startDiscovery(int port) {
    broadcastPort = port;
    discovering = true;
    
    std::cout << "Started discovery on port " << port << std::endl;
}

void NetworkDiscovery::stopDiscovery() {
    if (discovering) {
        discovering = false;
        std::cout << "Stopped discovery" << std::endl;
    }
}

void NetworkDiscovery::update() {
    if (discovering) {
        // Simulate discovery
    }
    
    auto now = std::chrono::system_clock::now();
    discoveredServers.erase(
        std::remove_if(discoveredServers.begin(), discoveredServers.end(),
            [&now](const ServerInfo& server) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - server.lastSeen);
                return elapsed.count() > 10;
            }),
        discoveredServers.end()
    );
}

std::vector<ServerInfo> NetworkDiscovery::getDiscoveredServers() const {
    return discoveredServers;
}

void NetworkDiscovery::clearDiscoveredServers() {
    discoveredServers.clear();
}

void NetworkDiscovery::setServerFoundCallback(ServerFoundCallback callback) {
    serverFoundCallback = callback;
}

} // namespace Network
} // namespace JJM
