#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>

#include <thread>
#include <mutex>
#include <shared_mutex>


// Demonstrate hand-over-hand locking with a simple linked list of
// strings. The contained locks are covered by shared_mutex, which is
// C++'s implementation of reader-writer locks.
//
// The list provides one funciton to acquire an exlusive lock on a
// particular node.  We use shared locks to traverse the list ot that
// point.


// A verbose shared_mutex type
// Allows us to see what's going on.
//
// Note: I am not making the output thread-safe here!

class verbose_mutex {
    static unsigned int number;

    std::shared_mutex  m;
    unsigned int       serial_num;

public:
    verbose_mutex();

    void lock();
    void unlock();

    void lock_shared();
    void unlock_shared();
};

unsigned int verbose_mutex::number = 0;

verbose_mutex::verbose_mutex()
{
    serial_num = number++;
    std::cout << "****** Creating lock # " << serial_num << '\n';
}

void verbose_mutex::lock()
{
    std::cout << "****** Exclusive acquire # " << serial_num << '\n';
    m.lock();
}

void verbose_mutex::unlock()
{
    std::cout << "****** Exclusive release # " << serial_num << '\n';
    m.unlock();
}

void verbose_mutex::lock_shared()
{
    std::cout << "****** Shared acquire # " << serial_num << '\n';
    m.lock_shared();
}

void verbose_mutex::unlock_shared()
{
    std::cout << "****** Shared release # " << serial_num << '\n';
    m.unlock_shared();
}


// A slingly-linked list of strings, with a sentinel node
struct node {
    verbose_mutex      mutex;
    std::string        element;
    node              *next;

    node(std::string _elt);
};


node::node(std::string _elt)
    : element(_elt), next(nullptr)
{
}

// Reminder: this points to a sentinel value, not a real node
static node *list; 

// Note: not thread safe
void list_add(std::string s)
{
    node *new_node = new node(s);

    // Find last element in list
    node *insert_point = list;
    while (insert_point->next) {
	insert_point = insert_point->next;
    }

    insert_point->next = new_node;
}

// Note: not thread safe
void populate_list()
{
    std::string contents = "The quick brown fox jumped over the lazy dog";
    std::istringstream ss(contents);

    std::string s;

    while (ss >> s) {
	list_add(s);
    }

    std::cout << "\n\n";
}


node *lookup(unsigned int n, std::unique_lock<verbose_mutex> &lock)
// REQUIRES: list is not empty
//           0 < n <= length of the list
//           lock does not hold its associated mutex, if any
//
// MODIFIES: lock
//
// EFFECTS: returns a pointer to the nth node of the list, with lock
//          holding the result node's mutex exclusively. Is
//          thread-safe
{

    // Acquire a read lock on the sentinel node, and initialize our
    // traversal/return value. Note that the initialization of result
    // _must_ come after the instantiation of the read_lock or else it
    // is an unsafe read of a shared variable!

    std::shared_lock<verbose_mutex>  read_lock(list->mutex);
    node                            *result = list->next;


    // Check requirements of the function.
    if (!result) {
	throw std::runtime_error("lookup: list is empty");
    }
    if (!n || lock.owns_lock()) {
	throw std::runtime_error("lookup: invalid argument");
    }

    // Now we have to traverse the list from the sentinel to the node
    // _prior_to_ the node we want to return. If we are returning the
    // first node in the list (i.e. n==1) we are already
    // there. Otherwise we need to walk forward. We do this
    // hand-over-hand with shared locks
    //
    // The loop invariants on entry are:
    //
    //         n-1: number of steps forward we need to
    //              take. Decremented each iteration.
    //
    //         read_lock: holds a shared lock of the node with a next
    //                    value equal to result. Moved forward one
    //                    node each iteration
    //
    //         result: points to the node _after_ the one currently
    //                 locked; this node must exist. Advanced each
    //                 iteration.

    while (--n) {

	// Acquire a read lock on next node in the list
	std::shared_lock<verbose_mutex> next_lock(result->mutex);

	// before the next line of code, read_lock holds the lock of
	// some node X, while next_lock holds the lock of the node
	// X+1.

	read_lock.swap(next_lock);

	// After that line of code, the situation is
	// reversed. read_lock holds the lock on node X+1, while
	// next_lock holds the lock on X


	// Advance the result pointer and check that we haven't run
	// off the end of the list
	result = result->next;
	if (!result) {
	    throw std::runtime_error("lookup: list too short");
	}

	// At the end of this iteration of the loop, next_lock goes
	// out of scope. As a consequence, we drop the lock it holds
	// on node X's mutex.
    }


    // At this point, result points to the node we want to return, but
    // it is not yet locked. We hold the lock on its predecessor node
    // in read_lock. So, we will acquire the lock on the target node:

    std::unique_lock<verbose_mutex>    result_lock(result->mutex);

    // Swap it with the caller's argument (which must be modified to
    // hold that lock)
    lock.swap(result_lock);

    // When we return the pointer to the target node, both result_lock
    // and read_lock will go out of scope. result_lock doesn't hold
    // anything by (verified) REQUIRES assumption. read_lock holds the
    // shared mutex on the predecessar node, which we no longer need.

    return result;
}

int main()
{
    // Set up list: not thread safe
    std::cout << "\nPopulating list:\n";
    list = new node("");
    populate_list();
    // Try to grab a few locks
    {
	std::unique_lock<verbose_mutex>     node_lock1, node_lock2;
	node                               *node_p1, *node_p2;

	std::cout << "Looking up node 6:\n";
	node_p1 = lookup(6, node_lock1);
	std::cout << "node #6: " << node_p1->element << "\n\n";
	

	std::cout << "\nLooking up node 4:\n";
	node_p2 = lookup(4, node_lock2);
	std::cout << "node #4: " << node_p2->element << "\n\n";

	std::cout << "Node locks 4 and 6 going out of scope\n";
	// Drop both of those locks when they go out of scope
    }


    // Grab one more
    std::unique_lock<verbose_mutex>    node_lock;
    node                              *node_ptr;

    std::cout << "\nLooking up node 9:\n";
    node_ptr = lookup(9, node_lock);
    std::cout << "node #9: " << node_ptr->element << "\n\n";

    
    // node_lock goes out of scope here and is dropped.
    std::cout << "Node lock 9 going out of scope\n";
    return 0;
}
