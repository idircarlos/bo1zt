# Toolchain
CXX     := g++
CC      := gcc
WINDRES := windres
ARCH    := -m32

# EXE
EXE_CFLAGS  := -std=c++20 -Wall -Wextra -pedantic $(ARCH) \
               -Iinclude -Iexternal/libui -Iexternal/iniparser/src \
               -Iexternal/miniz -Ires -Ishared
EXE_LDFLAGS := -Llib $(ARCH) -static -static-libgcc -static-libstdc++ \
               -lui -liniparser -lole32 -luuid -lcomctl32 -lgdi32 \
               -lmsimg32 -loleaut32 -ld2d1 -ldwrite -luxtheme -lopengl32 -lgdiplus

# DLL
DLL_CFLAGS  := -Wall -O2 $(ARCH) -Ishared -Iexternal/cdl86 -Iexternal/miniz -Idll -Idll/gsc
DLL_LDFLAGS := -shared -s $(ARCH) -luser32 -lkernel32

# Targets
TARGET     := build/bo1zt.exe
DLL_TARGET := build/bo1zt.dll

# Sources
SRC     := $(shell find src -type f -name "*.c")
DLL_SRC := $(shell find dll -type f -name "*.c")
OBJ     := $(patsubst src/%.c,build/%.o,$(SRC))
DLL_OBJ := $(patsubst dll/%.c,build/dll/%.o,$(DLL_SRC))

CDL86_OBJ := build/external/cdl86.o
MINIZ_OBJ := build/external/miniz.o

.PHONY: all run clean clean-dll dll

all: $(TARGET)
dll: $(DLL_TARGET)
run: $(TARGET)
	./$(TARGET)
clean:
	rm -rf build lib
clean-dll:
	rm -f $(DLL_TARGET)

# EXE
$(TARGET): $(OBJ) lib/libui.a lib/libiniparser.a lib/libminiz.a build/resources.o
	@echo "Linking $@"
	$(CXX) $(EXE_CFLAGS) -o $@ $^ $(EXE_LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CXX) $(EXE_CFLAGS) -MMD -MP -c $< -o $@

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

# Resources
build/gsc.zip: gsc/*
	@mkdir -p build
	@powershell -Command "Compress-Archive -Path 'gsc/*' -DestinationPath 'build/gsc.zip' -Force"

build/resources.o: res/resources.rc res/example.static.manifest res/resource_ids.h res/fonts/*.ttf $(DLL_TARGET) build/gsc.zip
	@mkdir -p build
	$(WINDRES) -I res --target=pe-i386 $< -O coff -o $@

-include $(OBJ:.o=.d)
-include $(DLL_OBJ:.o=.d)
