#include "DecodeResult.h"
#include "icov_assert.h"
#include <numeric>

const std::shared_ptr<icov::decoder::BlockDecoder> icov::decoder::DecodeResult::getParent()
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

const std::vector<icov::decoder::DecodeResult::block_infos_t> &icov::decoder::DecodeResult::infos() const
{
    return m_infos;
}

icov::Rate icov::decoder::DecodeResult::frameBitrate() const
{
    return std::accumulate(m_infos.cbegin(), m_infos.cend(), 0.,
                           [](Rate sum, const block_infos_t &elem) { return sum + elem.rate; }) /
           m_infos.size();
}

void icov::decoder::DecodeResult::_beforeOrderingLoop()
{
    m_infos.clear();
}

void icov::decoder::DecodeResult::_afterOrderingLoop()
{
}

void icov::decoder::DecodeResult::_processBlock(const omnidirectional_layer::Block &b,
                                                omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
    m_infos.push_back(block_infos_t{.id = b.getIdSelf(), .rate = getParent()->rate(), .prediction = prediction});
}
