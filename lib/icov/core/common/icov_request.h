#pragma once

namespace icov
{

typedef float angle_t;
typedef unsigned long frame_t;

struct viewpoint_t
{
    angle_t angleX;
    angle_t angleY;

    viewpoint_t() : angleX(0), angleY(0){};
    viewpoint_t(angle_t p_angleX, angle_t p_angleY)
        : angleX(p_angleX), angleY(p_angleY){};
};

struct request_t
{
    viewpoint_t viewpoint;
    frame_t frameID;

    request_t() : viewpoint(), frameID(0) {}
    request_t(viewpoint_t p_viewpoint, frame_t p_frameID)
        : viewpoint(p_viewpoint), frameID(p_frameID) {}

    request_t(angle_t p_angleX, angle_t p_angleY, frame_t p_frameID)
        : viewpoint(p_angleX, p_angleY), frameID(p_frameID) {}
};

inline bool operator==(const viewpoint_t &a, const viewpoint_t &b)
{
    return a.angleX == b.angleX && a.angleY == b.angleY;
}

inline bool operator==(const request_t &a, const request_t &b)
{
    return a.viewpoint == b.viewpoint && a.frameID == b.frameID;
}

inline bool operator!=(const viewpoint_t &a, const viewpoint_t &b)
{
    return !(a == b);
}

inline bool operator!=(const request_t &a, const request_t &b)
{
    return !(a == b);
}

} // namespace icov
