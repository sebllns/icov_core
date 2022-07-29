#include "SymboleMode.h"

#include "TestSequenceInput.h"
#include "TestSequenceResult.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "tools/DebugTools.h"

#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <stdexcept>
#include <fstream>

namespace fs = boost::filesystem;

//estimate probability and rate
//    std::vector<std::pair<float,float>> sim_result;
//float estimated_rate = estimate_rate(b.src_bits[i], i < ly ? b.si_bits[i] : zero_vec, b.sim_result);
float estimate_rate(const BitVec &src, const BitVec &si, const std::vector<std::pair<float, float>> &sim_result)
{
    //    for(int i = 0 ; i < src.size() ; i++)
    //    {
    //        std::cout << src[i] << ", ";
    //    }

    //    std::cout << std::endl;

    //    for(int i = 0 ; i < si.size() ; i++)
    //    {
    //        std::cout << si[i] << ", ";
    //    }

    std::cout << std::endl;

    float r = 0.0;
    float p = 0.0;

    int lx = src.size();
    int ly = si.size();

    p = std::abs(lx-ly);

    for(int i = 0 ; i < std::min(lx,ly) ; i++)
    {
        if(src[i] != si[i])
        {
            p++;
        }
    }

    p /= src.size();

    //std::cout << "p error = " << p << std::endl;

    int i = 0;
    while(i < (int)sim_result.size() && sim_result[i].first < p)
    {
        i++;
    }

    //todo interpolation ?
    r = sim_result[i].second;

    std::cout << "estimated rate=" << r << std::endl;

    return r;
}



void SymboleMode::AddProgramOptions(boost::program_options::options_description &desc)
{
    desc.add_options()
            (dataset_dir_id.c_str(),   po::value<std::string>(),      "Input directoty")
            (reference_dir_id.c_str(),   po::value<std::string>(),      "Reference directoty")
            (result_dir_id.c_str(),   po::value<std::string>(),      "Result directoty")
            (max_si_count_id.c_str(),   po::value<int>()->default_value(8),      "Side info count")
            (enable_q_quantization_id.c_str(),   po::value<bool>()->default_value(true),      "Enable quantization of parameter q")
            (enable_z_quantization_id.c_str(),   po::value<bool>()->default_value(true),      "Enable quantization of parameters zmin, zmax")

            ;
}

SymboleMode::SymboleMode(const boost::program_options::variables_map &args):
    m_coder(CoderFactory::create_coder(args)),
    n(m_coder->syndromeMgr()->get_n()),
    bsp(0,0,0,0,0,0),
    predec(n, 0),
    stored_synd(n),
    lx(0),
    numcode(0)
{
    if(!args.count(dataset_dir_id))
    {
        throw std::invalid_argument(dataset_dir_id + " is missing");
    }

    if(!args.count(result_dir_id))
    {
        throw std::invalid_argument(result_dir_id + " is missing");
    }

    if(!args.count(reference_dir_id))
    {
        throw std::invalid_argument(reference_dir_id + " is missing");
    }

    m_dataset_dir = fs::absolute(args[dataset_dir_id].as<std::string>()).make_preferred();
    m_references_dir = m_dataset_dir / "results" / args[reference_dir_id].as<std::string>();
    m_result_dir = fs::absolute(args[result_dir_id].as<std::string>()).make_preferred();
    max_si_count = args[max_si_count_id].as<int>();
    enable_q_quantization = args[enable_q_quantization_id].as<bool>();
    enable_z_quantization = args[enable_z_quantization_id].as<bool>();

    q_values = {0.125, 0.375, 0.5625, 0.6875, 0.78125, 0.84375, 0.90625, 0.96875};

    helpers::path::assert_directory_exists(m_dataset_dir.string());
    helpers::path::assert_directory_exists(m_references_dir.string());
    helpers::path::try_create_directories(m_result_dir.string());
    helpers::path::assert_directory_exists(m_result_dir.string());
}

void SymboleMode::run()
{
    auto measure_path = fs::path(m_result_dir) / "metric.csv";
    measure_file = std::ofstream(measure_path.string());
    measure_file << "file\tsi\tabs diff\trelative diff\n";
    if(!measure_file.is_open())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("can't open measure file " + measure_path.string()));
    }
    fs::path seq_list = fs::path(m_dataset_dir) / "files.txt";
    process_input_list(seq_list.string());
    measure_file.close();
}

void SymboleMode::encode()
{
    auto res = std::vector<std::vector<float>>(lx, std::vector<float>(2, 0.0));
    for(int i = 0; i < lx; i++)
    {
        try
        {
            encode_bitplane(i, res);
        } catch (...)
        {
            std::cerr << "Error: Could not process bitplane " << i << std::endl;
            std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        }
    }
    rate_results.push_back(res);
}

bool SymboleMode::check_decoding(int i)
{
//    std::cout<<"Check decoding"<<std::endl;
    auto start = std::chrono::steady_clock::now();
    m_coder->decode(dec_bits, numcode, stored_synd);
    auto end = std::chrono::steady_clock::now();
    auto elapsed_seconds = end-start;
    if(!m_coder->testConvergence(dec_bits, src_bits[i]))
    {
        std::cerr<<"ERROR: Decoding check failed for bitplane "<< i <<std::endl;
        std::cerr<<"dec_bits - src_bits = ";
        for(size_t di = 0 ; di < dec_bits.size() ; di++)
        {
            std::cerr << dec_bits[di] -  src_bits[i][di] << " ";
        }
        std::cerr<<std::endl;
        return false;
    }

    return true;
}

void SymboleMode::fill_predec(int i)
{
    int bitplane = (lx - i - 1);

    //fill predecoded bit values
    for(int k = 0 ; k < n ; k++)
    {
        predec[k] = std::abs(src_symbols[k]) >> bitplane;
        if(i == (lx - 1))
        {
            int x_reconstructed = (1-2*src_bits[0][k]) * predec[k];
            if(x_reconstructed != src_symbols[k]) std::cerr<<"ERROR: X could not be reconstructed: " << x_reconstructed << " != " << src_symbols[k] <<std::endl;
        }
    }
}

void SymboleMode::write_results(const std::string& sequence_name)
{    
    TestSequenceResult tsr;
    tsr.read_from_file((m_references_dir / (sequence_name + ".mat")).string());

    std::ofstream result_file((fs::path(m_result_dir) / (sequence_name + ".txt")).string());

    for(int i = 0; i < 2; i++)
    {
        result_file << ((i == 0) ? "# estimated rate" : "# actual rate") << std::endl;
        for(auto &si : rate_results)
        {
            for(auto &bp : si)
            {
                result_file<< std::fixed << std::setw( 7 ) << std::setprecision( 6 )
                           << std::setfill( '0' ) << bp[i] << " ";
            }
            result_file << std::endl;
        }
        result_file << std::endl;
    }

    result_file <<"# Fangping's rate"  << std::endl;
    auto &frangping_rate = tsr["R"];
    for(int i = 0; i < std::min(max_si_count, frangping_rate.n_rows); i++)
    {
        for(int j = 0; j < frangping_rate.n_cols ; j++)
        {
            result_file<< std::fixed << std::setw( 7 ) << std::setprecision( 6 )
                       << std::setfill( '0' ) << frangping_rate.get_data(i,frangping_rate.n_cols - 1 - j) << " ";
        }
        result_file << std::endl;
    }

    result_file << std::endl;

//    if(p.enable_binarization_check)
//    {
//        result_file << "# X binarization" << std::endl;

//        for(int i = 0 ; i < p.N ; i++)
//        {
//            result_file << b.src_symbols[i] << "\t   ";
//            for(int j = 0 ; j < lx ; j++)
//            {
//                result_file << b.src_bits[j][i] << " ";
//            }
//            result_file << std::endl;
//        }
//    }


    result_file.close();


    auto syndromeMgr = m_coder->syndromeMgr();
    float min_rate = (1.0 * syndromeMgr->get_m(0))/syndromeMgr->get_n();
    for(int si = 0; si < rate_results.size() ; si++)
    {
        float icov_rate = 0.0, estim_rate = 0.0, fp_rate = 0.0;
        for(int bp = 0; bp < rate_results[si].size() ; bp++)
        {
            estim_rate += rate_results[si][bp][0];
            icov_rate += rate_results[si][bp][1];
            fp_rate += std::max(tsr["R"].get_data(si,frangping_rate.n_cols - 1 - bp), min_rate);
        }

        measure_file << sequence_name << "\t" << si + 1 << "\t" << std::fixed << std::setw( 7 ) << std::setprecision( 6 ) << std::setfill( '0' ) << (icov_rate - fp_rate) << "\t"<< (icov_rate - fp_rate)/fp_rate << std::endl;
    }

    measure_file.flush();
}

void SymboleMode::process_input_list(const std::string &file)
{
    std::ifstream seq_list(file);
    if(!seq_list.is_open()) BOOST_THROW_EXCEPTION(std::invalid_argument("cannot open file " + file));

    std::string sequence_name;
    while (std::getline(seq_list, sequence_name))
    {
        try
        {
            fs::path seq_path = fs::path(m_dataset_dir) / "sequences" / (sequence_name + ".txt");

            process_input_file(seq_path.string());
            write_results(sequence_name);
        }
        catch (...)
        {
            std::cerr << "Error: Could not process sequence file " << sequence_name << std::endl;
            std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        }
    }
    seq_list.close();
}

void SymboleMode::process_input_file(const std::string &file)
{
    std::cout << file << std::endl;

    rate_results.clear();

//    std::ifstream seq(file);
//    if(!seq.is_open()) BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot open file " + file));

    TestSequenceInput tsi(n);
    tsi.read_from_file(file);
    src_symbols = tsi.get_source();

    compute_src_bits();

    int si_count = std::min(tsi.get_side_info_count(), max_si_count);

    //encode for each SI
    for(int si = 0 ; si < si_count ; si++)
    {
        std::cout << "si=" << si+1 << "/" << si_count << std::endl;
        side_info = tsi.get_side_info(si);
        process_si();
        encode();
    }
}

void SymboleMode::compute_src_bits()
{
    //init bit planes of X
    src_bits = BitSymbolProvider::compute_bits(src_symbols);
    lx = src_bits.size();
}

void SymboleMode::process_si()
{
    init_bsp();
    compute_si_bits();
}

void SymboleMode::compute_si_bits()
{
    //get length for binary representation of Y
//    int ly =  std::floor(std::log2f(std::max(std::abs(y_min), std::abs(y_max)))) + 2;
//    ly = lx;

    int ly = lx;

    //init bit planes of Y
    si_bits = Bitplanes(ly, BitVec(n, 0));
    for(int i = 0 ; i < n ; i++)
    {

        si_bits[0][i] = (side_info[i] < 0);

        for(int j = 1 ; j < ly ; j++)
        {
            si_bits[ly-j][i] = (std::abs(side_info[i]) >> (j-1)) & 1U;
        }
    }
}

void SymboleMode::init_bsp()
{

    //init intrinsic SI parameters
    float q = 0.0;
    int z_min = 999999;
    int z_max = -999999;


    for(int i = 0 ; i < n ; i++)
    {
        int z = src_symbols[i] - side_info[i];

        //q = N0/N
        if(z == 0)
        {
            q += 1.0/n;
        }

        //z_min = min(Z[k], 0<=k<N)
        if(z < z_min) z_min = z;

        //z_max = max(Z[k], 0<=k<N)
        if(z > z_max) z_max = z;
    }

    //check zmax = xmax - ymin
    //check zmin = xmin - ymax

    if(DebugTools::debug_trace_enabled) DebugTools::logFile << "\t zmin=" << z_min << " zmax=" << z_max << std::endl;

    if(enable_z_quantization)
    {
        int z_step = 16;
        int z_limit = 1024;
        z_min = quantize(z_min, z_step, -z_limit, z_limit);
        z_max = quantize(z_max, z_step, -z_limit, z_limit);
        if(DebugTools::debug_trace_enabled) DebugTools::logFile << "\t After quantization zmin=" << z_min << " zmax=" << z_max << std::endl;
    }

    if(DebugTools::debug_trace_enabled) DebugTools::logFile << "\t q=" << q << std::endl;
    if(enable_q_quantization)
    {
        q = q_values[quantize(q, q_values)];
        if(DebugTools::debug_trace_enabled) DebugTools::logFile << "\t After quantization q=" << q << std::endl;
    }

    //y_min = min(Y[k], 0<=k<N)
    int y_min = *std::min_element(side_info.begin(), side_info.end());

    //y_max = max(Y[k], 0<=k<N)
    int y_max = *std::max_element(side_info.begin(), side_info.end());


    bsp = BitSymbolProvider(z_min, z_max, y_min, y_max, q, lx);

}

int SymboleMode::quantize(int value, int step, int min, int max)
{
    return value == 0 ? 0 : std::clamp((int)((std::abs(value)/value) * step * std::ceil((1.0 * std::abs(value)) / step)), min, max);
}

int SymboleMode::quantize(float value, const std::vector<float> &v)
{
    int idx = std::upper_bound(v.begin(), v.end(), value) - v.begin();
    idx = std::clamp(idx, 0, (int)v.size()-1);
    int idx_prev = std::clamp(idx-1, 0, (int)v.size()-1);

    if(std::abs(value - v[idx_prev]) <  std::abs(value - v[idx]))
    {
        idx = idx_prev;
    }
    return idx;
}

void SymboleMode::encode_bitplane(int i, std::vector<std::vector<float>>& result)
{
    auto llrProvider = m_coder->llrProvider();
    auto syndromeMgr = m_coder->syndromeMgr();
    auto matrixPvdr = m_coder->matrixProvider();

    //    syndrome accumulation
    syndromeMgr->init_accumulation(src_bits[i]);

    if(i == 0)
    {
        //get LLR for first bitplane level i.e. sign
        llrProvider->initLLR_s(bsp, side_info, src_bits[i]);
    }
    else
    {
        //get LLR for bitplane
        llrProvider->initLLR_b(i-1, bsp, side_info,  src_bits[0], predec, src_bits[i]);
    }

    //save LLR (in case of doping)
    llr = llrProvider->llr();

    numcode = 0;
    numerrors = m_coder->encode(numcode, stored_synd, src_bits[i]);

    double rate = ( ((double)syndromeMgr->get_m(matrixPvdr->getNumcode(numcode))) / ((double)n));
    float estimated_rate = 0.0f;
    result[i][0] = estimated_rate;
    result[i][1] = rate;

    //reset doping state and LLR
    syndromeMgr->reset_doping();
    for(int l = 0; l < n ; l++)
    {
        llrProvider->operator[](l) = llr[l];
    }

    if(numerrors != 0)
    {
        std::cerr<<"ERROR: No convergence for bitplane="<< i <<std::endl;
    }
    else
    {
        std::cout << "b=" << i <<" rate="<<rate<<std::endl;
        check_decoding(i);
    }

    if(i > 0) fill_predec(i);
}

const std::string SymboleMode::dataset_dir_id = "symbole-data-dir";
const std::string SymboleMode::reference_dir_id = "symbole-reference-dir";
const std::string SymboleMode::result_dir_id = "symbole-result-dir";
const std::string SymboleMode::max_si_count_id = "symbole-si-count";
const std::string SymboleMode::enable_q_quantization_id = "symbole-q-quant";
const std::string SymboleMode::enable_z_quantization_id = "symbole-z-quant";
