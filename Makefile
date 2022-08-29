# FLAGS
CFLAGS= -O3 -fPIC -std=c++11
CFLAGS+= -I./../hdf5/HDF5-1.12.2-Linux/HDF_Group/HDF5/1.12.2/include
CFLAGS+= -I./include `pkg-config --cflags squids nusquids hdf5`
LDFLAGS+= `pkg-config --libs squids nusquids hdf5` -lhdf5_hl -lpthread
LDFLAGS+= -L/home/oalterkait/decayrepo/hdf5/HDF5-1.12.2-Linux/HDF_Group/HDF5/1.12.2/lib


all: examples/exCross.o examples/partial_rate_example examples/couplings_example examples/uBFlux_example

examples/exCross.o : include/exCross.h examples/exCross.cpp
	@ $(CXX) $(CFLAGS) -c examples/exCross.cpp -o $@

examples/partial_rate_example : examples/partial_rate_example.cpp examples/exCross.o include/nusquids_decay.h
	@echo Compiling partial_rate_example
	@ $(CXX) $(CFLAGS) examples/partial_rate_example.cpp examples/exCross.o  -o $@ $(LDFLAGS)

examples/couplings_example : examples/couplings_example.cpp examples/exCross.o include/nusquids_decay.h
	@echo Compiling couplings_example
	@ $(CXX) $(CFLAGS) examples/couplings_example.cpp examples/exCross.o  -o $@ $(LDFLAGS)

examples/uBFlux_example : examples/uBFlux_example.cpp examples/exCross.o include/nusquids_decay.h
	@echo Compiling uBFlux_example
	@ $(CXX) $(CFLAGS) examples/uBFlux_example.cpp examples/exCross.o  -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf ./examples/partial_rate_example ./examples/couplings_example ./examples/uBFlux_example ./examples/test  ./examples/exCross.o
