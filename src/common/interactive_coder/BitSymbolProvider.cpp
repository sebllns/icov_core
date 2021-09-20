#include "BitSymbolProvider.h"

BitSymbolProvider::BitSymbolProvider(int z_min, int z_max, int y_min, int y_max, float q):
    z_min(z_min), z_max(z_max), y_min(y_min), y_max(y_max), q0(q)
{

}

float BitSymbolProvider::q1()
{
    return q0/(z_max - z_min);
}

int BitSymbolProvider::z0()
{
    return (z_min > 0) ? z_min : (z_max < 0 ? z_max : 0);
}

float BitSymbolProvider::pz(int z)
{
    return (z == z0()) ? q0 : ( (z < z_min || z > z_max) ? 0 : q1() );
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
    return std::ceil(std::log2f(std::max(std::abs(z_min), std::abs(z_max))));
}

int BitSymbolProvider::x_min_estimated()
{
    return z_max+ y_min;
}

int BitSymbolProvider::x_max_estimated()
{
    return z_min + y_max;
}

int BitSymbolProvider::x_binary_length()
{
    return std::ceil(std::log2f(std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated()))));
}

bool BitSymbolProvider::pqkb(bool negative, int abs_predec, int l, int b, int yk, float &p0, float &p1, float &p)
{
    int zkmin = this->z_min  - yk;
    int zkmax = this->z_max  - yk;

    int zk0 = abs_predec << (l-b);

    p0 = 0.0;
    p1 = 0.0;
    p = 0.0f;

    int zk1 =  zk0 + (1<<(l-b-1)) - 1;
    int zk2 =  zk0 + (1<<(l-b)) - 1;

    bool intersect = negative ?  (-zk1 <= zkmax && -zk0 >= zkmin) : (zk0 <= zkmax && zk1 >= zkmin);

    if(!intersect)
    {
        return false;
    }

    if(negative && (zk0+zk1)==0)
    {
        zk0 = 1;
        zk1 = 0;
        zk2 = 1;
    }
    else
    {
        zk0 = std::abs(std::clamp(negative ? std::min(-zk0, -1) : zk0, zkmin, zkmax));
        zk1 = std::abs(std::clamp(negative ? std::min(-zk1, -1) : zk1, zkmin, zkmax));
        zk2 = std::abs(std::clamp(negative ? std::min(-zk2, -1) : zk2, zkmin, zkmax));
    }

    int z0 = this->z0() - yk;
    int abs_z0 = std::abs(z0);

    p0 = (zk1 - zk0 + 1)*this->q1();
    p1 = (zk2 - zk0 + 1)*this->q1();

    if((negative == (z0 < 0)) && (abs_z0 >= zk0) && (abs_z0 <= zk2))
    {
        p1 += this->q0 - this->q1();
        if(abs_z0 <= zk1)
        {
            p0 += this->q0 - this->q1();
        }
    }

    p = p1 == 0 ? 0 : p0/p1;

    return true;
}

float BitSymbolProvider::pqks(int yk)
{
    int imax = std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated()));
    float p0 = 0.0f;
    for(int i = 0 ; i < imax ; i++)
    {
        p0 += this->pz(i - yk);
    }

    return p0;
}
