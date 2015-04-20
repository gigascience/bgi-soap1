CC=	g++

FLAGS= -O3 -static -DMAXGAP=3 -DMAXHITS=100 -DTHREAD #-pg
REF=  -DDB_CONTIG #  DB_SHORT  DB_HUGE DB_CHR DB_CONTIG
OLIGOLEN= -DREAD_60  #-DREAD_36 -DREAD_48

LIBS=  
THREAD=	-lpthread

OBJS=	align.o dbseq.o main.o pairs.o param.o reads.o utilities.o

all: soap soap_dealign

%.o:%.cpp
	$(CC) $(FLAGS) $(LIBS) $(REF) $(OLIGOLEN) -c $< -o $@
soap: $(OBJS)
	$(CC) $(FLAGS) $(LIBS) $(REF) $(OLIGOLEN) $^ -o $@ $(THREAD)
soap_dealign: soap_dealign.o dealign.o param.o
	$(CC) $(FLAGS) $(LIBS) $^ -o $@
	
clean:
	rm -f *.o soap soap_dealign
install:
	cp soap soap_dealign ../bin
