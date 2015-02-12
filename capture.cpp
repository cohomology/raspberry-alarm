#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <functional>
#include <fstream>
#include <mutex>

#include <omxcam.h>

#include "basealloc.h"

template<uint32_t bufferSizeT=5242880>
class RaspiStillBufferT {
public:
  RaspiStillBufferT(BaseAllocator& alloc) 
    : m_alloc(alloc), m_buffer(reinterpret_cast<uint8_t*>(alloc.allocate(bufferSizeT))), m_current(m_buffer) {
  }
  ~RaspiStillBufferT() { 
    if (m_buffer) 
      m_alloc.deallocate(m_buffer);
  } 
  RaspiStillBufferT(const RaspiStillBufferT<bufferSizeT>& other) 
    : RaspiStillBufferT(other.m_alloc) { 
  }
  RaspiStillBufferT(RaspiStillBufferT&& other) 
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
  RaspiStillBufferT& operator=(RaspiStillBufferT&& other) = delete; 
  RaspiStillBufferT& operator=(const RaspiStillBufferT& other) = delete;
  friend std::ostream& operator<<(std::ofstream& os, const RaspiStillBufferT& buf) { 
    return os.write(reinterpret_cast<const char*>(buf.m_buffer), buf.m_current-buf.m_buffer); 
  }
private:
  BaseAllocator& m_alloc;
  uint8_t* m_buffer;
  uint8_t* m_current;
};

typedef RaspiStillBufferT<> RaspiStillBuffer;

class RaspiStillImage {
public:
  RaspiStillImage(BaseAllocator& alloc) 
    : m_alloc(alloc), m_settings() { 
    omxcam_still_init (&m_settings);
    m_settings.camera.drc = OMXCAM_DRC_HIGH;
    m_settings.camera.exposure = OMXCAM_EXPOSURE_NIGHT;
  }

  RaspiStillBuffer shoot() {
    RaspiStillBuffer buffer(m_alloc);
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
  static RaspiStillBuffer * m_buffer;
};

RaspiStillBuffer* RaspiStillImage::m_buffer = nullptr;

int main (){
  BlockAllocator<102400> alloc; 
  RaspiStillBuffer image = RaspiStillImage(alloc).shoot();
  std::ofstream outfile("image.jpg", std::ofstream::binary);
  outfile << image;
  outfile.close();
  return 0;
}
