﻿//
//--------------------------------------------------------------------------------
//
//	MIT License
//
//	Copyright (c) 2019 Calmholm
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//--------------------------------------------------------------------------------
//
//
//
//  A low-level low-latency thread-safe software fifo template class.
//
//
//  About this file
//  ===============
//
//  This file contains experimental C++ code which has been developed for the purpose of implementing
//  a thread-safe software fifo template class.
//  This file also contains a main() function which has been developed for the purpose of implementing
//  a Windows Console App that performs rudimentary testing of an instantiation of the software fifo
//  template class.
//
//
//  The design brief
//  ================
//
//  To develop a C++ template FIFO class which can be used to pass large work items (e.g. a struct or
//  vector) from multiple threads ("writer threads") to a single thread (the "reader thread") where
//  the work is done.
//
//  The class should be able to queue a number of items in a thread-safe manner.
//  Timing is critical and latency should be minimised.
//  Use of the Standard Template Library (STL) and C++11 functionality should be weighed and considered.
//
//  The class needs to provide the following functions;
//
//  - push
//  A writer thread calls this function to push an item into the queue.
//  If there is no room in the queue for the item, this function should return immediately indicating
//  to the calling thread that the item was not pushed to the queue.
//
//  - pop_try
//  The reader thread calls this function to fetch the next available item.
//  If no items are available the function should return immediately indicating this condition to the
//  calling thread.
//
//  - pop
//  The reader thread calls this function to fetch the next available item.
//  If no items are available the reader thread should be put to sleep until an item becomes available.
//
//
//  The scope of this solution
//  ==========================
//                
//  This solution is an attempt to satisfy the design brief above for the Windows Operating System.
//
//
//  Design considerations and evolution of this solution
//  ====================================================
//
//  Consideration was given to using classes from the Standard Template Library, in particular classes
//  std::<array>, std::<deque> and std::<queue>.
//
//  Memory re-allocations are expensive, compromising footprint and performance. Since this exercise
//  uses a fixed-length fifo, memory re-allocations can be avoided.
//
//  The std::<[lists]> classes were deemed non-optimal because being linked lists they embed a level of
//  indirection which may compromise element access times. Conversely, fast sequential access will
//  minimise latency giving better performance in a FIFO implementation, as per the design brief.
//
//  The C++11 std::<array> container class seems to offer little added value in this exercise compared to
//  using an ordinary Array[]. Indeed, even its member function array::size(), rather than returning
//  the current occupancy in terms of number of elements, merely returns the same as array::max_size(),
//  which is just the array's (fixed value) capacity, the second parameter used to instantiate the class.
//  Obtaining current fifo population would require crafting the relevant function. This, along with the
//  baggage of a host of other member functions largely irrelevant to this exercise, was seen as adequate
//  reason to consider use of <array> as non-optimal here.
//
//  The std::<queue> container adapter class, whether or not based on an underlying std::<deque> container
//  (the default), seems upon cursory consideration to be well suited to a fifo application, and indeed
//  some C++11 documentation suggests this.
//  Class <queue> is based on a dynamic array, which has the ability to re-size. This ability is not
//  needed in this exercise, and thus this entire functionality is superfluous. Also there is need to
//  instantiate (or copy) a <queue> from a <deque> or <list>, which adds more code baggage and complexity.
//  Generally <queue> was considered a better STL choice but still non-optimal for this exercise.
//
//  After due consideration of the repercussions of using STL classes in this exercise it was decided to
//  instead use a bespoke low-level approach. This has been done for the primary reasons of efficiency,
//  fitness for purpose, simplicity and comprehensibilty, thereby realising;
//
//  1. Low latency - as per the design brief
//  2. Small footprint - no superfluous functionality wasting space (crucial in memory-constrained systems)
//  3. Simplicity* - simple uncomplicated design devoid of gratuitous or unwarranted abstraction or obfuscation
//  4. Comprehensibility* - generous comments describing step-by-step what the code does and how it works,
//     with no difficult-to-understand constructs, no obscure code and no arcane C++ evangelism.
//
//  * On the points of simplicity and comprehensibility, the essential aim here is that Software Engineers
//  or other persons inspecting this code should be able to understand it quickly with little or ideally
//  no need to look elsewhere for an explanation of what it does or how it works.
//
//
//  Brief details of this fifo class implementation
//  ===============================================
//
//  The fifo is implemented as a circular buffer (using an ordinary array) of type T and of size
//  FIFO_EXAMPLE_MAX_CAPACITY.
//  It has two associated indices, notably a data insertion index and a data extraction index.
//  It also has a (volatile) population counter that tracks item insertions (pushes) and extractions (pops).
//  Accesses are protected from corruption through multi-thread assault by a CRITICAL_SECTION (a mutex).
//  Inter-thread signalling uses a Windows Event.
//
//
//  Thread priorities
//  =================
//
//  For this exercise it is assumed that any thread can have any relative OS scheduling priority, so that;
//  - the "reader" thread can pre-empt any "writer" thread
//  - any "writer" thread can pre-empt the "reader" thread
//  - any "writer" thread can pre-empt another "writer" thread at any time.
//                
//
//  Fifo item data types
//  ====================
//
//  The data type "T" in the fifo template class may be a simple type or it can be a pointer to something.
//  Pointers will be appropriate for "large work items (e.g. a struct or vector)" as per the design brief.
//
//  The Windows Console App main() code here uses ints ("Fifo<int> int_test_fifo") for testing, but we could
//  equally have for example;
//
//  "Fifo<short> short_test_fifo"
//  "Fifo<float> float_test_fifo"
//  "Fifo<void*> voidPointer_test_fifo"
//  "Fifo<my_structure*> structPtr_test_fifo"
//
//
//  Fifo functions return values
//  ============================
//
//  Rather than returning merely 'success' or 'failure', the fifo functions may return values that might be
//  more useful, especially in testing.
//
//  In this exercise these are;
//
//  FIFO_STATUS_SUCCESS	  - returned by functions push() and pop_try(). Success, the operation had no problems.
//  FIFO_STATUS_FULL	  - returned by function push(). There was no space left in the FIFO for a new item.
//  FIFO_STATUS_EMPTY	  - returned by function pop_try(). There were no items in the FIFO to fetch.
//  FIFO_STATUS_LOCKED	  - returned by function push(). This "writer" thread failed to push a new item because
//                          the FIFO was busy (so try again later).
//  FIFO_STATUS_PREEMPTED - returned by function push(). This "writer" thread failed to push a new item because
//                          it was pre-empted by another and the FIFO is in fact now stuffed (FIFO_STATUS_FULL).
//
//
//  Building the Windows Console App
//  ================================
//
//  The app can be built using Visual Studio (VS2017) as follows;
//
//  1. Start VS2017. Let it churn a while until stable. Select from the Menu bar: File Tab->New->Project
//  2. Select "Windows Console Application"
//  3. Type in a name for the project or accept the default
//  4. Click on 'OK' (in the bottom-right of the dialog)
//  5. Copy this entire file (CTRL+A, CTRL+C)
//  6. In VS2017 replace the contents of file "<project_name>.cpp" with the text copied from this file (CTRL+A, CTRL+V)
//  7. Select from the Menu bar: Build Tab->Build Solution
//  8. If there are no build errors, run the Console App from within VS2017 using Debug->Start [Without] Debugging
//
//
//  Suggestions for more comprehensive multi-threaded testing
//  =========================================================
//
//  To do this it would first be necessary to rewrite main() so that it creates and operates at least three
//  threads, notably two writer threads plus the reader thread.
//  Proper testing needs a way to hit the fifo with requests from these threads, to push and pop items in a
//  pseudo-random (i.e., reproducible) way.
//  Suggestions for this include use of timed requests, or requests having a fixed average frequency but
//  with variable timing modulation (frequency modulation), engineered for deliberate occasional concurrency
//  which will test the FIFO_STATUS_PREEMPTED return status.
//  Such timing modulation might be obtained from a Shift-register PRBG, a Mersenne Twister (available in
//  C++11), or other PRNG, and should give generally reproducible results but with some variability caused
//  by vacillations in the OS.
//
//


#include "pch.h"		// Pre-compiled headers (pch)
#include <iostream>


#include <windows.h>		// For the Windows Event
#include <string>		// For the string class



using namespace std;



#define FIFO_EXAMPLE_MAX_CAPACITY	((unsigned) 5)

#define FIFO_STATUS_SUCCESS		((unsigned) 0)
#define FIFO_STATUS_FULL		((unsigned) 1)
#define FIFO_STATUS_EMPTY		((unsigned) 2)
#define FIFO_STATUS_LOCKED		((unsigned) 3)
#define FIFO_STATUS_PREEMPTED		((unsigned) 4)


string status_Strings[]{
	"FIFO_STATUS_SUCCESS",
	"FIFO_STATUS_FULL",
	"FIFO_STATUS_EMPTY",
	"FIFO_STATUS_LOCKED",
	"FIFO_STATUS_PREEMPTED"
};




template <class T, unsigned capacity = FIFO_EXAMPLE_MAX_CAPACITY>
class Fifo {

	HANDLE DataAvailableEvent;  // At least one array slot in items[] contains data
	CRITICAL_SECTION mutex;	    // Critical Section "mutex" protects items[] AND ITS INDEXES from simultaneous multithread assault

private:

	T items[capacity];	    // The FIFO is implemented as a basic array of T - this basic array is called "items"

	unsigned InsertionIndex, ExtractionIndex;  // Array insertion and extraction indices

	volatile unsigned population;  // Current population of items[] array


public:

	Fifo() : InsertionIndex(0), ExtractionIndex(0), population(0) {

		// CreateEvent(Security attributes (Null=default), Is a manual-reset event?, Initial state is Signaled?, Name)
		DataAvailableEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("DataAvailableEvent"));

		InitializeCriticalSection(&mutex);
	}


	~Fifo() {

		CloseHandle(DataAvailableEvent);
		DeleteCriticalSection(&mutex);
	}


	unsigned push(T item) {

		//	- push
		//	A "writer thread" calls this function to push an item into the queue.
		//	If there is no room in the queue for the item, this function should return immediately indicating
		//	to the calling thread that the item was not pushed to the queue.
		//
		//	This function may be called from multiple threads ("writer threads")
		//

		// If there's no space in the FIFO then return appropriate status code immediately
		if (population >= capacity) return FIFO_STATUS_FULL;

		// One thread at a time now...
		// Attempt to acquire the mutex (this thread will continue if it's acquired) or alternatively return
		// appropriate status code if another thread has it
		if (TryEnterCriticalSection(&mutex) == 0) return FIFO_STATUS_LOCKED;

		// NOTE - Depending on how the OS does its thread scheduling this will likely be a rare occurrence, but...
		//
		// The mutex has been acquired - test again - if another writer thread previously here bumped the poulation to
		// maximum and thereafter released the mutex so that this thread could then acquire it, did that
		// writer thread bump the population to maximum AFTER this thread passed the not-full-capacity test above
		// but BEFORE it could test and acquire the mutex?
		if (population >= capacity) {

			// Yes it did - the FIFO is in fact full - release the mutex
			LeaveCriticalSection(&mutex);

			// No space in the FIFO so return appropriate status code immediately
			return FIFO_STATUS_PREEMPTED;
		}

		// There's space in the FIFO...
		// Store the item in the FIFO at the current insertion position
		items[InsertionIndex] = item;
		// Bump insertion position and FIFO population
		InsertionIndex = (InsertionIndex + 1) % capacity;
		population++;

		// Release the mutex
		LeaveCriticalSection(&mutex);

		// Set the 'Data Available' Event. This action might release the reader thread if that thread is waiting on it
		SetEvent(DataAvailableEvent);

		// Return success
		return FIFO_STATUS_SUCCESS;
	}


	unsigned pop_try(T* itemPtr) {

		//	- pop_try
		//	The "reader thread" calls this function to fetch the next available item.
		//	If no items are available the function should return immediately indicating this condition to the
		//	calling thread.
		//
		//	This function is only ever called from a single thread (the "reader thread")
		//

		// If no items in the FIFO return appropriate status code immediately
		if (population == 0) return FIFO_STATUS_EMPTY;

		// Data items are available in the FIFO...

		// One thread at a time now...
		// Wait if necessary until a writer thread has released the mutex
		EnterCriticalSection(&mutex);

		// Obtain the item at the current extraction position
		*itemPtr = items[ExtractionIndex];
		// Bump extraction position and decrement FIFO population
		ExtractionIndex = (ExtractionIndex + 1) % capacity;
		population--;

		// Has the action of popping this item rendered the FIFO empty?
		if (population == 0) {

			// Yes it has - item is no longer available so reset the Event flag
			ResetEvent(DataAvailableEvent);
		}

		// Release the mutex
		LeaveCriticalSection(&mutex);

		// Return success
		return FIFO_STATUS_SUCCESS;
	}


	void pop(T* itemPtr) {

		//	- pop
		//	The "reader thread" calls this function to fetch the next available item.
		//	If no items are available this thread is put to sleep until an item becomes available.
		//
		//	This function is only ever called from a single thread (the "reader thread")
		//

		// If no items are available put this (single reader) thread to sleep until item is available,
		// i.e, until the DataAvailableEvent is set by a writer thread calling Fifo<T>::push()
		if (population == 0) WaitForSingleObject(DataAvailableEvent, INFINITE); // indefinite wait

		// If we're here this thread either hasn't waited or alternatively "the sleeper has awakened".
		// Back to reality, we know that data items are now available in the FIFO...

		// One thread at a time now...
		// Wait if necessary until a writer thread has released the mutex
		EnterCriticalSection(&mutex);

		// Obtain the item at the current extraction position
		*itemPtr = items[ExtractionIndex];
		// Bump extraction position and decrement FIFO population
		ExtractionIndex = (ExtractionIndex + 1) % capacity;
		population--;

		// Has the action of popping this item rendered the FIFO empty?
		if (population == 0) {

			// Yes it has - item is no longer available so reset the Event flag
			ResetEvent(DataAvailableEvent);
		}

		// Release the mutex
		LeaveCriticalSection(&mutex);
	}


	// This function is only here for testing by main() below - it can be deleted or commented-out
	// when no longer needed
	unsigned getPopulation(void) {
		return population;
	}

};




int main()
{

	//std::cout << "Hello World!\n";
	cout << "** Experimental Software FIFO - (Very) basic Test Rig **" << endl;


	Fifo<int> int_test_fifo;
	int value = -1;
	unsigned status;
	unsigned currentPop;
	unsigned testNum = 0;


	// display the fifo population at start-up, with nothing pushed onto it yet nor pops attempted
	currentPop = int_test_fifo.getPopulation();
	cout << endl << "Fifo population at start-up is " << currentPop << endl;


	// Perform a test - try to pop a value from the fifo (even though it should be empty since we haven't pushed anything yet)
	testNum++;
	cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 7;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push another value onto the fifo
	testNum++;
	value = 8;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 1000;	// Should be overwritten by upcoming pop_try()
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 9;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 10;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 11;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 12;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - push a value onto the fifo
	testNum++;
	value = 13;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Pushing the value " << value << " onto fifo" << endl;
	status = int_test_fifo.push(value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 2000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;



	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 3000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 4000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 5000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 6000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Perform a test - try to pop a value from the fifo
	testNum++;
	value = 7000;
	//cout << "Current value is " << value << endl;
	cout << endl << "** Test " << testNum << " ** Trying to pop a value from fifo" << endl;
	cout << "Current value (may be overwritten by forthcoming pop_try) is " << value << endl;
	status = int_test_fifo.pop_try(&value);
	// display the status resulting from the operation
	cout << "Status result of operation was " << status_Strings[status] << endl;
	cout << "Fifo population after test is " << int_test_fifo.getPopulation() << endl;
	cout << "Current value is " << value << endl;


	// Return some non-zero value from main() just for the sheer joy and unadulterated pleasure of it
	std::cout << endl << "Returning from main() with return value 1" << std::endl;
	return 1;
}


// The following is produced automatically by VS2017 as part of project creation...


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
