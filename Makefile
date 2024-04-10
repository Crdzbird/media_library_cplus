.PHONY: all

all: main

liblibrary.so:
#	/usr/bin/clang++ -o libvideo_library.so VideoExtension.cpp -std=c++20 -O3 -Wall -Wextra -fPIC -shared -L/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include -lavformat


	/usr/bin/clang++ -o liblibrary.so library.cpp  -std=c++20 -O3 -Wall -Wextra -fPIC -shared -lavformat -L/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/lib/ -I/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include


#	g++ -o liblibrary.so *.cpp  -std=c++20 -O3 -Wall -Wextra -fPIC -shared -lavformat -L/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/lib/ -I/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include

main: liblibrary.so
