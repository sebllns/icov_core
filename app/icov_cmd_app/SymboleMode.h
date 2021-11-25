/****************************************************************************/
/** @addtogroup icov-core
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file SymboleMode.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 0.0.1
 *  @date 2021
 *  @copyright Copyright (C) 2021 by Inria. All rights reserved.
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

#include "common.h"
#include "interactive_coder/IcovCoder.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;


class SymboleMode
{
public:
    SymboleMode(const po::variables_map &args);
    ~SymboleMode() = default;

    void run();

    static void AddProgramOptions(po::options_description &desc);

private:

    void encode();
    void write_results();
    void process_input_list(const std::string& file);
    void process_input_file(const std::string& file);
    void compute_src_bits();
    void process_si();
    void compute_si_bits();
    void init_bsp();
    void encode_bitplane(int i);
    bool check_decoding(int i);
    void fill_predec(int i);

    int quantize(int value, int step, int min, int max);
    int quantize(float value, const std::vector<float>& v);

    std::shared_ptr<AffInteractiveCoder> m_coder;
    int n;

    std::string m_dataset_dir;
    std::string m_result_dir;

    std::vector<CodedVal> src_symbols;
    std::vector<std::vector<CodedVal>> src_bits;

    std::vector<SIVal> side_info;
    std::vector<std::vector<CodedVal>> si_bits;

    std::vector<LLRVal> llr;
    std::vector<CodedVal> dec_bits;
    std::vector<CodedVal> dec_signs;
    std::vector<CodedVal> dec_symbols;
    std::vector<std::pair<float,float>> sim_result;
    std::vector<float> q_values;
    BitSymbolProvider bsp;
    std::vector<int> predec;

    int lx;
    Syndrome stored_synd;
    int max_si_count;

    bool enable_q_quantization;
    bool enable_z_quantization;

    uint32_t numcode;
    int numerrors;

    static const std::string dataset_dir_id;
    static const std::string result_dir_id;
    static const std::string max_si_count_id;
    static const std::string enable_q_quantization_id;
    static const std::string enable_z_quantization_id;


};
