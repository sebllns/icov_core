#include "IMT164MatrixBuilder.h"

#include <cstdint>
#include <fstream>
#include <sstream>

#include "io/PcmData.h"

using namespace icov;

AffMatrix *IMT164MatrixBuilder::buildMatrix(int numcode) const
{
    int m = m_tools->get_m(numcode);
    int n = m_tools->get_n();

    AffMatrix *H = new AffMatrix(n, m);

    std::cout << "IMT164 " << numcode  << ", " << m << ", " << n << ", " << H->get_n_cols() << std::endl;

    int level_idx = m_tools->is_level(m);


    if(level_idx == m_tools->get_level_count()-1)
    {
        m_tools->current_level = level_idx;

        m_tools->h[level_idx] = H;

        PcmData::read_matrix(get_pcm(level_idx), *H);
        PcmData::read_mapping_vector(get_pcm(level_idx-1), m_tools->mapping_file[level_idx-1]);


        //std::cout << H->get_n_rows() << "x" << H->get_n_cols() << ": " << H->get_cols_max_degree() << ", " << H->get_rows_max_degree()<<std::endl;

        m_tools->h_dec.insert(m_tools->h_dec.begin(), H);

        m_tools->h_indexes = std::vector<int>(H->get_n_cols());
        std::iota(m_tools->h_indexes.begin(), m_tools->h_indexes.end(), 0);

        return  H;
    }

    int row_count = m_tools->get_m_level(m_tools->get_level(m_tools->current_level)) - m_tools->get_m(numcode);
    //std::cout << m_tools->current_level << ", " << row_count << std::endl;

    int row_dst = row_count - 1;
    auto &indexes = m_tools->h_indexes;

    int row_src1 = m_tools->mapping_file[m_tools->current_level-1].at(row_dst, 0);
    int row_src2 = m_tools->mapping_file[m_tools->current_level-1].at(row_dst, 1);

    std::vector<int> indexes2;

    int row1_idx = 0;
    int row2_idx = 0;

    for(int k = 0 ; k < (int)indexes.size() ; k++)
    {
        if(indexes[k] == row_src1)
        {
            row1_idx = k + row_dst;
        }
        else if(indexes[k] == row_src2)
        {
            row2_idx = k + row_dst;
        }
        else
        {
            indexes2.push_back(indexes[k]);
        }
    }



    for(int j = 0; j < (int)H->get_n_rows() ; j++)
    {
        int offset = 0;


        for(int i = 0; i < (int)H->get_n_cols() ; i++)
        {

            if(i == row_dst)
            {

                if(m_tools->h_dec[0]->at(j, row1_idx) ^ m_tools->h_dec[0]->at(j, row2_idx))
                {
                    H->add_connection(j,i);
                }

                offset--;
            }
            else
            {

                if(i < row_dst)
                {
                    offset = 0;
                }
                else if((i+offset) == row1_idx || (i+offset) == row2_idx)
                {
                    offset++;
                }


                if(m_tools->h_dec[0]->at(j, i + offset))
                {
                    H->add_connection(j,i);
                }

            }
        }
    }


    indexes = indexes2;



//    for(int j = 0 ; j < n ; j++)
//    {
//        //first part : accumulation until
//        for(int i = 0 ; i < row_count ; i++)
//        {

//            int row1 = m_tools->mapping_file[m_tools->current_level-1].at(i, 0);
//            int row2 = m_tools->mapping_file[m_tools->current_level-1].at(i, 1);

//            if(m_tools->h[m_tools->current_level]->at(j, row1) ^ m_tools->h[m_tools->current_level]->at(j, row2))
//            {
//                H->add_connection(j,i);
//            }
//        }

//        //second part : copy of current matrix
//        for(int i = row_count ; i < m ; i++)
//        {
//            if(m_tools->h[m_tools->current_level]->at(j,i)) H->add_connection(j,i);
//        }

        //interactive_coder->getMatrix(c).get_cols_max_degree() << ", " << interactive_coder->getMatrix(c).get_rows_max_degree()
  //  }

    if(level_idx > 0)
    {
        PcmData::read_mapping_vector(get_pcm(level_idx-1), m_tools->mapping_file[level_idx-1]);
    }


    if(m == m_tools->get_m_min())
    {
        std::vector<std::vector<index_t>> syndrome_index;

        int offset = 0;
        for(int k = 0 ; k < m_tools->get_level_count() ; k++)
        {
            int l = m_tools->get_level(k);

            int m_min = ((k== 0) ? (m_tools->get_m_level(l)) : (m_tools->get_m_level( m_tools->get_level(k-1)) + 1));

            int m_max =  m_tools->get_m_level(l) + 1;

            for(int ml =  m_min ; ml < m_max ; ml++ )
            {
                if(k == 0)
                {
                    syndrome_index.resize(ml);
                    for(int i = 0 ; i < ml ; i++ )
                    {
                        syndrome_index[i] = std::vector<index_t>(1, i+1);
                    }
                }
                else
                {
                    int m_diff = ml - (m_min - 1);
                    int m_previous = ml - 2 * m_diff;

                    auto erased = syndrome_index[m_previous];
                    syndrome_index.erase(syndrome_index.begin() + m_previous);

                    //C2
                    erased.push_back(ml);

                    //C1
                    auto inserted = std::vector<index_t>(1, ml);


                    //m_max-m_min
                    int crow = m_max-ml-1;
                    index_t c1 = m_tools->mapping_file[k-1].at(crow, 0);
                    index_t c2 = m_tools->mapping_file[k-1].at(crow, 1);

                    index_t c1pos = syndrome_index.size();
                    index_t c2pos = c1pos+1;

                    //c1pos < c2pos ? c2pos++ : c1pos++;

                    for(int c = m_max-ml-1 ; c < m_max-m_min ; c++)
                    {
                        index_t d1 = m_tools->mapping_file[k-1].at(c, 0);
                        index_t d2 = m_tools->mapping_file[k-1].at(c, 1);
                        if(c1 < d1) c1pos--;
                        if(c1 < d2 && d2 != c2) c1pos--;
                        if(c2 < d1) c2pos--;
                        if(c2 < d2) c2pos--;
                    }

                    syndrome_index.insert(syndrome_index.begin() + c1pos, inserted);
                    syndrome_index.insert(syndrome_index.begin() + c2pos, erased);

                    std::cout << syndrome_index.size() << std::endl;

                }


                m_tools->m_decoded_syndrome.push_back(syndrome_index);
            }
        }
    }

    //std::cout << H->get_n_rows() << "x" << H->get_n_cols() << ": " << H->get_cols_max_degree() << ", " << H->get_rows_max_degree()<<std::endl;

    if(level_idx >= 0)
    {
        m_tools->current_level = level_idx;
        m_tools->h[level_idx] = H;

        m_tools->h_indexes = std::vector<int>(H->get_n_cols());
        std::iota(m_tools->h_indexes.begin(), m_tools->h_indexes.end(), 0);
    }

    m_tools->h_dec.insert(m_tools->h_dec.begin(), H);

//    for(int i = 0; i < (int)H->get_n_cols() ; i++)
//    {
//        std::cout << i+1 << ":";

//        for(int j = 0; j < (int)H->get_n_rows(); j++)
//        {
//            if( H->at(j,i))
//            {
//                std::cout  << " " << j+1;
//            }
//        }
//        std::cout << std::endl;
//    }


    return  H;
}


std::string IMT164MatrixBuilder::get_pcm(int level_idx) const
{
    std::string filename = "H";
    if(level_idx < m_tools->get_level_count()-1)
    {
        filename  = "C1"+ std::to_string(m_tools->get_level(level_idx));
    }
    return root_dir + "/" + std::to_string(m_tools->get_n()) + "/" + filename + ".pcm";
}

