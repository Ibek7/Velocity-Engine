#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// Vehicle physics simulation
namespace Engine {

enum class DriveType {
    FrontWheel,
    RearWheel,
    AllWheel
};

struct Wheel {
    float radius;
    float width;
    float suspensionStiffness;
    float suspensionDamping;
    float suspensionRestLength;
    float frictionCoefficient;
    float rollResistance;
    bool isSteering;
    bool isPowered;
    float steeringAngle;
    float rotationAngle;
    float suspensionCompression;
};

class Vehicle {
public:
    Vehicle(int entityId);
    ~Vehicle();

    // Setup
    void setMass(float mass) { m_mass = mass; }
    void setCenterOfMass(float x, float y, float z);
    void setDriveType(DriveType type) { m_driveType = type; }
    void addWheel(const Wheel& wheel, float offsetX, float offsetY, float offsetZ);
    
    // Control inputs
    void setThrottle(float throttle); // 0-1
    void setBrake(float brake); // 0-1
    void setSteering(float steering); // -1 to 1
    void setHandbrake(bool engaged);
    
    // Engine parameters
    void setEnginePower(float power) { m_enginePower = power; }
    void setMaxRPM(float rpm) { m_maxRPM = rpm; }
    void setGearRatios(const std::vector<float>& ratios);
    void setCurrentGear(int gear);
    
    // Physics
    void update(float deltaTime);
    void applyForce(float x, float y, float z);
    void applyTorque(float x, float y, float z);
    
    // Query
    float getSpeed() const { return m_speed; }
    float getRPM() const { return m_rpm; }
    int getCurrentGear() const { return m_currentGear; }
    bool isGrounded() const;
    const std::vector<Wheel>& getWheels() const { return m_wheels; }

private:
    void updateWheels(float deltaTime);
    void updateEngine(float deltaTime);
    void updateSteering(float deltaTime);
    void applyDrivingForce();
    void applyBraking();
    void updateSuspension(float deltaTime);
    void calculateDownforce();

    int m_entityId;
    float m_mass;
    float m_centerOfMassX;
    float m_centerOfMassY;
    float m_centerOfMassZ;
    DriveType m_driveType;
    
    std::vector<Wheel> m_wheels;
    std::vector<float> m_wheelOffsetsX;
    std::vector<float> m_wheelOffsetsY;
    std::vector<float> m_wheelOffsetsZ;
    
    float m_throttle;
    float m_brake;
    float m_steering;
    bool m_handbrake;
    
    float m_enginePower;
    float m_maxRPM;
    float m_rpm;
    std::vector<float> m_gearRatios;
    int m_currentGear;
    
    float m_speed;
    float m_velocityX;
    float m_velocityY;
    float m_velocityZ;
};

class VehicleSystem {
public:
    static VehicleSystem& getInstance();

    // Vehicle management
    Vehicle* createVehicle(int entityId);
    Vehicle* getVehicle(int entityId);
    void removeVehicle(int entityId);
    
    // Update
    void update(float deltaTime);
    
    // Global physics settings
    void setGravity(float x, float y, float z);
    void setAirDensity(float density) { m_airDensity = density; }
    void enableCollisionDetection(bool enable) { m_collisionEnabled = enable; }

private:
    VehicleSystem();
    VehicleSystem(const VehicleSystem&) = delete;
    VehicleSystem& operator=(const VehicleSystem&) = delete;

    std::map<int, Vehicle*> m_vehicles;
    float m_gravityX;
    float m_gravityY;
    float m_gravityZ;
    float m_airDensity;
    bool m_collisionEnabled;
};

} // namespace Engine
