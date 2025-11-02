# Detectar cambios en cualquier fuente de external/libui

CC=g++
CFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iexternal/libui -Iexternal/iniparser/src
LDFLAGS=-Llib -lui -liniparser -lole32 -luuid -lcomctl32 -lgdi32 -lmsimg32 -loleaut32 -ld2d1 -ldwrite -static-libgcc -static-libstdc++ -luxtheme
TARGET=build/bo1zt.exe

SRC := $(wildcard src/*.c src/*.h src/**/*.h src/**/*.c src/**/**/*.c src/**/**/*.h src/**/**/**/*.c src/**/**/**/*.h src/**/*.rc)
LIBUI_SRC := $(wildcard external/libui/**/*.c external/libui/**/*.h external/libui/**/*.cpp external/libui/**/*.rc)
INIPARSER_SRC := $(wildcard external/iniparser/src/*.c external/iniparser/src/*.h)
RESOURCES=build/resources.o

all: $(TARGET)

$(TARGET): $(SRC) lib/libui.a lib/libiniparser.a $(RESOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) lib/libui.a lib/libiniparser.a $(RESOURCES) $(LDFLAGS)

# =========================
# ===  LIBUI  SECTION  ===
# =========================
lib/libui.a: external/libui/build/meson-out/libui.a
	cp external/libui/build/meson-out/libui.a lib/libui.a

external/libui/build/meson-out/libui.a: $(LIBUI_SRC)
	cd external/libui && meson setup build --default-library=static
	cd external/libui && meson compile -C build

# =========================
# === INIPARSER SECTION ===
# =========================
lib/libiniparser.a: external/iniparser/build/libiniparser.a
	cp external/iniparser/build/libiniparser.a lib/libiniparser.a

external/iniparser/build/libiniparser.a: $(INIPARSER_SRC)
	cd external/iniparser && mkdir -p build && cd build && cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .. && mingw32-make

# =========================
# === RESOURCE SECTION ===
# =========================
$(RESOURCES): res/resources.rc res/example.static.manifest build
	windres res/resources.rc $@

build:
	mkdir -p build
	mkdir -p lib

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
