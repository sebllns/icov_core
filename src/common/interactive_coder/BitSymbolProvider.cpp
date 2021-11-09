#include "BitSymbolProvider.h"

#include "tools/DebugTools.h"

namespace icov
{
BitSymbolProvider::BitSymbolProvider(int z_min, int z_max, int y_min, int y_max, float q, int l):
    m_z_min(z_min), m_z_max(z_max), m_y_min(y_min), m_y_max(y_max), m_q0(q), m_l(l)
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

int BitSymbolProvider::x_max_estimated()
{
    return m_z_max + m_y_min;
}

int BitSymbolProvider::x_min_estimated()
{
    return m_z_min + m_y_max;
}

int BitSymbolProvider::x_binary_length()
{
    return m_l;
    return std::floor(std::log2f(std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated())))) + 2;
}

bool BitSymbolProvider::pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0, float &p1, float &p)
{
    abs_predec = abs_predec << (l-b);

    p0 = 0.0;
    p1 = 0.0;
    p = 0.0f;

    //IMPLEMENTATION FANGPING
    int imax = (1<<(l-b));
    for(int i = 0 ; i < imax ; i++)
    {
        int z = (abs_predec + i ) * (1 - 2 * negative) - yk;

        p0 += this->pz(z);
        p1 += this->pz(z + imax * (1 - 2 * negative));
    }

   // DebugTools::logFile <<  "\t\t\t yk=" << yk << " a=" << p0 <<  " b=" << p1 << std::endl;

    //attention b = a+b
    float pb = p1+p0;

    //DebugTools::logFile <<  "\t\t\t yk=" << yk << std::endl;

    if(DebugTools::debug_trace_enabled)
    {
        if(pb == 0)
        {
            std::cerr << "WARNING: pb = 0" << std::endl;

            DebugTools::logFile <<  "\t\t\t WARNING: (a + b) = 0" << std::endl;
            DebugTools::logFile <<  "\t\t\t yk=" << yk << " a=" << p0 <<  " b=" << p1 << " imax=" << ((1<<(l-b))) << std::endl;



            DebugTools::logFile <<  "\t\t\t a = ";
            int imax = (1<<(l-b));
            for(int i = 0 ; i < imax ; i++)
            {
                int z = (abs_predec + i ) * (1 - 2 * negative) - yk;
                DebugTools::logFile <<  "pz(" << z << ") + ";
            }

            DebugTools::logFile << std::endl <<  "\t\t\t b = ";

            for(int i = 0 ; i < imax ; i++)
            {
                int z = (abs_predec + i ) * (1 - 2 * negative) - yk + imax * (1 - 2 * negative);
                DebugTools::logFile <<  "pz(" << z << ") + ";


            }

            DebugTools::logFile << std::endl;
        }


        if(std::abs(p0/pb + p1/pb - 1) > 0.00001)
        {
            DebugTools::logFile <<  "\t\t\t WARNING: sum = " << (p0/pb + p1/pb) << " != 1" << std::endl;
        }
    }


    p = pb == 0 ? 0.5 : p0/pb;

    return pb != 0;

    //IMPLEMENTATION SEBASTIEN
    int zk0 = (1-2*negative) * (abs_predec << (l-b))  - yk;
    int zk1 =  (1<<(l-b-1));
    int zk2 =  (1<<(l-b));

    //std::cout << l <<", " << b << ", " << zk0 << ", " << zk1 << ", " << zk2 << std::endl;

    for(int i = 0 ; i < zk2 ; i++)
    {
        float pi = this->pz(zk0 + i);
        p1 += pi;
        if(i < zk1) p0 += pi;
        //std::cout << "i="<<i << " p=" << this->pz(i - yk) << std::endl;
    }

    //    p1 = std::max(p1, (float)0.000001);

    if(p1 == 0) std::cerr << "WARNING: p1 = 0" << std::endl;

    p = p1 == 0 ? 0.5 : p0/p1;

    return p1 != 0;
}

float BitSymbolProvider::pqks(bool negative, int yk)
{

    int imax = (1 << x_binary_length()); //std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated()));
    float p0 = 0.0f;
    float p1 = 0.0f;

    for(int i = 0 ; i < imax ; i++)
    {

        if((i-yk) != 0 && (-i-yk) != 0)
        {
            p0 += this->pz(i-yk);
            if(i != 0) p1 += this->pz(-i-yk);
        }
        else if(negative)
        {
            p1 += this->pz(0);
        }
        else
        {
            p0 += this->pz(0);
        }
    }
    return p0/(p0+p1);
}
}
