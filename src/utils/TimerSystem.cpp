#include "utils/TimerSystem.h"
#include <algorithm>

namespace JJM {
namespace Utils {

// Timer implementation
Timer::Timer(float duration, Callback callback, bool repeat)
    : duration(duration), elapsed(0), repeat(repeat),
      paused(false), finished(false), callback(callback), id(0) {
}

void Timer::update(float deltaTime) {
    if (paused || finished) return;
    
    elapsed += deltaTime;
    
    if (elapsed >= duration) {
        if (callback) {
            callback();
        }
        
        if (repeat) {
            elapsed -= duration;
        } else {
            finished = true;
        }
    }
}

void Timer::pause() {
    paused = true;
}

void Timer::resume() {
    paused = false;
}

void Timer::reset() {
    elapsed = 0;
    finished = false;
    paused = false;
}

void Timer::stop() {
    finished = true;
}

// Coroutine implementation
Coroutine::Coroutine(Function func)
    : func(func), state(State::RUNNING), waitTime(0), elapsed(0), id(0) {
}

void Coroutine::update(float deltaTime) {
    if (state == State::FINISHED) return;
    
    if (state == State::WAITING) {
        elapsed += deltaTime;
        if (elapsed >= waitTime) {
            state = State::RUNNING;
            elapsed = 0;
            waitTime = 0;
        } else {
            return;
        }
    }
    
    if (state == State::RUNNING && func) {
        bool shouldContinue = func(deltaTime);
        if (!shouldContinue) {
            state = State::FINISHED;
        }
    }
}

void Coroutine::waitFor(float seconds) {
    state = State::WAITING;
    waitTime = seconds;
    elapsed = 0;
}

void Coroutine::finish() {
    state = State::FINISHED;
}

// TimerSystem implementation
TimerSystem* TimerSystem::instance = nullptr;

TimerSystem::TimerSystem() : nextTimerId(1), nextCoroutineId(1) {
}

TimerSystem* TimerSystem::getInstance() {
    if (!instance) {
        instance = new TimerSystem();
    }
    return instance;
}

TimerSystem::~TimerSystem() {
    clearAllTimers();
    clearAllCoroutines();
}

void TimerSystem::update(float deltaTime) {
    // Update timers
    for (auto it = timers.begin(); it != timers.end();) {
        (*it)->update(deltaTime);
        
        if ((*it)->isFinished()) {
            it = timers.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update coroutines
    for (auto it = coroutines.begin(); it != coroutines.end();) {
        (*it)->update(deltaTime);
        
        if ((*it)->isFinished()) {
            it = coroutines.erase(it);
        } else {
            ++it;
        }
    }
}

int TimerSystem::setTimeout(float seconds, Timer::Callback callback) {
    auto timer = std::make_shared<Timer>(seconds, callback, false);
    timer->setId(nextTimerId++);
    timers.push_back(timer);
    return timer->getId();
}

int TimerSystem::setInterval(float seconds, Timer::Callback callback) {
    auto timer = std::make_shared<Timer>(seconds, callback, true);
    timer->setId(nextTimerId++);
    timers.push_back(timer);
    return timer->getId();
}

void TimerSystem::clearTimer(int id) {
    timers.erase(
        std::remove_if(timers.begin(), timers.end(),
            [id](const std::shared_ptr<Timer>& t) {
                return t->getId() == id;
            }),
        timers.end()
    );
}

void TimerSystem::clearAllTimers() {
    timers.clear();
}

void TimerSystem::pauseTimer(int id) {
    auto it = std::find_if(timers.begin(), timers.end(),
        [id](const std::shared_ptr<Timer>& t) {
            return t->getId() == id;
        });
    
    if (it != timers.end()) {
        (*it)->pause();
    }
}

void TimerSystem::resumeTimer(int id) {
    auto it = std::find_if(timers.begin(), timers.end(),
        [id](const std::shared_ptr<Timer>& t) {
            return t->getId() == id;
        });
    
    if (it != timers.end()) {
        (*it)->resume();
    }
}

int TimerSystem::startCoroutine(Coroutine::Function func) {
    auto coroutine = std::make_shared<Coroutine>(func);
    coroutine->setId(nextCoroutineId++);
    coroutines.push_back(coroutine);
    return coroutine->getId();
}

void TimerSystem::stopCoroutine(int id) {
    coroutines.erase(
        std::remove_if(coroutines.begin(), coroutines.end(),
            [id](const std::shared_ptr<Coroutine>& c) {
                return c->getId() == id;
            }),
        coroutines.end()
    );
}

void TimerSystem::clearAllCoroutines() {
    coroutines.clear();
}

int TimerSystem::getActiveTimerCount() const {
    return static_cast<int>(timers.size());
}

int TimerSystem::getActiveCoroutineCount() const {
    return static_cast<int>(coroutines.size());
}

} // namespace Utils
} // namespace JJM
