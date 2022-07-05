#include "Counter.h"

using namespace icov::profiling;

Counter::Counter(int size):
    m_values(size, 0.0)
{

}

void Counter::add(int index, float value)
{
    m_values[index] += value;
}
