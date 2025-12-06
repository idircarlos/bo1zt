# === Compiler and flags ===
CC = g++
CFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iinclude -Iexternal/libui -Iexternal/iniparser/src -Ires -Ishared -m32
LDFLAGS = -Llib -lui -liniparser -lole32 -luuid -lcomctl32 -lgdi32 -lmsimg32 -loleaut32 -ld2d1 -ldwrite -static-libgcc -static-libstdc++ -static -luxtheme -lopengl32 -m32

# === DLL Compiler (32-bit) ===
DLL_CC = gcc
DLL_CFLAGS = -Wall -O2 -m32 -Ishared
DLL_LDFLAGS = -shared -s -m32
DLL_LIBS = -luser32 -lkernel32

# === Paths ===
TARGET = build/bo1zt.exe
DLL_TARGET = build/bo1zt.dll

# === Source files ===
SRC := $(shell find src -type f -name "*.c")
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

# === DLL source files ===
DLL_SRC := $(shell find dll -type f -name "*.c")
DLL_OBJ := $(patsubst dll/%.c,build/dll/%.o,$(DLL_SRC))

LIBUI_SRC := $(wildcard external/libui/**/*.c external/libui/**/*.cpp)
INIPARSER_SRC := $(wildcard external/iniparser/src/*.c)

# === Default target ===
all: $(TARGET)

# === DLL ===
$(DLL_TARGET): $(DLL_OBJ)
	@echo Linking bo1zt.dll
	$(DLL_CC) $(DLL_CFLAGS) $^ -o $@ $(DLL_LDFLAGS) $(DLL_LIBS)

# === Compile DLL objects ===
build/dll/%.o: dll/%.c
	@mkdir -p $(dir $@)
	$(DLL_CC) $(DLL_CFLAGS) -MMD -MP -c $< -o $@

# === Link ===
$(TARGET): $(OBJ) lib/libui.a lib/libiniparser.a build/resources.o
	@echo Linking $@
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# === Compile ===
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# === External libraries ===
lib/libui.a: external/libui/build/meson-out/libui.a
	@mkdir -p lib
	@cp $< $@

external/libui/build/meson-out/libui.a: $(LIBUI_SRC)
	cd external/libui && rm -rf build && CC=$(DLL_CC) CXX=g++ CFLAGS="-m32" CXXFLAGS="-m32" LDFLAGS="-m32" meson setup build --default-library=static
	cd external/libui && meson compile -C build

lib/libiniparser.a: external/iniparser/build/libiniparser.a
	@mkdir -p lib
	@cp $< $@

external/iniparser/build/libiniparser.a: $(INIPARSER_SRC)
	cd external/iniparser && mkdir -p build && cd build && cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="$(DLL_CC)" -DCMAKE_C_FLAGS="-m32" -DCMAKE_SHARED_LINKER_FLAGS="-m32" -DBUILD_SHARED_LIBS=OFF .. && mingw32-make

# === Resources ===
build/resources.o: res/resources.rc res/example.static.manifest res/resource_ids.h res/fonts/*.ttf $(DLL_TARGET)
	@mkdir -p build
	windres -I res --target=pe-i386 res/resources.rc -O coff -o $@

# === Targets ===
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build lib

clean-dll:
	rm -f $(DLL_TARGET)

dll: $(DLL_TARGET)

# === Dependencies ===
-include $(OBJ:.o=.d)
-include $(DLL_OBJ:.o=.d)

.PHONY: all run clean clean-dll dll
