#ifndef BITSYMBOLPROVIDER_H
#define BITSYMBOLPROVIDER_H

#include "common.h"

namespace icov
{
class BitSymbolProvider
{
public:


    static std::vector<std::vector<CodedVal>> compute_bits(const std::vector<CodedVal> &input);

    static std::vector<CodedVal> compute_symboles(const std::vector<std::vector<CodedVal>> &input);

    BitSymbolProvider(int z_min, int z_max, int y_min, int y_max, float q, int l);

    float z_min();
    float z_max();

    float y_min();
    float y_max();

    //q
    float q0();

    //q'
    float q1();

    //z associated with q0 = min(|zk|)
    int z0();

    //..0..0 [q'..q' q q'..q'] 0..0..
    float pz(int z);

    float pz(int zmin, int zmax);

    //L
    int z_binary_length();

    int x_min_estimated();
    int x_max_estimated();

    int x_binary_length();


    bool pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0, float &p1, float &p);
    float pqks(bool negative, int yk);

private:
    int m_z_min, m_z_max;
    int m_y_min, m_y_max;
    //q
    float m_q0;

    int m_l;
};
}
#endif // BITSYMBOLPROVIDER_H
