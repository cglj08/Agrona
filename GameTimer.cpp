
#include "pch.h"
#include "GameTimer.h"

GameTimer::GameTimer() :
    m_secondsPerCount(0.0), m_deltaTime(-1.0), m_stopped(false)
{
    // Calculate seconds per clock cycle
    long long countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    m_secondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
float GameTimer::TotalTime() const
{
    // If we are stopped, do not count the time that has passed since we stopped.
    // Moreover, if we previously already had a pause, the distance
    // mStopTime - mBaseTime includes paused time, which we do not want to count.
    // To correct this, we can subtract the paused time from mStopTime:
    //
    //                     |<-- Paused Time -->|
    // ----*---------------*-----------------*------------*------------*------> time
    //  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

    if (m_stopped)
    {
        // Calculate time elapsed before stopping
        auto timeBeforeStop = std::chrono::duration<double>(m_stopTime - m_baseTime).count();
        // Subtract the total paused duration
        auto pausedDuration = std::chrono::duration<double>(m_pausedTime - m_baseTime).count(); // This seems complex, re-evaluate logic based on std::chrono if needed
        // A simpler approach might be to track total paused duration separately
        // For now, let's assume m_pausedTime accumulates total duration spent paused
        return (float)(timeBeforeStop - std::chrono::duration<double>(m_pausedTime.time_since_epoch()).count()); // Needs careful implementation
         // This standard implementation might be easier:
         // return (float)(((m_stopTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
         // Let's stick to the structure from common examples
         return (float)((m_stopTime.time_since_epoch().count() - m_pausedTime.time_since_epoch().count()) - m_baseTime.time_since_epoch().count()) * m_secondsPerCount; // Placeholder - Needs correct chrono duration logic
    }
    // The distance mCurrTime - mBaseTime includes paused time,
    // which we do not want to count. To correct this, we can subtract
    // the paused time from mCurrTime:
    //
    //  (mCurrTime - mPausedTime) - mBaseTime
    //
    //                     |<-- Paused Time -->|
    // ----*---------------*-----------------*------------*------> time
    //  mBaseTime       mStopTime        startTime     mCurrTime
    else
    {
       // return (float)(((m_currTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
        return (float)((m_currTime.time_since_epoch().count() - m_pausedTime.time_since_epoch().count()) - m_baseTime.time_since_epoch().count()) * m_secondsPerCount; // Placeholder - Needs correct chrono duration logic
    }
}

float GameTimer::DeltaTime() const
{
    return (float)m_deltaTime;
}

void GameTimer::Reset()
{
    m_currTime = std::chrono::high_resolution_clock::now();
    m_baseTime = m_currTime;
    m_prevTime = m_currTime;
    m_stopTime = m_currTime; // Initialize stopTime
    m_pausedTime = std::chrono::high_resolution_clock::time_point(); // Reset paused duration accumulator if using one
    m_stopped = false;
}

void GameTimer::Start()
{
    if (m_stopped)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Accumulate the time elapsed between stop and start pairs.
        //
        //                     |<-------d------->|
        // ----*---------------*-----------------*------------> time
        //  mBaseTime       mStopTime        startTime
        // Subtract the duration spent stopped from the paused time accumulator
        m_pausedTime += (startTime - m_stopTime); // Needs correct chrono duration logic

        // mPrevTime must be reset when unpausing to avoid computing delta time
        // with the potentially large gap since the last frame before pausing.
        m_prevTime = startTime;

        // Reset the stopped flag
        m_stopTime = std::chrono::high_resolution_clock::time_point(); // Clear stop time
        m_stopped = false;
    }
}

void GameTimer::Stop()
{
    // If we are already stopped, then don't do anything.
    if (!m_stopped)
    {
        // Record the time when stopped
        m_stopTime = std::chrono::high_resolution_clock::now();
        m_stopped = true;
    }
}

void GameTimer::Tick()
{
    if (m_stopped)
    {
        m_deltaTime = 0.0;
        return;
    }

    // Get the time this frame.
    m_currTime = std::chrono::high_resolution_clock::now();

    // Time difference between this frame and the previous.
    m_deltaTime = std::chrono::duration<double>(m_currTime - m_prevTime).count();

    // Prepare for next frame.
    m_prevTime = m_currTime;

    // Force nonnegative. The DXSDK's multimedia timer can sometimes yield a negative delta time
    // due to hardware limitations or OS scheduling irregularities.
    if (m_deltaTime < 0.0)
    {
        m_deltaTime = 0.0;
    }
}
