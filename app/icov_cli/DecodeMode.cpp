#include "DecodeMode.h"
#include "encoder/DemoEncoder.h"
#include "encoder/FakeEncoder.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/implementation/EquiRectPicture.h"
#include "omnidirectional_layer/implementation/IcovAccessBlockManager.h"
#include "tools/DebugTools.h"
#include <boost/format.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>

const char *DecodeMode::params::inputFile_id = "decode-input-file";

void DecodeMode::AddProgramOptions(
    boost::program_options::options_description &desc) {
  desc.add_options()

      (DecodeMode::params::inputFile_id,
       po::value<std::string>()->default_value("/tmp/video.mp4"),
       "File to decode");
}

DecodeMode::DecodeMode(const po::variables_map &args) {

  m_parameters.inputFile =
      args[DecodeMode::params::inputFile_id].as<std::string>();

  //    CoderFactory::create_coder_yuv(args, m_ycoder, m_uvcoder);

  ICOV_INFO << boost::format("%-15s %-s") % "Input File:" %
                        m_parameters.inputFile;
}

struct icov_header {
  std::string icov_version;
  std::string coder_type;
  bool has_metadata;
  float content_framerate;
  bool is_sov_fixed_size;

  friend class boost::serialization::access;
  // When the class Archive corresponds to an output archive, the
  // & operator is defined similar to <<.  Likewise, when the class Archive
  // is a type of input archive the & operator is defined similar to >>.
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &icov_version;
    ar &coder_type;
    ar &has_metadata;
    ar &content_framerate;
    ar &is_sov_fixed_size;
  }
};

void DecodeMode::run() {

  std::string filename = "dummy.icov";

  {
    std::ofstream ofs(filename);
    boost::archive::binary_oarchive o_archive(ofs, boost::archive::no_header);

    u_int16_t a = 1234;
    u_int16_t b = 5678;
    std::vector<int> vec = {-111, 222, -333, 444, -555};

    icov_header header;
    header.icov_version = "icov" ICOV_CORE_VERSION;
    header.content_framerate = 12.8455;
    header.coder_type = "imt164";
    header.has_metadata = false;
    header.is_sov_fixed_size = true;

    o_archive << header;
    o_archive << a;
    o_archive << b;
    o_archive << vec;
  }

  {

    std::ifstream ifs(filename);
    boost::archive::binary_iarchive i_archive(ifs, boost::archive::no_header);

    icov_header header;
    i_archive >> header;

    uint16_t word1, word2;
    i_archive >> word1 >> word2;
    std::cout << word1 << "," << word2 << std::endl;

    std::vector<int> vec2;
    i_archive >> vec2;

    icov::helpers::io::write_vector(std::cout, vec2, ' ');

    std::cout << std::endl;
  }
}
