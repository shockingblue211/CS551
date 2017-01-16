// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - RENJIAN JIANG

#include <stdlib.h>
#include <stdio.h>
#include <math.h>


typedef struct _seg {  /* definition of new type "seg" */
    int  bits[256];
    struct _seg  *next,*prev;        
      }seg  ;

#define BITSPERSEG  (8*256*sizeof(int))

 //define as global (even outside main function), so it's not in main's scope and can be used by whichseg     
 seg *head,*pt;	//Linklist head, listlist pointer


// Which Seg
seg* whichseg (int j){
	seg *pntr; 
	int segnum, i;
	segnum = (j + BITSPERSEG) / BITSPERSEG;//the number of bit in the bitmap part of seg (not including the two referece variable)
		pntr = head;
		for (i = 1; i < segnum; i++) { //Just Forward Links for Now
		pntr=pntr->next;
		}
	return pntr;
}

// int whichsegint (int j){
// 	int segnum, i;
// 	segnum = (j + BITSPERSEG) / BITSPERSEG;//the number of bit in the bitmap part of seg (not including the two referece variable)
// 	return segnum;
// }

int whichint (int j){
		return ((j % BITSPERSEG) / (8 * sizeof(int)));
}

int whichbit (int j){
	return ((j % BITSPERSEG) % (8 * sizeof(int)));
}


 void main(int argc, char const *argv[])
 {
 	int N, N_odd, howmany;	//N, number of odds<=N

 		if (argc == 2) sscanf(argv[1],"%d",&N);
		else scanf("%d",&N);

	printf("Calculating odd primes up to %d... \n", N);

//Calculate the number of odd numbers <= N:
	if ((N%2) == 0)
		N_odd = (N/2) - 1; //no need to store 1, so -1.
	else
		N_odd = ((N+1)/2) - 1; // same as above.
	// printf("Number of odd numbers<= N:%d\n", N_odd );

//Calculate the number of segments to be allocated:
	howmany = (N_odd +BITSPERSEG -1)/BITSPERSEG;

	// printf("Number of segments needed:%d\n", howmany );

//Allocate enough segments:
	head= (  seg * ) malloc(sizeof(seg));
	pt=head;
	int i;
	for (i=1;i<howmany;i++) { //Just Forward Links for Now
		pt->next = (  seg *) malloc(sizeof (seg)); 
		pt=pt->next;
		}

	// printf("Done allocating %d nodes\n",i);

	// printf("The 25th int in the first segment: %d\n", head->bits[24]);
	//OK, all initially 0.

	int OddN ; //used for the 3 to sqrt(N) for loop
	int bitpt; 
	double sqrtNd; 

	sqrtNd= ceil(sqrt(N));
	// printf("sqrtN as double:%lf\n", sqrtNd);
	int sqrtN = sqrtNd;
	// printf("sqrtN as int:%d\n", sqrtN);


// TODO: change OddN to be less than sqrt(N) later.
	for (OddN = 3; OddN< sqrtN; OddN += 2){
		bitpt = (OddN - 3) / 2; //used for the bit coordinate of OddN
		// printf("The odd number:%d\n", OddN);
		// printf("The coordinating bit location:%d\n",  bitpt);
		//OddN and bitpt all works, just need to make sure the first OddN is not set to 1.

		bitpt = bitpt + OddN;
		// printf("Starting bit location:%d\n", bitpt);
// TODO: set compst to be less than N_odd later
		while(bitpt<N_odd){
			//confirming bit location: 
			// printf("Increased bit location: %d\n", bitpt);
			// printf("Which Seg:%d\n", whichsegint(bitpt));

			whichseg (bitpt) -> bits[whichint(bitpt)] |= 1 << (whichbit(bitpt)); 

			//confirming the odd number that the bit location coordinates to: 
			int OddN2 = ((bitpt * 2) + 3);
			// printf("Increased odd number's value: %d\n", OddN2);		
			//increase compst by the odd number:
			bitpt += OddN;
		}

		// printf("Which Seg:%d\n", whichsegint(bitpt));
		// printf("Which int:%d\n", whichint(bitpt));
		// printf("Which bit:%d\n", whichbit(bitpt));
	}

	//confirming the prime bit map:
	int l, checkprm, yesno, count;
	count = 0;
	for (l=0; l < N_odd; l++){
		checkprm = (l * 2) + 3; 
		yesno = ( (whichseg(l) -> bits[whichint(l)] & (1 << (whichbit(l)) )) != 0 ) ;
		if (!yesno) count++;
		// printf("Odd number=%d, it %d a composite \n", checkprm, yesno);
		// if (!yesno) printf("%d\n", checkprm);
	}
	printf("The number of odd primes less than or equal to %d is: %d\n", N, count);
	// so it works.

	printf("Enter Even Numbers >5 for Goldbach Tests\n");
	int even;
	while(scanf( "%d" , &even )!= EOF){

		if (even >= N) {
			printf("Error, the number you entered is larger than %d.\n", N);
			continue;
		}
		
		else if (even % 2 != 0){
			printf("Error, the number you entered is not an even number.\n");
			continue;
		}

		else if (even < 5){
			printf("Error, the number you entered <5.\n");
			continue;
		}
		
		else {
//TODO: Find the largest prime here:
			int solcnt, sm, lg, smbl, lgbl, evenhlf, evenhlfbl, smprm, lgprm;
			seg *smpt, *lgpt;
			// intial sm and lg odds from two ends:
			sm = 3;
			lg = even - 3;
			// half of the even number:
			evenhlf = even / 2;
			smbl = (sm - 3) / 2; 
			lgbl = (lg - 3) / 2;
			evenhlfbl = (evenhlf - 3) / 2;
			smpt = head; 
			lgpt = whichseg(lgbl);
			solcnt = 0;

			while (smbl <= evenhlfbl){
				// if both primes, overwrite smprm and lgprm:
				if (
					(( smpt -> bits[whichint(smbl)] & (1 << (whichbit(smbl)) )) == 0 )
					&
					(( lgpt -> bits[whichint(lgbl)] & (1 << (whichbit(lgbl)) )) == 0 )
																					)
				{
					smprm = (smbl * 2) + 3;
					lgprm = (lgbl * 2) + 3;
					solcnt++;
				}

				// increase the small bit location, decrease the large bit location:
				smbl++;
				lgbl--;

				// if modular == 0 after divided by BITSPERSEG, indicates need to jump to the next/ prev seg:
				if (smbl % BITSPERSEG == 0) smpt = smpt -> next;
				if (lgbl % BITSPERSEG == 0) lgpt = lgpt -> prev;

			}

			printf("Largest %d = %d + %d out of %d solutions\n", even, smprm, lgprm, solcnt);
			continue;
		}

	}
 }
