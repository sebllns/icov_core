#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include <aff3ct.hpp>

#include "common.h"
#include "tools/DebugTools.h"
#include <bitset>
#include <boost/dynamic_bitset.hpp>

using namespace aff3ct;
using namespace icov;

/****************************************************************************************/
// I - AFF3CT CONFIG & STRUCTS
/****************************************************************************************/
struct params
{

};

void init_params(params &p, int argc, char* argv[])
{

}

struct node
{
    //val (0/1)
    //child 0
    //child 1

    node(node* pchild_0, node* pchild_1):
        child_0(pchild_0), child_1(pchild_1)
    {

    };

    bool is_leaf()
    {
        return child_0 != nullptr || child_1 != nullptr;
    }

    node* child_0;
    node* child_1;
};


struct bitsymbol
{
    int z_min, z_max;
    int y_min, y_max;

    //q
    float q0;

    //q'
    float q1()
    {
        return q0/(z_max - z_min);
    }

    //z associated with q0 = min(|zk|)
    int z0()
    {
        return (z_min > 0) ? z_min : (z_max < 0 ? z_max : 0);
    }

    //..0..0 [q'..q' q q'..q'] 0..0..
    float pz(int z)
    {
        return (z == z0()) ? q0 : ( (z < z_min || z > z_max) ? 0 : q1() );
    }

    float pz(int zmin, int zmax)
    {
        float p = 0;
        int n = zmax - zmin + 1;
        if(zmin == z0())
        {
            p += q0;
            n--;
        }

        p += n * q1();
    }

    //L
    int binary_length()
    {
        return std::ceil(std::log2f(std::max(std::abs(z_min + y_min), std::abs(z_max + y_max))));
    }
};

/****************************************************************************************/
// III - MAIN
/****************************************************************************************/
int main(int argc,  char* argv[])
{
    auto start_main = std::chrono::steady_clock::now();

    srand((unsigned)time(NULL));

    /****************************************************************************************/
    // 1 - INIT
    /****************************************************************************************/

    params p;
    init_params(p, argc, argv);

    int x_min = 0;
    int x_max = 7;
    int y_min = 0;
    int y_max = 3;

    int n = 16;

    float q0 = 0.5; //0 ; 0.25 ; 0.5 ; 0.75 ; 1 ; q = #0/n

    int z_min = x_min - y_max;
    int z_max = x_max - y_min;

    bitsymbol bs;
    bs.z_min = z_min;
    bs.z_max = z_max;
    bs.y_min = y_min;
    bs.y_max = y_max;

    bs.q0 = q0;

    std::vector<int> y(n);

    for(int i = 0 ; i < n ; i++)
    {
        y[i] = rand()%(y_max-y_min+1) + y_min;
    }

    boost::dynamic_bitset<> predecoded;
    int l = bs.binary_length() - 1;
    for(int b = 1 ; b < l ; b++)
    {
        predecoded.resize(b);
        predecoded.reset();
        for(int k = 0 ; k < (1 << b) ; k++)
        {

            predecoded = boost::dynamic_bitset<> (b, k);
            std::cout << "decoded=" <<  predecoded << std::endl;
            std::cout << "calculate p for "  << b+1 << "th bit is 0" << std::endl;
            float p0 = (1<<(l-b-1))*bs.q1();
            float p1 = (1<<(l-b))*bs.q1();

            int z0 = 4;
            int z0_mask = z0 >> (l-b);

            //std::cout << boost::dynamic_bitset<> (b, z0_mask) << std::endl;

            if(k == z0_mask)
            {
                p1 += bs.q0 - bs.q1();

                bool bit_b = ((z0 >> (l-b)) & 1U) == 0;

                if( (z0 >> (l-b-1)) == (k << 1))
                {
                    p0 += bs.q0 - bs.q1();
                }
            }

            float p = p0/p1;
            std::cout << "p=" <<  p << std::endl;
        }
    }

    //PQ(k)|y
    std::vector<float> pqs0(n);
    std::vector<float> llr_s(n);

    std::vector<std::vector<float>> pqb0(n);
    std::vector<float> llr_b(n);

    int imax = pow(2, bs.binary_length());

    for(int k = 0 ; k < n ; k++)
    {
        for(int i = 0 ; i < imax ; i++)
        {
            //std::cout << bs.pz(i - y[k]) << std::endl;
            pqs0[k] += bs.pz(i - y[k]);
        }

         llr_s[k] = std::log(pqs0[k] / (1.0 - pqs0[k]));

         y[k] = 0;

         int zymin =  bs.z_min - y[k];
         zymin = (zymin <  bs.z_min) ? bs.z_min : (zymin > bs.z_max) ? bs.z_max : zymin;

         int zymax =  bs.z_max - y[k];
         zymax = (zymax <  bs.z_min) ? bs.z_min : (zymax > bs.z_max) ? bs.z_max : zymax;

         int zy0 = bs.z0() - y[k];

         if(zymax < zymin) throw std::runtime_error("zymax < zymin");

         int z1min = zymin;
         int z1max = 0, z2min = 0, z2max = 0;
         int n1 = 0, n2 = 0, t1 = 0, t2 = 0;
         int b = 0;

         int next_z1min = 0;

         //std::cout << std::bitset<32>(-3).to_string() << std::endl;
         std::cout << std::bitset<32>(3).to_string() << std::endl;
         std::cout << std::bitset<32>((~0 << (b+1))).to_string() << std::endl;
         std::cout << std::bitset<32>(3 & (~0 << (b+1))).to_string() << std::endl;

         while (z1min < zymax)
         {
             if(z1min < 0)
             {
                 z1max = std::min(zy0-1, -(std::abs(z1min) & (~0 << (b+1))));

                 //from z1min set b last bits to 0 to get z2min
                 z2min = -(std::abs(z1min) & (~0 << (b+1)));

                 //from z2min set b-1 last bits to 1 to get z2max
                 z2max = std::min(zy0-1, -(std::abs(z2min) | ((1 << b)-1)));

                 t1 = 0;
                 t2 = 0;


                 //next zmin is set to b bit as 0 then b-1 last bits all 1
                 //next_z1min =
             }
             else
             {
                 t1 = z1min == zy0 ? 1 : 0;
                 t2 = z2min == zy0 ? 1 : 0;
             }

             n1 = z1max - z1min + 1 - t1;
             n2 = z2max - z2min + 1 - t2;

             float p0 = (t2 * bs.q0 + n2 * bs.q1()) / (t1 * bs.q0 + n1 * bs.q1());

             std::cout << "k=" << k << ", z1min=" << z1min << ", z1max=" << z1max << ", z2min=" << z2min << ", z2max=" << z2max << ", p0=" << p0 << std::endl;

             //when b+1 bit is already 0 increment b
             bool bit_b = ((z1min >> (b+1)) & 1U) == 1;
             if(!bit_b)
             {
                 b++;
             }

            // z1min = next_z1min;
             z1min = z1max + 1;
         }

         /*for(int j = 0 ; j < bs.binary_length() ; j++)
         {
             int b = bs.binary_length() - 1 - j;
             std::vector<float> &pqkb0 = pqb0[k];


         }*/
    }




    auto end_main = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_main-start_main;

    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF SIMULATION\n";
    std::cout << "Duration:\t" << elapsed_seconds.count()/60 << " mins"<<  std::endl;
    std::cout << "*************************************************************************" << std::endl;

    return 0;
}
