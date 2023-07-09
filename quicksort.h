#include <algorithm>
#include <iostream>
#include <cstdio>
#include <memory>
#include <chrono>
#include <thread>

#include "parallel.h"

using namespace parlay;
using Type = long long;

// TODO: Remove
using namespace std;


/*
	naiveScan: 
		- Sequential scan to compare performance between parallel & iterative
		- Uses flag array and modifies prefix sum
*/

void naiveScan(int *psum, bool *flags, size_t start, size_t end) {
	psum[start] = flags[start];
	for (size_t i = start+1; i < end; i++) {
		psum[i] = flags[i] + psum[i-1];
	}
	return;
}

/*
	realFilter: 
		- Filters in parallel using Parlay's par_for 
*/

template <class T>
void RealFilter(T *A, bool *flags1, bool *flags2, Type pivot, size_t start, size_t end) {
	parallel_for(start, end, [&](size_t i) {
			flags1[i] = (A[i] < pivot);
			flags2[i] = (A[i] > pivot);
			});
	return;
}

/*
	upsweep / downsweep:
		- helper functions for sweepScan

*/
template <class T>
void upsweep(T *A, size_t s, size_t t) {
	if (s == t) {
		return;
	}
	auto f1 = [&]() {upsweep(A, s, (s+t)/2);};
	auto f2 = [&]() {upsweep(A, ((s+t)/2)+1, t);};
	par_do(f1,f2);
	A[t] = A[t] + A[((s+t)/2)];
}	

template <class T>
void downsweep(T *A, size_t s, size_t t, Type p) {
	if (s == t) {
		A[s] = p;
		return;
	}
	auto LeftSum = A[((s+t)/2)];
	auto f1 = [&]() {downsweep(A, s, (s+t)/2, p);};
	auto f2 = [&]() {downsweep(A, (s+t)/2 + 1, t, p+LeftSum);};
	par_do(f1,f2);
}



template <class T>
void sweepScan(T *A, size_t s, size_t t, Type p) {
	upsweep(A, s, t);
	downsweep(A, s, t, p);
	return;
}


/*
	showScan:
		- Used to output any array for debugging

*/
template <class T>
void showScan(T *flags, size_t n) {
	for (size_t i = 0; i < n; i++) {
		std::cout << flags[i] << " ";
	}
	std::cout << '\n';
}

template <class T>
void ccopy(T *psum, T *flags, size_t start, size_t end){
	parallel_for(start, end, [&](size_t i) {
			psum[i] = flags[i];
			});
	return;
}

template <class T>
void show(T *A, size_t s, size_t e) {
	for (size_t i = s; i < e; i++) {
		cout << A[i] << " ";
	}
	cout << endl;
}


/*
	partition:
		- Main function to create quicksort partitions. 
		- Returns a std::pair of the bounds of the partition (start / end)

*/


template <class T>
pair<size_t, size_t> partition(T *A, T *B, bool *flags1, bool *flags2, int *psum1, int *psum2, size_t start, size_t end, size_t n, size_t pivIndex) {
	Type pivVal = A[pivIndex];
	RealFilter(A, flags1, flags2, pivVal, start, end);

	auto fun1 = [&]() {ss(psum1, flags1, start, end);};
	auto fun2 = [&]() {ss(psum2, flags2, start, end);};
	par_do(fun1, fun2);

	size_t numLessElements = psum1[end-1];
	size_t numGreaterElements = psum2[end-1];

	auto fun3 = [&]() {parallel_for (start, end, [&](size_t i) {
				if (flags1[i] == 0 && flags2[i] == 0) {
					size_t idx = i+numLessElements - psum1[i] - psum2[i];
					B[idx] = pivVal;
				}
				});};
	auto fun4 = [&]() {parallel_for (start, end, [&](size_t i) {
				if (flags1[i] == 1) {
					size_t idx = start + psum1[i]-1;
					B[idx] = A[i];
				}
				else if (flags2[i] == 1) {
					size_t idx = end - psum2[i];
					B[idx] = A[i];
				}
	});};
	par_do(fun3, fun4);	
	ccopy(A, B, start, end);

	pair p = make_pair(start + numLessElements, end-numGreaterElements);
	return p;
}


template <class T>
void helper(T *A, T *B, bool *flags1, bool *flags2, int *psum1, int *psum2, size_t start, size_t end, size_t n) {
	
	// Granularity 
	if (n <= 10000) {
		sort(A+start, A+end);
		return;
	}
	if (start == end) {
		return;
	}
	size_t sz = end - start;
	auto t = partition(A, B, flags1, flags2, psum1, psum2, start, end, n, (start + ((start + end)/ 2) % sz));
	if (t.first == t.second) {
		return;
	}
	auto f1 = [&]() {helper(A, B, flags1, flags2, psum1, psum2, start, t.first, n-t.second);};
	auto f2 = [&]() {helper(A, B, flags1, flags2, psum1, psum2, t.second, end, n-t.first);};

	par_do(f1, f2);
}


template <class T>
void quicksort(T *A, size_t n) {
	
	bool* flags1 = (bool*)malloc(n * sizeof(bool));
	bool* flags2 = (bool*)malloc(n * sizeof(bool));
	int* psum1   = (int*)malloc(n * sizeof(int));
	int* psum2   = (int*)malloc(n * sizeof(int));
	T* B         = (Type*)malloc(n * sizeof(Type));

	helper(A, B, flags1, flags2, psum1, psum2, 0, n, n);

	free(flags1);
	free(flags2);
	free(psum1);
	free(psum2);
	free(B);

	return;
}
