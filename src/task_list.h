// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <type_traits>
#include "irq_guard.h"

namespace detail {

  template<typename ...Tasks>
  class Task_list;

  template<typename Task>
  class Task_list<Task>
  {
  private:
    [[no_unique_address]] Task _h;

  public:
    using wakeup_time_type = typename std::decay_t<Task>::wakeup_time_type;
    using update_time_type = typename std::decay_t<Task>::update_time_type;
    Task_list() = default;

    template<typename T>
    explicit constexpr Task_list(T &&t) : _h(std::forward<T>(t)) {};

    template<typename ...Args>
    void
    init(Args &&...args)
    {
      _h.init(std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void
    update(Args &&...args)
    {
      _h.update(std::forward<Args>(args)...);
    }

    bool
    might_sleep() const
    {
      return _h.might_sleep();
    }

    bool
    might_power_down() const
    {
      return _h.might_power_down();
    }

    template<typename ...Args>
    bool wakeup_pending(Args &&...args) const
    {
      return _h.wakeup_pending(std::forward<Args>(args)...);
    }

    void clear_wakeups()
    {
      _h.clear_wakeups();
    }
  };


  template<typename T1, typename T2>
    struct _ctt { using type = std::common_type_t<T1, T2>; };

  template<typename T2>
    struct _ctt<void, T2> { using type = T2; };

  template<typename T1>
    struct _ctt<T1, void> { using type = T1; };

  template<>
    struct _ctt<void, void> { using type = void; };

  template<typename Head, typename ...Tail>
  class Task_list<Head, Tail...>
  {
  private:
    using Tail_list = Task_list<Tail...>;
    [[no_unique_address]] Head _h;
    [[no_unique_address]] Tail_list _t;

    using head_wu_t = typename std::decay_t<Head>::wakeup_time_type;
    using head_up_t = typename std::decay_t<Head>::update_time_type;

    using tail_wu_t = typename Tail_list::wakeup_time_type;
    using tail_up_t = typename Tail_list::update_time_type;

    template<typename T1, typename T2>
    using ctt = typename detail::_ctt<T1, T2>::type;

  public:
    using wakeup_time_type = ctt<head_wu_t, tail_wu_t>;
    using update_time_type = ctt<head_up_t, tail_up_t>;

    Task_list() = default;

    template<typename T, typename ...Args>
    explicit constexpr Task_list(T &&t, Args &&...a) : _h(std::forward<T>(t)), _t(std::forward<Args>(a)...) {};

    template<typename ...Args>
    void init(Args &&...args)
    {
      _h.init(args...);
      _t.init(std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void
    update(Args &&...args)
    {
      _h.update(args...);
      _t.update(std::forward<Args>(args)...);
    }

    bool
    might_sleep() const
    {
      return _h.might_sleep() && _t.might_sleep();
    }

    bool
    might_power_down() const
    {
      return _h.might_power_down() && _t.might_power_down();
    }

    template<typename ...Args>
    bool wakeup_pending(Args &&...args) const
    {
      return _h.wakeup_pending(args...) || _t.wakeup_pending(std::forward<Args>(args)...);
    }

    void clear_wakeups()
    {
      _h.clear_wakeups();
      _t.clear_wakeups();
    }
  };

  template<typename S, typename = void>
    struct callable : std::false_type {};

  template<typename S>
    struct callable<S, std::void_t<decltype(std::declval<S>()())>> : std::true_type {};
}

template<typename ...T>
struct Task_list : public detail::Task_list<T...>
{
  using detail::Task_list<T...>::Task_list;

private:
  template<typename S, typename R = decltype(std::declval<S>()())>
    R _get(S &&s) { return s(); }

  template<typename S, typename R = decltype(*std::declval<S>()),
    typename = std::enable_if_t<!detail::callable<S>::value>>
    R _get(S &&s) { return *s; }

public:
  template<typename CLOCK, typename SLEEP_CTL, typename STATE>
   __attribute__((always_inline))
  void task_loop(CLOCK &&clock, SLEEP_CTL &&sleep_ctl, STATE &&state)
  {
    for (;;)
      {
        typename Task_list::update_time_type now;
        // irq guard scope
          {
            cxx::Irq_guard g;
            this->clear_wakeups();
            clock.template now<true>(now); // used locked version, we have IRQs of already
          }

        this->update(now, _get(state));

        if (!this->might_sleep())
          continue;

        // irq guard scope
          {
            cxx::Irq_guard g;
            typename Task_list::wakeup_time_type now;
            clock.template now<true>(now);
            if (this->wakeup_pending(now))
              continue;

            if (this->might_power_down())
              sleep_ctl.prepare_power_down();

            // sleep
            sleep_ctl.do_sleep();
            // switch to idle sleep mode
            sleep_ctl.prepare_idle_sleep();
          }
      }
  }
};

