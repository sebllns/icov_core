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

        for(int z = zmin ; z <= zmax ; z++)
        {
            p+=this->pz(z);
        }

//        int n = zmax - zmin + 1;
//        if(zmin == z0())
//        {
//            p += q0;
//            n--;
//        }

//        p += n * q1();
    }

    //L
    int z_binary_length()
    {
        return std::ceil(std::log2f(std::max(std::abs(z_min), std::abs(z_max))));
    }


    int x_min_estimated()
    {
         return z_max+ y_min;
    }

    int x_max_estimated()
    {
         return z_min + y_max;
    }



    int x_binary_length()
    {
        return std::ceil(std::log2f(std::max(std::abs(x_min_estimated()), std::abs(x_max_estimated()))));
    }

//    int clamp_abs(int z, bool negative)
//    {
//        if(negative && z_min >= 0) return z_min;

//        if(negative)
//        {

//        }
//        else
//        {
//            return std::max(z_min, std::min(z, z_max));
//        }
//    }

    //pqk = 0
    float pqk0(int b, int yk)
    {

        std::cout << "\n-- b=" << b << std::endl;

        float p = 0.0;
        int l = this->z_binary_length();//-1
        float pb = 0.0f;

        int zbmax =  (1 << b);
        int zbmin =-zbmax; //std::max(this->z_min, -zbmax);
        zbmax = std::min(this->z_max, zbmax);


        int zkmin = this->z_min  - yk;
        int zkmax = this->z_max  - yk;

        //std::cout  << zbmin << ", " << zbmax << std::endl;

        boost::dynamic_bitset<>  tmp;

        for(int k = zbmin ; k < zbmax ; k++)
        {
            int abs_predec = std::abs(k);
            bool negative = k < 0;

            boost::dynamic_bitset<>  bin_predec = boost::dynamic_bitset<> (b+1, abs_predec);


            bin_predec.set(b, 0);
            abs_predec = (int) bin_predec.to_ulong();


            if(negative) bin_predec.set(b);
//            std::cout << "dec=" <<  bin_predec << std::endl;




            float p0 = 0.0f, p1 = 0.0f, p = 0.0f;


            int zk0 = abs_predec << (l-b);

//            if((zk0 * (negative ? -1 : 1)) > zkmax)
//            {
//                continue;
//            }


            int zk1 =  zk0 + (1<<(l-b-1)) - 1;
            int zk2 =  zk0 + (1<<(l-b)) - 1;

            bool intersect = negative ?  (-zk1 <= zkmax && -zk0 >= zkmin) : (zk0 <= zkmax && zk1 >= zkmin);


            if(!intersect)
            {

//                std::cout << "***IMPOSSIBLE***" << std::endl;
                continue;
            }



//            tmp = boost::dynamic_bitset<> (l, zk0);
//            std::cout << "zk0=" <<  tmp << std::endl;

//            tmp = boost::dynamic_bitset<> (l, zk1);
//            std::cout << "zk1=" <<  tmp << std::endl;

//            tmp = boost::dynamic_bitset<> (l, zk2);
//            std::cout << "zk2=" <<  tmp << std::endl;

//            std::cout << k << ": "  << zk0 << ", " << zk1 << ", " << zk2 << std::endl;

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

//                        tmp = boost::dynamic_bitset<> (l, zk0);
//                        std::cout << "zk0=" <<  tmp << std::endl;

//                        tmp = boost::dynamic_bitset<> (l, zk1);
//                        std::cout << "zk1=" <<  tmp << std::endl;

//                        tmp = boost::dynamic_bitset<> (l, zk2);
//                        std::cout << "zk2=" <<  tmp << std::endl;

//            std::cout << k << ": "  << zk0 << ", " << zk1 << ", " << zk2 << std::endl;

            int z0 = this->z0() -  yk;
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


//            for(int z = zk0 ; z <= zk2 ; z++)
//            {
//                if(!(negative && z == 0))
//                {

//                    p1 += pz((negative ? -1 : 1)*z-yk);
//                    if(z <= zk1)
//                    {
//                        p0 += pz((negative ? -1 : 1)*z-yk);
//                    }
//                }
//            }

            //std::cout << boost::dynamic_bitset<> (b, z0_mask) << std::endl;

            /*int z0_mask = z0 >> (l-b);
            if(abs_predec == z0_mask)
            {
                p1 += this->q0 - this->q1();

                //bool bit_b = ((z0 >> (l-b)) & 1U) == 0;

                if( (z0 >> (l-b-1)) == (abs_predec << 1))
                {
                    p0 += this->q0 - this->q1();
                }
            }*/


            p = p1 == 0 ? 0 : p0/p1;
            //std::cout << "-- p1=" << p1 << std::endl;
            pb += p1;

            if(negative) bin_predec.set(b);
            std::cout << "--- decoded=" <<  bin_predec << "\tp0=" <<  p << std::endl;
        }
        std::cout << "-- pb=" << pb << std::endl;

        if(pb != 1)
        {
            std::cerr << "**ERROR**" << std::endl;
            getchar();
        }


        return p;
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

    std::vector<int> x(n);
    std::vector<int> y(n);
    std::vector<int> z(n);

    z_min = x_max - y_min;
    z_max = x_min - y_max;

    for(int i = 0 ; i < n ; i++)
    {
        x[i] = rand()%(x_max-x_min+1) + x_min;
        y[i] = rand()%(y_max-y_min+1) + y_min;
        z[i] = x[i] - y[i];

        if(z[i] < z_min)
        {
            z_min = z[i];
        }

        if(z[i] > z_max)
        {
            z_max = z[i];
        }
    }



    bitsymbol bs;
    bs.z_min = z_min;
    bs.z_max = z_max;
    bs.y_min = y_min;
    bs.y_max = y_max;

    bs.q0 = q0;



    /*******************************************************/
    //PQ(k)|y
    std::vector<float> pqs0(n);
    std::vector<float> llr_s(n);

    std::vector<std::vector<float>> pqb0(n);
    std::vector<float> llr_b(n);

    int l = bs.z_binary_length();
    int imax = pow(2, bs.x_binary_length());

    for(int k = 0 ; k < n ; k++)
    {
        for(int i = 0 ; i < imax ; i++)
        {
            //std::cout << bs.pz(i - y[k]) << std::endl;
            pqs0[k] += bs.pz(i - y[k]);
        }

         llr_s[k] = std::log(pqs0[k] / (1.0 - pqs0[k]));



         int yk = y[k];

          std::cout << "- y="  << yk << std::endl;

         for(int b = 0 ; b < l ; b++)
         {
             bs.pqk0(b, yk);
         }
    }




    auto end_main = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_main-start_main;

    std::cout << "*************************************************************************" << std::endl;
    std::cout << "END OF SIMULATION\n";
    std::cout << "Duration:\t" << elapsed_seconds.count()/60 << " mins"<<  std::endl;
    std::cout << "*************************************************************************" << std::endl;

    return 0;
}
