#include "geometry.h"

bool is_visible(const point3d &p, const point2d &viewport_size)
{
    //std::cout << viewport_size.x() << ", " << viewport_size.y() << std::endl;
    return p.x() > 0
            && abs(p.y()) < p.x() * viewport_size.x()/2
            && abs(p.z()) < p.x() * viewport_size.y()/2;
}


point3d inverse_YPR(const point3d &p, float yaw, float pitch, float roll)
{
    float3 rot = boost::qvm::rot_mat_xyz<3, float>(-roll, -pitch, -yaw) * float3{p.x(), p.y(), p.z()};
    return point3d(boost::qvm::X(rot), boost::qvm::Y(rot), boost::qvm::Z(rot));
}

point3d rotate_YPR(const point3d &p, float yaw, float pitch, float roll)
{
    float3 rot = boost::qvm::rot_mat_zyx<3, float>(yaw, pitch, roll) * float3{p.x(), p.y(), p.z()};
    return point3d(boost::qvm::X(rot), boost::qvm::Y(rot), boost::qvm::Z(rot));
}

angle_radian viewport_geometry::getPov() const
{
    return pov;
}

void viewport_geometry::setPov(angle_radian newPov)
{
    pov = newPov;
}
