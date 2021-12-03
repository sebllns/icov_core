#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"
#include "SymboleMode.h"
#include "ExportMode.h"
#include "CheckMode.h"

ProgramController::ProgramController():
    m_options_desc("Program Usage", 1024, 512)
{
        m_options_desc.add_options()
                ("help,h",     "produce help message")
                ("mode,m",   po::value<std::string>()->required(),      "set the program mode")
                ("coder,c",   po::value<std::string>()->required(),      "set the coder type")
                ("data-path,d",   po::value<std::string>()->required(),             "set the data path")
                ("code-length,n",   po::value<int>()->required(),             "set the code length")
                ("bp-ite,it",   po::value<int>()->default_value(100),             "set iteration count for BP LDPC coder")
                ("numcodes",   po::value<std::string>()->default_value("numcodes.txt"),             "set the data path")
                ;

        BinaryMode::AddProgramOptions(m_options_desc);
        SymboleMode::AddProgramOptions(m_options_desc);
        ExportMode::AddProgramOptions(m_options_desc);
        CheckMode::AddProgramOptions(m_options_desc);
}

const boost::program_options::options_description &ProgramController::optionsDesc() const
{
    return m_options_desc;
}

void ProgramController::processCommandLine(int argc, char **argv)
{
    po::store(po::parse_command_line(argc, argv, m_options_desc), m_options_map);

    if (m_options_map.count("help"))
    {
        std::cout << m_options_desc << std::endl;
    }
    else
    {
        po::notify(m_options_map);
    }
}

void ProgramController::launchProgram()
{
    auto mode = programMode(m_options_map);

    if(mode == "binary")
    {
        std::cout << "binary mode started" << std::endl;
        BinaryMode mode(m_options_map);
        mode.run();
        std::cout << "end binary\n";
    }
    else if(mode == "symbole")
    {
        std::cout << "symbole mode started" << std::endl;
        SymboleMode mode(m_options_map);
        mode.run();
        std::cout << "end symbole\n";
    }
    else if(mode == "export")
    {
        std::cout << "export mode started" << std::endl;
        ExportMode mode(m_options_map, codeLength(m_options_map), dataPath(m_options_map));
        mode.run();
        std::cout << "end export\n";
    }
    else if(mode == "check")
    {
        std::cout << "check mode started" << std::endl;
        CheckMode mode(m_options_map, dataPath(m_options_map));
        mode.run();
        std::cout << "end check\n";
    }
    else
    {
        throw std::invalid_argument("Unknown program mode: " + mode);
    }

    std::cout << "end program\n";
}

const std::string &ProgramController::programMode(const po::variables_map &args)
{
    return args["mode"].as<std::string>();
}

const std::string &ProgramController::coderType(const po::variables_map &args)
{
    return args["coder"].as<std::string>();
}

const std::string &ProgramController::dataPath(const po::variables_map &args)
{
    return args["data-path"].as<std::string>();
}

int ProgramController::codeLength(const po::variables_map &args)
{
    return args["code-length"].as<int>();
}

int ProgramController::bp_ite(const po::variables_map &args)
{
    return args["bp-ite"].as<int>();
}

