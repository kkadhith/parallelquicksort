#include <algorithm>
#include <iostream>
#include <cstdio>
#include <memory>
#include <chrono>
#include <thread>

#include "parallel.h"
//#include "schedulers/sequential.h"

using namespace parlay;

using Type = long long;
using namespace std;
//Type* B = (Type*)malloc(n * sizeof(Type));
//Type* A = (Type*)malloc(50 * sizeof(Type));
//Type* flags = (Type*)malloc(n * sizeof(Type));
//Type* psum = (Type*)malloc(n * sizeof(Type));
//
/*
Type *B;
Type *flags1;
Type *flags2;
Type *psum1;
Type *psum2;

Type *flags3;
Type *psum3;
*/

/*template <class T>
void scansequential(T *psum, T *flags, size_t s, size_t n) {
	psum[s] = 0;
	//psum[s] = flags[0];
	for (size_t i = s+1; i < n; i++) {
		psum[i] = flags[i] + psum[i-1];
	}
	return;
}*/

//template <class T>
void ss(int *psum, bool *flags, size_t start, size_t end) {
	psum[start] = flags[start];
	//psum[s] = flags[0];
	for (size_t i = start+1; i < end; i++) {
		psum[i] = flags[i] + psum[i-1];
	}
	return;
}

/*template <class T>
void filter(T *A, T *flags, size_t n, size_t pivIndex, bool lessThan) {
	Type pivot = A[pivIndex];
	if (lessThan == true) {
	parallel_for(0, n,
                 [&](size_t i) {
			if (A[i] < pivot) {
	       			flags[i] = 1;
			}
			else {
				flags[i] = 0;
			}		
	});
	}
	else {
		parallel_for(0, n,
                 [&](size_t i) {
			if (A[i] > pivot) {
	       			flags[i] = 1;
			}
			else {
				flags[i] = 0;
			}		
	});
	}
	return;
}*/

template <class T>
void RealFilter(T *A, bool *flags1, bool *flags2, Type pivot, size_t start, size_t end) {
	parallel_for(start, end, [&](size_t i) {
			flags1[i] = (A[i] < pivot);
			flags2[i] = (A[i] > pivot);
			});
	return;
}

template <class T>
void equalFilter(T *A, T *flags, size_t n, size_t pivIndex) {
	Type pivot = A[pivIndex];
	parallel_for(0,n, [&](size_t i) {
			if (A[i] == pivot) {
				flags[i] = 1;
			}
			else {
				flags[i] = 0;
			}
			});
	return;
}


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

/*size_t pp(size_t start, size_t end, size_t sz) {
        size_t p = (size_t)rand64((size_t)sz);
        p = p % sz;
        p = p + start;
        return p;
}*/


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
// Try RETURN TUPLE FOR START AND END 
//

template <class T>
pair<size_t, size_t> partition(T *A, T *B, bool *flags1, bool *flags2, int *psum1, int *psum2, size_t start, size_t end, size_t n, size_t pivIndex) {
	/*if (start == end) {
		// RETURN WHEN THEY ARE EQUAL (WE DOnt iterate)
		pair p = make_pair(start, end);
		return p;
	}*/
	// Filter LT and GT
	Type pivVal = A[pivIndex];
	//cout << "Pivot Value: " << pivVal << endl;
	RealFilter(A, flags1, flags2, pivVal, start, end);
	//cout << "A: ";
	//show(A, start, end);
	//cout << "flags:" << endl;
	//show(flags1, start, end);
	//show(flags2, start, end);
	//cout << '\n';
	//std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	// Compute Scans in Parallel
	// sweepScan();	
	auto fun1 = [&]() {ss(psum1, flags1, start, end);};
	auto fun2 = [&]() {ss(psum2, flags2, start, end);};
	par_do(fun1, fun2);
	//cout << "psum:" << endl;
	//show(psum1, start, end);
	//show(psum2, start, end);
	//cout << '\n' << '\n';
	//cout << "start: " << start << '\n';
	//cout << "end: " << end << '\n';
	size_t numLessElements = psum1[end-1];
	size_t numGreaterElements = psum2[end-1];
	//size_t numEqualElements = n - (numLessElements + numGreaterElements);
	//cout << "start:" << numLessElements << " -> " << numLessElements+numEqualElements << endl;
	/*auto fun3 = [&]() {parallel_for(numLessElements, numLessElements+numEqualElements, [&](size_t i) {
			B[i] = pivVal;
			});};
	
	auto fun4 = [&]() {parallel_for(start, end, [&](size_t i) {
			if (flags1[i] == true) {
				B[psum1[i]-1] = A[i];
				}
			else if (flags2[i] == true) {
				B[numLessElements+numEqualElements+psum2[i]-1] = A[i];
				}
				});};
	par_do(fun3, fun4);*/
	auto fun3 = [&]() {parallel_for (start, end, [&](size_t i) {
				if (flags1[i] == 0 && flags2[i] == 0) {
					size_t idx = i+numLessElements - psum1[i] - psum2[i];
					B[idx] = pivVal;
				}
				});};
	auto fun4 = [&]() {parallel_for (start, end, [&](size_t i) {
				if (flags1[i] == 1) {
					//size_t idx = start + psum1[i];
					size_t idx = start + psum1[i]-1;
					B[idx] = A[i];
				}
				else if (flags2[i] == 1) {
					size_t idx = end - psum2[i];
					//size_t idx = end - 1 - psum2[i];
					B[idx] = A[i];
				}
	});};
	par_do(fun3, fun4);	
	//showScan(B, n);
	ccopy(A, B, start, end);
	//cout << "A: ";
	//show(A, start, end);
	//cout << '\n' << '\n';
	//cout << "RET1: " << start+numLessElements << endl;
	//cout << "RET2: " << end - numGreaterElements << endl;
	//cout << "================================" << endl;
	//pair p = make_pair(start + numLessElements, (end - numGreaterElements) - 1);
	// end GREATER THAN # of greater elements
	// [3, 35, 8, 12, 2]
	// (s)     (e)
	pair p = make_pair(start + numLessElements, end-numGreaterElements);
	return p;
}

/*template <class T>
size_t partition(T *A, T *B, bool *flags1, bool *flags2, int *psum1, int *psum2, size_t n, size_t pivIndex) {

	// Filter LT and GT
	Type pivVal = A[pivIndex];

	RealFilter(A, flags1, flags2, pivVal, n);
	ccopy(psum1, flags1, n);
	ccopy(psum2, flags2, n);

	// Compute Scans in Parallel
	
	auto fun1 = [&]() {ss(psum1, flags1, n);};
	auto fun2 = [&]() {ss(psum2, flags2, n);};
	par_do(fun1, fun2);


	size_t numLessElements = psum1[n-1];
	size_t numGreaterElements = psum2[n-1];
	size_t numEqualElements = n - (numLessElements + numGreaterElements);
	
	auto fun3 = [&]() {parallel_for(numLessElements, numLessElements+numEqualElements, [&](size_t i) {
			B[i] = pivVal;
			});};
	auto fun4 = [&]() {parallel_for(0, n, [&](size_t i) {
			if (flags1[i] == true) {
				B[psum1[i]-1] = A[i];
				}
			else if (flags2[i] == true) {
				B[numLessElements+numEqualElements+psum2[i]-1] = A[i];
				}
				});};
	par_do(fun3, fun4);
	showScan(B, n);
	ccopy(A, B, n);
	return numLessElements;
}*/
/*
template <class T>
size_t miniPart(T *A, T *B, bool *flags1, bool *flags2, T *flags3, T *psum1, T *psum2, T *psum3, size_t n, size_t pivIndex) {

	// Filter Elements Less than Pivot
	filter(A, flags1, n, pivIndex, true);
	// Filter elements greater than pivot
	filter(A, flags2, n, pivIndex, false);
	// Filter elements equal to pivot
	equalFilter(A, flags3, n, pivIndex);
	ccopy(psum1, flags1, n);
	ccopy(psum2, flags2, n);
	ccopy(psum3, flags3, n);
	std::cout << "Elements less than " << piv << ": ";
	showScan(flags1, n);
	std::cout << "Elements equal than " << piv << ": ";
	showScan(flags3, n);
	std::cout << "Elements greater than " << piv << ": ";
	showScan(flags2, n);
	cout << '\n' << '\n';
	
	// Compute scan for "less than" elements

	//sweepScan(psum1, 0, n, 0);
	scansequential(psum1, flags1, 0, n);

	size_t offSet;	
	if (flags1[n-1] == 1) {
		offSet = psum1[n-1]+1;
	}
	else {
		offSet = psum1[n-1];
	}

	parallel_for(0, n, [&](size_t i) {
			if (flags1[i] == 1) {
						B[psum1[i]] = A[i];
				}
				});
	//sweepScan(psum2, 0, n, 0);
	scansequential(psum2, flags2, 0, n);
	//sweepScan(psum3, 0, n, 0);
	scansequential(psum3, flags3, 0, n);

	size_t otheroff;
	
	if (flags3[n-1] == 1) {
		otheroff = psum3[n-1]+1;
	}
	else {
		otheroff = psum3[n-1];
	}
	parallel_for(offSet, otheroff+offSet, [&](size_t i) {
			B[i] = A[pivIndex];
			});
	parallel_for(0, n, [&](size_t i) {
			if (flags2[i] == 1) {
				B[psum2[i]+offSet+otheroff] = A[i];
				}
				});
	ccopy(A, B, n);
	return offSet;	
}*/

/*template <class T>
void helper(T *A, T *B, T *flags1, T *flags2, T *flags3, T *psum1, T *psum2, T*psum3, size_t n) {
	if (n <= 10000) {
		sort(A, A+n);
		return;
	}
	auto t = miniPart(A, B, flags1, flags2, flags3, psum1, psum2, psum3, n, n/2);
	// helper(A,t);
	// helper(A+t, n-t);
	// helper(A+t+1, n-t-1);
	
	auto f1 = [&]() {helper(A, B, flags1, flags2, flags3, psum1, psum2, psum3, t);};
	auto f2 = [&]() {helper(A+t, B+t, flags1+t, flags2+t, flags3+t, psum1+t, psum2+t, psum3+t, n-t);};
	par_do(f1, f2);
}*/

template <class T>
void helper(T *A, T *B, bool *flags1, bool *flags2, int *psum1, int *psum2, size_t start, size_t end, size_t n) {
	if (n <= 10000) {
		//cout << "Sorting from range: " << start << " to " << end << endl;
		sort(A+start, A+end);
		//cout << "------------" << endl;
		return;
	}
	if (start == end) {
		return;
	}
	size_t sz = end - start;
	auto t = partition(A, B, flags1, flags2, psum1, psum2, start, end, n, (start + ((start + end)/ 2) % sz));
	//auto pv = pp(start, end, sz);
	//auto t = partition(A, B, flags1, flags2, psum1, psum2, start, end, n, sz);
	if (t.first == t.second) {
		//cout << "we returned when :" << t.first << "==" << t.second << endl;
		return;
	}
	//cout << "start: "<< t.first << endl;
	//cout << "end: "<< t.second << endl;
	auto f1 = [&]() {helper(A, B, flags1, flags2, psum1, psum2, start, t.first, n-t.second);};
	auto f2 = [&]() {helper(A, B, flags1, flags2, psum1, psum2, t.second, end, n-t.first);};
	//f1();
	//f2();
	par_do(f1, f2);
}


template <class T>
void quicksort(T *A, size_t n) {
	
	bool* flags1 = (bool*)malloc(n * sizeof(bool));
	bool* flags2 = (bool*)malloc(n * sizeof(bool));
	int* psum1  = (int*)malloc(n * sizeof(int));
	int* psum2  = (int*)malloc(n * sizeof(int));
	T* B      = (Type*)malloc(n * sizeof(Type));

	helper(A, B, flags1, flags2, psum1, psum2, 0, n, n);

	free(flags1);
	free(flags2);
	free(psum1);
	free(psum2);
	free(B);

	return;
}
