#include "video_common.h"
#include "CommonDef.h"
#include "CommonLib/version.h"
#include "Rom.h"
#include "Utilities/program_options_lite.h"

void
icov::video::init_video_lib(int argc, char* argv[])
{
  std::cout << "VVCSoftware: VTM Encoder Version " << NEXT_SOFTWARE_VERSION
            << std::endl;

  fprintf(stdout, NVM_ONOS);
  fprintf(stdout, NVM_COMPILEDBY);
  fprintf(stdout, NVM_BITS);

#if ENABLE_SIMD_OPT // NEEDED FOR VVC
  std::string SIMD;
  df::program_options_lite::Options opts;
  opts.addOptions()("SIMD", SIMD, std::string(""), "")(
    "c", df::program_options_lite::parseConfigFile, "");
  df::program_options_lite::SilentReporter err;
  df::program_options_lite::scanArgv(opts, argc, (const char**)argv, err);
  fprintf(stdout, "[SIMD=%s] ", read_x86_extension(SIMD));
#endif
#if ENABLE_TRACING
  fprintf(stdout, "[ENABLE_TRACING] ");
#endif
  fprintf(stdout, "\n");

  // initialize global variables
  initROM(); // really necessary, but it should be called once
             // END NEEDED FOR VVC
}

std::string icov::video::chromaSubsamplingToStr(int value)
{
  switch (static_cast<ChromaFormat>(value))
  {

  case CHROMA_400:
      return "4:0:0";
  case CHROMA_420:
      return "4:2:0";
  case CHROMA_422:
      return "4:2:2";
  case CHROMA_444:
      return "4:4:4";
  case NUM_CHROMA_FORMAT:
  default:
      return "unknown";
  }
}
