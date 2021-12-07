#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#ifndef COMMON_H
#error Include common.h not common_types.h
#endif

#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace icov
{

template <typename T>
class FastArray
{
public:

    FastArray(): m_capacity(0),
        m_size(0), m_data(nullptr){        std::cout<<"new empty " << m_size << std::endl;
}

    FastArray(const FastArray& f):
        m_capacity(f.m_capacity),
        m_size(f.m_size),
        m_data(new T[f.m_capacity])
    {
        //std::cout<<"new copy " << m_size << std::endl;
         std::memcpy(m_data, f.m_data, m_size * sizeof (T));
    }

    FastArray(size_t size):
        m_capacity(size),
        m_size(size),
        m_data(new T[size])
    {
        //std::cout<<"new size " << size << std::endl;

        //(T*)malloc(size * sizeof (T))
    }

   virtual ~FastArray()
    {
        //std::cout<<"delete\n";

        if(m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }
    }

    FastArray<T>& operator = (const FastArray<T> & f)
    {

        if(m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }
        this->m_capacity = f.m_capacity;
        this->m_size = f.m_size;
        m_data = new T [m_capacity];
        std::memcpy(m_data, f.m_data, m_size * sizeof (T));

        std::cout<<"copy " << m_capacity << " "  << m_size << std::endl;

        return *this;
    }

    T operator [] (int i) const {return m_data[i];}
    T& operator [] (int i) {return m_data[i];}

    T at(int i) const {return m_data[i];}
    T& at(int i) {return m_data[i];}

    void fill(T val)
    {
        for(int i = 0 ; i < this->size() ; i++)
        {
            this->m_data[i] = val;
        }
    }

    size_t size() const {return m_size;}
    size_t capacity() const {return m_capacity;}

    void resize(size_t new_size)
    {
        if(new_size > m_capacity)
        {
            T *tmp = m_data;
            m_data = new T[new_size];

            if(tmp != nullptr)
            {
                if(m_size > 0) std::memcpy(m_data, tmp, m_size * sizeof (T));
                delete[] tmp;
            }
            m_capacity = new_size;
        }
        m_size = new_size;
    }

    std::string to_string() const
    {
        std::stringstream sstr;

        sstr << "Array " << typeid (T).name()<< m_size << "; max" << m_capacity << std::endl;

        sstr << "[ ";

        for(int k = 0; k < m_size ; k++)
        {

             sstr << this->m_data[k] << ", ";
        }

        sstr << " ]"<< std::endl;

        return sstr.str();
    }


protected:
    T* m_data;
    size_t m_capacity;
    size_t m_size;
};

template <typename T>
class FastMatrix : public FastArray<T>
{
public:

    FastMatrix(): FastArray<T>(0),
        m_rows(0),
        m_cols(0)
    {

    }

     FastMatrix(size_t rows, size_t cols): FastArray<T>(rows*cols),
         m_rows(rows),
         m_cols(cols)
     {
         std::cout<<"new " << this->m_size << std::endl;

     }

     FastMatrix(const FastMatrix& f): FastArray<T>(f.m_rows*f.m_cols),
       m_rows(f.m_rows),
       m_cols(f.m_cols)
     {
         std::cout<<"new copy " << this->m_size << std::endl;
          std::memcpy(this->m_data, f.m_data, this->m_size * sizeof (T));
     }

     FastMatrix<T>& operator = (const FastMatrix<T> & f)
     {
         this->m_rows = f.m_rows;
         this->m_cols = f.m_cols;

         if(this->m_data != nullptr)
         {
             delete[] this->m_data;
             this->m_data = nullptr;
         }
         this->m_capacity = f.m_capacity;
         this->m_size = f.m_size;
         this->m_data = new T [this->m_capacity];
         std::memcpy(this->m_data, f.m_data, this->m_size * sizeof (T));

         std::cout<<"copy " << this->m_capacity << " "  << this->m_size << std::endl;

         return *this;

     }


     T at(int i, int j) const
     {
         return this->m_data[i*m_cols + j];
     }


     T& at(int i, int j)
     {
         //std::cout << i*m_cols + j << std::endl;
         return this->m_data[i*m_cols + j];
     }

     std::string to_string() const
     {
         std::stringstream sstr;

         sstr << "Matrix " << typeid (T).name()<< m_rows << "x" << m_cols << std::endl;

         for(int l = 0; l < m_cols ; l++)
         {
             sstr << "\t[" <<l << "]";
         }

         sstr << std::endl;
         sstr << std::endl;

         for(int k = 0; k < m_rows ; k++)
         {

             sstr << "[" <<k << "]\t";

             for(int l = 0; l < m_cols ; l++)
             {
                 sstr << this->m_data[k*m_cols + l] << '\t';
             }

             sstr << std::endl;
         }

         return sstr.str();
     }


      size_t rows() const {return m_rows;}
      size_t cols() const {return m_cols;}

      void resize(size_t new_size) = delete;

      void resize(size_t rows, size_t cols)
      {
          m_rows = rows;
          m_cols = cols;
          this->resize(rows*cols);
      }

protected:
    size_t m_rows;
    size_t m_cols;
};


class BitVector
{
    typedef unsigned char byte;
    typedef bool bit;


public:
    BitVector() = default;

    BitVector(size_t size, byte value = 0):
        m_actual_size(size)
    {
        size_t byte_count = m_actual_size/sizeOfByte() + (m_actual_size % sizeOfByte() != 0);

        m_data = std::vector<byte>(byte_count, value);
    }

    static size_t sizeOfByte() {return 8 * sizeof (byte);}

    size_t resize(size_t size)
    {
        m_actual_size = size;

        if(size > max_size())
        {
            size_t byte_count = m_actual_size/sizeOfByte() + (m_actual_size % sizeOfByte() != 0);
            m_data.resize(byte_count);
        }
        return max_size();
    }
    size_t size() const {return m_actual_size;}
    size_t max_size() const {return byte_count() * sizeOfByte();}
    size_t byte_count() const {return m_data.size();}

    bit operator [] (int i) const
    {
        return get_bit(i);
    }

//    byte& operator [] (int i)
//    {
//        return m_data[i];
//    }

    void get_bit_position(int bit_index, int &o_byte_index, int &o_bit_offset) const
    {
        o_byte_index = bit_index / sizeOfByte();
        o_bit_offset = bit_index % sizeOfByte();
    }

    byte& byte_at(int i){return m_data[i];}

    byte& getByteFromBitPos(int bit_index)
    {
        int byte_index = 0;
        int bit_offset = 0;
        get_bit_position(bit_index, byte_index, bit_offset);
        return this->m_data[byte_index];
    }


    byte getByteFromBitPos(int bit_index) const
    {
        int byte_index = 0;
        int bit_offset = 0;
        get_bit_position(bit_index, byte_index, bit_offset);
        return this->m_data[byte_index];
    }

    bit get_bit(int bit_index) const
    {
        int byte_index = 0;
        int bit_offset = 0;
        get_bit_position(bit_index, byte_index, bit_offset);

        return (this->m_data[byte_index] >> bit_offset) & 1;
    }

    float get_bit_value(int bit_index) const
    {
        return (get_bit(bit_index) ) ? 1.0 : 0.0;
    }

    void set(int bit_index, bit bit_val)
    {
        int byte_index = 0;
        int bit_offset = 0;
        get_bit_position(bit_index, byte_index, bit_offset);

        int mask = 1 << bit_offset;

        this->m_data[byte_index] = ((this->m_data[byte_index] & ~mask) | ((byte)bit_val << bit_offset));
    }

    void print() const
    {
        for(int i = 0; i < m_actual_size ; i++)
        {
            std::cout << get_bit(i) << " ";
        }
        std::cout << std::endl;
    }

protected:

    size_t m_actual_size;
    std::vector<byte> m_data;
};

typedef bool bit;
typedef float LLRVal;
typedef int CodedVal;
typedef int SIVal;
typedef bit SyndVal;
typedef uint16_t index_t;

//FastArray is faster
using Syndrome = FastArray<bit>;
using CodedVector = std::vector<CodedVal>;

template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T> &v,  std::vector<size_t>& idx)
{
  // initialize original index locations
  //std::vector<size_t> idx(v.size());
  //std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  std::stable_sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

  return idx;
}

template <typename T>
std::vector<size_t> sort_indexes_abs(const std::vector<T> &v,  std::vector<size_t>& idx)
{
  // initialize original index locations
  //std::vector<size_t> idx(v.size());
  //std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  std::stable_sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return std::fabs(v[i1]) < std::fabs(v[i2]);});

  return idx;
}

}

#endif // COMMON_TYPES_H
