#include "physics/VehiclePhysics.h"
#include <cmath>
#include <algorithm>

namespace Engine {

// Vehicle implementation
Vehicle::Vehicle(int entityId)
    : m_entityId(entityId)
    , m_mass(1000.0f)
    , m_centerOfMassX(0.0f)
    , m_centerOfMassY(-0.5f)
    , m_centerOfMassZ(0.0f)
    , m_driveType(DriveType::RearWheel)
    , m_throttle(0.0f)
    , m_brake(0.0f)
    , m_steering(0.0f)
    , m_handbrake(false)
    , m_enginePower(150000.0f)
    , m_maxRPM(7000.0f)
    , m_rpm(1000.0f)
    , m_currentGear(1)
    , m_speed(0.0f)
    , m_velocityX(0.0f)
    , m_velocityY(0.0f)
    , m_velocityZ(0.0f)
{
    // Default gear ratios
    m_gearRatios = {-3.0f, 0.0f, 3.5f, 2.5f, 1.8f, 1.2f, 0.9f};
}

Vehicle::~Vehicle() {
}

void Vehicle::setCenterOfMass(float x, float y, float z) {
    m_centerOfMassX = x;
    m_centerOfMassY = y;
    m_centerOfMassZ = z;
}

void Vehicle::addWheel(const Wheel& wheel, float offsetX, float offsetY, float offsetZ) {
    m_wheels.push_back(wheel);
    m_wheelOffsetsX.push_back(offsetX);
    m_wheelOffsetsY.push_back(offsetY);
    m_wheelOffsetsZ.push_back(offsetZ);
}

void Vehicle::setThrottle(float throttle) {
    m_throttle = std::max(0.0f, std::min(1.0f, throttle));
}

void Vehicle::setBrake(float brake) {
    m_brake = std::max(0.0f, std::min(1.0f, brake));
}

void Vehicle::setSteering(float steering) {
    m_steering = std::max(-1.0f, std::min(1.0f, steering));
}

void Vehicle::setHandbrake(bool engaged) {
    m_handbrake = engaged;
}

void Vehicle::setGearRatios(const std::vector<float>& ratios) {
    m_gearRatios = ratios;
}

void Vehicle::setCurrentGear(int gear) {
    if (gear >= 0 && gear < static_cast<int>(m_gearRatios.size())) {
        m_currentGear = gear;
    }
}

void Vehicle::update(float deltaTime) {
    updateEngine(deltaTime);
    updateSteering(deltaTime);
    updateSuspension(deltaTime);
    applyDrivingForce();
    applyBraking();
    updateWheels(deltaTime);
    calculateDownforce();
    
    // Update speed
    m_speed = std::sqrt(m_velocityX * m_velocityX + 
                       m_velocityY * m_velocityY + 
                       m_velocityZ * m_velocityZ);
}

void Vehicle::applyForce(float x, float y, float z) {
    // Apply force to vehicle (F = ma, so a = F/m)
    float accelX = x / m_mass;
    float accelY = y / m_mass;
    float accelZ = z / m_mass;
    
    m_velocityX += accelX;
    m_velocityY += accelY;
    m_velocityZ += accelZ;
}

void Vehicle::applyTorque(float x, float y, float z) {
    // TODO: Apply torque to vehicle rotation
}

bool Vehicle::isGrounded() const {
    for (const auto& wheel : m_wheels) {
        if (wheel.suspensionCompression > 0.0f) {
            return true;
        }
    }
    return false;
}

void Vehicle::updateWheels(float deltaTime) {
    for (auto& wheel : m_wheels) {
        // Update wheel rotation based on speed
        float wheelCircumference = 2.0f * 3.14159f * wheel.radius;
        float rotationSpeed = (m_speed / wheelCircumference) * 360.0f;
        wheel.rotationAngle += rotationSpeed * deltaTime;
        
        // Normalize angle
        while (wheel.rotationAngle >= 360.0f) {
            wheel.rotationAngle -= 360.0f;
        }
    }
}

void Vehicle::updateEngine(float deltaTime) {
    if (m_currentGear == 0) {
        // Neutral
        m_rpm = 1000.0f;
        return;
    }
    
    // Calculate RPM based on speed and gear ratio
    float gearRatio = m_gearRatios[m_currentGear];
    m_rpm = std::abs(m_speed * gearRatio * 60.0f);
    
    // Clamp RPM
    m_rpm = std::max(800.0f, std::min(m_maxRPM, m_rpm));
    
    // Auto shift
    if (m_rpm > m_maxRPM * 0.9f && m_currentGear < static_cast<int>(m_gearRatios.size()) - 1) {
        m_currentGear++;
    } else if (m_rpm < 1500.0f && m_currentGear > 1) {
        m_currentGear--;
    }
}

void Vehicle::updateSteering(float deltaTime) {
    float maxSteeringAngle = 35.0f;
    
    for (auto& wheel : m_wheels) {
        if (wheel.isSteering) {
            wheel.steeringAngle = m_steering * maxSteeringAngle;
        }
    }
}

void Vehicle::applyDrivingForce() {
    if (m_currentGear == 0) {
        return;
    }
    
    // Calculate engine force
    float engineForce = m_enginePower * m_throttle;
    
    if (m_driveType == DriveType::AllWheel) {
        // Distribute to all wheels
        for (const auto& wheel : m_wheels) {
            if (wheel.isPowered) {
                // Apply force forward
            }
        }
    }
}

void Vehicle::applyBraking() {
    float brakeForce = m_brake * 50000.0f;
    
    if (m_handbrake) {
        brakeForce *= 2.0f;
    }
    
    // Apply brake force to slow down
    float deceleration = brakeForce / m_mass;
    float speedReduction = deceleration * 0.016f; // Assuming 60fps
    
    if (m_speed > 0) {
        m_speed = std::max(0.0f, m_speed - speedReduction);
    }
}

void Vehicle::updateSuspension(float deltaTime) {
    for (auto& wheel : m_wheels) {
        // TODO: Raycast to ground
        // Calculate suspension compression
        // Apply spring force
        wheel.suspensionCompression = wheel.suspensionRestLength * 0.5f;
    }
}

void Vehicle::calculateDownforce() {
    // Simple downforce based on speed
    float downforce = m_speed * m_speed * 0.1f;
    // TODO: Apply downforce to vehicle
}

// VehicleSystem implementation
VehicleSystem::VehicleSystem()
    : m_gravityX(0.0f)
    , m_gravityY(-9.81f)
    , m_gravityZ(0.0f)
    , m_airDensity(1.225f)
    , m_collisionEnabled(true)
{
}

VehicleSystem& VehicleSystem::getInstance() {
    static VehicleSystem instance;
    return instance;
}

Vehicle* VehicleSystem::createVehicle(int entityId) {
    Vehicle* vehicle = new Vehicle(entityId);
    m_vehicles[entityId] = vehicle;
    return vehicle;
}

Vehicle* VehicleSystem::getVehicle(int entityId) {
    auto it = m_vehicles.find(entityId);
    if (it != m_vehicles.end()) {
        return it->second;
    }
    return nullptr;
}

void VehicleSystem::removeVehicle(int entityId) {
    auto it = m_vehicles.find(entityId);
    if (it != m_vehicles.end()) {
        delete it->second;
        m_vehicles.erase(it);
    }
}

void VehicleSystem::update(float deltaTime) {
    for (auto& pair : m_vehicles) {
        pair.second->update(deltaTime);
    }
}

void VehicleSystem::setGravity(float x, float y, float z) {
    m_gravityX = x;
    m_gravityY = y;
    m_gravityZ = z;
}

} // namespace Engine
