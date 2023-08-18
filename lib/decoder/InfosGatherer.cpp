#include "InfosGatherer.h"
#include "icov_assert.h"
#include <numeric>

const std::shared_ptr<icov::decoder::BlockDecoder> icov::decoder::InfosGatherer::getParent()
{
    if (!m_decoderParent)
    {
        auto p = this->parent();
        assert::not_null(p.get(), "Parent cannot be null");
        m_decoderParent = std::dynamic_pointer_cast<BlockDecoder>(p);
        assert::not_null(m_decoderParent.get(), "Downcast to decoder type failed");
    }
    return m_decoderParent;
}

const std::vector<icov::decoder::InfosGatherer::block_infos_t> &icov::decoder::InfosGatherer::infos() const
{
    return m_infos;
}

icov::Rate icov::decoder::InfosGatherer::frameBitrate() const
{
    return std::accumulate(m_infos.cbegin(), m_infos.cend(), 0.,
                           [](Rate sum, const block_infos_t &elem) { return sum + elem.rate; }) /
           m_infos.size();
}

void icov::decoder::InfosGatherer::_beforeOrderingLoop(const request_t &r)
{
    m_infos.clear();
}

void icov::decoder::InfosGatherer::_afterOrderingLoop(const request_t &r)
{
}

void icov::decoder::InfosGatherer::_processBlock(
    const request_t &r, const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
    m_infos.push_back(block_infos_t{.id = b.getIdSelf(), .rate = getParent()->rate(), .prediction = prediction});
}
