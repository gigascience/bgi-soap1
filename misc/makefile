CC=	g++

FLAGS= -O3 -static

LIBS=  
THREAD=	-lpthread

all: kmerfreq kmerfreq_vsref

%.o:%.cpp
	$(CC) $(FLAGS) $(LIBS) -c $< -o $@

kmerfreq: kmerfreq.o
	$(CC) $(FLAGS) $(LIBS) $^ -o $@
	
kmerfreq_vsref: kmerfreq_vsref.o
	$(CC) $(FLAGS) $(LIBS) $^ -o $@
	
clean:
	rm -f *.o kmerfreq kmerfreq_vsref
install:
	chmod 755 *.pl
	cp kmerfreq kmerfreq_vsref *.pl  ../bin
	
