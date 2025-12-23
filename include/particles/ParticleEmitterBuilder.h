#ifndef JJM_PARTICLE_EMITTER_BUILDER_H
#define JJM_PARTICLE_EMITTER_BUILDER_H

#include <string>
#include <functional>

namespace JJM {
namespace Particles {

class ParticleEmitter;

class ParticleEmitterBuilder {
public:
    ParticleEmitterBuilder();
    
    ParticleEmitterBuilder& setMaxParticles(int max);
    ParticleEmitterBuilder& setEmissionRate(float rate);
    ParticleEmitterBuilder& setLifetime(float min, float max);
    ParticleEmitterBuilder& setSpeed(float min, float max);
    ParticleEmitterBuilder& setSize(float min, float max);
    ParticleEmitterBuilder& setColor(float r, float g, float b, float a);
    ParticleEmitterBuilder& setGravity(float x, float y, float z);
    ParticleEmitterBuilder& setPosition(float x, float y, float z);
    ParticleEmitterBuilder& setSpread(float angle);
    ParticleEmitterBuilder& setTexture(const std::string& path);
    ParticleEmitterBuilder& setBurst(bool burst);
    ParticleEmitterBuilder& setLooping(bool loop);
    
    ParticleEmitter* build();

private:
    int maxParticles;
    float emissionRate;
    float lifetimeMin, lifetimeMax;
    float speedMin, speedMax;
    float sizeMin, sizeMax;
    float colorR, colorG, colorB, colorA;
    float gravityX, gravityY, gravityZ;
    float posX, posY, posZ;
    float spread;
    std::string texture;
    bool burst;
    bool looping;
};

} // namespace Particles
} // namespace JJM

#endif
