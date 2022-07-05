#ifndef PROFILER_H
#define PROFILER_H

#include <iostream>
#include <vector>
#include <map>
#include <memory>
namespace icov::profiling
{

struct Counter
{
    int n_ite;
    int currentIdx;
    std::vector<float> values;
};

class Profiler
{
public:
    void createCounter(const std::string& name, int size, int n_ite);
    void addValue(const std::string& id, float value);
    void store(const std::string &id, const std::string &folder) const;
    void storeAll(const std::string &folder) const;

    ~Profiler() = default;
    static Profiler& get() {return *s_pinstance;};
private:
    Profiler() = default;
    std::map<std::string, Counter> m_counters;
    static std::unique_ptr<Profiler> s_pinstance;
};
}
#endif // PROFILER_H
