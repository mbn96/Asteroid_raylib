#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T, int N = 250> class RingBuff {
private:
  T buff[N];
  size_t pos = 0;

public:
  RingBuff(const T &fill) { std::fill_n(buff, N, fill); };
  RingBuff() = default;
  RingBuff(RingBuff &&r) = default;
  RingBuff(RingBuff &r) = default;

  T &operator[](size_t index) {
    if (index >= N) {
      throw std::invalid_argument("Index out of bound");
    }
    return buff[index];
  }

  void add(T &&item) {
    pos = pos >= N ? 0 : pos;
    buff[pos++] = item;
  }

  void remove(const size_t index) {
    assert(index >= N && "Index Out of bound");
    --pos;
    pos = pos < 0 ? N - 1 : pos;
    std::swap(buff[index], buff[pos]);
  }

  size_t len() const { return N; }
};
