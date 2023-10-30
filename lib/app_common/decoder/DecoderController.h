/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup DecoderController
 * DecoderController package (client side).
 *  @ingroup ICOV
 */

/** @file DecoderController.h
 *  @brief Defines an actual ICOV DecoderController.
 *  @ingroup DecoderController
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
 *  @copyright Copyright (C) 2023 by Inria. All rights reserved.
 * The copyright in this software is being made available under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * <br/>
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * <br/>
 * <b> For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License. </b>
 * <br/>
 * See contact section in README file for more information.
 * <br/>
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * <br/>
 * If you have questions regarding the use of this software, please contact
 * Inria using the contact section in the README file.
 * <br/>
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *****************************************************************************/

#pragma once

#include "app_common/decoder/SinglePassDecoder.h"
#include "app_common/program/icov_program_options.h"
#include "decoder/CsvOutput.h"
#include "encoder/InputMedia.h"
#include "icov_io.h"
#include "icov_path.h"
#include "icov_request.h"
#include "icov_types.h"

#include <boost/format.hpp>
#include <memory>

namespace icov {
namespace decoder {

struct decoder_param_t {
  path::path_t input_icov_file;

  bool enable_dummyViewport;

  bool enable_position_file;
  path::path_t positions_file;

  bool enable_export_csv;
  path::path_t csv_export_file;

  bool enable_ground_truth;
  path::path_t ground_truth_file;

  decoder_param_t(const path::path_t &icov_file, const options::args_t &args)
      : input_icov_file("/tmp/file.icov"), enable_dummyViewport(false),
        enable_position_file(false), positions_file("/tmp/positions.csv"),
        enable_export_csv(false), csv_export_file("/tmp/csv_output.csv"),
        enable_ground_truth(false), ground_truth_file("")

  {
    try {
      this->input_icov_file = icov_file.string();
      ICOV_INFO << boost::format("%-30s %-s") % "ICOV file:" %
                       this->input_icov_file;

      this->enable_dummyViewport = options::parseDummyViewportOption(args);
      ICOV_INFO << boost::format("%-30s %-b") % "Dummy viewport enabled:" %
                       this->enable_dummyViewport;

      this->positions_file = options::parsePositionsInputFile(args);
      this->enable_position_file =
          icov::path::file_exists(this->positions_file.string());
      ICOV_INFO << boost::format("%-30s %-b") % "Position file enabled:" %
                       this->enable_position_file;
      if (this->enable_position_file) {
        ICOV_INFO << boost::format("%-30s %-s") % "Position file:" %
                         this->positions_file;
      }

      this->csv_export_file = options::parseCsvOutputFile(args);
      this->enable_export_csv = !this->csv_export_file.empty();
      ICOV_INFO << boost::format("%-30s %-b") % "Csv output enabled:" %
                       this->enable_export_csv;
      if (this->enable_export_csv) {
        ICOV_INFO << boost::format("%-30s %-s") % "Csv prefix:" %
                         this->csv_export_file;
      }

      this->ground_truth_file = options::parseGroundTruth(args);
      this->enable_ground_truth =
          icov::path::file_exists(this->ground_truth_file.string());
      ICOV_INFO << boost::format("%-30s %-b") % "Ground truth enabled:" %
                       this->enable_ground_truth;
      if (this->enable_ground_truth) {
        ICOV_INFO << boost::format("%-30s %-s") % "Ground truth file:" %
                         this->ground_truth_file;
      }
    }
    CATCH_ALL(ICOV_DEBUG_HEADER("Error while parsing decoder parameters"))
  }
};

struct decoder_factory_t {
  decoder_param_t Params;
  icov_unique_ptr(decoder::SinglePassDecoder) Decoder;

  decoder_factory_t(const path::path_t &icov_file, const options::args_t &args)
      : Params(icov_file, args),
        Decoder(std::make_unique<decoder::SinglePassDecoder>()) {
    path::assert_file_exists(icov_file.string());
    Decoder->initLocal(decoder::SinglePassDecoder::params{
        .DummyViewport = Params.enable_dummyViewport,
        .IcovFile = Params.input_icov_file,
        .LdpcDatabase = options::parseDatabase(args)});
    Decoder->start();
  }
};

struct decoder_analyzer_t {
  icov_shared_ptr(icov::decoder::CsvOutput) CsvOutput;
  icov_unique_ptr(io::CsvFileWriter) RD_ERP;
  icov_unique_ptr(io::CsvFileWriter) RD_DISP;
  icov_unique_ptr(encoder::InputMedia) GroundTruth;

  bool hasGroundTruth() const { return GroundTruth != nullptr; }
  bool hasCSV() const { return CsvOutput != nullptr; }
};

class DecoderController {
public:
  DecoderController(const path::path_t &icov_file, const options::args_t &args)
      : m_factory(icov_file, args) {}

  const decoder_param_t getDecoderParams() const { return m_factory.Params; }

  const icov_unique_ptr(decoder::SinglePassDecoder) & getDecoder() const {
    return m_factory.Decoder;
  }

  void initAnalyzer() {
    if (getDecoderParams().enable_export_csv) {
      m_analyzer.CsvOutput = std::make_shared<decoder::CsvOutput>(
          getDecoderParams().csv_export_file.string());
      IBlockProcessor::addChild(getDecoder()->blockDecoder(),
                                m_analyzer.CsvOutput);
      m_analyzer.RD_ERP =
          std::make_unique<io::CsvFileWriter>(getDecoderParams()
                                                  .input_icov_file.parent_path()
                                                  .append("rd_erp.csv")
                                                  .string());
      m_analyzer.RD_ERP->addColumn("rate");
      m_analyzer.RD_ERP->addColumn("psnr");

      m_analyzer.RD_DISP =
          std::make_unique<io::CsvFileWriter>(getDecoderParams()
                                                  .input_icov_file.parent_path()
                                                  .append("rd_disp.csv")
                                                  .string());
      m_analyzer.RD_DISP->addColumn("rate");
      m_analyzer.RD_DISP->addColumn("psnr");
    }

    if (getDecoderParams().enable_ground_truth) {
      encoder::instantiateInputMedia(getDecoderParams().ground_truth_file,
                                     m_analyzer.GroundTruth);
    }
  }

  std::queue<request_t> readRequests() {
    std::queue<request_t> requests;

    if (!getDecoderParams().enable_position_file)
      return requests;

    std::string positions_file = getDecoderParams().positions_file.string();

    path::assert_file_exists(positions_file);

    std::ifstream inputFile(positions_file);

    if (!inputFile.is_open()) {
      ICOV_ERROR << "Failed to open file at:" << positions_file;
    } else {
      std::string line;
      while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        std::vector<double> values;

        std::string token;
        while (std::getline(iss, token, ',')) {
          try {
            double value = std::stod(token);
            values.push_back(value);
          } catch (const std::exception &e) {
            ICOV_ERROR << "Error parsing value: " << e.what();
          }
        }

        if (values.size() == 3) {
          ///@todo TODO: interpolate frame from timestamp
          frame_t frame = 0;
          angle_t pov_x = values[2];
          angle_t pov_y = values[1];
          requests.push({.viewpoint = {.angleX = pov_x, .angleY = pov_y},
                         .frameID = frame});
        }
      }
      inputFile.close();
    }

    return requests;
  }

  void decodePositions(const std::queue<request_t> &p) {
    std::queue<request_t> pos(p);

    while (!pos.empty()) {
      angle_t pov_x = pos.front().viewpoint.angleX;
      angle_t pov_y = pos.front().viewpoint.angleY;
      frame_t frame = pos.front().frameID;

      pos.pop();
      std::cout << "POV " << pov_x << ", " << pov_y << std::endl;

      // decode
      getDecoder()->request(pov_x, pov_y, frame);
    }
  }

  const decoder_analyzer_t &analyzer() const { return m_analyzer; }

private:
  decoder_factory_t m_factory;
  decoder_analyzer_t m_analyzer;
};

} // namespace decoder
} // namespace icov
