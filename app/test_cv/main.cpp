#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <memory>

// VIRTUAL MEDIA (can be an image or video)
template <typename T> class IMedia {
protected:
  std::string m_fileaddress;
  int m_frame;
  T data;
  IMedia() {}

public:
  explicit IMedia(const std::string &fileaddress)
      : m_fileaddress(fileaddress) {}
  virtual ~IMedia() = default;
  virtual const T &getData() const { return data; }
  virtual const std::string &getPath() const { return m_fileaddress; }
  void setFrame(int frame) { m_frame = frame; }
};

// ACTUAL IMAGE based on OPENCV
class Image : public IMedia<cv::Mat> {
public:
  explicit Image(const std::string &fileaddress) : IMedia(fileaddress) {
    data = cv::imread(fileaddress);
  }
  ~Image() = default;
};

// VIRTUAL ENCODER
template <typename T> class IEncoder {
public:
  virtual ~IEncoder() = default;
  virtual void encode(std::ostream &output, IMedia<T> *input) = 0;

protected:
  virtual void processInput(std::ostream &output, const IMedia<T> *input) = 0;
};

// VIRTUAL ENCODER FOR ONE FRAME
template <typename T> class IFrameEncoder : public IEncoder<T> {
public:
  explicit IFrameEncoder(int frame) : m_frame(frame){};
  virtual ~IFrameEncoder() = default;
  void encode(std::ostream &output, IMedia<cv::Mat> *t) override {
    output << "encoding one frame from file " << t->getPath() << std::endl;
    output << "frame " << m_frame << std::endl;
    t->setFrame(m_frame);
    this->processInput(output, t);
  }

protected:
  int m_frame;
  IFrameEncoder() = default;
};

// VIRTUAL ENCODER FOR ONE MEDIA
template <typename T> class IMediaEncoder : public IEncoder<T> {
public:
  IMediaEncoder() = default;
  virtual ~IMediaEncoder() = default;
  void encode(std::ostream &output, IMedia<T> *t) override {
    std::cout << "encoding file " << t->getPath() << std::endl;
    this->processInput(output, t);
    int frameCount = 3; // fake

    for (int frame = 0; frame < frameCount; frame++) {
      auto pFrameEnc = createFrameEncoder(frame);
      pFrameEnc->encode(output, t);
    }
  }

protected:
  virtual std::unique_ptr<IFrameEncoder<T>> createFrameEncoder(int frame) = 0;
};

// ACTUAL ENCODER FOR ONE FRAME OPENCV
class FrameEncoder : public IFrameEncoder<cv::Mat> {
  using IFrameEncoder<cv::Mat>::IFrameEncoder;

public:
  ~FrameEncoder() = default;
  // IEncoder interface
protected:
  void processInput(std::ostream &output,
                    const IMedia<cv::Mat> *input) override {

    cv::Mat frameToEncode = input->getData().clone();
    std::string title = "frame_" + std::to_string(m_frame);
    cv::namedWindow(title, cv::WINDOW_NORMAL);
    cv::imshow(title, frameToEncode);
  }
};

// ACTUAL ENCODER FOR ONE MEDIA OPENCV
class MediaEncoder : public IMediaEncoder<cv::Mat> {
public:
  ~MediaEncoder() = default;
  // IEncoder interface
protected:
  void processInput(std::ostream &output,
                    const IMedia<cv::Mat> *input) override {

    output << "processInput for encoding media " << input->getPath()
           << std::endl;
  }

  // IMediaEncoder interface
protected:
  std::unique_ptr<IFrameEncoder<cv::Mat>>
  createFrameEncoder(int frame) override {
    return std::make_unique<FrameEncoder>(frame);
  }
};

void initMedia(const std::string &fileaddress,
               std::unique_ptr<IMedia<cv::Mat>> &output) {
  output.reset(new Image(fileaddress));
}

int main(int argc, char *argv[]) {
  const std::string fileaddr = argv[1];
  std::unique_ptr<IMedia<cv::Mat>> pMedia;
  std::unique_ptr<IEncoder<cv::Mat>> pEnc;

  // load media source
  initMedia(fileaddr, pMedia);
  cv::Mat test = pMedia->getData();
  cv::namedWindow("outside", cv::WINDOW_NORMAL);
  cv::imshow("outside", test);

  // encode media
  pEnc = std::make_unique<MediaEncoder>();
  pEnc->encode(std::cout, pMedia.get());

  cv::waitKey(0);
  return 0;
}
