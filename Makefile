# Toolchain
CXX     := g++
CC      := gcc
WINDRES := windres
ARCH    := -m32

# OAT (OpenAssetTools)
OAT_DIR     := external/oat
OAT_API_DIR := $(OAT_DIR)/lib

# EXE
EXE_CFLAGS  := -std=c++20 -Wall -Wextra -pedantic $(ARCH) \
               -Iinclude -Iexternal/libui -Iexternal/iniparser/src \
               -Iexternal/miniz -Iexternal/argparse -I$(OAT_API_DIR) -Ires -Ishared
EXE_LDFLAGS := -Llib $(ARCH) -static -static-libgcc -static-libstdc++ \
               -lui -liniparser -lole32 -luuid -lcomctl32 -lgdi32 \
               -lmsimg32 -loleaut32 -ld2d1 -ldwrite -luxtheme -lopengl32 -lgdiplus -lshlwapi -lshell32 -lws2_32 -lwinhttp -lwinmm -lcrypt32

# DLL
DLL_CFLAGS  := -Wall -O2 $(ARCH) -Ishared -Iexternal/cdl86 -Iexternal/miniz -Idll -Idll/gsc
DLL_LDFLAGS := -shared -s $(ARCH) -luser32 -lkernel32 -static-libgcc

# Targets
TARGET       := build/bo1zt.exe
DLL_TARGET   := build/bo1zt.dll

# Sources
SRC     := $(shell find src -type f -name "*.c")
DLL_SRC := $(shell find dll -type f -name "*.c")
OBJ     := $(patsubst src/%.c,build/%.o,$(SRC))
DLL_OBJ := $(patsubst dll/%.c,build/dll/%.o,$(DLL_SRC))

CDL86_OBJ := build/external/cdl86.o
MINIZ_OBJ := build/external/miniz.o
ARGPARSE_OBJ := build/external/argparse.o

.PHONY: all run clean clean-dll deepclean dll release

all: $(TARGET)

# Release build
release: EXE_CFLAGS  += -O2 -DNDEBUG
release: EXE_LDFLAGS += -mwindows -s
release: $(TARGET)
	@echo "Release build complete: $(TARGET)"
dll: $(DLL_TARGET)

run: $(TARGET)
	./$(TARGET)
clean:
	rm -rf build lib
clean-dll:
	rm -f $(DLL_TARGET)
deepclean: clean
	rm -rf external/libui/build external/libui/.cache
	rm -rf external/iniparser/build
	rm -rf external/oat/build

# EXE
$(TARGET): $(OBJ) $(ARGPARSE_OBJ) lib/libui.a lib/libiniparser.a lib/libminiz.a lib/liboat.a build/resources.o
	@echo "Linking $@"
	$(CXX) $(EXE_CFLAGS) -o $@ $^ $(EXE_LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CXX) $(EXE_CFLAGS) -MMD -MP -c $< -o $@

# argparse
$(ARGPARSE_OBJ): external/argparse/argparse.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 -O2 $(ARCH) -Iexternal/argparse -c $< -o $@

# DLL
$(DLL_TARGET): $(DLL_OBJ) $(CDL86_OBJ) $(MINIZ_OBJ)
	@echo "Linking $@"
	$(CC) $(DLL_CFLAGS) $^ -o $@ $(DLL_LDFLAGS)

build/dll/%.o: dll/%.c
	@mkdir -p $(dir $@)
	$(CC) $(DLL_CFLAGS) -MMD -MP -c $< -o $@

# External libs
$(CDL86_OBJ): external/cdl86/cdl.c
	@mkdir -p $(dir $@)
	$(CC) $(DLL_CFLAGS) -c $< -o $@

$(MINIZ_OBJ): external/miniz/miniz.c
	@mkdir -p $(dir $@)
	$(CC) $(DLL_CFLAGS) -c $< -o $@

lib/libminiz.a: external/miniz/miniz.c
	@mkdir -p lib
	$(CC) -Wall -O2 $(ARCH) -Iexternal/miniz -c $< -o build/miniz.o
	ar rcs $@ build/miniz.o
	@rm -f build/miniz.o

lib/libui.a: external/libui/build/meson-out/libui.a
	@mkdir -p lib
	@cp $< $@

external/libui/build/meson-out/libui.a:
	cd external/libui && rm -rf build && \
		CC=$(CC) CXX=$(CXX) CFLAGS="$(ARCH)" CXXFLAGS="$(ARCH)" LDFLAGS="$(ARCH)" \
		meson setup build --default-library=static
	cd external/libui && meson compile -C build

lib/libiniparser.a: external/iniparser/build/libiniparser.a
	@mkdir -p lib
	@cp $< $@

external/iniparser/build/libiniparser.a:
	cd external/iniparser && mkdir -p build && cd build && \
		cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="$(CC)" -DCMAKE_C_FLAGS="$(ARCH)" \
		-DCMAKE_SHARED_LINKER_FLAGS="$(ARCH)" -DBUILD_SHARED_LIBS=OFF .. && \
		mingw32-make

# OAT - Unlinker as a static lib
PREMAKE_VERSION  := 5.0.0-beta8
PREMAKE          := $(OAT_DIR)/build/premake5.exe
PREMAKE_ZIP      := $(OAT_DIR)/build/premake.zip
PREMAKE_URL      := https://github.com/premake/premake-core/releases/download/v$(PREMAKE_VERSION)/premake-$(PREMAKE_VERSION)-windows.zip
OAT_BUILD_LIBDIR := $(OAT_DIR)/build/lib/Release_x86
OAT_LIB_NAMES    := Unlinking ObjWriting ObjLoading ObjImage ObjCommon \
                    ZoneWriting ZoneLoading ZoneCommon ZoneCodeGeneratorLib \
                    Cryptography Parser Common Utils XMemCompress \
                    libtomcrypt libtommath lz4 lzx minilzo minizip salsa20 zlib
OAT_STAMP        := $(OAT_DIR)/build/.oat_libs_built
OAT_API_CFLAGS   := -std=c++23 -O2 $(ARCH) -DARCH_x86 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS \
                    -D__STDC_WANT_LIB_EXT1__=1 -I$(OAT_API_DIR) \
                    -I$(OAT_DIR)/src/Unlinking -I$(OAT_DIR)/src/Utils -I$(OAT_DIR)/src/ObjWriting \
                    -I$(OAT_DIR)/src/ZoneCommon -I$(OAT_DIR)/src/Common -I$(OAT_DIR)/src/ObjCommon

OAT_JOBS         ?= $(NUMBER_OF_PROCESSORS)

$(PREMAKE):
	@mkdir -p $(OAT_DIR)/build
	powershell -NoProfile -Command "\$$ProgressPreference='SilentlyContinue'; Invoke-WebRequest '$(PREMAKE_URL)' -OutFile '$(PREMAKE_ZIP)'; Expand-Archive -Force '$(PREMAKE_ZIP)' '$(OAT_DIR)/build'"
	@rm -f $(PREMAKE_ZIP)

$(OAT_DIR)/build/Makefile: $(PREMAKE)
	cd $(OAT_DIR) && build/premake5.exe gmake

$(OAT_STAMP): $(OAT_DIR)/build/Makefile
	mingw32-make -C $(OAT_DIR)/build config=release_x86 -j$(OAT_JOBS) $(OAT_LIB_NAMES)
	@echo built > $@

build/external/oat.o: $(OAT_API_DIR)/oat.cpp $(OAT_API_DIR)/oat.h
	@mkdir -p $(dir $@)
	$(CXX) $(OAT_API_CFLAGS) -c $< -o $@

lib/liboat.a: build/external/oat.o $(OAT_STAMP)
	@mkdir -p lib
	@echo "Archiving $@"
	@{ echo create $@; \
	   for l in $(OAT_BUILD_LIBDIR)/*.lib; do echo addlib $$l; done; \
	   echo addmod $<; echo save; echo end; } | ar -M

# Resources
build/gsc.zip: gsc/*
	@mkdir -p build
	@powershell -Command "Compress-Archive -Path 'gsc/*' -DestinationPath 'build/gsc.zip' -Force"

build/resources.o: res/resources.rc res/example.static.manifest res/resource_ids.h res/fonts/*.ttf $(DLL_TARGET) build/gsc.zip res/camo/weapons.json res/camo/$$black.iwi res/shaders/*.vert res/shaders/*.frag res/markdown/*.md
	@mkdir -p build
	$(WINDRES) -I res --target=pe-i386 $< -O coff -o $@

-include $(OBJ:.o=.d)
-include $(DLL_OBJ:.o=.d)
