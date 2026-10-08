# Directories
SRC_DIR := src
BUILD_DIR := build
WIN_BUILD_DIR := build-mingw
BIN_DIR := bin
INC_DIRS := $(wildcard $(SRC_DIR)/*/)

# Create build & bin directories if they don't exist
$(shell mkdir -p $(BUILD_DIR) $(WIN_BUILD_DIR) $(BIN_DIR))
$(shell mkdir -p $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(INC_DIRS)) $(patsubst $(SRC_DIR)/%,$(WIN_BUILD_DIR)/%,$(INC_DIRS)))

# Compiler & Flags
CXX := g++
CXXVERSION := -std=c++17
CXXFLAGS := -march=native -I$(SRC_DIR) $(patsubst %,-I%,$(INC_DIRS))
CXXBOOST := -Iboost_1_83_0/build/include -Lboost_1_83_0/build/lib -lboost_thread -lboost_chrono
CXXFFTW := -Ifftw-3.3.10/build/include -Lfftw-3.3.10/build/lib -lfftw3 -fopenmp

# Windows Specific for twophoton (Windows 7)
WIN_CXX := x86_64-w64-mingw32-g++
WIN_CXXFLAGS := -D_WIN32_WINNT=0x0601 -static-libgcc -static-libstdc++ -lws2_32 -Wa,-mbig-obj -Os -I$(SRC_DIR) $(patsubst %,-I%,$(INC_DIRS))
WIN_CXXBOOST := -Iboost_1_83_0/build-mingw/include -Lboost_1_83_0/build-mingw/lib -lboost_thread -lboost_chrono -lwinmm

# Object Files
TWOPHOTON_OBJS := $(WIN_BUILD_DIR)/main_twophoton.o \
						$(WIN_BUILD_DIR)/networking/websocket_client_session.o $(WIN_BUILD_DIR)/networking/image_buffer.o \
                        $(WIN_BUILD_DIR)/networking/tcp_client_session.o \
						$(WIN_BUILD_DIR)/prairieview/prairieview_client_session.o

FORTYNINETY_OBJS := $(BUILD_DIR)/main_fortyninety.o \
					$(BUILD_DIR)/core/config.o \
					$(BUILD_DIR)/value_saving/value_saver.o \
					$(BUILD_DIR)/image_saving/image_saver.o $(BUILD_DIR)/image_saving/stb_image_write_wrapper.o \
					$(BUILD_DIR)/image_processing/calcium_response_processor.o \
					$(BUILD_DIR)/image_processing/cpu_fft.o $(BUILD_DIR)/image_processing/cpu_median_filter_256.o \
					$(BUILD_DIR)/timing/timing.o \
					$(BUILD_DIR)/trial_engine/trial_engine.o \
					$(BUILD_DIR)/networking/websocket_server.o $(BUILD_DIR)/networking/websocket_server_session.o $(BUILD_DIR)/networking/tcp_client_session.o \
					$(BUILD_DIR)/core/fortyninety_server_session.o

RPIBETA_OBJS := $(BUILD_DIR)/main_rpibeta.o \
					$(BUILD_DIR)/networking/websocket_client_session.o $(BUILD_DIR)/networking/image_buffer.o \
					$(BUILD_DIR)/core/rpibeta_client_session.o

# Targets
# all is for random custom changes
all:
	$(MAKE) twophoton_one
	$(MAKE) twophoton_two
	$(MAKE) twophoton_three
	$(MAKE) twophoton_transfer
	$(MAKE) fortyninety_one
	$(MAKE) fortyninety_two
	$(MAKE) fortyninety_three
	./$(BIN_DIR)/main_fortyninety

main_twophoton: twophoton_one twophoton_two twophoton_three

main_fortyninety: fortyninety_one fortyninety_two fortyninety_three

main_rpibeta: rpibeta_one rpibeta_two rpibeta_three





$(FORTYNINETY_OBJS): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXVERSION) -c $< -o $@ -mavx2 $(CXXFLAGS) $(CXXBOOST) $(CXXFFTW)

fortyninety_one:
	$(CXX) $(CXXVERSION) -x c++-header -o $(BUILD_DIR)/boost_header.h.gch $(SRC_DIR)/boost_header.h -Iboost_1_83_0/build/include

fortyninety_two: $(BUILD_DIR)/boost_header.h.gch $(FORTYNINETY_OBJS)

fortyninety_three:
	$(CXX) $(CXXVERSION) $(FORTYNINETY_OBJS) -o $(BIN_DIR)/main_fortyninety -mavx2 $(CXXFLAGS) $(CXXBOOST) $(CXXFFTW)





$(TWOPHOTON_OBJS): $(WIN_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(WIN_CXX) $(CXXVERSION) -include boost_header.h -c $< -o $@ $(WIN_CXXFLAGS) $(WIN_CXXBOOST)

twophoton_one:
	$(WIN_CXX) $(CXXVERSION) -x c++-header -o $(WIN_BUILD_DIR)/boost_header.h.gch $(SRC_DIR)/boost_header.h -D_WIN32_WINNT=0x0601 -Iboost_1_83_0 -Wa,-mbig-obj -Os

twophoton_two: $(WIN_BUILD_DIR)/boost_header.h.gch $(TWOPHOTON_OBJS)

twophoton_three:
	$(WIN_CXX) $(CXXVERSION) -static ${TWOPHOTON_OBJS} -o $(BIN_DIR)/main_twophoton_stimtrain.exe $(WIN_CXXFLAGS)



# Run on Raspberry Pi 4 OS. Will take a long time, but it's fine
$(RPIBETA_OBJS): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXVERSION) -include boost_header.h -c $< -o $@ $(CXXFLAGS) $(CXXBOOST)

rpibeta_one:
	$(CXX) $(CXXVERSION) -x c++-header -o $(BUILD_DIR)/boost_header.h.gch $(SRC_DIR)/boost_header.h -Iboost_1_83_0/build/include -Wa,-mbig-obj -Os

rpibeta_two: $(BUILD_DIR)/boost_header.h.gch $(RPIBETA_OBJS)

rpibeta_three:
	$(CXX) $(CXXVERSION) $(RPIBETA_OBJS) -o bin/main_rpibeta \
    -Iboost_1_83_0/build/include -Lboost_1_83_0/build/lib \
    -Wl,-Bstatic -lboost_thread -lboost_chrono -Wl,-Bdynamic -pthread -ldl \
	-lpigpio -lrt



clean:
	rm -rf $(BUILD_DIR) $(WIN_BUILD_DIR) $(BIN_DIR)