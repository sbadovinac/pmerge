// Sam Badovinac
// Dan Mallerdino
// Pmerge
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <time.h>
#include <cstdlib>
#include <math.h>
#include <algorithm>
#include "mpi.h" // message passing interface

using namespace std;

// New compile and run commands for MPI!
// mpicxx -o blah file.cpp
// mpirun -q -np 32 blah

//global variables
int *a = new int[32];
int my_rank;
int p;

//ran into issues with "pmerge not declared" so i moved it to the bottom
void pmerge(int *, int, int, int, int);
void smerge(int *, int *, int, int, int, int, int);
void mergeSort(int *, int, int);
int Rank(int *, int, int, int);
void print(int *, int);

int main(int argc, char *argv[])
{
	int source;		   // rank of the sender
	int dest;		   // rank of destination
	int tag = 0;	   // message number
	char message[100]; // message itself
	MPI_Status status; // return status for receive

	// Start MPI
	MPI_Init(&argc, &argv);

	// Find out my rank!
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	// Find out the number of processes!
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// THE REAL PROGRAM IS HERE

	//Split up the problem
	int n;
	
	if (my_rank == 0)
	{
		cout <<"Array size: ";
		cin >> n;
	}
	//sending n to each processor
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	 
	int *a = new int[n];
	//Process 0 creates and brodcasts the entire array
	if (my_rank == 0)
	{
		srand(1251);
		int b[n];
		//Fill the array with random numbers
		for (int x = 0; x < n; x++)
		{
			a[x] = rand() % 100000;
			b[x] = a[x];
		}

		
	}
	// Broadcast the array to all processes
	MPI_Bcast(&a[0], n, MPI_INT, 0, MPI_COMM_WORLD);

	mergeSort(a, 0, n - 1);

	// Print final merged array
	if (my_rank == 1)
	{
		cout << "~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		cout << "FINAL = {";
		for (int h = 0; h < n; h++)
			cout << a[h] << " ";

		cout << "}" << endl;
	}

	MPI_Finalize();
	delete[] a;
	return 0;
}

void pmerge(int *a, int first, int last, int mid, int n)
{
	int currentprocessor = my_rank;
	//Calculate SRANKA and SRANKB size, might need tweaking with log2(n/2) being a decimal
	double size = ceil((double)(n / 2));
	int SRANKsize = size / (log(size) / log(2));

	//cout << "SRANKsize: " << SRANKsize << endl;

	int *localSRANKA = new int[SRANKsize];
	int *localSRANKB = new int[SRANKsize];
	int *SRANKA = new int[SRANKsize];
	int *SRANKB = new int[SRANKsize];

	for (int x = 0; x < SRANKsize; x++) // filling arrays with 0
	{
		localSRANKA[x] = 0;
		localSRANKB[x] = 0;
		SRANKA[x] = 0;
		SRANKB[x] = 0;
	}

	//Calculate SRANK
	for (int x = my_rank; x < SRANKsize; x += p)
	{
		int Rightvaluetofind = a[(int)(x * (log(size) / log(2)))]; // sampled values
		int Leftvaluetofind = a[mid + 1 + (int)(x * (log(size) / log(2)))];

		int *b = new int[(int)(size)];
		for (int i = 0; i < (int)size; i++) b[i] = a[mid + 1 + i];
		
		localSRANKA[x] = Rank(&a[n/2], 0, mid, Rightvaluetofind);
	
		localSRANKB[x] = Rank(&a[0], 0, mid, Leftvaluetofind);
	}

	//will need to reduce localsrank to srank
	MPI_Allreduce(localSRANKA, SRANKA, SRANKsize, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(localSRANKB, SRANKB, SRANKsize, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

	//filling Right Hand Merge Array
	int *MergeArrayA = new int[2 * SRANKsize];

	for (int x = 0; x < SRANKsize; x++)
		MergeArrayA[x] = x * (log(size) / log(2));

	for (int x = SRANKsize; x < 2 * SRANKsize; x++)
		MergeArrayA[x] = SRANKA[x - SRANKsize];

	MergeArrayA[2 * SRANKsize] = n / 2; //Filling in the final value of MergeArrayA

	sort(MergeArrayA, MergeArrayA + (2 * SRANKsize)); //SORT MERGEARRAYA

	//Filling Left Hand Merge Array
	int *MergeArrayB = new int[2 * SRANKsize];

	for (int x = 0; x < SRANKsize; x++)
		MergeArrayB[x] = x * (log(size) / log(2));

	for (int x = SRANKsize; x < 2 * SRANKsize; x++)
		MergeArrayB[x] = SRANKB[x - SRANKsize];

	MergeArrayB[2 * SRANKsize] = n / 2; //Filling in the final value of MergeArrayB

	sort(MergeArrayB, MergeArrayB + (2 * SRANKsize)); //SORT MERGEARRAYB

	

	//output array when smerge works
	int *WIN = new int[n];
	for (int i = 0; i < n; i++)
		WIN[i] = 0;

	int *FINAL = new int[n];
	for (int i = 0; i < n; i++)
		FINAL[i] = 0;
	
	for (int x = my_rank; x < (2 * SRANKsize) ; x += p)
		smerge(&WIN[0], &a[0], MergeArrayB[(int)x], MergeArrayB[(int)x + 1] - 1, MergeArrayA[x] + (int)size, MergeArrayA[x + 1] + (int)size - 1, n);
	

	MPI_Allreduce(WIN, FINAL, n, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	for (int i = 0; i < n; i++)
		a[i] = FINAL[i];
	
	delete[] localSRANKA;
	delete[] localSRANKB;
	return;
}

void smerge(int *WIN, int *a, int first1, int last1, int first2, int last2, int n)
{
	int i, j, k;
	int sizeL = last1 - first1 + 1;
	int sizeR = last2 - first2 + 1;

	if (sizeL < 1)
		sizeL = 0;

	if (sizeR < 1)
		sizeR = 0;

	int *L = new int[sizeL]; //left half array
	int *R = new int[sizeR]; //right half array
							 //Setting L and R to 0

	for (int h = 0; h < sizeL; h++)
		L[h] = 0;

	for (int h = 0; h < sizeR; h++)
		R[h] = 0;

	if (first1 > last1 && sizeL > 0)
		for (i = 0; i < 1; i++)
			L[i] = 0;
	
	else
		for (i = 0; i < sizeL; i++)
			L[i] = a[first1 + i];
	

	if (first2 > last2 && sizeR > 0)
		for (j = 0; j < 1; j++)
			R[j] = 0;
	
	else
		for (j = 0; j < sizeR; j++)
			R[j] = a[first2 + j];
	

	i = 0;						   // Initial index of first subarray
	j = 0;						   // Initial index of second subarray
	k = first1 + first2 - (n / 2); // Initial index of merged subarray

	while (i < sizeL && j < sizeR)//do the merge
		if (L[i] < R[j])
			WIN[k++] = L[i++];
		else
			WIN[k++] = R[j++];
	

	while (i < sizeL)
		WIN[k++] = L[i++];
	

	while (j < sizeR)//toss the remaining items into that young array
		WIN[k++] = R[j++];
	
}

void mergeSort(int *a, int first, int last)
{
	int n = last + 1;
	int middle = first + (last - first) / 2;

	if (n <= 4)
	{
		sort(a, a + 4);
		return;
	}

	if (first < last)
	{
		mergeSort(&a[0], first, middle);

		mergeSort(&a[n / 2], first, middle);

		pmerge(&a[first], first, last, middle, n);
	}
}

int Rank(int *b, int first, int last, int valToFind)
{
	//Special case
	if (valToFind > b[last - 1]){return last+1;}

	// trivial case, array size of 1
	if (last == 1)
		if (valToFind <= b[0])
			return 0;
		else
			return 1;
	else
		if (valToFind < b[(last / 2) -1])// if value is less than middle value try again with left half of array
			return Rank(&b[0], 0, last / 2, valToFind);
		else //&a[last/2] cuts the pointer of the array such that it is indexed starting at last/2 as position 0 in order to look at right half of array
			return Rank(&b[(last / 2)], 0, last / 2, valToFind) + (last / 2);
}

void print(int *a, int n)
{
	for (int x = 0; x < n; x++)
	{
		if (x % 16 != 0 && a[x] < 10)
			cout << "  " << a[x];
		else if (x % 16 != 0 && a[x] >= 10)
			cout << " " << a[x];
		else if (x % 16 == 0 && a[x] < 10 && x != 0)
			cout << "  " << a[x] << endl;
		else if (x % 16 == 0 && a[x] >= 10 && x != 0)
			cout << " " << a[x] << endl;
		else if (a[x] >= 10 && x == 0)
			cout << " " << a[x];
		else if (a[x] < 10 && x == 0)
			cout << "  " << a[x];
	}
	cout << endl;
}
