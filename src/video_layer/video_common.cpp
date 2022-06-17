#include "video_common.h"
#include "CommonLib/version.h"

void icov::video::init_video_lib()
{
    std::cout << "VVCSoftware: VTM Encoder Version " << VTM_VERSION << std::endl;

#if ENABLE_SIMD_OPT //NEEDED FOR VVC
    std::string SIMD;
    df::program_options_lite::Options opts;
    opts.addOptions()
            ( "SIMD", SIMD, string( "" ), "" )
            ( "c", df::program_options_lite::parseConfigFile, "" );
    df::program_options_lite::SilentReporter err;
    df::program_options_lite::scanArgv( opts, argc, ( const TChar** ) argv, err );
    std::cout << "[SIMD=" << read_x86_extension( SIMD ) << "]" << std::endl;
#endif
    // initialize global variables
    initROM();  // really necessary, but it should be called once
    //END NEEDED FOR VVC
}
