#include "enc_helpers.h"


void resize_covering_blocks(const cv::Mat &in_img, const cv::Size &blockSize, cv::Mat &out_img, bool keepAspectRation)
{
    cv::Size resizingSize = in_img.size();

    if(keepAspectRation)
    {
        if(in_img.cols <= in_img.rows)
        {
            if(resizingSize.width % blockSize.width != 0)
                resizingSize.width -= resizingSize.width % blockSize.width;
            resizingSize.height = round(double(resizingSize.width) * (double(in_img.rows)/double(in_img.cols)));
        }
        else
        {
            if(resizingSize.height % blockSize.height != 0)
                resizingSize.height -= resizingSize.height % blockSize.height;
            resizingSize.width = round(double(resizingSize.height) * (double(in_img.cols)/double(in_img.rows)));
        }

    }
    else
    {

        if(resizingSize.width % blockSize.width != 0)
            resizingSize.width -= resizingSize.width % blockSize.width;

        if(resizingSize.height % blockSize.height != 0)
            resizingSize.height -= resizingSize.height % blockSize.height;
    }

    if( (resizingSize.width % blockSize.width != 0) || (resizingSize.height % blockSize.height != 0))
        throw std::logic_error("Something is not correct in resizing calculation");

    cv::resize(in_img, out_img, resizingSize, 0, 0, cv::INTER_AREA);
}

void resize_covering_blocks(const cv::Mat &in_img, const cv::Size &blockSize, cv::Mat &out_img)
{
    //We have seen some equi images which their aspect ratio is not integer (have decimal number like 2.36364);
    resize_covering_blocks(in_img, blockSize, out_img, ((in_img.cols % in_img.rows ==0) || (in_img.rows % in_img.cols ==0)));

}

void resize_covering_blocks(cv::Mat &img, const cv::Size &blockSize)
{
    resize_covering_blocks(img, blockSize, img);
}
