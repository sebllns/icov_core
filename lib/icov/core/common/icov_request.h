#pragma once

namespace icov
{

struct viewpoint_t
{
    float angleX;
    float angleY;
};

struct request_t
{
    viewpoint_t viewpoint;
    unsigned long frameID;
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
