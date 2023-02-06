/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */

/** @file OrderingDemoMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.3
 *  @date 2023
 *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
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

#include "geometry.h"
#include "interface/IBlockProcessor.h"
#include "opencv2/core/mat.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

class BlockProcessor : public IBlockProcessor {

private:
  cv::Point m_line_point;
  uint16_t m_blockCount;
  uint16_t m_single_si_count;
  uint16_t m_double_si_count;
  std::ofstream m_csv_file;
  std::ifstream m_encoded_image;
  int header[4];

public:
  angle_degree Pov;
  cv::Mat BlockRequested;
  bool b_show_side_infos;
  bool b_show_access_block;
  bool b_show_block_grid;
  bool b_show_block_number;
  bool b_show_path;
  bool b_use_encoded_stream;

  BlockProcessor(const std::string &encoded_image);
  ~BlockProcessor();

  // IBlockProcessor interface
  void processBlock(const Block &b, uint16_t prediction) override;
  void endProcess() override;
  void startProcess() override;
  std::ostream encodingStream();
};

class OrderingDemoMode {
public:
  OrderingDemoMode(const po::variables_map &args);
  ~OrderingDemoMode() = default;

  void run();

  static void AddProgramOptions(po::options_description &desc);

private:
  struct params {

    static const char *inputFile_id;
    static const char *icuSize_id;
    static const char *viewportFov_id;

    fs::path inputFile;
    int icuSize;
    float viewportFov;
  } m_parameters;
};
