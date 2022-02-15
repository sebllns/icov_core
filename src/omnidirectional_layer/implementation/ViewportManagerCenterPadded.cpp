#include "ViewportManagerCenterPadded.h"

ViewportManagerCenterPadded::ViewportManagerCenterPadded(float padding):
    m_padding(padding)
{

}

bool ViewportManagerCenterPadded::isBlockVisible(const Block &b, const viewport_geometry &vpg) const
{
    auto rotated = inverse_YPR(b.getCartesianCoord(0), vpg.pov.x(), vpg.pov.y(), 0);

    auto roi = vpg.roi_size;

    if(rotated.x() < 0) return false;

    bg::multiply_value(roi, m_padding);//todo auto factor from size
    return is_visible(rotated, roi);
}
