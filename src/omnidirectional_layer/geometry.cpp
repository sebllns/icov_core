#include "geometry.h"


point2d get_viewport_coordinates(const point3d &p, const viewport_geometry &viewport)
{
    float u = 0, v = 0;

    float x = p.x();
    float y = p.y();
    float z = p.z();

    float ku = viewport.resolution.x();
    float kv = viewport.resolution.y();

    float cu = viewport.center.x();
    float cv = viewport.center.y();

    float f = viewport.focale;

    u = ((ku * cu * x) + (ku * f * z)) / x;
    v = ((kv * cv * x) + (kv * f * y)) / x;

    return point2d(u,v);
}

bool is_visible(const point2d &p, const box &viewport)
{
    return bg::within(p, viewport);
}

box get_roi(const point2d &resolution, const point2d &padding)
{
    return box(point2d(-0.5f*padding.x(), -0.5f*padding.y()), point2d(resolution.x() + 0.5f*padding.x(), resolution.y() + 0.5f*padding.y()));
}

point3d rotate_YPR(const point3d &p, float yaw, float pitch, float roll)
{
    float3 rot = boost::qvm::rot_mat_zyx<3, float>(yaw, pitch, roll) * float3{p.x(), p.y(), p.z()};

    return point3d(boost::qvm::X(rot), boost::qvm::Y(rot), boost::qvm::Z(rot));
}
