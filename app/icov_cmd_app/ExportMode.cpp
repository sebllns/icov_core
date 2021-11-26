#include "ExportMode.h"
#include "interactive_coder/factory/CoderFactory.h"
#include "tools/DebugTools.h"

void ExportMode::AddProgramOptions(boost::program_options::options_description &desc)
{
    desc.add_options()
            (export_matrix_id.c_str(),   po::value<bool>()->default_value(false),  "Enable matrices export")
            (export_numcode_id.c_str(),   po::value<int>()->default_value(2),  "Enable numcode export / 0 = none, 1 = all, 2 = auto")
            (rate_min_id.c_str(),   po::value<float>(),  "rate min")
            (rate_max_id.c_str(),   po::value<float>()->default_value(0.5),  "rate max")
            ;

}


ExportMode::ExportMode(const boost::program_options::variables_map &args):
     m_coder(CoderFactory::create_coder(args))
{
    export_matrix_enabled = args[export_matrix_id].as<bool>();
    export_numcode_mode = args[export_numcode_id].as<int>();
}

void ExportMode::run()
{
    export_matrices();
    export_numcode();
}

void ExportMode::export_matrices()
{
    if(export_matrix_enabled)
    {

    }
}

void ExportMode::export_numcode()
{
    std::vector<int> numcodes;
    int n = m_coder->syndromeMgr()->get_n();

    switch (export_numcode_mode)
    {
    case 0:
        return;
    case 1:
        numcodes.resize((max_rate-min_rate) * n);
        std::iota(numcodes.begin(), numcodes.end(), 0);

        break;
    case 2:
        break;
    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument("Unknown numcode export mode: " + std::to_string(export_numcode_mode)));
    }

    DebugTools::write_vector("./numcodes.txt", numcodes, '\n');
}

const std::string ExportMode::rate_min_id = "export-min-rate";
const std::string ExportMode::rate_max_id = "export-max-rate";
const std::string ExportMode::export_matrix_id = "export-matrix";
const std::string ExportMode::export_numcode_id = "export-numcode";



