///****************************************************************************/
///** @addtogroup ICOV
// *  ICOV, open source Interactive Coder for Omnidirectional Videos.
// */

///** @addtogroup IExtractor
// * IExtractor package (client side).
// *  @ingroup ICOV
// */

///** @file IExtractor.h
// *  @brief Defines an abstract ICOV IExtractor.
// *  @ingroup IExtractor
// *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
// *  @version 1.0.1
// *  @date 2022
// *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
// * The copyright in this software is being made available under
// * the terms of the GNU Affero General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// * (at your option) any later version.
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// * See the GNU Affero General Public License for more details.
// * <br/>
// * You should have received a copy of the GNU Affero General Public License
// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
// * <br/>
// * <b> For using ICOV under terms that will not comply with the GNU
// * AGPL, please contact Inria about acquiring an ICOV Professional
// * Edition License. </b>
// * <br/>
// * See contact section in README file for more information.
// * <br/>
// * This software was developed at:
// * Inria Rennes - Bretagne Atlantique
// * Campus Universitaire de Beaulieu
// * 35042 Rennes Cedex
// * France
// * <br/>
// * If you have questions regarding the use of this software, please contact
// * Inria using the contact section in the README file.
// * <br/>
// * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// *
// *****************************************************************************/

//#pragma once

//#include "Media.h"
//#include "common.h"
//#include "omnidirectional_layer/BlockPrediction.h"
//#include "omnidirectional_layer/interface/IAccessBlockManager.h"
//#include "omnidirectional_layer/interface/IBlockProcessor.h"
//#include "omnidirectional_layer/interface/IPicture.h"

//#include <memory>

// namespace icov {
// namespace extractor {

/////////////////////////////////////////////////////////////////////////////////
/////
///// @class IExtractor
///// @todo COMMENT !
/////
/////////////////////////////////////////////////////////////////////////////////
// template <typename T> class IExtractor {

//  /* METHODS */
// public:
//  ///
//  /// \brief IExtractor ctor: instanciate extractor from file
//  /// \param icov_file
//  ///
//  IExtractor(const fs::path icov_file);
//  virtual ~IExtractor() = default;
//  virtual void extract(const IPicture &picture,
//                       const IAccessBlockManager &access_blocks,
//                       const IBlockPrediction &block_prediction) = 0;

//  virtual const IInputMedia<T> *media() const { return m_pMedia.get(); }

// protected:
//  virtual void processInput(const std::istream &input) = 0;
//  std::unique_ptr<IInputMedia<T>> m_pMedia;

// private: // cpy ctor
//};

/////////////////////////////////////////////////////////////////////////////////
/////
///// @class IFrameExtractor
///// @todo COMMENT !
/////
/////////////////////////////////////////////////////////////////////////////////
// template <typename T> class IBlockExtractor : public IBlockProcessor {
// protected:
//  int m_frame;

// public:
//  IFrameExtractor(int frame) : m_frame(frame) {}
//  virtual void extract(std::ostream &output, IInputMedia<T> &input,
//                       const IPicture &picture,
//                       const IAccessBlockManager &access_blocks,
//                       const IBlockPrediction &block_prediction);

// protected:
//  virtual void extractBlock(std::ostream &output, const Block &b,
//                            bool access_block,
//                            const IBlockPrediction &block_prediction);
//  virtual void extractBlockWithPrediction(std::ostream &output, const Block
//  &b,
//                                          uint16_t prediction) = 0;
//  virtual std::vector<uint16_t>
//  getAvailablePredictions(const Block &b, bool access_block,
//                          const IBlockPrediction &block_prediction) {
//    return block_prediction.getAvailablePredictions(b, access_block);
//  }

// private:
//  IFrameExtractor() = default;
//  IFrameExtractor(const IFrameExtractor &) = default;
//};

// template <typename T>
// void IFrameExtractor<T>::extract(std::ostream &output, IInputMedia<T> &input,
//                                 const IPicture &picture,
//                                 const IAccessBlockManager &access_blocks,
//                                 const IBlockPrediction &block_prediction) {

//  input.setFrame(m_frame);

//  ICOV_ASSERT_MSG(input.getPixelCount() > 0, "Cannot extract empty frame");
//  ICOV_ASSERT_MSG(input.getHeight() == picture.height() &&
//                      input.getWidth() == picture.width(),
//                  "Picture is not the right size");

//  this->processInput(output, input);
//  for (const auto &b : picture.icus()) {
//    extractBlock(output, b, access_blocks.isAccessBlock(b.getId(Block::SELF)),
//                 block_prediction);
//  }
//}

// template <typename T>
// void IFrameExtractor<T>::extractBlock(
//    std::ostream &output, const Block &b, bool access_block,
//    const IBlockPrediction &block_prediction) {
//  for (uint16_t p : getAvailablePredictions(b, access_block,
//  block_prediction))
//    extractBlockWithPrediction(output, b, p);
//}

/////////////////////////////////////////////////////////////////////////////////
/////
///// @class IMediaExtractor
///// @todo COMMENT !
/////
/////////////////////////////////////////////////////////////////////////////////
// template <typename T> class IMediaExtractor : public IExtractor<T> {

// public:
//  virtual void extract(std::ostream &output, IInputMedia<T> &input,
//                       const IPicture &picture,
//                       const IAccessBlockManager &access_blocks,
//                       const IBlockPrediction &block_prediction);

// protected:
//  virtual std::unique_ptr<IFrameExtractor<T>>
//  createFrameExtractor(int frame) = 0;
//};

// template <typename T>
// void IMediaExtractor<T>::extract(std::ostream &output, IInputMedia<T> &input,
//                                 const IPicture &picture,
//                                 const IAccessBlockManager &access_blocks,
//                                 const IBlockPrediction &block_prediction) {
//  processInput(output, input);
//  int frameCount = input.getFrameCount();

//  for (int i = 0; i < frameCount; i++) {
//    auto pFrameEnc = createFrameExtractor(i);
//    pFrameEnc->extract(output, input, picture, access_blocks,
//    block_prediction);
//  }
//}
//} // namespace extractor
//} // namespace icov
