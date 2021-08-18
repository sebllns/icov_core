#include "Profiler.h"
#include "DebugTools.h"

using namespace icov::profiling;

std::unique_ptr<Profiler> Profiler::s_pinstance(new Profiler);

void Profiler::createCounter(const std::string&  name, int size, int n_ite)
{
    Counter c;

    c.n_ite = n_ite;
    c.values = std::vector<float>(size, 0.0);
    c.currentIdx = 0;

    m_counters.insert({name, c});
}

void Profiler::addValue(const std::string& id, float value)
{
    if(!m_counters.count(id)) return;
    Counter &c = m_counters[id];
    c.values[c.currentIdx] += value/c.n_ite;
    c.currentIdx = (c.currentIdx+1)%c.values.size();
}

void Profiler::store(const std::string &id, const std::string &folder) const
{
    if(!m_counters.count(id)) return;
    std::string filename = folder + "/" + id;
    DebugTools::write_vector(filename, m_counters.at(id).values, '\n');
}

void Profiler::storeAll(const std::string &folder) const
{
    for (auto const& p : m_counters)
    {
        store(p.first, folder);
    }
}
