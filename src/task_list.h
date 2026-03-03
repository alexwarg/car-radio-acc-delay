// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

template<typename ...Tasks>
class Task_list;

template<typename Task>
class Task_list<Task>
{
private:
  [[no_unique_address]] Task _h;

public:
  Task_list() = default;

  template<typename T>
  explicit constexpr Task_list(T &&t) : _h(std::forward<T>(t)) {};

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
};

template<typename Head, typename ...Tail>
class Task_list<Head, Tail...>
{
private:
  [[no_unique_address]] Head _h;
  [[no_unique_address]] Task_list<Tail...> _t;

public:
  Task_list() = default;

  template<typename T, typename ...Args>
  explicit constexpr Task_list(T &&t, Args &&...a) : _h(std::forward<T>(t)), _t(std::forward<Args>(a)...) {};

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
};

