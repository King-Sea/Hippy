#pragma once

#include <stdint.h>

#include <chrono>
#include <iosfwd>

#include "time_delta.h"

namespace tdf {
namespace base {

// A TimePoint represents a point in time represented as an integer number
// of nanoseconds elapsed since an arbitrary point in the past.
//
// WARNING: This class should not be serialized across reboots, or across
// devices: the reference point is only stable for a given device between
// reboots.
class TimePoint {
 public:
  // Default TimePoint with internal value 0 (epoch).
  constexpr TimePoint() = default;

  static TimePoint Now() {
    return TimePoint(std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count());
  };

  static constexpr TimePoint Min() { return TimePoint(std::numeric_limits<int64_t>::min()); }

  static constexpr TimePoint Max() { return TimePoint(std::numeric_limits<int64_t>::max()); }

  static constexpr TimePoint FromEpochDelta(TimeDelta ticks) {
    return TimePoint(ticks.ToNanoseconds());
  }

  TimeDelta ToEpochDelta() const { return TimeDelta::FromNanoseconds(ticks_); }

  // Compute the difference between two time points.
  TimeDelta operator-(TimePoint other) const {
    return TimeDelta::FromNanoseconds(ticks_ - other.ticks_);
  }

  TimePoint operator+(TimeDelta duration) const {
    return TimePoint(ticks_ + duration.ToNanoseconds());
  }
  TimePoint operator-(TimeDelta duration) const {
    return TimePoint(ticks_ - duration.ToNanoseconds());
  }

  bool operator==(TimePoint other) const { return ticks_ == other.ticks_; }
  bool operator!=(TimePoint other) const { return ticks_ != other.ticks_; }
  bool operator<(TimePoint other) const { return ticks_ < other.ticks_; }
  bool operator<=(TimePoint other) const { return ticks_ <= other.ticks_; }
  bool operator>(TimePoint other) const { return ticks_ > other.ticks_; }
  bool operator>=(TimePoint other) const { return ticks_ >= other.ticks_; }

 private:
  explicit constexpr TimePoint(int64_t ticks) : ticks_(ticks) {}

  int64_t ticks_ = 0;
};

}  // namespace base
}  // namespace tdf
