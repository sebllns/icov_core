/****************************************************************************/
/** @addtogroup CLI
 *  Command Line Interface
 *  @ingroup ICOV
 */


/** @file TestSequenceResult.h
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup CLI
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

#include "common.h"
#include <map>

enum result_data_type {
  UNKNOWN = -1,
  SCALAR = 0,
  MATRIX = 1,
};

struct result_data {
  typedef float data_type;

  std::string name;
  result_data_type type;
  int n_rows;
  int n_cols;
  std::vector<data_type> data;

  float get_data(int row, int col) { return data[row * n_cols + col]; }

  bool operator==(const result_data &rhs) const {
    return (this->name == rhs.name) && (this->type == rhs.type) &&
           (this->n_rows == rhs.n_rows) && (this->n_cols == rhs.n_cols) &&
           (this->data == rhs.data);
  }

  bool operator!=(const result_data &rhs) const { return !operator==(rhs); }

  result_data() { type = UNKNOWN; }
};

class TestSequenceResult {
public:
  TestSequenceResult();
  bool read_from_file(std::string filename);

  bool operator==(const TestSequenceResult &rhs) const {
    return (this->m_data == rhs.m_data);
  }

  bool operator!=(const TestSequenceResult &rhs) const {
    return !operator==(rhs);
  }

  result_data &operator[](std::string id) { return m_data[id]; }

  const result_data &operator[](std::string id) const { return m_data.at(id); }

protected:
  std::map<std::string, result_data> m_data;
};
