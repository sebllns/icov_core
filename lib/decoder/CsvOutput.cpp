#include "CsvOutput.h"
#include "icov_assert.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/VideoMetrics.h"
#include <chrono>
#include <numeric>

icov::decoder::CsvOutput::CsvOutput(const std::string &path_prefix)
    : omnidirectional_layer::IBlockProcessor(), m_requestFile(path_prefix + "_requests.csv"),
      m_frameFile(path_prefix + "_viewports.csv"), m_blockFile(path_prefix + "_blocks.csv"), m_requestID(0)
{
    setName("CsvOutput");

    m_requestFile << "id,timestamp,viewpoint x,viewpoint y,frame" << std::endl;
    m_blockFile << "request id,block id,center u,center v, prediction,bitrate" << std::endl;
    m_frameFile << "request id,block count,bitrate" << std::endl;
}

void icov::decoder::CsvOutput::_beforeOrderingLoop(const request_t &r)
{
    m_requestFile << ++m_requestID << ',';
    m_requestFile << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    m_requestFile << ',' << r.viewpoint.angleX << ',' << r.viewpoint.angleY << ',' << r.frameID << std::endl;
}

void icov::decoder::CsvOutput::_afterOrderingLoop(const request_t &r)
{
    // write the block count in the viewport into CSV
    m_frameFile << m_requestID << ',' << icov::video_layer::VideoMetrics::get_const_instance().metrics.size() << ','
                << icov::video_layer::VideoMetrics::get_const_instance().viewportRate() << std::endl;
}

void icov::decoder::CsvOutput::_processBlock(const request_t &r, const omnidirectional_layer::Block &b,
                                             omnidirectional_layer::IBlockPredictionManager::prediction_t prediction)
{
    m_blockFile << m_requestID << ',' << b.getIdSelf() << ',' << b.center_u() << ',' << b.center_v() << ','
                << omnidirectional_layer::BlockPrediction::toString(
                       omnidirectional_layer::BlockPrediction::toPredType(prediction))
                << ',' << icov::video_layer::VideoMetrics::get_const_instance().getBlock(b).encodedBitlength << ','
                << icov::video_layer::VideoMetrics::get_const_instance().getBlock(b).decodedBitlength << ','
                << icov::video_layer::VideoMetrics::get_const_instance().getBlock(b).rate() << ','
                << icov::video_layer::VideoMetrics::get_const_instance().getBlock(b).mse << std::endl;
}
