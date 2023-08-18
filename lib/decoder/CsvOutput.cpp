#include "CsvOutput.h"
#include "icov_assert.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include <chrono>
#include <numeric>

icov::decoder::CsvOutput::CsvOutput(const std::string &path_prefix)
    : omnidirectional_layer::IBlockProcessor(), m_requestFile(path_prefix + "_requests.csv"),
      m_frameFile(path_prefix + "_frames.csv"), m_blockFile(path_prefix + "_blocks.csv"), m_requestID(0)
{
    setName("CsvOutput");

    m_requestFile << "id,timestamp,viewpoint x,viewpoint y,frame" << std::endl;
    m_blockFile << "request id,block id,center u,center v,prediction,bitrate" << std::endl;
    m_frameFile << "request id,block count,bitrate" << std::endl;
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
    m_requestFile << ++m_requestID << ',';
    m_requestFile << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    m_requestFile << ',' << r.viewpoint.angleX << ',' << r.viewpoint.angleY << ',' << r.frameID << std::endl;
}

void icov::decoder::CsvOutput::_afterOrderingLoop(const request_t &r)
{
    m_frameFile << m_requestID << ',' << getParent()->infos().size() << ',' << getParent()->frameBitrate() << std::endl;
}

void icov::decoder::CsvOutput::_processBlock(const request_t &r, const omnidirectional_layer::Block &b,
                                             omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
    m_blockFile << m_requestID << ',' << b.getIdSelf() << ',' << b.center_u() << ',' << b.center_v() << ','
                << omnidirectional_layer::BlockPrediction::toString(
                       omnidirectional_layer::BlockPrediction::toPredType(prediction))
                << ',' << getParent()->getParent()->rate() << std::endl;
}
