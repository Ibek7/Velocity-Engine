#include "particles/ParticleEmitterBuilder.h"
#include <iostream>

namespace JJM {
namespace Particles {

class ParticleEmitter {
public:
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

ParticleEmitterBuilder::ParticleEmitterBuilder()
    : maxParticles(1000), emissionRate(10.0f),
      lifetimeMin(1.0f), lifetimeMax(3.0f),
      speedMin(1.0f), speedMax(3.0f),
      sizeMin(0.1f), sizeMax(0.5f),
      colorR(1.0f), colorG(1.0f), colorB(1.0f), colorA(1.0f),
      gravityX(0.0f), gravityY(-9.8f), gravityZ(0.0f),
      posX(0.0f), posY(0.0f), posZ(0.0f),
      spread(45.0f), burst(false), looping(true) {
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setMaxParticles(int max) {
    maxParticles = max;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setEmissionRate(float rate) {
    emissionRate = rate;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setLifetime(float min, float max) {
    lifetimeMin = min;
    lifetimeMax = max;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setSpeed(float min, float max) {
    speedMin = min;
    speedMax = max;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setSize(float min, float max) {
    sizeMin = min;
    sizeMax = max;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setColor(float r, float g, float b, float a) {
    colorR = r;
    colorG = g;
    colorB = b;
    colorA = a;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setGravity(float x, float y, float z) {
    gravityX = x;
    gravityY = y;
    gravityZ = z;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setPosition(float x, float y, float z) {
    posX = x;
    posY = y;
    posZ = z;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setSpread(float angle) {
    spread = angle;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setTexture(const std::string& path) {
    texture = path;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setBurst(bool b) {
    burst = b;
    return *this;
}

ParticleEmitterBuilder& ParticleEmitterBuilder::setLooping(bool loop) {
    looping = loop;
    return *this;
}

ParticleEmitter* ParticleEmitterBuilder::build() {
    ParticleEmitter* emitter = new ParticleEmitter();
    emitter->maxParticles = maxParticles;
    emitter->emissionRate = emissionRate;
    emitter->lifetimeMin = lifetimeMin;
    emitter->lifetimeMax = lifetimeMax;
    emitter->speedMin = speedMin;
    emitter->speedMax = speedMax;
    emitter->sizeMin = sizeMin;
    emitter->sizeMax = sizeMax;
    emitter->colorR = colorR;
    emitter->colorG = colorG;
    emitter->colorB = colorB;
    emitter->colorA = colorA;
    emitter->gravityX = gravityX;
    emitter->gravityY = gravityY;
    emitter->gravityZ = gravityZ;
    emitter->posX = posX;
    emitter->posY = posY;
    emitter->posZ = posZ;
    emitter->spread = spread;
    emitter->texture = texture;
    emitter->burst = burst;
    emitter->looping = looping;
    
    std::cout << "Built particle emitter with " << maxParticles << " max particles" << std::endl;
    return emitter;
}

} // namespace Particles
} // namespace JJM
