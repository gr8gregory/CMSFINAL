/*

Heap.h - Interaface for Heap functions

*/

#pragma once



typedef struct Heap heapH;


struct Heap {
	char message[140];			// Message text
	int priority;				// Priority of message
};

void swap(heapH *x, heapH *y);

// A class for Min Heap 
class MinHeap
{
	heapH* harr; // pointer to array of elements in heap 
	int capacity; // maximum possible size of min heap 
	heapH* copyharr;
	int heap_size; // Current number of elements in min heap 

public:
	// Constructor 
	MinHeap(int capacity);

	// to heapify a subtree with the root at given index 
	void MinHeapify(int);

	int parent(int i) { return (i - 1) / 2; }

	// to get index of left child of node at index i 
	int left(int i) { return (2 * i + 1); }

	// to get index of right child of node at index i 
	int right(int i) { return (2 * i + 2); }

	// to extract the root which is the minimum element 
	heapH extractMin();

	// Decreases key value of key at index i to new_val 
	void decreaseKey(int i, int new_val);

	// Returns the minimum key (key at root) from min heap 
	int getMin() { return harr[0].priority; }

	// Deletes a key stored at index i 
	void deleteKey(int i);

	// Inserts a new key 'k' 
	void insertKey(int k, char* c);

	void Print(void);
};


extern MinHeap heaper;