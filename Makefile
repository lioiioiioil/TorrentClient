all: compile run

compile:
	cmake -S torrent-client-prototype -B cmake-build
	cd cmake-build && make

run: compile
	./cmake-build/torrent-client-prototype -d ./dir -p 100 ./resources/example.torrent

clean:
	rm -rf cmake-build
	rm -rf dir
