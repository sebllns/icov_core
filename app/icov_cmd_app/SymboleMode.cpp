#include "SymboleMode.h"

#include "TestSequenceInput.h"
#include "TestSequenceResult.h"
#include "interactive_coder/factory/CoderFactory.h"
#include "tools/DebugTools.h"

#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <stdexcept>


namespace fs = boost::filesystem;


void SymboleMode::AddProgramOptions(boost::program_options::options_description &desc)
{
    desc.add_options()
            (dataset_dir_id.c_str(),   po::value<std::string>(),      "Input directoty")
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
    stored_synd(n)
{
    if(!args.count(dataset_dir_id))
    {
        throw std::invalid_argument(dataset_dir_id + " is missing");
    }

    if(!args.count(result_dir_id))
    {
        throw std::invalid_argument(result_dir_id + " is missing");
    }

    m_dataset_dir = args[dataset_dir_id].as<std::string>();
    m_result_dir = args[result_dir_id].as<std::string>();
    max_si_count = args[max_si_count_id].as<int>();
    enable_q_quantization = args[enable_q_quantization_id].as<bool>();
    enable_z_quantization = args[enable_z_quantization_id].as<bool>();

    q_values = {0.125, 0.375, 0.5625, 0.6875, 0.78125, 0.84375, 0.90625, 0.96875};
}

void SymboleMode::run()
{
    fs::path seq_list = fs::path(m_dataset_dir) / "files.txt";
    process_input_list(seq_list.string());
}

void SymboleMode::encode()
{   
    for(int i = 0; i < lx; i++)
    {
        try
        {
            encode_bitplane(i);
        } catch (...)
        {
            std::cerr << "Error: Could not process bitplane " << i << std::endl;
            std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        }
    }
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
    }
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

void SymboleMode::write_results()
{

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
            sequence_name += ".txt";

            fs::path seq_path = fs::path(m_dataset_dir) / "sequences" / sequence_name;

            process_input_file(seq_path.string());

        }
        catch (...)
        {
            std::cerr << "Error: Could not process sequence file " << sequence_name << std::endl;
            std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        }
    }
    seq_list.close();
}

void SymboleMode::process_input_file(const string &file)
{
    std::cout << file << std::endl;

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
    //find x_min and x_max values in source X
    int x_min = *std::min_element(src_symbols.begin(), src_symbols.end());
    int x_max = *std::max_element(src_symbols.begin(), src_symbols.end());

    //get length for binary representation of X
    lx =  std::floor(std::log2f(std::max(std::abs(x_min), std::abs(x_max)))) + 2;

    //init bit planes of X
    src_bits = std::vector<std::vector<CodedVal>>(lx, std::vector<CodedVal>(n, 0));

    for(int i = 0 ; i < n ; i++)
    {
        int x_reconstruction = 0;
        src_bits[0][i] = (src_symbols[i] < 0);

        for(int j = 1 ; j < lx ; j++)
        {
            src_bits[lx-j][i] = (std::abs(src_symbols[i]) >> (j-1)) & 1U;
            x_reconstruction |= src_bits[lx-j][i] << (j-1);
        }

        //sanity check for X binarization
        x_reconstruction *= (1 - 2*src_bits[0][i]);
        if(x_reconstruction != src_symbols[i]) throw std::runtime_error("ERROR: Binarization failed: " + std::to_string(x_reconstruction) + " != " + std::to_string(src_symbols[i]));
    }
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
    si_bits = std::vector<std::vector<CodedVal>>(ly, std::vector<CodedVal>(n, 0));
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

void SymboleMode::encode_bitplane(int i)
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

    //reset doping state and LLR
    syndromeMgr->reset_doping();
    for(int l = 0; l < n ; l++)
    {
        llrProvider->operator[](l) = llr[l];
    }

    //rate_results[si][i][1] = rate;

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
const std::string SymboleMode::result_dir_id = "symbole-result-dir";
const std::string SymboleMode::max_si_count_id = "symbole-si-count";
const std::string SymboleMode::enable_q_quantization_id = "symbole-q-quant";
const std::string SymboleMode::enable_z_quantization_id = "symbole-z-quant";
