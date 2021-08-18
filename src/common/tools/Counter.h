#ifndef COUNTER_H
#define COUNTER_H

#include <iostream>
#include <vector>
namespace icov::profiling
{
class Counter
{
public:
    Counter(int size);
    void add(int index, float value);
protected:
    std::vector<float> m_values;
};
}
#endif // COUNTER_H
