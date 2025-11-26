#ifndef TIMER_SYSTEM_H
#define TIMER_SYSTEM_H

#include <functional>
#include <vector>
#include <queue>
#include <memory>

namespace JJM {
namespace Utils {

class Timer {
public:
    using Callback = std::function<void()>;
    
private:
    float duration;
    float elapsed;
    bool repeat;
    bool paused;
    bool finished;
    Callback callback;
    int id;
    
public:
    Timer(float duration, Callback callback, bool repeat = false);
    
    void update(float deltaTime);
    void pause();
    void resume();
    void reset();
    void stop();
    
    bool isFinished() const { return finished; }
    bool isPaused() const { return paused; }
    float getElapsed() const { return elapsed; }
    float getRemaining() const { return duration - elapsed; }
    float getProgress() const { return elapsed / duration; }
    int getId() const { return id; }
    
    void setId(int i) { id = i; }
};

class Coroutine {
public:
    enum class State {
        RUNNING,
        WAITING,
        FINISHED
    };
    
    using Function = std::function<bool(float)>;
    
private:
    Function func;
    State state;
    float waitTime;
    float elapsed;
    int id;
    
public:
    Coroutine(Function func);
    
    void update(float deltaTime);
    void waitFor(float seconds);
    void finish();
    
    State getState() const { return state; }
    bool isFinished() const { return state == State::FINISHED; }
    int getId() const { return id; }
    
    void setId(int i) { id = i; }
};

class TimerSystem {
private:
    std::vector<std::shared_ptr<Timer>> timers;
    std::vector<std::shared_ptr<Coroutine>> coroutines;
    int nextTimerId;
    int nextCoroutineId;
    
    static TimerSystem* instance;
    TimerSystem();
    
public:
    static TimerSystem* getInstance();
    ~TimerSystem();
    
    void update(float deltaTime);
    
    int setTimeout(float seconds, Timer::Callback callback);
    int setInterval(float seconds, Timer::Callback callback);
    
    void clearTimer(int id);
    void clearAllTimers();
    
    void pauseTimer(int id);
    void resumeTimer(int id);
    
    int startCoroutine(Coroutine::Function func);
    void stopCoroutine(int id);
    void clearAllCoroutines();
    
    int getActiveTimerCount() const;
    int getActiveCoroutineCount() const;
};

} // namespace Utils
} // namespace JJM

#endif // TIMER_SYSTEM_H
