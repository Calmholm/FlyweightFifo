# FlyweightFifo

A low-level low-latency thread-safe software fifo template class.


About this project
==================

This project contains experimental C++ code which has been developed for the purpose of implementing a thread-safe software fifo template class.
This file also contains a main() function which has been developed for the purpose of implementing a Windows Console App that performs rudimentary testing of an instantiation of the software fifo template class.


The design brief
================

To develop a C++ template FIFO class which can be used to pass large work items (e.g. a struct or vector) from multiple threads ("writer threads") to a single thread (the "reader thread") where the work is done.

The class should be able to queue a number of items in a thread-safe manner.
Timing is critical and latency should be minimised.
Use of the Standard Template Library (STL) and C++11 functionality should be weighed and considered.

The class needs to provide the following functions;

- push.
A writer thread calls this function to push an item into the queue.
If there is no room in the queue for the item, this function should return immediately indicating to the calling thread that the item was not pushed to the queue.

- pop_try.
The reader thread calls this function to fetch the next available item.
If no items are available the function should return immediately indicating this condition to the calling thread.

- pop.
The reader thread calls this function to fetch the next available item.
If no items are available the reader thread should be put to sleep until an item becomes available.


The scope of this solution
==========================
                
This solution is an attempt to satisfy the design brief above for the Windows Operating System.


Design considerations and evolution of this solution
====================================================

Consideration was given to using classes from the Standard Template Library, in particular classes array, deque and queue.

Memory re-allocations are expensive, compromising footprint and performance. Since this exercise uses a fixed-length fifo, memory re-allocations can be avoided.

The list classes were deemed non-optimal because being linked lists they embed a level of indirection which may compromise element access times. Conversely, fast sequential access will minimise latency thereby giving better performance in a fifo implementation, as per the design brief.

The C++11 array container class seems to offer little added value in this exercise compared to using an ordinary Array[]. Indeed, even its member function size(), rather than returning the current occupancy in terms of number of elements, merely returns the same as max_size(), which is just the array's (fixed value) capacity, the second parameter used to instantiate the class.
Obtaining current fifo population would require crafting the relevant function. This, along with the baggage of a host of other member functions largely irrelevant to this exercise, was seen as adequate reason to consider use of the array class as non-optimal here.

The queue container adapter class, whether or not based on an underlying deque container (the default), seems upon cursory consideration to be well suited to a fifo application, and indeed some C++11 documentation suggests this.
The queue class is based on a dynamic array, which has the ability to re-size. This ability is not needed in this exercise, and thus this entire functionality is superfluous. Also there is need to instantiate (or copy) a queue from a deque or list, which adds more code baggage and complexity.
Generally the queue class was considered a better STL choice but still non-optimal for this exercise.

After due consideration of the repercussions of using STL classes in this exercise it was decided to instead use a bespoke low-level approach. This has been done for the primary reasons of efficiency, fitness for purpose, simplicity and comprehensibilty, thereby realising;

1. Low latency - as per the design brief
2. Small footprint - no superfluous functionality wasting space (crucial in memory-constrained systems)
3. Simplicity* - simple uncomplicated design devoid of gratuitous or unwarranted abstraction or obfuscation
4. Comprehensibility* - generous comments describing step-by-step what the code does and how it works,
    with no difficult-to-understand constructs, no obscure code and no arcane C++ evangelism.

* On the points of simplicity and comprehensibility, the essential aim here is that Software Engineers or other persons inspecting this code should be able to understand it quickly with little or ideally no need to look elsewhere for an explanation of what it does or how it works.


Brief details of this fifo class implementation
===============================================

The fifo is implemented as a circular buffer (using an ordinary array) of type T and of size FIFO_EXAMPLE_MAX_CAPACITY.
It has two associated indices, notably a data insertion index and a data extraction index.
It also has a (volatile) population counter that tracks item insertions (pushes) and extractions (pops).
Accesses are protected from corruption through multi-thread assault by a CRITICAL_SECTION (a mutex).
Inter-thread signalling uses a Windows Event.


Thread priorities
=================

For this exercise it is assumed that any thread can have any relative OS scheduling priority, so that;
- the "reader" thread can pre-empt any "writer" thread
- any "writer" thread can pre-empt the "reader" thread
- any "writer" thread can pre-empt another "writer" thread at any time.
                

Fifo item data types
====================

The data type "T" in the fifo template class may be a simple type or it can be a pointer to something.
Pointers will be appropriate for "large work items (e.g. a struct or vector)" as per the design brief.

The Windows Console App main() code here uses ints ("Fifo< int > int_test_fifo") for testing, but we could equally have for example;

- "Fifo< short > short_test_fifo"
- "Fifo< float > float_test_fifo"
- "Fifo< void* > voidPointer_test_fifo"
- "Fifo< my_structure* > structPtr_test_fifo"


Fifo functions return values
============================

Rather than returning merely 'success' or 'failure', the fifo functions may return values that might be more useful, especially in testing.

In this exercise these are;

- FIFO_STATUS_SUCCESS   - returned by functions push() and pop_try(). Success, the operation had no problems.
- FIFO_STATUS_FULL      - returned by function push(). There was no space left in the FIFO for a new item.
- FIFO_STATUS_EMPTY     - returned by function pop_try(). There were no items in the FIFO to fetch.
- FIFO_STATUS_LOCKED    - returned by function push(). This "writer" thread failed to push a new item because
                        the FIFO was busy (so try again later).
- FIFO_STATUS_PREEMPTED - returned by function push(). This "writer" thread failed to push a new item because
                        it was pre-empted by another and the FIFO is in fact now stuffed (FIFO_STATUS_FULL).


Building the Windows Console App
================================

The app can be built using Visual Studio (VS2017) as follows;

1. Start VS2017. Let it churn a while until stable. Select from the Menu bar: File Tab->New->Project
2. Select "Windows Console Application"
3. Type in a name for the project or accept the default
4. Click on 'OK' (in the bottom-right of the dialog)
5. Copy the entire contents of the .cpp source file (CTRL+A, CTRL+C)
6. In VS2017 replace the contents of file "<project_name>.cpp" with the text copied from the source file (CTRL+A, CTRL+V)
7. Select from the Menu bar: Build Tab->Build Solution
8. If there are no build errors, run the Console App from within VS2017 using Debug->Start [Without] Debugging


Suggestions for more comprehensive multi-threaded testing
=========================================================

To do this it would first be necessary to rewrite main() so that it creates and operates at least three threads, notably two writer threads plus the reader thread.
Proper testing needs a way to hit the fifo with requests from these threads, to push and pop items in a pseudo-random (i.e., reproducible) way.
Suggestions for this include use of timed requests, or requests having a fixed average frequency but with variable timing modulation (frequency modulation), engineered for deliberate occasional concurrency which will test the FIFO_STATUS_PREEMPTED return status.
Such timing modulation might be obtained from a Shift-register PRBG, a Mersenne Twister (available in C++11), or other PRNG, and should give generally reproducible results but with some variability caused by vacillations in the OS.

