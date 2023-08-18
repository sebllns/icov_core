/****************************************************************************/
/** @addtogroup Omnidirectional
 *  Omnidirectional is the adaptation of the coder into 360 context.
 *  @ingroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @file IBlockProcessor.h
 *  @brief IBlockProcessor conveying IBlockProcessor Units (VPU) for VCL data
 *  @ingroup Omnidirectional
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

#include "icov_debug.h"
#include "icov_log.h"
#include "icov_request.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/interface/IBlockPredictionManager.h"
#include <memory>
#include <vector>

namespace icov {
namespace omnidirectional_layer {

/** @class IBlockProcessor
 *  @brief IBlockProcessor
 *
 *  A IBlockProcessor is
 */
class IBlockProcessor
{
  public:
    using func_t = std::function<void(const request_t &, const Block &, IBlockPredictionManager::prediction_t)>;

    IBlockProcessor();

    virtual ~IBlockProcessor() = default;

    ///
    /// \brief startProcess : executed BEFORE loop on processBlock
    ///
    void beforeOrderingLoop(const request_t &r);

    ///
    /// \brief processBlock executed in a loop on each block to process
    /// \param b
    /// \param prediction
    ///
    void processBlock(const request_t &r, const Block &b, IBlockPredictionManager::prediction_t prediction);

    ///
    /// \brief startProcess : executed AFTER loop on processBlock
    ///
    void afterOrderingLoop(const request_t &r);

    const std::shared_ptr<IBlockProcessor> &parent() const;
    const std::string prettyName() const;
    const std::string &name() const;
    const std::vector<std::shared_ptr<IBlockProcessor>> &childs() const;
    void setName(const std::string &newName);

    static void removeParent(const std::shared_ptr<IBlockProcessor> &node);
    static void addChild(const std::shared_ptr<IBlockProcessor> &parent, const std::shared_ptr<IBlockProcessor> &child);

    bool enabled() const;
    void setEnabled(bool newEnabled);

    void setBefore(const func_t &newBefore);
    void setAfter(const func_t &newAfter);

  protected:
    virtual void _beforeOrderingLoop(const request_t &r){};
    virtual void _afterOrderingLoop(const request_t &r){};
    virtual void _processBlock(const request_t &r, const Block &b, IBlockPredictionManager::prediction_t prediction){};

  private:
    std::string m_name;
    std::shared_ptr<IBlockProcessor> m_parent;
    std::vector<std::shared_ptr<IBlockProcessor>> m_childs;
    bool m_enabled;

    func_t m_before;
    func_t m_after;
};

inline IBlockProcessor::IBlockProcessor() : m_name(""), m_parent(nullptr), m_enabled(true)
{
}

inline void IBlockProcessor::beforeOrderingLoop(const request_t &r)
{
    if (!m_enabled)
        return;

    if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "calling startProcess() for BlockProcessor (" << prettyName() << ")";

    _beforeOrderingLoop(r);

    for (auto &ch : m_childs)
        ch->beforeOrderingLoop(r);
}

inline void IBlockProcessor::processBlock(const request_t &r, const Block &b,
                                          IBlockPredictionManager::prediction_t prediction)
{
    if (!m_enabled)
        return;

    if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "calling processBlock(block_id=" << b.getIdSelf() << ", pred_id=" << prediction
                   << ") for BlockProcessor (" << prettyName() << ")";

    if (m_before)
        m_before(r, b, prediction);

    _processBlock(r, b, prediction);

    for (auto &ch : m_childs)
        ch->processBlock(r, b, prediction);

    if (m_after)
        m_after(r, b, prediction);
}

inline void IBlockProcessor::afterOrderingLoop(const request_t &r)
{
    if (!m_enabled)
        return;

    if (icov::debug::debug_trace_enabled)
        ICOV_TRACE << "calling endProcess() for BlockProcessor (" << prettyName() << ")";

    _afterOrderingLoop(r);

    for (auto &ch : m_childs)
        ch->afterOrderingLoop(r);
}

inline const std::shared_ptr<IBlockProcessor> &IBlockProcessor::parent() const
{
    return m_parent;
}

inline const std::string IBlockProcessor::prettyName() const
{
    return (m_name.empty() ? "unnamed" : m_name);
}

inline const std::string &IBlockProcessor::name() const
{
    return m_name;
}

inline const std::vector<std::shared_ptr<IBlockProcessor>> &IBlockProcessor::childs() const
{
    return m_childs;
}

inline void IBlockProcessor::removeParent(const std::shared_ptr<IBlockProcessor> &node)
{
    if (node->m_parent)
    {
        auto &vec = node->m_parent->m_childs;
        vec.erase(std::remove(vec.begin(), vec.end(), node), vec.end());
    }
    node->m_parent = std::shared_ptr<IBlockProcessor>(nullptr);
}

inline void IBlockProcessor::addChild(const std::shared_ptr<IBlockProcessor> &parent,
                                      const std::shared_ptr<IBlockProcessor> &child)
{
    removeParent(child);
    child->m_parent = parent;
    if (parent)
        parent->m_childs.push_back(child);
}

inline bool IBlockProcessor::enabled() const
{
    return m_enabled;
}

inline void IBlockProcessor::setEnabled(bool newEnabled)
{
    m_enabled = newEnabled;
}

inline void IBlockProcessor::setAfter(const func_t &newAfter)
{
    m_after = newAfter;
}

inline void IBlockProcessor::setBefore(const func_t &newBefore)
{
    m_before = newBefore;
}

inline void IBlockProcessor::setName(const std::string &newName)
{
    m_name = newName;
}

} // namespace omnidirectional_layer
} // namespace icov
