BUILD_DIR ?= cmake-build
TORRENT ?= resources/example.torrent
OUTPUT ?= downloads
PERCENT ?= 100

.PHONY: all compile run test clean
all: compile

compile:
	cmake -S torrent-client-prototype -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Debug
	cmake --build "$(BUILD_DIR)" --parallel

run: compile
	"$(BUILD_DIR)/torrent-client-prototype" -d "$(OUTPUT)" -p "$(PERCENT)" "$(TORRENT)"

test:
	cmake -S torrent-client-prototype -B "$(BUILD_DIR)-tests" -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Debug
	cmake --build "$(BUILD_DIR)-tests" --parallel
	ctest --test-dir "$(BUILD_DIR)-tests" --output-on-failure

clean:
	rm -rf -- "$(BUILD_DIR)" "$(BUILD_DIR)-tests"

