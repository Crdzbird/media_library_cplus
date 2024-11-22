.PHONY: all

all: main

liblibrary.so:
#	/usr/bin/clang++ -o libvideo_library.so VideoExtension.cpp -std=c++20 -O3 -Wall -Wextra -fPIC -shared -L/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include -lavformat

	/usr/bin/clang++ -o liblibrary.so library.cpp  -std=c++20 -O3 -Wall -Wextra -fPIC -shared  -lavformat -lavcodec -lavutil -lswscale -ljpeg  -L/opt/homebrew/Cellar/ffmpeg/7.1_3/lib/  -I/opt/homebrew/Cellar/ffmpeg/7.1_3/include -L/opt/homebrew/Cellar/jpeg-turbo/3.0.4/lib/ -I/opt/homebrew/Cellar/jpeg-turbo/3.0.4/include
#	/usr/bin/clang++ -o liblibrary.so library.cpp  -std=c++20 -O3 -Wall -Wextra -fPIC -shared -lavformat -L/opt/homebrew/Cellar/ffmpeg/7.1_3/lib/ -I/opt/homebrew/Cellar/ffmpeg/7.1_3/include -L/opt/homebrew/Cellar/jpeg-turbo/3.0.4/lib/ -I/opt/homebrew/Cellar/jpeg-turbo/3.0.4/include  -lavcodec -lavutil -lswscale


#	g++ -o liblibrary.so *.cpp  -std=c++20 -O3 -Wall -Wextra -fPIC -shared -lavformat -L/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/lib/ -I/opt/homebrew/Cellar/ffmpeg/7.0-with-options_1/include

main: liblibrary.so
