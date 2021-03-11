/****************************************************************************/
/** @addtogroup icov-core 
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup decoder
 * Decoder package (client side).
 *  @ingroup icov-core
 */

/** @file decoder.cpp
 *  @brief Implements an actual ICOV decoder.
 *  @ingroup decoder
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

/// \todo remove this after the test is done
#pragma warning( disable : 4244 ) 

#include "decoder.h"
#include <cmath>

Decoder::Decoder()
{
  cout << "new Decoder" << endl;
  m_startTime = clock();
}

Decoder::~Decoder()
{
  cout << "destroy Decoder" << endl;
}

void Decoder::decode()
{
  cout << "Decoder::decode" << endl;

  int width = m_width;
  int height = m_height;
  unsigned char *dst = (unsigned char *)m_pixels;

  float t = getTime();


  for (int y = 0; y < height; ++y)
  {
    unsigned char *ptr = dst;
    for (int x = 0; x < width; ++x)
    {

      // Write the texture pixel
      //ptr[0] = 127.0f + (xAngle / 360.0f) * 127.0f;
      //ptr[1] = 127.0f + (yAngle / 360.0f) * 127.0f;
      //ptr[2] = 0.0f;

      ptr[0] = 0.0f;
      ptr[1] = 0.0f;
      ptr[2] = 0.0f;
      ptr[3] = 0.0f;

      if (x % 128 == 0 || y % 128 == 0)
      {
        ptr[1] = 255.0f;
        ptr[3] = 255.0f;
      }

      if (x > 0.4f * width && x < 0.6f * width && y > 0.4f * height && y < 0.6f * height)
      {
        ptr[2] = ptr[3] = 127.0f * 0.5f * (1 + sinf(0.8f * t));
      }

      // To next pixel (our pixels are 4 bpp)
      ptr += 4;
    }

    // To next image row
    dst += m_rowPitch;
  }

  m_renderer->drawPixels(m_pixels, m_width, m_height);
}

float Decoder::getTime() 
{
  return (float)(clock() - m_startTime)/CLOCKS_PER_SEC;
}
