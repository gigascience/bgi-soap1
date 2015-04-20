#include<unistd.h>
#include<cstdio>
#include<iostream>
#include<ostream>
#include<fstream>
#include<string>
#include<vector>
#include "reads.h"
#include "dbseq.h"
#include "align.h"
#include "param.h"
#include "pairs.h"
#include "utilities.h"

#ifdef THREAD
#include<pthread.h>
#endif

using namespace std;

//global variables
Param param;
string query_a_file;
string query_b_file;
string ref_file;
string out_align_file;

ifstream fin_db;
ifstream fin_a;
ifstream fin_b;
ofstream fout;
ReadClass read_a;
ReadClass read_b;
RefSeq ref;
bit32_t QC_filtered=0;  //number of reads filter for low-quality
bit32_t QC_filtered_pairs=0;
bit32_t QC_filtered_a=0;
bit32_t QC_filtered_b=0;

#ifdef THREAD
pthread_mutex_t mutex_fin=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_fout=PTHREAD_MUTEX_INITIALIZER;

void *t_SingleAlign(void *)
{
	SingleAlign a;
	int n;
	bit32_t cur_at;
	a.ImportFileFormat(read_a._file_format);
	a.SetFlag('a');
	while(1)
	{
		pthread_mutex_lock(&mutex_fin);
		n=read_a.LoadBatchReads(fin_a);
		cur_at=read_a._index;
		a.ImportBatchReads(read_a.num, read_a.mreads);
		pthread_mutex_unlock(&mutex_fin);
		if(!n)
			break;
		a.Do_Batch(ref);
		pthread_mutex_lock(&mutex_fout);
		fout<<a._str_align;
		cout<<cur_at<<" reads finished. "<<Cal_AllTime()<<" secs passed"<<endl;
		pthread_mutex_unlock(&mutex_fout);		
	}
	pthread_mutex_lock(&mutex_fout);
	QC_filtered+=a.n_filtered;
	pthread_mutex_unlock(&mutex_fout);
};
void Do_SingleAlign()
{
	read_a.CheckFile(fin_a);
	vector<pthread_t> pthread_ids(param.num_procs);
	//create
	for(int i=0; i<param.num_procs; i++)
		pthread_create(&pthread_ids[i], NULL, t_SingleAlign, NULL);
	//join
	for (int i=0; i<param.num_procs; i++)
		pthread_join(pthread_ids[i], NULL);
};


void *t_PairAlign(void *)
{
	PairAlign a;
	int n1, n2;
	bit32_t cur_at;
	a.ImportFileFormat(read_a._file_format, read_b._file_format);
	while(1) {
		pthread_mutex_lock(&mutex_fin);
		n1=read_a.LoadBatchReads(fin_a);
		n2=read_b.LoadBatchReads(fin_b);
		cur_at=read_a._index;
		a.ImportBatchReads(n1, read_a.mreads, read_b.mreads);
		pthread_mutex_unlock(&mutex_fin);
		if(!n1||(n1!=n2))
			break;
		a.Do_Batch(ref);
		pthread_mutex_lock(&mutex_fout);
		fout<<a._str_align;
		cout<<cur_at<<" reads finished. "<<Cal_AllTime()<<" secs passed"<<endl;
		pthread_mutex_unlock(&mutex_fout);		
	}
	pthread_mutex_lock(&mutex_fout);
	QC_filtered_pairs+=a.n_filtered_pairs;
	QC_filtered_a+=a.n_filtered_a;
	QC_filtered_b+=a.n_filtered_b;
	pthread_mutex_unlock(&mutex_fout);		
};

void Do_PairAlign()
{
	read_a.CheckFile(fin_a);
	read_b.CheckFile(fin_b);
	
	vector<pthread_t> pthread_ids(param.num_procs);
	//create
	for(int i=0; i<param.num_procs; i++)
		pthread_create(&pthread_ids[i], NULL, t_PairAlign, NULL);
	//join
	for (int i=0; i<param.num_procs; i++)
		pthread_join(pthread_ids[i], NULL);
	//
};

void *t_SeedFreq_ab(void *)
{
	ref.t_CalKmerFreq_ab();
}
void *t_SeedFreq_ac(void *)
{
	ref.t_CalKmerFreq_ac();
}
void *t_SeedFreq_ad(void *)
{
	ref.t_CalKmerFreq_ad();
}
void *t_Index_ab(void *)
{
	ref.t_CreateIndex_ab();
}
void *t_Index_ac(void *)
{
	ref.t_CreateIndex_ac();
}
void *t_Index_ad(void *)
{
	ref.t_CreateIndex_ad();
}
void Do_Formatdb()
{
	ref.InitialIndex();
	pthread_t pids_ab, pids_ac, pids_ad;
	//cal kmer freq
	pthread_create(&pids_ab, NULL, t_SeedFreq_ab, NULL);
	pthread_create(&pids_ac, NULL, t_SeedFreq_ac, NULL);
	pthread_create(&pids_ad, NULL, t_SeedFreq_ad, NULL);
	pthread_join(pids_ab, NULL);
	pthread_join(pids_ac, NULL);
	pthread_join(pids_ad, NULL);
	ref.AllocIndex();
	//record kmer locations
	pthread_create(&pids_ab, NULL, t_Index_ab, NULL);
	pthread_create(&pids_ac, NULL, t_Index_ac, NULL);
	pthread_create(&pids_ad, NULL, t_Index_ad, NULL);
	pthread_join(pids_ab, NULL);
	pthread_join(pids_ac, NULL);
	pthread_join(pids_ad, NULL);
	//
	ref._blocks.clear();
	cout<<"Create seed table. "<<Cal_AllTime()<<" secs passed"<<endl;	
}
#else
void Do_SingleAlign()
{
	read_a.CheckFile(fin_a);
	SingleAlign a;
	a.ImportFileFormat(read_a._file_format);
	a.SetFlag('a');
	while(read_a.LoadBatchReads(fin_a))
	{
		a.ImportBatchReads(read_a.num, read_a.mreads);
		a.Do_Batch(ref);
		fout<<a._str_align;
		cout<<read_a._index<<" reads finished. "<<Cal_AllTime()<<" secs passed"<<endl;
	}
	QC_filtered=a.n_filtered;	
}

void Do_PairAlign()
{
	read_a.CheckFile(fin_a);
	read_b.CheckFile(fin_b);
	PairAlign a;
	int n1, n2;
	while(1)
	{
		n1=read_a.LoadBatchReads(fin_a);
		n2=read_b.LoadBatchReads(fin_b);
		if(!n1||(n1!=n2))
			break;
		a.ImportBatchReads(n1, read_a.mreads, read_b.mreads);
		a.Do_Batch(ref);
		fout<<a._str_align;		
		cout<<read_a._index<<" reads finished. "<<Cal_AllTime()<<" secs passed"<<endl;
	}
	QC_filtered_pairs=a.n_filtered_pairs;
	QC_filtered_a=a.n_filtered_a;
	QC_filtered_b=a.n_filtered_b;	
}

void Do_Formatdb()
{
	ref.CreateIndex();
	cout<<"Create seed table. "<<Cal_AllTime()<<" secs passed"<<endl;
}
#endif

//usage
void usage(void)
{
	cout<<"Usage:	soap [options]\n"
		<<"	-a	<string>	query a file, *.fq or *.fa format\n"
		<<"	-d	<string>	reference sequences file, *.fa format\n"
		<<"	-o	<string>	output align file\n"
		<<"	-s	<int>	seed size, default="<<param.seed_size<<"\n"
		<<"	-v	<int>	maximum number of snps allowed on a read, <="<<MAXSNPS<<". default="<<param.max_snp_num<<"bp\n"
		<<"	-g	<int>	maximum gap size allowed on a read, default="<<param.max_gap_size<<"bp\n"
		<<"	-e	<int>	will not allow gap exist inside n-bp edge of a read, default="<<param.gap_edge<<"bp\n"
		<<"	-q	<int>	quality threshold of snp, 0-40, default="<<int(param.qual_threshold)<<"\n"
		<<"	-z	<char>	initial quality, default="<<param.zero_qual<<"\n"
		<<"	-c	<int>	how to trim low-quality at 3end?\n"
		<<"		0:	don't trim;\n"
		<<"		1-10:	trim n-bps at 3-end;\n"
		<<"		11-20:	trim first bp and (n-10)-bp at 3-end;\n"
		<<"		21-30:	trim (n-20)-bp at 3-end if no hit for original read;\n"
		<<"		31-40:	trim first bp and (n-30)-bp at 3-end if no hit for original read;\n"
		<<"		41-50:	iteratively trim (n-40)-bp at 3-end until get hits;\n"
		<<"		51-60:	same as 40-49, but trim first bp at beginning;\n"
//		<<"		100:	automated trimming according to quality values.\n"
		<<"		default:	"<<param.trim_lowQ<<"\n"
		<<"	-f	<int>	filter reads containing >n Ns, default="<<param.max_ns<<"\n"
		<<"	-r	[0,1,2]	how to report repeat hits, 0=none; 1=random one; 2=all, default="<<param.report_repeat_hits<<"\n"
		<<"	-t	output read index, default output read id\n"
		<<"	-n	<int>	do alignment for which chain? 0:both; 1:direct only; 2:complementary only. default="<<param.chains<<"\n"
#ifdef THREAD
		<<"	-p	<int>	number of processors, default="<<param.num_procs<<"\n"
#endif	
		<<"	-h	help\n\n"
		<<"Options for pair-end alignment:\n"
		<<"	-b	<string>	query b file\n"
		<<"	-m	<int>	minimal insert size of pair-end, default="<<param.min_insert<<"\n"
		<<"	-x	<int>	maximal insert size of pair-end, default="<<param.max_insert<<"\n"
		<<"Options for mRNA tag alignment:\n"
		<<"	-T	<int>	type of tag, 0:DpnII, GATC+16; 1:NlaIII, CATG+17. default="<<param.tag_type<<"\n"
		<<"Options for miRNA alignment:\n"
		<<"	-A	<string>	3-end adapter sequence, default="<<param.adapter<<"\n"
		<<"	-S	<int>	number of mismatch allowed in adapter, default="<<param.admis<<"\n"
		<<"	-M	<int>	minimum length of miRNA, default="<<param.mirna_min<<"\n"
		<<"	-X	<int>	maximum length of miRNA, default="<<param.mirna_max<<"\n"
		<<"Batch command: pessat -d <string> <parameter file>\n\n"
		<<"Note: all numbers are counted from 1\n"
		<<"seed_size*2+3<=min_read_size, AND seed_size<=12\n"
		<<"format of alignment:\n"
		<<"id, seq, qual, # of hits, a/b, length, +/-, chr, location, type\n"
		<<"types: 0, exact match;\n"
		<<"       n OffsetAlleleQual, n snps with offset, allele and quality, ex: 1 C->10T30, 1-bp snp at location+10 of chr, ref allele C, query allele T;\n"
		<<"       100+n Offset, n-bp insertion on query, ex: 101 15, 1-bp insertion on query, start after 15bp of chr\n"
		<<"       200+n Offset, n-bp deletion on query, ex: 201 16, 1-bp deletion on query, start after 16bp of chr\n"
		<<endl;
	exit(1);
};

int mGetOptions(int rgc, char *rgv[])
{
	//[options]
	int i;
	for(i=1; i<rgc; i++) {
		if(rgv[i][0]!='-')
			return i;
		switch(rgv[i][1]) {
			case 'a': query_a_file = rgv[++i]; break;
			case 'b': query_b_file = rgv[++i]; break;
			case 'd': ref_file = rgv[++i]; break;
			case 's': param.SetSeedSize(atoi(rgv[++i])); break;
			case 'o': out_align_file = rgv[++i]; break;
			case 'm': param.min_insert = atoi(rgv[++i]); break;
			case 'x': param.max_insert = atoi(rgv[++i]); break;
			case 'v': param.max_snp_num = atoi(rgv[++i]); if(param.max_snp_num>MAXSNPS) usage(); break;
			case 'g': param.max_gap_size = atoi(rgv[++i]); break;
			case 'e': param.gap_edge = atoi(rgv[++i]); break;
			case 'q': param.qual_threshold = atoi(rgv[++i]); break;
			case 'c': param.trim_lowQ=atoi(rgv[++i]); break;
			case 'f': param.max_ns=atoi(rgv[++i]); break;
			case 'z': param.zero_qual = rgv[++i][0]; break;
			case 'r': param.report_repeat_hits = atoi(rgv[++i]); break;
			case 'p': param.num_procs=atoi(rgv[++i]); break;
			case 't': param.output_id=0; break;
			case 'n': param.chains=atoi(rgv[++i]); break;
			case 'T': param.SetMrnaTag(atoi(rgv[++i])); break;
			case 'A': param.adapter=rgv[++i]; break;
			case 'S': param.admis=atoi(rgv[++i]); break;
			case 'M': param.mirna_min=atoi(rgv[++i]); break;
			case 'X': param.mirna_max=atoi(rgv[++i]); break;
			case 'h':usage();    //usage information
			case '?':usage();    //unrecognizable input
		}
	}
	return i;
}
void RunProcess(void)
{
	//pair-end alignment
	if((!query_a_file.empty()) && (!query_b_file.empty()))
	{
		cout<<"Pair-end alignment:\n";
		cout<<"Query: "<<query_a_file<<"  "<<query_b_file<<"  Reference: "<<ref_file<<"  Output: "<<out_align_file<<endl;
		fin_a.open(query_a_file.c_str());
		if(!fin_a) {
			cerr<<"failed to open file: "<<query_a_file<<endl;
			exit(1);
		}
		fin_b.open(query_b_file.c_str());
		if(!fin_b) {
			cerr<<"failed to open file: "<<query_b_file<<endl;
			exit(1);
		}		
		fout.open(out_align_file.c_str());
		if(!fout) {
			cerr<<"failed to open file: "<<out_align_file<<endl;
			exit(1);
		}
		read_a.InitialIndex();
		read_b.InitialIndex();
		Do_PairAlign();
		fin_a.close();
		fin_b.close();
		fout.close();
		cout<<"Total number of filtered low-quality reads: \n"
			<<"pairs:    "<<QC_filtered_pairs<<"\n"
			<<"single a: "<<QC_filtered_a<<"\n"
			<<"single b: "<<QC_filtered_b<<"\n";
	}	
	//single-read alignment
	else
	{
		if(!query_a_file.empty()) {
			fin_a.open(query_a_file.c_str());
			if(!fin_a) {
				cerr<<"b failed to open file: "<<query_a_file<<endl;
				exit(1);
			}
		}
		else
		{
			cerr<<"missing query file(s)\n";
			exit(1);
		}
		cout<<"Single read alignment:\n";
		cout<<"Query: "<<query_a_file<<"  Reference: "<<ref_file<<"  Output: "<<out_align_file<<endl;
		fout.open(out_align_file.c_str());
		if(!fout) {
			cerr<<"failed to open file: "<<out_align_file<<endl;
			exit(1);
		}
		read_a.InitialIndex();
		Do_SingleAlign();
		fin_a.close();
		fout.close();
		cout<<"Total number of filtered low-quality reads: "<<QC_filtered<<"\n";
	}

	cout<<"Done.\n";
	cout<<"Finished at "<<Curr_Time();
	cout<<"Total time consumed:  "<<Cal_AllTime()<<" secs\n";
}
int main(int argc, char *argv[])
{
	//print usage
	if (argc == 1)
	{
		usage();
	}
	Initial_Time();
	cout<<"Start at:  "<<Curr_Time()<<endl;
	int noptions;
	noptions=mGetOptions(argc, argv);
	fin_db.open(ref_file.c_str());
	if(!fin_db) {
		cerr<<"fatal error: failed to open ref file\n";
		exit(1);
	}
	ref.Run_ConvertBinseq(fin_db);
	cout<<"Load in "<<ref.total_num<<" db seqs, total size "<<ref.sum_length<<" bp. "<<Cal_AllTime()<<" secs passed"<<endl;			
	//single command:
	if(noptions==argc) {
		Do_Formatdb();
		RunProcess();
	}
  else {
  	int old_seed_size=0;
  	char * margv[1000];
  	for(int i=0; i<1000; i++) {
  		margv[i] = new char[1000];
  	}
  	char ch[10000];
  	ifstream fin_batch(argv[noptions]);
  	while(!fin_batch.eof()) {
  		fin_batch.getline(ch, 10000);
  		if(fin_batch.eof())
  			break;
  		cout<<"Line of options:  "<<ch<<endl;
  		bool is_word=0;
  		int margc=0;
  		char *q=margv[margc];
  		for(int i=0; ch[i]!='\0'; i++) {
  			if((ch[i]>=33)&&(ch[i]<=126)) {
  				if(!is_word) {
  					*q='\0';
  					margc++;
  					q=margv[margc];
  				}
  				is_word=1;
  				*q++=ch[i];
  			}
  			else {
  				is_word=0;
  			}
  		}
  		*q='\0';
  		margc++;
  		mGetOptions(margc, margv);
  		if(param.seed_size!=old_seed_size) {
  			if(ref.total_kmers>0)
  				ref.ReleaseIndex();
  			Do_Formatdb();
  			old_seed_size=param.seed_size;
  		}
  		RunProcess();
  	}
  	fin_batch.close();
  }
	return 0;
}
