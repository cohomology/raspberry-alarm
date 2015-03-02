#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <functional>
#include <fstream>
#include <mutex>

#include <omxcam.h>
#include <opencv/cv.h>

#include "basealloc.h"

template<uint32_t bufferSizeT>
class RaspiStillBuffer {
public:
  RaspiStillBuffer(BaseAllocator& alloc) 
    : m_alloc(alloc), m_buffer(reinterpret_cast<uint8_t*>(alloc.allocate(bufferSizeT))), m_current(m_buffer) {
  }
  ~RaspiStillBuffer() { 
    if (m_buffer) 
      m_alloc.deallocate(m_buffer);
  } 
  RaspiStillBuffer(const RaspiStillBufferT<bufferSizeT>& other) 
    : RaspiStillBuffer(other.m_alloc) { 
  }
  RaspiStillBuffer(RaspiStillBufferT&& other) 
    : m_alloc(other.m_alloc), m_buffer(other.m_buffer), m_current(other.m_current) { 
    other.m_buffer = nullptr;
    other.m_current = 0;
  }
  void append(uint8_t* buffer, uint32_t length) { 
    if (m_current-m_buffer+length > bufferSizeT)
      throw std::bad_alloc();     
    memcpy(m_current, buffer, length);
    m_current += length;
  }
  const uint8_t* getRawBuffer() { 
    return m_buffer;
  }
  RaspiStillBuffer& operator=(RaspiStillBufferT&& other) = delete; 
  RaspiStillBuffer& operator=(const RaspiStillBufferT& other) = delete;
  friend std::ostream& operator<<(std::ofstream& os, const RaspiStillBuffer& buf) { 
    return os.write(reinterpret_cast<const char*>(buf.m_buffer), buf.m_current-buf.m_buffer); 
  }
private:
  BaseAllocator& m_alloc;
  uint8_t* m_buffer;
  uint8_t* m_current;
};

template<uint32_t bufferSizeT>
class RaspiStillImage {
public:
  RaspiStillImage(BaseAllocator& alloc, uint32_t width, 
    uint32_t height) 
    : m_alloc(alloc), m_settings() { 
    omxcam_still_init (&m_settings);
    m_settings.camera.drc = OMXCAM_DRC_HIGH;
    m_settings.camera.exposure = OMXCAM_EXPOSURE_NIGHT;
    m_settings.width = width; 
    m_settings.height = height; 
  }

  RaspiStillBuffer<bufferSizeT> shoot() {
    RaspiStillBuffer<bufferSizeT> buffer(m_alloc);
    m_buffer = &buffer; 
    m_settings.on_data = data_callback; 
    try { 
      omxcam_still_start(&m_settings);
    } 
    catch(const std::bad_alloc&) {
      omxcam_still_stop(); 
      throw;
    }
    omxcam_still_stop(); 
    m_buffer = nullptr;
    return buffer;
  } 

protected:
  static void data_callback(omxcam_buffer_t source) { 
    m_buffer->append(source.data, source.length);
  }
private:
  BaseAllocator& m_alloc;
  omxcam_still_settings_t m_settings;
  static RaspiStillBuffer<bufferSizeT> * m_buffer;
};

class OpenCvReadBuffer {

  OpenCvReadBuffer(uint32_t width, uint32_t height)
    : m_width(width), m_height(height)
  { }

  template<uint32_t bufferSizeT>
  void readImage(const RaspiStillBuffer<bufferSizeT>& buffer) {
    cv::Mat imgMat = cv::imdecode(cv::Mat(m_width, 
      m_height, CV_8U, buffer.getRawBuffer()), CV_LOAD_IMAGE_COLOR);
  }


private:
  const uint32_t m_width;
  const uint32_t m_height;
};

template<uint32_t bufferSizeT>
RaspiStillBuffer<bufferSizeT>* RaspiStillImage<bufferSizeT>::m_buffer = nullptr;

int main (){
  const uint32_t bufferSize = 5242880;
  const uint32_t width = 2592;
  const uint32_t height = 1944;

  BlockAllocator<bufferSize> alloc; 
  RaspiStillBuffer<bufferSize> image 
    = RaspiStillImage(alloc, width, height).shoot<bufferSize>();
  OpenCvReadBuffer openCv(width, height);
  openCv.readImage(image);
  std::ofstream outfile("image.jpg", std::ofstream::binary);
  outfile << image;
  outfile.close();
  return 0;
}
