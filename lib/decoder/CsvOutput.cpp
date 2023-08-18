#include "CsvOutput.h"
#include "icov_assert.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include <numeric>

icov::decoder::CsvOutput::CsvOutput(const std::string &path_prefix)
    : omnidirectional_layer::IBlockProcessor(), m_requestFile(path_prefix + "_requests.csv"),
      m_frameFile(path_prefix + "_frames.csv"), m_blockFile(path_prefix + "_blocks.csv")
{
    setName("CsvOutput");
}

const std::shared_ptr<icov::decoder::InfosGatherer> icov::decoder::CsvOutput::getParent()
{
    if (!m_decoderParent)
    {
        auto p = this->parent();
        assert::not_null(p.get(), "Parent cannot be null");
        m_decoderParent = std::dynamic_pointer_cast<InfosGatherer>(p);
        assert::not_null(m_decoderParent.get(), "Downcast to parent type failed");
    }
    return m_decoderParent;
}

void icov::decoder::CsvOutput::_beforeOrderingLoop(const request_t &r)
{
}

void icov::decoder::CsvOutput::_afterOrderingLoop(const request_t &r)
{
}

void icov::decoder::CsvOutput::_processBlock(const request_t &r, const omnidirectional_layer::Block &b,
                                             omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
}
