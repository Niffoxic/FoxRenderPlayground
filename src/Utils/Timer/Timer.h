//
// Created by niffo on 7/22/2025.
//

#ifndef TIMER_H
#define TIMER_H

#include "Common/Core.h"
#include <chrono>
#include <mutex>
#include <atomic>

template<typename T>
class Timer
{
    static_assert(std::is_arithmetic_v<T>, "Timer<T> must be arithmetic");
public:
     Timer() = default;
    ~Timer() = default;

    void Reset()
    {
        std::scoped_lock lock(m_threadMutex);

        m_bPaused.store(false);
        m_bRunning.store(false);

        m_nElapsedTime.store(T{ 0 });
        m_nTotalTime.store(T{ 0 });
        m_nDeltaTime.store(T{ 0 });
    }

    void Tick()
    {
        std::scoped_lock lock(m_threadMutex);

        if (!m_bRunning.load() || m_bPaused.load())
            return;

        const auto now = Clock::now();

        auto delta = std::chrono::duration_cast<std::chrono::duration<T>>(now - m_tpEndTime);
        m_tpEndTime = now;
        m_nDeltaTime.store(delta.count());

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<T>>(now - m_tpStartTime);
        m_nElapsedTime.store(elapsed.count());

        m_nTotalTime.fetch_add(delta.count());
    }

    void Start()
    {
        std::scoped_lock lock(m_threadMutex);

        const auto now = Clock::now();
        m_tpStartTime = now;
        m_tpEndTime = now;

        m_bRunning.store(true);
        m_bPaused.store(false);
    }

    void Stop()
    {
        std::scoped_lock lock(m_threadMutex);

        if (m_bRunning.load())
        {
            m_tpEndTime = Clock::now();

            auto elapsed = std::chrono::duration_cast<std::chrono::duration<T>>(m_tpEndTime - m_tpStartTime);
            m_nElapsedTime.store(elapsed.count());
            m_nTotalTime.fetch_add(elapsed.count());

            m_bRunning.store(false);
        }
    }

    void Pause()
    {
        std::scoped_lock lock(m_threadMutex);

        if (m_bRunning.load() && !m_bPaused.load())
        {
            m_tpPauseTime = Clock::now();

            auto elapsed = std::chrono::duration_cast<std::chrono::duration<T>>(m_tpPauseTime - m_tpStartTime);
            m_nElapsedTime.store(elapsed.count());
            m_nTotalTime.fetch_add(elapsed.count());

            m_bPaused.store(true);
        }
    }

    void Resume()
    {
        std::scoped_lock lock(m_threadMutex);

        if (m_bRunning.load() && m_bPaused.load())
        {
            const TimePoint resumeTime = Clock::now();
            m_tpStartTime = resumeTime;
            m_bPaused.store(false);
        }
    }

    _fox_Return_safe T GetElapsedTime()
    {
        return m_nElapsedTime.load();
    }

    _fox_Return_safe T GetTotalTime()
    {
        return m_nTotalTime.load();
    }

    _fox_Return_safe float GetDeltaTime()
    {
        return m_nDeltaTime.load();
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    mutable std::mutex m_threadMutex;

    TimePoint m_tpStartTime{};
    TimePoint m_tpEndTime{};
    TimePoint m_tpPauseTime{};

    std::atomic<T> m_nElapsedTime{};
    std::atomic<T> m_nTotalTime{};
    std::atomic<T> m_nDeltaTime{};

    std::atomic<bool> m_bPaused{ false };
    std::atomic<bool> m_bRunning{ false };
};

#endif //TIMER_H
