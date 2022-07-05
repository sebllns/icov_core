/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#ifndef MYSTORAGE_H
#define MYSTORAGE_H
#include "CommonLib/CommonDef.h"
#include "CommonLib/Unit.h"
#include "video_layer/video_common.h"
#include "common.h"

using namespace icov::video;


template<typename T>
struct MyStorage : public UnitBuf<T>
{
private:
    struct StorageData
    {
        T* m_origin[MAX_NUM_COMPONENT];
        unsigned int _refCount;
        StorageData()
        {
            _refCount = 0;
            for( UInt i = 0; i < MAX_NUM_COMPONENT; i++ )
            {
              m_origin[i] = nullptr;
            }
        }

        void deallocate()
        {
//            std::cout << "deallocating MyStorage data" << std::endl;
            for( UInt i = 0; i < MAX_NUM_COMPONENT; i++ )
            {
              if( m_origin[i] )
              {
                xFree( m_origin[i] );
//                delete [] m_origin[i];
//                m_origin[i] = nullptr;
              }
            }
        }
        void allocate(UInt index, UInt size)
        {
            m_origin[index] = ( T* ) xMalloc( T, size );
//            m_origin[index] = new T[size];
        }
    };
    StorageData* m_d;
public:
    MyStorage() : m_d(nullptr)
    {

    }

    MyStorage(const MyStorage& other)  : m_d(nullptr)
    {
        (*this) = other;
    }

    MyStorage(MyStorage&& other)    // double ampersand "&&" declares an rvalue reference in c++11. it is provided for move semantics in move constructor
    {
        std::cout << "rvalue reference constructor is used in move constructor" << std::endl;
        std::cout << "if it didn't call operator=(MyStorage &&other) then I have to write if by myself" << std::endl;
        (*this) = other;
    }

    ~MyStorage()
    {
        destroy();
    }

    MyStorage& operator=(const MyStorage &other)
    {
        if( this != &other)
        {
            if(other.m_d && other.m_d->m_origin[0])
                other.m_d->_refCount++;
//                *const_cast<unsigned int *> (&other.m_d->_refCount) = other.m_d->_refCount + 1;
//                __atomic_fetch_add((unsigned*)(&other.m_d->_refCount), (unsigned)(1), __ATOMIC_ACQ_REL);


            release();
            this->m_d = other.m_d;
            this->bufs = other.bufs;
            this->chromaFormat = other.chromaFormat;

        }
        return *this;
    }

    MyStorage& operator=(MyStorage &&other) // double ampersand "&&" declares an rvalue reference in c++11. it is provided for move semantics in move constructor
    {
        std::cout << "rvalue reference constructor is used in operator=" << std::endl;

        if(this == &other)
            return *this;

        release();
        this->m_d = other.m_d;
        this->bufs = other.bufs;
        this->chromaFormat = other.chromaFormat;

        other.m_d = nullptr;
        other.chromaFormat = NUM_CHROMA_FORMAT;
        other.bufs.clear();

        return *this;
    }

    void destroy()
    {
        if(m_d && m_d->m_origin[0] && m_d->_refCount--==1) // checking pointer m_origin[0] is enough and more reliable because they always allocated together and somtimes depending on ChromaFormat m_origin[1], m_origin[2] may not be allocated
        {
            m_d->deallocate();

            if(this->m_d)
                delete this->m_d;
        }

        this->m_d = nullptr;
        this->chromaFormat = NUM_CHROMA_FORMAT;
        this->bufs.clear(); // TODO: shall I do this like deallocate?
    }

    void release()    { destroy();}

    bool empty() {return !m_d || !m_d->m_origin[0];}
    void create( const ChromaFormat &_chromaFormat, const Area& _lumaArea, const unsigned _maxCUSize = 0, const unsigned _margin = 0, const unsigned _alignment = 0, const bool _scaleChromaMargin = true )
    {
//        CHECK( !this->bufs.empty(), "Trying to re-create an already initialized buffer" );

        release();

        this->chromaFormat = _chromaFormat;

        const UInt numCh = getNumberValidComponents( _chromaFormat );

        unsigned extHeight = _lumaArea.height;
        unsigned extWidth  = _lumaArea.width;

        if( _maxCUSize )
        {
          extHeight = ( ( _lumaArea.height + _maxCUSize - 1 ) / _maxCUSize ) * _maxCUSize;
          extWidth  = ( ( _lumaArea.width  + _maxCUSize - 1 ) / _maxCUSize ) * _maxCUSize;
        }

        m_d = new StorageData();
        for( UInt i = 0; i < numCh; i++ )
        {
          const ComponentID compID = ComponentID( i );
          const unsigned scaleX = ::getComponentScaleX( compID, _chromaFormat );
          const unsigned scaleY = ::getComponentScaleY( compID, _chromaFormat );

          unsigned scaledHeight = extHeight >> scaleY;
          unsigned scaledWidth  = extWidth  >> scaleX;
          unsigned ymargin      = _margin >> (_scaleChromaMargin?scaleY:0);
          unsigned xmargin      = _margin >> (_scaleChromaMargin?scaleX:0);
          unsigned totalWidth   = scaledWidth + 2*xmargin;
          unsigned totalHeight  = scaledHeight +2*ymargin;

          if( _alignment )
          {
            // make sure buffer lines are align
            CHECK( _alignment != MEMORY_ALIGN_DEF_SIZE, "Unsupported alignment" );
            totalWidth = ( ( totalWidth + _alignment - 1 ) / _alignment ) * _alignment;
          }
          UInt area = totalWidth * totalHeight;
          CHECK( !area, "Trying to create a buffer with zero area" );

          m_d->allocate(i, area);
          T* topLeft = m_d->m_origin[i] + totalWidth * ymargin + xmargin;
          this->bufs.push_back( AreaBuf<T>( topLeft, totalWidth, _lumaArea.width >> scaleX, _lumaArea.height >> scaleY ) );
        }

        addref();
    }

    void create( const UnitArea &_UnitArea )
    {
        create( _UnitArea.chromaFormat, _UnitArea.blocks[0] );
    }

//    void createFromBuf( UnitBuf<      T> buf )
//    {
//        this->chromaFormat = buf.chromaFormat;

//        const UInt numCh = ::getNumberValidComponents( this->chromaFormat );

//        this->bufs.resize(numCh);

//        for( UInt i = 0; i < numCh; i++ )
//        {
//          AreaBuf<T> cBuf = buf.get( ComponentID( i ) );
//          this->bufs[i] = AreaBuf<T>( cBuf.bufAt( 0, 0 ), cBuf.stride, cBuf.width, cBuf.height );
//        }
//        // No need to set m_origin, becasue it is NOT a deep copy and the task of deleting the buffer at the end should be handled by buf itself
//    }

    AreaBuf<T>& getBuf( const ComponentID CompID )
    {
      return this->bufs[CompID];
    }

    const AreaBuf<T>& getBuf( const ComponentID CompID ) const
    {
      return this->bufs[CompID];
    }

    AreaBuf<T> getBuf( const CompArea &blk )
    {
      const AreaBuf<T>& r = this->bufs[blk.compID];

      CHECKD( rsAddr( blk.bottomRight(), r.stride ) >= ( ( r.height - 1 ) * r.stride + r.width ), "Trying to access a buf outside of bound!" );

      return AreaBuf<T>( r.buf + rsAddr( blk, r.stride ), r.stride, blk );
    }

    const AreaBuf<const T> getBuf( const CompArea &blk ) const
    {
      const AreaBuf<T>& r = this->bufs[blk.compID];
      return AreaBuf<const T>( r.buf + rsAddr( blk, r.stride ), r.stride, blk );
    }

    UnitBuf<T> getBuf( const UnitArea &unit )
    {
      return ( this->chromaFormat == CHROMA_400 ) ? UnitBuf<T>( this->chromaFormat, getBuf( unit.Y() ) ) : UnitBuf<T>( this->chromaFormat, getBuf( unit.Y() ), getBuf( unit.Cb() ), getBuf( unit.Cr() ) );
    }

    const UnitBuf<const T> getBuf( const UnitArea &unit ) const
    {
      return ( this->chromaFormat == CHROMA_400 ) ? UnitBuf<const T>( this->chromaFormat, getBuf( unit.Y() ) ) : UnitBuf<const T>( this->chromaFormat, getBuf( unit.Y() ), getBuf( unit.Cb() ), getBuf( unit.Cr() ) );
    }

private:

    void addref()
    {
        if(m_d)
            m_d->_refCount++;
    }
};

typedef AreaBuf<      bool>  BoolBuf;
typedef AreaBuf<const bool> CBoolBuf;
typedef UnitBuf<      bool>  BoolUnitBuf;
typedef UnitBuf<const bool> CBoolUnitBuf;
typedef MyStorage<    bool>  BoolStorage;
typedef MyStorage<    TCoeff>  TCoeffStorage;
typedef MyStorage<    TCoeff>  QTCoeffStorage;  // quantized transform coefficients
typedef MyStorage<    TCoeff>  IQTCoeffStorage;  // inverse quantized transform coefficients


/*
template<typename T>
struct SparseStorage
{

    typedef static_vector<cv::SparseMat_<T>,       MAX_NUM_COMPONENT> SparseUnitBufBuffers;

    ChromaFormat chromaFormat;
    SparseUnitBufBuffers sparseBufs;

    SparseStorage()
    {

    }

    void create( const ChromaFormat &_chromaFormat, const Area& _lumaArea)
    {
//        CHECK( !this->bufs.empty(), "Trying to re-create an already initialized buffer" );

        release();

        this->chromaFormat = _chromaFormat;

        const UInt numCh = getNumberValidComponents( _chromaFormat );

        for( UInt i = 0; i < numCh; i++ )
        {
          const ComponentID compID = ComponentID( i );
          const unsigned scaleX = ::getComponentScaleX( compID, _chromaFormat );
          const unsigned scaleY = ::getComponentScaleY( compID, _chromaFormat );

          int size[] = {_lumaArea.width >> scaleX,  _lumaArea.height >> scaleY};
          this->sparseBufs.push_back(cv::SparseMat_<T>( 2, size));
        }

    }
    void create( const UnitArea &_UnitArea )
    {
        create( _UnitArea.chromaFormat, _UnitArea.blocks[0] );
    }

    void destroy()
    {
        this->chromaFormat = NUM_CHROMA_FORMAT;
        this->sparseBufs.clear(); // TODO: shall I do this like deallocate?
    }

    void release()    { destroy();}

    cv::SparseMat_<T>& getBuf( const ComponentID CompID )
    {
      return this->sparseBufs[CompID];
    }

    const cv::SparseMat_<T>& getBuf( const ComponentID CompID ) const
    {
      return this->sparseBufs[CompID];
    }

          T& at( const ComponentID& CompID, const int &x, const int &y )          { return this->sparseBufs[CompID].ref(x,y); }
    const T& at( const ComponentID& CompID, const int &x, const int &y ) const    { return this->sparseBufs[CompID].ref(x,y); }


};
typedef SparseStorage<    double>  SparseTransformationBandProbabilityStorage;
*/
#endif // MYSTORAGE_H
