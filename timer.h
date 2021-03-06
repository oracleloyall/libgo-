#pragma once
#include "config.h"
#include "spinlock.h"

namespace co
{

class CoTimer;
typedef std::shared_ptr<CoTimer> CoTimerPtr;

typedef std::chrono::system_clock::time_point SystemTimePoint;
typedef std::chrono::steady_clock::time_point SteadyTimePoint;

class CoTimer
{
public:
    typedef std::function<void()> fn_t;
    typedef std::multimap<SystemTimePoint, CoTimerPtr>::iterator SystemToken;
    typedef std::multimap<SteadyTimePoint, CoTimerPtr>::iterator SteadyToken;

    uint64_t GetId();
    void operator()();

private:
    explicit CoTimer(fn_t const& fn);
    bool Cancel();
    bool BlockCancel();

    enum class e_token_state
    {
        none,
        system,
        steady
    };

private:
    uint64_t id_;
    static atomic_t<uint64_t> s_id;
    fn_t fn_;
    bool active_;
    LFLock fn_lock_;
    SystemToken system_token_;
    SteadyToken steady_token_;
    e_token_state token_state_;

    friend class CoTimerMgr;
};
typedef std::shared_ptr<CoTimer> CoTimerPtr;
typedef CoTimerPtr TimerId;

// 瀹氭椂鍣ㄧ鐞�
class CoTimerMgr
{
public:
    typedef std::multimap<SystemTimePoint, CoTimerPtr> SystemDeadLines;
    typedef std::multimap<SteadyTimePoint, CoTimerPtr> SteadyDeadLines;

    CoTimerMgr();

    CoTimerPtr ExpireAt(SystemTimePoint const& time_point, CoTimer::fn_t const& fn);

#ifndef UNSUPPORT_STEADY_TIME
    CoTimerPtr ExpireAt(SteadyTimePoint const& time_point, CoTimer::fn_t const& fn);
#endif

    template <typename Duration>
    CoTimerPtr ExpireAt(Duration const& duration, CoTimer::fn_t const& fn)
    {
		return ExpireAt(CoTimerMgr::SteadyNow() + duration, fn);
    }

    bool Cancel(CoTimerPtr co_timer_ptr);
    bool BlockCancel(CoTimerPtr co_timer_ptr);

    // @returns: 涓嬩竴涓Е鍙戠殑timer鏃堕棿(鍗曚綅: milliseconds)
    long long GetExpired(std::list<CoTimerPtr> &result, uint32_t n = 1);

    std::size_t Size();

private:
	inline static SystemTimePoint SystemNow() { return SystemTimePoint::clock::now(); }
	inline static SteadyTimePoint SteadyNow() { return SteadyTimePoint::clock::now(); }

    void __Cancel(CoTimerPtr co_timer_ptr);

    long long GetNextTriggerTime();

    void SetNextTriggerTime(SystemTimePoint const& sys_tp);

#ifndef UNSUPPORT_STEADY_TIME
    void SetNextTriggerTime(SteadyTimePoint const& sdy_tp);
#endif

private:
    SystemDeadLines system_deadlines_;
    SteadyDeadLines steady_deadlines_;
    LFLock lock_;

    // 涓嬩竴涓猼imer瑙﹀彂鐨勬椂闂�
    //  鍗曚綅: milliseconds
    // 杩欎釜鍊肩敱GetExpired鏃舵垚鍔焞ock鐨勭嚎绋嬫潵璁剧疆, 鏈猯ock鎴愬姛鐨勭嚎绋嬩篃鍏佽璇诲彇.
    atomic_t<long long> system_next_trigger_time_;
    atomic_t<long long> steady_next_trigger_time_;
};

} //namespace co
