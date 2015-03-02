CXX=g++
CFLAGS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS \
                -DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
                -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -DHAVE_LIBOPENMAX=2 -DOMX \
                -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST \
                -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -fPIC -ftree-vectorize -pipe \
                -Werror -g -Wall -O2 -fvisibility=hidden 
CXXFLAGS=-std=c++11 
LDFLAGS = -L/opt/vc/lib -lopenmaxil -lbcm_host -lvchiq_arm -lpthread -lomxcam
INCLUDES = -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads \
                -I/opt/vc/include/interface/vmcs_host/linux 

capture: capture.o
	$(CXX) $(LDFLAGS) -o capture capture.o 

capture.o: capture.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o capture.o capture.cpp

.PHONY: clean
clean:
	@rm -f *.o 
	@rm -f capture 
	@rm -f *.jpg 
