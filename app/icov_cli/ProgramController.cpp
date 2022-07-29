#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"
#include "SymboleMode.h"
#include "ExportMode.h"
#include "CheckMode.h"
#include "EncodeMode.h"
#include "helpers.h"

ProgramController::ProgramController():
    m_options_desc("Program Usage", 1024, 512)
{
        m_options_desc.add_options()
                ("help,h",     "produce help message")
                ("mode,m",   po::value<std::string>()->required(),      "set the program mode")
                ((std::string(OPT_CODER_TYPE)+",c").c_str(),   po::value<std::string>()->required(),      "set the coder type")
                ((std::string(OPT_DATA_PATH)+",d").c_str(),   po::value<std::string>()->required(),             "set the data path")
                ((std::string(OPT_CODE_LENGTH)+",n").c_str(),   po::value<int>()->required(),             "set the code length")
                ((std::string(OPT_BP_ITE)+",it").c_str(),   po::value<int>()->default_value(100),             "set iteration count for BP LDPC coder")
                (OPT_NUMCODES,   po::value<std::string>()->default_value("numcodes.txt"),             "set the numcodes steps for iterative coder")
                ;

        BinaryMode::AddProgramOptions(m_options_desc);
        SymboleMode::AddProgramOptions(m_options_desc);
        ExportMode::AddProgramOptions(m_options_desc);
        CheckMode::AddProgramOptions(m_options_desc);
        EncodeMode::AddProgramOptions(m_options_desc);
}

const boost::program_options::options_description &ProgramController::optionsDesc() const
{
    return m_options_desc;
}

void ProgramController::processCommandLine(int argc, char **argv)
{
    po::store(po::parse_command_line(argc, argv, m_options_desc), m_options_map);
}

void ProgramController::launchProgram()
{
    if (m_options_map.count(OPT_HELP))
    {
        std::cout << m_options_desc << std::endl;
        return;
    }
    else
    {
        po::notify(m_options_map);
    }

    auto mode_id = programMode(m_options_map);

    if(mode_id == "binary")
    {
        std::cout << "binary mode started" << std::endl;
        BinaryMode mode(m_options_map);
        mode.run();
        std::cout << "end binary\n";
    }
    else if(mode_id == "symbole")
    {
        std::cout << "symbole mode started" << std::endl;
        SymboleMode mode(m_options_map);
        mode.run();
        std::cout << "end symbole\n";
    }
    else if(mode_id == "export")
    {
        std::cout << "export mode started" << std::endl;
        ExportMode mode(m_options_map, codeLength(m_options_map), dataPath(m_options_map));
        mode.run();
        std::cout << "end export\n";
    }
    else if(mode_id == "check")
    {
        std::cout << "check mode started" << std::endl;
        CheckMode mode(m_options_map, dataPath(m_options_map));
        mode.run();
        std::cout << "end check\n";
    }
    else if(mode_id == "encode")
    {
        std::cout << "encode mode started" << std::endl;
        EncodeMode mode(m_options_map);
        mode.run();
        std::cout << "end encode\n";
    }
    else
    {
        ICOV_THROW_INVALID_ARGUMENT("Unknown program mode: " + mode_id);
    }

    std::cout << "end program\n";
}

const std::string &ProgramController::programMode(const po::variables_map &args)
{
    return args["mode"].as<std::string>();
}

const std::string &ProgramController::coderType(const po::variables_map &args)
{
    return args[OPT_CODER_TYPE].as<std::string>();
}

const std::string &ProgramController::dataPath(const po::variables_map &args)
{
    return args[OPT_DATA_PATH].as<std::string>();
}

int ProgramController::codeLength(const po::variables_map &args)
{
    return args[OPT_CODE_LENGTH].as<int>();
}

int ProgramController::bp_ite(const po::variables_map &args)
{
    return args[OPT_BP_ITE].as<int>();
}

