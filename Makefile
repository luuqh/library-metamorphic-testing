CC = g++
IPATH = -I/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/include \
		-I./libs
LPATH = -L/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs \
		-L./libs
LIB = -lisl
CFLAGS = $(IPATH) $(LPATH) $(LIB) -g
OBJ = small.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

small: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -r *.o
