#ifndef AVSYNC_H
#define AVSYNC_H

#include <chrono>
#include <ctime>
#include <math.h>
using namespace std::chrono;

class AVSync
{
public:
    AVSync() = default;

    void initClock() { setClock(NAN); }

    double getClock()
    {
        double time = getMicroseconds() / 1000000.0;
        return m_diff + time;
    }

    void setClock(double pts)
    {
        double time = getMicroseconds() / 1000000.0; // us -> s
        setClockAt(pts, time);
    }

private:
    double m_pts = 0;
    double m_diff = 0;

    void setClockAt(double pts, double time)
    {
        m_pts = pts;
        m_diff = m_pts - time;
    }

    time_t getMicroseconds()
    {
        system_clock::time_point time_point_new = system_clock::now();
        system_clock::duration   duration = time_point_new.time_since_epoch();

        time_t us = duration_cast<microseconds>(duration).count();
        return us;
    }
};

#endif // AVSYNC_H
