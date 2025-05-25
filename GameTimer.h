#pragma once

#include <chrono>

class GameTimer
{
public:
    GameTimer();

    float TotalTime() const; // in seconds
    float DeltaTime() const; // in seconds

    void Reset(); // Call before message loop.
    void Start(); // Call when unpaused.
    void Stop();  // Call when paused.
    void Tick();  // Call once per frame.

private:
    double m_secondsPerCount;
    double m_deltaTime;

    std::chrono::high_resolution_clock::time_point m_baseTime;
    std::chrono::high_resolution_clock::time_point m_pausedTime;
    std::chrono::high_resolution_clock::time_point m_stopTime;
    std::chrono::high_resolution_clock::time_point m_prevTime;
    std::chrono::high_resolution_clock::time_point m_currTime;

    bool m_stopped;
};
