#include "Encoder.h"
#include <cmath>
#include <boost/filesystem.hpp>
#include <boost/gil.hpp>
#include <boost/mpl/vector.hpp>

#ifndef BOOST_GIL_NO_IO
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#endif

namespace fs = boost::filesystem;

using namespace boost::gil;

template <typename tag_t> using reader_t =
scanline_reader<typename get_read_device<const char*
                                                 , tag_t
                                                 >::type
                        , tag_t
                        >;

typedef reader_t<tiff_tag> reader_tiff;

typedef any_image<boost::mpl::vector<gray8_image_t, bgr8_image_t, argb8_image_t,
                                     rgb8_image_t, rgb8_planar_image_t,
                                     cmyk8_image_t, cmyk8_planar_image_t,
                                     rgba8_image_t, rgba8_planar_image_t> > any_image_t;

typedef rgba8_image_t image_t;

Encoder::Encoder()
{
  std::cout << "new encoder" << std::endl;
}

Encoder::~Encoder()
{
  std::cout << "destroy encoder" << std::endl;
}

void Encoder::openFile(const std::string &filename)
{
    rgb8_image_t imgRGB;


    fs::path filepath(filename);
    filepath = fs::complete(filepath);

    ICOV_ASSERT_MSG(fs::exists(filepath), "File not found: " + filepath.string());

    if(filepath.extension() == ".tif")
    {

    }
    else
    {
        ICOV_THROW_INVALID_ARGUMENT("Extension is not supported: " + filepath.extension().string());
    }
}

void Encoder::dispose()
{
}

void Encoder::encode(const IPicture &p)
{
}

void Encoder::encodeBlock(const Block &b)
{
}

void Encoder::encodeBlockWithPrediction(const Block &b, uint16_t prediction)
{
}
