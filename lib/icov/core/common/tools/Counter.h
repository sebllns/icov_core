#ifndef COUNTER_H
#define COUNTER_H

#include <iostream>
#include <vector>
namespace icov::profiling {
class Counter {
public:
  Counter(int size);

  void set(int index, float value);
  void add(int index, float value);

  float get(int index) const;
  size_t size() const;

protected:
  std::vector<float> m_values;
};
} // namespace icov::profiling
#endif // COUNTER_H
