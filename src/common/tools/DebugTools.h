#ifndef DEBUGTOOLS_H
#define DEBUGTOOLS_H

#include <fstream>
#include <iostream>

#include <vector>
#include <string>


namespace icov {

class DebugTools
{
public:



    DebugTools()
    {

    }


    template<typename T> static void write_vector(const std::string& filename, const std::vector<T>& data, char sep )
    {
        std::ofstream file(filename);
        file << data.size();
        for(int i = 0; i < data.size(); i++)
        {
            file  << sep << data[i];
        }
        file.close();
    }


    template<typename T> static void read_vector(const std::string& filename,  std::vector<T>& data)
    {
        std::ifstream file(filename);
        int n = 0;
        file >> n;
        data = std::vector<T>(n);

        T val;
        for(int i = 0; i < data.size(); i++)
        {
            file  >> val;
            data[i] = val;
        }
        file.close();
    }

    template<typename T> static bool compare_vector(const std::vector<T>& v1, const std::vector<T>& v2)
    {
        if(v1.size() != v2.size())
        {
            std::cout << "different size\n";
            return false;
        }

        bool equal = true;

        for(int i = 0; i < v1.size() ; i++)
        {
            if(v1[i] != v2[i])
            {
                equal = false;
                std::cout << i << " ";
            }

        }


        std::cout << std::endl << "vectors are " << (equal ? "equal\n" : "not equal\n");


        return equal;
    }

    template<typename T> static void print_array(const T *arr, int n, const std::string& name)
    {
        std::cout << "\n\n********************************************************************************************\n" << name << "[";
        for (int i = 0; i < n - 1; i++)
        {
            std::cout << arr[i] << ", ";
        }

        std::cout << arr[n-1] << "]\n" << std::endl;

        //getchar();
    }

};

}
#endif // DEBUGTOOLS_H
