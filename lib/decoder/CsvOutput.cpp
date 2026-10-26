#include "CsvOutput.h"
#include "icov_assert.h"
#include "icov_math.h"
#include "icov_types.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include "omnidirectional_layer/interface/IBlockProcessor.h"
#include "video_layer/VideoMetrics.h"
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <numeric>
#include <sstream>
#include <string>

icov::decoder::CsvOutput::CsvOutput(const std::string &path_prefix)
    : omnidirectional_layer::IBlockProcessor(), m_requestFile(path_prefix + "_requests.csv"),
      m_viewportFile(path_prefix + "_viewports.csv"), m_blockFile(path_prefix + "_blocks.csv"),
      m_infosFile(path_prefix + "_metadata.csv"), m_requestID(0), m_viewportBlockCount(0)
{
    setName("CsvOutput");

    m_requestFile.addColumn("id");          // 1
    m_requestFile.addColumn("timestamp");   // 2
    m_requestFile.addColumn("viewpoint x"); // 3
    m_requestFile.addColumn("viewpoint y"); // 4
    m_requestFile.addColumn("frame");       // 5
    m_requestFile.writeColumnNames();

    m_blockFile.addColumn("request id");                        // 1
    m_blockFile.addColumn("block id");                          // 2
    m_blockFile.addColumn("center u");                          // 3
    m_blockFile.addColumn("center v");                          // 4
    m_blockFile.addColumn("area pixels");                       // 5
    m_blockFile.addColumn("decoding order");                    // 6
    m_blockFile.addColumn("prediction");                        // 7
    m_blockFile.addColumn("encoded bitrate");                   // 8
    m_blockFile.addColumn("all intra bitrate");                 // 9
    m_blockFile.addColumn("encoded bits per decoded pixel");    // 11
    m_blockFile.addColumn("all intra bits per decoded pixel");  // 12
    m_blockFile.addColumn("bitrate ratio (encoded/all intra)"); // 10
    m_blockFile.addColumn("mse");                               // 13
    m_blockFile.addColumn("psnr");                              // 14
    m_blockFile.addColumn("transmitted bytes");                 // 15
    m_blockFile.writeColumnNames();

    m_viewportFile.addColumn("request id");                        // 1
    m_viewportFile.addColumn("block count");                       // 2
    m_viewportFile.addColumn("decoded pixel area (erp)");          // 3
    m_viewportFile.addColumn("display pixel area");                // 3
    m_viewportFile.addColumn("encoded bitrate");                   // 3
    m_viewportFile.addColumn("all intra bitrate");                 // 4
    m_viewportFile.addColumn("bitrate ratio (encoded/all intra)"); // 5
    m_viewportFile.addColumn("bits per decoded pixel");            // 6
    m_viewportFile.addColumn("bits per displayed pixel");          // 7
    m_viewportFile.addColumn("mse (erp)");                         // 8
    m_viewportFile.addColumn("mse (display)");                     // 9
    m_viewportFile.addColumn("psnr (erp)");                        // 10
    m_viewportFile.addColumn("psnr (display)");                    // 11
    m_viewportFile.writeColumnNames();

    m_infosFile.addColumn("c1");
    m_infosFile.addColumn("c2");
    m_infosFile.writeColumnNames();
}

void icov::decoder::CsvOutput::_beforeOrderingLoop(const request_t &r)
{
    m_viewportBlockCount = 0;
    m_viewportDecodedSize = 0;
    m_viewportEncodedSize = 0;
    m_erpDecodedArea = 0;
    m_ErpMse = 0;
    m_displayArea = 1920 * 1080;

    std::stringstream ss;
    ss << ++m_requestID;
    m_requestFile.setColumnValue("id", ss.str());

    ss = std::stringstream();
    ss << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    m_requestFile.setColumnValue("timestamp", ss.str());

    ss = std::stringstream();
    ss << r.viewpoint.angleX;
    m_requestFile.setColumnValue("viewpoint x", ss.str());

    ss = std::stringstream();
    ss << r.viewpoint.angleY;
    m_requestFile.setColumnValue("viewpoint y", ss.str());

    ss = std::stringstream();
    ss << r.frameID;
    m_requestFile.setColumnValue("frame", ss.str());

    m_requestFile.writeAndClearValues();
}

void icov::decoder::CsvOutput::_afterOrderingLoop(const request_t &r)
{
    m_ErpMse /= m_viewportBlockCount;

    m_viewportFile.setColumnValue("request id", m_requestID); // 1
    m_viewportFile.setColumnValue("decoded pixel area (erp)",
                                  m_erpDecodedArea);                        // 3
    m_viewportFile.setColumnValue("block count", m_viewportBlockCount);     // 2
    m_viewportFile.setColumnValue("encoded bitrate",
                                  m_viewportEncodedSize); // 3
    m_viewportFile.setColumnValue("all intra bitrate",
                                  m_viewportDecodedSize); // 4
    m_viewportFile.setColumnValue(
        "bitrate ratio (encoded/all intra)",
        static_cast<Rate>(m_viewportEncodedSize) /
            static_cast<Rate>(m_viewportDecodedSize)); // 5
    m_viewportFile.setColumnValue("bits per decoded pixel",
                                  static_cast<double>(m_viewportEncodedSize) /
                                      m_erpDecodedArea);  // 6
    m_viewportFile.setColumnValue("mse (erp)", m_ErpMse); // 8
    m_viewportFile.setColumnValue("psnr (erp)",
                                  icov::maths::psnr(m_ErpMse, 255)); // 10
}

void icov::decoder::CsvOutput::_processBlock(
    const request_t &r, const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction,
    frame_t previouslyDecoded) {
    BitLength encodedSize =
        icov::video_layer::VideoMetrics::get_const_instance()
            .getBlock(b)
            .encodedBitlength;

    BitLength decodedSize =
        icov::video_layer::VideoMetrics::get_const_instance()
            .getBlock(b)
            .decodedBitlength;

    Rate rate = icov::video_layer::VideoMetrics::get_const_instance()
                    .getBlock(b)
                    .rate();

    float mse =
        icov::video_layer::VideoMetrics::get_const_instance().getBlock(b).mse;

    float psnr = icov::video_layer::VideoMetrics::get_const_instance()
                     .getBlock(b)
                     .psnr();

    int area = b.iarea();

    m_viewportEncodedSize += encodedSize;
    m_viewportDecodedSize += decodedSize;
    m_erpDecodedArea += area;
    m_ErpMse += mse;

    m_blockFile.setColumnValue("request id", m_requestID);                // 1
    m_blockFile.setColumnValue("block id", b.getIdSelf());                // 2
    m_blockFile.setColumnValue("center u", b.center_u());                 // 3
    m_blockFile.setColumnValue("center v", b.center_v());                 // 4
    m_blockFile.setColumnValue("area pixels", area);                      // 5
    m_blockFile.setColumnValue("decoding order", ++m_viewportBlockCount); // 6
    m_blockFile.setColumnValue(
        "prediction", omnidirectional_layer::BlockPrediction::toString(
                          omnidirectional_layer::BlockPrediction::toPredType(
                              prediction)));                      // 7
    m_blockFile.setColumnValue("encoded bitrate", encodedSize);   // 8
    m_blockFile.setColumnValue("all intra bitrate", decodedSize); // 9
    m_blockFile.setColumnValue("bitrate ratio (encoded/all intra)",
                               rate); // 10
    m_blockFile.setColumnValue("encoded bits per decoded pixel",
                               static_cast<float>(encodedSize) /
                                   static_cast<float>(area)); // 11
    m_blockFile.setColumnValue("all intra bits per decoded pixel",
                               static_cast<float>(decodedSize) /
                                   static_cast<float>(area)); // 12
    m_blockFile.setColumnValue("mse", mse);                   // 13
    m_blockFile.setColumnValue("psnr", psnr);                 // 14
    m_blockFile.setColumnValue(
        "transmitted bytes",
        icov::video_layer::VideoMetrics::get_const_instance()
            .getBlock(b)
            .transmittedBytes); // 15

    m_blockFile.writeAndClearValues();
}
