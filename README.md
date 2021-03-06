# bgi-soap1
Source code for the SOAP alignment tools developed by BGI.

--

SOAP: Short Oligonucleotide Alignment Program

Author: Ruiqiang Li 
lirq@genomics.org.cn 
Bioinformatics Department
Beijing Genomics Institute, Beijing 101300, China

lirq@bmb.sdu.dk 
Department of Biochemistry and Molecular Biology 
University of Southern Denmark Campusvej 55, DK-5230 Odense M, Denmark

1. Introduction:

SOAP is a program for efficient gapped and ungapped alignment of short oligonucleotides onto reference sequences. The program is designed to handle the huge amounts of short reads generated by parallel sequencing using the new generation Illumina-Solexa sequencing technology. SOAP is compatible with numerous applications, including single-read or pair-end resequencing, small RNA discovery and mRNA tag sequence mapping. SOAP is a command-driven program, which supports multi-threaded parallel computing, and has a batch module for multiple query sets.

a) Single-end sequencing
SOAP will allow either a certain number of mismatches or one continuous gap for aligning a read onto the reference sequence. The best hit of each read which has minimal number of mismatches or smaller gap will be reported. For multiple equalbest hits, the user can instruct the program to report all, or randomly report one, or disregard all of them. Since the typical read length is 25-50 bp, hits with too many mismatches are unreliable which are hard to distinguish with random matches. By default, the program will allow at most two mismatches. Between two haplotype genome sequences, occurrence of single nucleotide polymorphism is much higher than that of small insertions or deletions, so ungapped hits have precedence over gapped hits. For gapped alignment only one continuous gap with a size ranging from 1 to 3 bp is accepted, while no mismatches are permitted in the flanking regions to avoid ambiguous gaps. The gap could be either insertion or deletion in the query or the reference sequence. As the intrinsic character of the sequencing technology, errors will accumulate during the sequencing process. Reads always exhibit a much higher number of sequencing errors at the 3'-end, which sometimes make them unalignable to the reference sequences. To deal with the problem, SOAP can iteratively trim several basepairs at the 3'-end and redo the alignment, until hits are detected or the remaining sequence is too short for specific alignment.

b) Pair-end sequencing
Pair-end sequencing means to sequence both ends of a DNA fragment. So the two reads belonging to a pair will always have the settled relative orientation and approximate distance between each other on the genome. The technology can significantly improve the accuracy of resequencing mapping, and is a powerful method for detection of structural variants including copy number variations (CNVs), rearrangements, inversions and etc. SOAP is able to align a pair of reads simultaneously. A pair will be aligned when two reads are mapped with the right orientation relationship and proper distance. Similar filter as single-read alignment, a certain number of mismatches are allowed in one or both reads of the pair. For gapped alignment, gap is only permitted on one read, and the other end should match exactly.

c) mRNA tag sequencing
On mRNA tag sequencing, there are two types of restriction enzyme digestion: (i) DpnII, which will specially recognize the site ¡®GATC¡¯ and cuts a 16 bp tag after the site; (ii) NlaIII, which exclusively recognizes the site ¡®CATG¡¯ and cuts 17 bp downstream. SOAP checks and trims off the 3'-end adapter sequence according to the enzyme type. Aligned hits should contain the enzyme site, and have at most one mismatch in the tag region.

d) Small RNA sequencing
Small RNAs have a size between 18 to 26 bp. According to the experimental protocol, the 3'-end of RNA sequence will be flanked by adapter sequences. SOAP will filter adapter sequence, and then align the remaining cancandidate small RNA to the reference sequence. A small RNA will be annotated if an adapter sequence is detected and the insert sequence match well with the reference sequence. Considering sequencing errors, one or two mismatches can be allowed insider either the adapter or the candidate RNA region according to user settings.

2. Download and Installation:

SOAP is distributed under GNU Public License (GPL). The source code is freely available from ftp.genomics.org.cn/soap/. If you find bugs or have constructive suggestions to the program, please feel free to send e-mail to the author.

SOAP is written in standard C++ language. It could be compiled and run under any type of linux or unix environment. For big reference sequences like human genome, the program has to be compiled into 64 bits. You can simply type "make" under the source directory to compile the program.

The RAM required for the program can be roughly calculated as:
a) 64 bits
RAM=L/3+(4*3+8*6)*(4^S)+(4+1)*3*L/4+4*(2^24).

Where L is the total length of the reference sequences; S is seed size. So for small reference like yeast, L=12Mb, and selected seed size S=10, about 200Mb RAM is needed; but for the whole human genome, L=3Gb and a selected seed size S=12bp, about 14Gb RAM will be needed.

b) 32 bits
RAM=L/3+(4*3+4*6)*(4^S)+(4+1)*3*L/4+4*(2^24).

The program supports multithreaded parallel computing, be sure to add "-DTHREAD" in the compiling options if you want to use the parallel version.

Compiling options:

1) Oligo length
    -DREAD_36
    -DREAD_48
    -DREAD_60

Maximum length of reads on the query file, There are three choices now: -DREAD_36, -DREAD_48, and -DREAD_60. Smaller one will runs faster; larger one is compatible to smaller one.

In principle, the program works for any length of query sequences, but for long queries (>=60bp) or low similarity search, we suggest standard Smith-Waterman alignment, or using approximate program like Blast to have proper sensitivity. In the current version of SOAP, the longest acceptable read length is 60bp. The program can be easily revised to support longer reads.

2) Maximum number of equal best hits
    -DMAXHITS=10000

This setting is to have a tradeoff between exhaustion and speed. If you want to get all equal best hits, then you should set bigger number; or if you are mainly interested in unique hits, then please set smaller number so that the program will not waste time on alignment highly repetitive hits. User can set their own threshold as required, by changing the setting of "-DMAXHITS" in makefile and throughly recompile the program.

3) Maximum gap size
    -DMAXGAP=3

Maximum size of a gap allowed in a read, then "-g" option during running should not exceed this definition.

4) Multi-threaded calculation
    -DTHREAD
Multi-thread compiling. Currently POSIX Threads.

5) Reference types
	-DDB_CHR	# of sequences <256, length of each <4Gb;
	-DDB_CONTIG	# of sequences <65,536, length of each <4Gb;
	-DDB_SHORT	# of sequences <4G, length of each <65Kb;
	-DDB_HUGE	# of sequences <4G, length of each <4Gb.

The current version of soap will compile all the 4 programs directly: soap, soap.contig, soap.short, soap.huge. Choose the proper program will save RAM while fit your job better.

3. Usage:

a) Common options:
Usage:	soap [options]
       -a  <str>   query a file, *.fq or *.fa format
       -d  <str>   reference sequences file, *.fa format
       -o  <str>   output alignment file
       -s  <int>   seed size, default=10. [read>18,s=8; read>22,s=10, read>26, s=12]
       -v  <int>   maximum number of mismatches allowed on a read, <=5. default=2bp
       -g  <int>   maximum gap size allowed on a read, default=0bp
       -w  <int>   maximum number of equal best hits to count, smaller will be faster, <=10000
       -e  <int>   will not allow gap exist inside n-bp edge of a read, default=3bp
       -z  <char>  initial quality, default=@ [Illumina is using '@', Sanger Institute is using '!']
       -c  <int>   how to trim low-quality at 3-end?
                   0:     don't trim;
                   1-10:  trim n-bps at 3-end for all reads;
                   11-20: trim first bp and (n-10)-bp at 3-end for all reads;
                   21-30: trim (n-20)-bp at 3-end and redo alignment if the original read have no hit;
                   31-40: trim first bp and (n-30)-bp at 3-end and redo alignment if the original read have no hit;
                   41-50: iteratively trim (n-40)-bp at 3-end until getting hits;
                   51-60: if no hit, trim first bp and iteratively trim (n-50)bp at 3-end until getting hits;
                   default: 0
       -f  <int>   filter low-quality reads containing >n Ns, default=5
       -r  [0,1,2] how to report repeat hits, 0=none; 1=random one; 2=all, default=1
       -t          read ID in output file, [name, order in input file], default: name
       -n  <int>   do alignment on which reference chain? 0:both; 1:forward only; 2:reverse only. default=0
       -p  <int>   number of processors to use, default=1

  Options for pair-end alignment:
       -b  <str>   query b file
       -m  <int>   minimal insert size allowed, default=400
       -x  <int>   maximal insert size allowed, default=600
       -2  <str>   output file of unpaired alignment hits
       -y          do not optimize for SV analysis, default will output hit a and hit b with smallest distance in unpaired alignment

  Options for mRNA tag alignment:
       -T  <int>   type of tag, 0:DpnII, GATC+16; 1:NlaIII, CATG+17. default=-1[not mRNA tag]

  Options for miRNA alignment:
       -A  <str>   3-end adapter sequence, default=[not miRNA]
       -S  <int>   number of mismatch allowed in adapter, default=0
       -M  <int>   minimum length of a miRNA, default=17
       -X  <int>   maximum length of a miRNA, default=26
       -h          help

 b) Command lines:
   single-end alignment: soap -a query.fa -d ref.fa -o out.sop -s 12
   pair-end alignment:   soap -a query_1.fa -b query_2.fa -d ref.fa -o out.sop -2 single.sop -m 100 -x 150
   batch model:          soap -d ref.fa <parameter file>

      SOAP provides batch model for alignment of multiple query datasets onto the same reference, which will avoid loading reference and constructing indexing hash for multiple times. the <parameter file> contains options for each query:
      <parameter file>:
      -a q1.fa -o out1.sop -s 12
      -a q2.fa -o out2.sop -s 12
      ...
      -a qn.fa -o outn.sop -s 10
     
4. Output format
One line for One hit. The columns are separated by '\t'.
1)id:	id of read;
2)seq:	full sequence of read. the read will be converted to the complementary sequence if mapped on the reverse chain of reference. And if it was mapped after trimming, then SOAP will report trimmed sequence;
3)qual:	quality of sequence. corresponding to sequence, to be consistent with seq, it will be converted too if mapped on reverse chain;
4)number of hits:	number of equal best hits. the reads with no hits will be ignored;
5)a/b:	flag only meaningful for pair-end alignment, to distinguish which file the read is belonging to;
6)length:	length of the read, if aligned after trimming, it will report the length and initial location of the trimmed read;
7)+/-:	alignment on the direct(+) or reverse(-) chain of the reference;
8)chr:	id of reference sequence;
9)location: location of first bp on the reference, counted from 1;
10)types:	type of hits.
	"0":	exact match
	"1~100	RefAllele->OffsetQueryAlleleQual":	number of mismatches, followed by detailed mutation sites and switch of allele types. Offset is relative to the initial location on reference. 'OffsetAlleleQual': offset, allele, and quality. Example: "2	A->10T30	C->13A32" means there are two mismatches, one on location+10 of reference, and the other on location+13 of reference. The allele on reference is A and C respectively, while query allele type and its quality is T,30 and A,32.
	"100+n	Offset":	n-bp insertion on read. Example: "101 15" means 1-bp insertion on read, start after location+15 on reference.
	"200+n Offset":	n-bp deletion on read. Example: "202 16" means 2-bp deletion on query, start after 16bp on reference.

5. Algorithm:
The program will load reference sequences into RAM, create hash tables for seed indexing. Then for each query, search for seeded hits, do alignment and output the result.

1) Load in reference sequences
On the contrary to Eland and Maq, which load query reads into RAM. SOAP stored the reference sequences in RAM. Two bits for each base, so one byte can store 4 bps. In theory, it will need L/4 bytes for reference with total sequence size L.

2) Create seed indexing tables
Suppose a read is splitted into 4 parts-a,b,c,d, two mismatches will be distributed on at most two of the 4 parts at the same time. So if use the combination of two parts as seed, and check for mismatches in the remainning parts, it will be able to get all hits with up to 2 mismatches. There are six combinations - ab,ac,ad,bc,bd,cd, and essentially 3 types of seeds-ab,ac,ad. So we build 3 index tables. To save RAM, we set a skip of 3-bp on the reference. The strategy is essentially the same as that used in the Eland and Maq program.

3) look up table
We used look up table to judge how many mismatches between reference and read. To have best efficiency, the table used 3 bytes to check a fragment of 12-bp on a time. The table occupied 2^24=16Mb RAM.

4) search for hits
We will search for identical hits first, if no hits, then 1-mismatch hits will be picked up, then 2-mismatch hits, then gapped hits.

6. Evaluation:
Tested on a real dataset of 9,914,527 query reads (length 32bp) against a 5Mb human region, we compared the speed and sensitivity among Blastn, blat, Eland, Maq and SOAP. See following results:

Program                 Time consumed (s)          Reads aligned (%)
blastn (-F F -W 11)        165780                         85.47
blastn (-F F -W 15)        150660                         84.66
Blat (-tileSize=8)           22032                          85.07
Eland                         166                             88.53
Maq                           458                             88.39
Soap                          134                             88.46
Soap iterative               161                              90.9
Soap iterative+gapped    486                             91.15  

SOAP and Eland is almost 300 (gapped) or 1200 (ungapped) times faster than blastn, while having better sensitivity. SOAP interative improved sensitivity, and gapped alignment will further identify the gapped hits.

7. Important notes:
1) Simple rules to set parameter of seed size
a)S*2+3<=Min(Read length);
b)Hash size=4^S, normally S<=12bp;
c)Larger S will be faster

2) Avoid very short sequence in reference
The program will collapse if there are some reference sequences with size shorter than the query reads. So please make sure that short reference sequences were prefiltered before launching the alignment.

8. Citation:
If you find SOAP did help your research, please cite the paper:
Ruiqiang Li, et. al. SOAP: short oligonucleotide alignment program. Bioinformatics. 2008 24: 713-714

9. The other utilities

Soap_dealign is a program to do all kinds of data analysis and statistics, including sorting by chromosome location, depth distribution along chromosome, hist of depth distribution, gc vs depth, QC, and etc.

10. Future developments
1) Since sequencing read is getting more and more longer, we are considering to split a read into 3 parts, then if allow at most two mismatches, there will always be at least one part can be used as the seed. This treatment will reduce almost half running time compared with the current version;

2) Support gzip input files;

3) Provide multiple output format;


