CC = g++ -g

CC_DEBUG = @$(CC) -std=c++11
CC_RELEASE = @$(CC) -std=c++11 -O3 -DNDEBUG

G_SRC = src/*.cpp *.cpp

# need libpng to build
#
G_INC = -Iinclude -Iapps -I/opt/local/include -L/opt/local/lib

all: image tests bench

image : $(G_SRC) include/*.h apps/image.cpp apps/image_recs.cpp
	$(CC_DEBUG) $(G_INC) $(G_SRC) apps/image.cpp apps/image_recs.cpp -lpng -o image

tests : $(G_SRC) include/*.h apps/tests.cpp apps/test_recs.cpp
	$(CC_DEBUG) $(G_INC) $(G_SRC) apps/tests.cpp apps/test_recs.cpp -lpng -o tests

bench : $(G_SRC) include/*.h apps/bench.cpp apps/bench_recs.cpp apps/GTime.cpp
	$(CC_RELEASE) $(G_INC) $(G_SRC) apps/bench.cpp apps/bench_recs.cpp apps/GTime.cpp -lpng -o bench

# needs xwindows to build
#
X_INC = -I/opt/X11/include -L/opt/X11/lib

DRAW_SRC = apps/draw.cpp apps/GWindow.cpp apps/GTime.cpp
draw: $(DRAW_SRC) $(G_SRC) include/*.h
	$(CC_DEBUG) $(X_INC) $(G_INC) $(G_SRC) $(DRAW_SRC) -lpng -lX11 -o draw


clean:
	@rm -rf image tests bench draw *.png *.dSYM

