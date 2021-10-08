#include "BitSymbolProvider.h"

BitSymbolProvider::BitSymbolProvider(int z_min, int z_max, int y_min, int y_max, float q):
    m_z_min(z_min), m_z_max(z_max), m_y_min(y_min), m_y_max(y_max), m_q0(q)
{

}

float BitSymbolProvider::z_min()
{
    return this->m_z_min;
}

float BitSymbolProvider::z_max()
{
    return this->m_z_max;
}

float BitSymbolProvider::y_min()
{
    return this->m_y_min;
}

float BitSymbolProvider::y_max()
{
    return this->m_y_max;
}

float BitSymbolProvider::q0()
{
    return this->m_q0;
}

float BitSymbolProvider::q1()
{
    if((m_z_max - m_z_min) == 0) return 0.0;
    return (1 - m_q0)/(m_z_max - m_z_min);
}

int BitSymbolProvider::z0()
{
    return (m_z_min > 0) ? m_z_min : (m_z_max < 0 ? m_z_max : 0);
}

float BitSymbolProvider::pz(int z)
{
    return (z == z0()) ? m_q0 : ( (z < m_z_min || z > m_z_max) ? 0 : q1() );
}

float BitSymbolProvider::pz(int zmin, int zmax)
{
    float p = 0;

    for(int z = zmin ; z <= zmax ; z++)
    {
        p+=this->pz(z);
    }
    return p;
}

int BitSymbolProvider::z_binary_length()
{
    int max = std::max(std::abs(m_z_min), std::abs(m_z_max));
    if(max == 0) return 1;
    return std::ceil(std::log2f(max));
}

int BitSymbolProvider::x_min_estimated()
{
    return m_z_max+ m_y_min;
}

int BitSymbolProvider::x_max_estimated()
{
    return m_z_min + m_y_max;
}

int BitSymbolProvider::x_binary_length()
{
    return std::floor(std::log2f(std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated())))) + 2;
}

bool BitSymbolProvider::pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0, float &p1, float &p)
{

    p0 = 0.0;
    p1 = 0.0;
    p = 0.0f;

    int zk0 = (1-2*negative) * (abs_predec << (l-b))  - yk;
    int zk1 =  (1<<(l-b-1));
    int zk2 =  (1<<(l-b));

    //std::cout << b << ", " << imax << ", " << zk0 << std::endl;

    for(int i = 0 ; i < zk2 ; i++)
    {
        float pi = this->pz(zk0 + i);
        p1 += pi;
        if(i < zk1) p0 += pi;
        //std::cout << "i="<<i << " p=" << this->pz(i - yk) << std::endl;
    }

    //    p1 = std::max(p1, (float)0.000001);

    p = p1 == 0 ? 0.5 : p0/p1;

    return p1 != 0;
}

float BitSymbolProvider::pqks(int yk)
{

    int imax = 1 << x_binary_length(); //std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated()));
    float p0 = 0.0f;
    for(int i = 0 ; i < imax ; i++)
    {
        p0 += this->pz(i - yk);
        //std::cout << "i="<<i << " p=" << this->pz(i - yk) << std::endl;
    }

    //std::cout << "yk=" << yk << " p=" << p0 << std::endl;

    return p0;
}
