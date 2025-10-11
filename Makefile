# Detectar cambios en cualquier fuente de external/libui

CC=g++
CFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iexternal/libui
LDFLAGS=-Llib -lui -lole32 -luuid -lcomctl32 -lgdi32 -lmsimg32 -loleaut32 -ld2d1 -ldwrite -static-libgcc -static-libstdc++ -luxtheme
TARGET=build/bo1zt.exe
SRC := $(wildcard src/*.c src/**/*.h src/**/*.c src/**/*.rc)
LIBUI_SRC := $(wildcard external/libui/**/*.c external/libui/**/*.h external/libui/**/*.cpp external/libui/**/*.rc)
RESOURCES=build/resources.o

all: $(TARGET)

$(TARGET): $(SRC) lib/libui.a $(RESOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) lib/libui.a $(RESOURCES) $(LDFLAGS)

lib/libui.a: external/libui/build/meson-out/libui.a
	cp external/libui/build/meson-out/libui.a lib/libui.a

external/libui/build/meson-out/libui.a: $(LIBUI_SRC)
	cd external/libui && meson setup build --default-library=static
	cd external/libui && meson compile -C build

$(RESOURCES): res/resources.rc res/example.static.manifest build
	windres res/resources.rc $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build