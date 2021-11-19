#include "ProgramController.h"
#include <iostream>

#include "BinaryMode.h"

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
                ;

        BinaryMode::AddProgramOptions(m_options_desc);
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
    auto mode = programMode();

    if(mode == "binary")
    {
        launchBinaryMode();
        std::cout << "end binary\n";
    }
    else if(mode == "symbole")
    {

    }
    else if(mode == "imt2alist")
    {

    }
    else
    {
        throw std::invalid_argument("Unknown program mode: " + mode);
    }
}

const std::string &ProgramController::programMode() const
{
    return m_options_map["mode"].as<std::string>();
}

const std::string &ProgramController::coderType() const
{
    return m_options_map["coder"].as<std::string>();
}

const std::string &ProgramController::dataPath() const
{
    return m_options_map["data-path"].as<std::string>();
}

int ProgramController::codeLength() const
{
    return m_options_map["code-length"].as<int>();
}

int ProgramController::bp_ite() const
{
    return m_options_map["bp-ite"].as<int>();
}

void ProgramController::launchBinaryMode()
{
    std::cout << "binary mode started" << std::endl;
    BinaryMode bm(m_options_map);
    bm.run();
}
