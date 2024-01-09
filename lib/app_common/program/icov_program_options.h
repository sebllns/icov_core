/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup Encoder
 *  ICOV Encoder Lib
 *  @ingroup ICOV
 */

/** @file SourceMedia.h
 *  @brief Defines an actual ICOV SourceMedia.
 *  @ingroup SourceMedia
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.5.0
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

#include "icov_request.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/factory/LdpcCoderParameters.h"
#include "ldpc_codec_layer/factory/aff3ct_coder_types.h"
#include <boost/any.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <queue>
#include <string>
#include <utility>

namespace po = boost::program_options;

namespace icov {
namespace options {

typedef po::variables_map args_t;
typedef po::options_description desc_t;

// GENERAL OPTIONS
const std::string help = "help";
const std::string mode = "mode";
const std::string log_lvl = "log-level";
const std::string input_file = "input";
const std::string output_file = "output";

// LDPC OPTIONS
const std::string code_length = "ldpc-code-length";
const std::string bp_iter = "ldpc-bp-ite";
const std::string coder_type = "ldpc-coder-type";
const std::string data_path = "ldpc-data-path";

// ENCODER OPTIONS
const std::string encode_allblocks = "encode-allblocks";
const std::string start_block = "encode-start-block";
const std::string block_count = "encode-block-count";
const std::string frame_start = "encode-start-frame";
const std::string frame_count = "encode-frame-count";
const std::string viewport_padding = "encode-viewport-padding";
const std::string viewport_erp_horizontal_ratio_id =
    "encode-viewport-erp-horizontal-ratio";
const std::string viewport_image_ratio_id = "encode-viewport-image-ratio";

const std::string icu_size = "encode-icu-size";
const std::string qp = "encode-qp";
const std::string fov = "encode-fov";
const std::string dummy_viewport = "encode-dummy-viewport";

// DECODER OPTIONS
const std::string pre_encode = "decode-preencode";
const std::string csv_output = "decode-csv-output";
const std::string positions_input = "decode-positions-input";
const std::string headless = "decode-headless";
const std::string ground_truth = "decode-ground-truth";

struct EncoderArgs {
  std::string viewport_erp_horizontal_ratio_id() const {
    return "{ENC}viewport-erp-horizontal-ratio";
  }

  std::string viewport_image_ratio_id() const {
    return "{ENC}viewport-image-ratio";
  }

  std::string viewport_fovx_id() const {
    return "{ENC}viewport-horizontal-fov";
  }

  std::string qp_id() const { return "{ENC}qp"; }

  std::string icu_size_id() const { return "{ENC}icu-size"; }
};

struct decoder_params {
  bool pre_encode;
  bool headless;
  fs::path csv_output;
  fs::path positions_input;
  fs::path ground_truth;

  decoder_params() : pre_encode(false) {}
};

struct DecoderArgs {
  std::string pre_encode_id() const { return "DEC-preencode"; }
  std::string csv_output_id() const { return "DEC-csv-output"; }
  std::string positions_input_id() const { return "DEC-positions-input"; }
  std::string headless_id() const { return "DEC-headless"; }
  std::string ground_truth_id() const { return "DEC-ground-truth"; }

  void addOptions(desc_t &output) const {
    decoder_params default_values;

    output.add_options()(
        pre_encode_id().c_str(),
        po::value<bool>()->default_value(default_values.pre_encode),
        "Pre-encode the media before decode (useful to get more "
        "debug informations)");
  }

  decoder_params parsePreencodeOption(const args_t &args) const {
    decoder_params p;

    p.pre_encode = args[pre_encode_id()].as<bool>();

    return p;
  }
};

inline void addEncoderOptions(desc_t &output) {
  output.add_options()(icu_size.c_str(), po::value<int>()->default_value(32),
                       "ICU width for encoding");
  // example:  22, 27, 32, 37, 42
  output.add_options()(qp.c_str(), po::value<int>()->default_value(27),
                       "QP value for encoding");

  output.add_options()(viewport_erp_horizontal_ratio_id.c_str(),
                       po::value<float>()->default_value(40.0 / 100.0),
                       "Viewport/ERP horizontal ratio");

  output.add_options()(viewport_image_ratio_id.c_str(),
                       po::value<float>()->default_value(16.0 / 9.0),
                       "Viewport image format i.e. width/height ratio");

  output.add_options()(
      viewport_padding.c_str(), po::value<float>()->default_value(1.2),
      "Radius factor used in icov viewport to check if a block is "
      "visible");

  output.add_options()((fov + ",f").c_str(),
                       po::value<float>()->default_value(80),
                       "Viewport horizontal field of view in degrees");

  output.add_options()(dummy_viewport.c_str(),
                       po::value<bool>()->default_value(false),
                       "Use dummy viewport (all blocks are visible)");

  output.add_options()(
      frame_count.c_str(), po::value<frame_t>()->default_value(0),
      "Frame number to be encoded from video (0 means all frames)");

  output.add_options()(frame_start.c_str(),
                       po::value<frame_t>()->default_value(0),
                       "Frame to start the encoding from");
}

inline void addDecoderOptions(desc_t &output) {
  output.add_options()(pre_encode.c_str(),
                       po::value<bool>()->default_value(false),
                       "Pre-encode the media before decode (useful to get more "
                       "debug informations)");

  output.add_options()(ground_truth.c_str(),
                       po::value<std::string>()->default_value(""),
                       "Path to use ground truth when non-empty");

  output.add_options()(positions_input.c_str(),
                       po::value<std::string>()->default_value(""),
                       "Path pointing to file that contains viewpoint "
                       "positions for decoding simulation");

  output.add_options()(
      csv_output.c_str(), po::value<std::string>()->default_value(""),
      "Prefix path if you need to export decoding results into CSV");

  output.add_options()(headless.c_str(),
                       po::value<bool>()->default_value(false),
                       "Use headless decoder which means no graphical output "
                       "(only meta-parameters will be extracted "
                       "from encoded file) but is faster to execute. Will use "
                       "ground truth as display when provided, "
                       "to replace graphical output");
}

inline void init_options_desc(const std::string &modes, desc_t &output) {
  output.add_options()

      ("help,h", "Produce help message")

          ((mode + ",m").c_str(), po::value<std::string>()->required(),
           (std::string("Set the program mode (see documentation for more "
                        "details on each mode)\nAccepted values: ") +
            modes)
               .c_str())

              ((coder_type + ",c").c_str(),
               po::value<std::string>()->default_value("imt164"),
               "Set the coder type")

                  ((data_path + ",d").c_str(),
                   po::value<std::string>()->default_value("data/"),
                   "Set the data path")

                      ((code_length + ",n").c_str(),
                       po::value<int>()->default_value(1024),
                       "Set the codeword length for LDPC")(
                          (std::string(bp_iter) + ",it").c_str(),
                          po::value<int>()->default_value(100),
                          "Set iteration count for belief propagation of LDPC "
                          "coder")

                          ((log_lvl + ",lg").c_str(),
                           po::value<int>()->default_value(2),
                           "Set log level filter")

                              ((input_file + ",i").c_str(),
                               po::value<std::string>()->default_value(
                                   "/tmp/input_file"),
                               "Set input file")

                                  ((output_file + ",o").c_str(),
                                   po::value<std::string>()->default_value(""),
                                   "Set output file");

  addEncoderOptions(output);
  addDecoderOptions(output);
}

#define ICOV_DATA_DIR(args)                                                    \
  fs::absolute(fs::path(args[icov::options::data_path].as<std::string>()))
#define ICOV_Q_VALUES_FILE(args) (ICOV_DATA_DIR(args) / "../q-ary/q")

template <typename T> T parse(const std::string &id, const args_t &args) {
  return args[id].as<T>();
}

inline std::string parseInputFile(const args_t &args) {
  return args[input_file].as<std::string>();
}

inline std::string parseOutputFile(const args_t &args) {
  return args[output_file].as<std::string>();
}

inline std::string parseCsvOutputFile(const args_t &args) {
  return args[csv_output].as<std::string>();
}

inline std::string parsePositionsInputFile(const args_t &args) {
  return args[positions_input].as<std::string>();
}

inline std::string parseGroundTruth(const args_t &args) {
  return args[ground_truth].as<std::string>();
}

inline std::string parseLdpcCoderType(const args_t &args) {
  return args[coder_type].as<std::string>();
}

inline std::string parseDatabase(const args_t &args) {
  return args[data_path].as<std::string>();
}

inline int parseCodeLength(const args_t &args) {
  return args[code_length].as<int>();
}

inline int parseQP(const args_t &args) { return args[qp].as<int>(); }

inline int parseBpIte(const args_t &args) { return args[bp_iter].as<int>(); }

inline float parseFov(const args_t &args) { return args[fov].as<float>(); }

inline int parseFrameCount(const args_t &args) {
  return args[frame_count].as<frame_t>();
}

inline int parseFrameStart(const args_t &args) {
  return args[frame_start].as<frame_t>();
}

inline float parseViewportErpRatio(const args_t &args) {
  return args[viewport_erp_horizontal_ratio_id].as<float>();
}

inline float parseViewportImageRatio(const args_t &args) {
  return args[viewport_image_ratio_id].as<float>();
}

inline float parseViewportPadding(const args_t &args) {
  return args[viewport_padding].as<float>();
}

inline bool parsePreEncodeOption(const args_t &args) {
  return args[pre_encode].as<bool>();
}

inline bool parseDummyViewportOption(const args_t &args) {
  return args[dummy_viewport].as<bool>();
}

inline int parseEncodeIcuSizeOption(const args_t &args) {
  return args[icu_size].as<int>();
}

inline void parseLdpcParams(const args_t &args,
                            const std::string &intented_type,
                            LdpcCoderParameters &params) {
  icov::assert::equal(params.type(), intented_type,
                      "Given type was \'" + params.type() +
                          "\' but intended type was \'" + intented_type + "\'");

  params.database = parseDatabase(args);
  params.codeLength = parseCodeLength(args);
  params.beliefPropagationIterations = parseBpIte(args);
}

inline std::unique_ptr<LdpcCoderParameters>
parseLdpcParams(const boost::program_options::variables_map &args) {
  std::string coder_type_val = parseLdpcCoderType(args);
  std::unique_ptr<LdpcCoderParameters> params =
      ldpc_codec_layer::createLdpcParams(coder_type_val);
  icov::assert::not_null(params.get());
  parseLdpcParams(args, coder_type_val, *params);
  return params;
}

inline ldpc_codec_layer::p_InteractiveCoder
create_coder(const boost::program_options::variables_map &args) {
  auto params = parseLdpcParams(args);
  return ldpc_codec_layer::LdpcCoderFactory::create_coder(*params);
}

} // namespace options
} // namespace icov
