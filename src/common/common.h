/****************************************************************************/
/** @addtogroup icov-core 
 * ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup icov-core
 */

/** @file common.h
 *  @brief Defines version information, constants and small in-line functions
 *  @ingroup common
 *  @author Sébastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
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

#ifndef COMMON_H
#define COMMON_H

#if _MSC_VER
#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#define NOMINMAX
#include <Windows.h>
#endif

#include <ctype.h>
#include <locale>
#include <locale.h>
#include <string>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>

using namespace std;

#include "common_types.h"
#include "typedef.h"
#include "version.h"
#include "helpers.hpp"

#endif // COMMON_H
