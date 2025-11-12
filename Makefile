# === Compiler and flags ===
CC = g++
CFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iexternal/libui -Iexternal/iniparser/src
LDFLAGS = -Llib -lui -liniparser -lole32 -luuid -lcomctl32 -lgdi32 -lmsimg32 -loleaut32 -ld2d1 -ldwrite -static-libgcc -static-libstdc++ -luxtheme -lopengl32

# === Paths ===
TARGET = build/bo1zt.exe
BUILD_DIR = build
SRC_DIR = src
RESOURCES = $(BUILD_DIR)/resources.o

# === Source files ===
SRC := $(shell find $(SRC_DIR) -type f -name "*.c")
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

LIBUI_SRC := $(wildcard external/libui/**/*.c external/libui/**/*.cpp)
INIPARSER_SRC := $(wildcard external/iniparser/src/*.c)

# === Default target ===
all: $(TARGET)

# === Link ===
$(TARGET): $(OBJ) lib/libui.a lib/libiniparser.a $(RESOURCES)
	@echo Linking $@
	$(CC) $(CFLAGS) -o $@ $(OBJ) lib/libui.a lib/libiniparser.a $(RESOURCES) $(LDFLAGS)

# === Compile each source ===
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# === External libraries ===
lib/libui.a: external/libui/build/meson-out/libui.a
	@cmp -s $< $@ || cp $< $@

external/libui/build/meson-out/libui.a: $(LIBUI_SRC)
	cd external/libui && (meson setup build --default-library=static || true)
	cd external/libui && meson compile -C build

lib/libiniparser.a: external/iniparser/build/libiniparser.a
	@cmp -s $< $@ || cp $< $@

external/iniparser/build/libiniparser.a: $(INIPARSER_SRC)
	cd external/iniparser && mkdir -p build && cd build && cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .. && mingw32-make

# === Resources ===
$(RESOURCES): res/resources.rc res/example.static.manifest res/resource_ids.h res/fonts/*.ttf | $(BUILD_DIR)
	windres -I res res/resources.rc -O coff -o $@

# === Directories ===
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p lib

# === Run and clean ===
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) lib/libui.a lib/libiniparser.a

# === Auto header dependencies ===
-include $(OBJ:.o=.d)
