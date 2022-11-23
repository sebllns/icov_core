#include "Counter.h"

using namespace icov::profiling;

Counter::Counter(int size) : m_values(size, 0.0) {}

void Counter::set(int index, float value) { m_values[index] = value; }
void Counter::add(int index, float value) { m_values[index] += value; }

float Counter::get(int index) const { return m_values[index]; }
size_t Counter::size() const { return m_values.size(); }
