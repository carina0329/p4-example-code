#include <cassert>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// Demonstrate the use of shared/weak pointers to manage "on demand"
// dynamic structures.

// This is a simple structure that counts allocations

struct serial_no {
    static unsigned int number;
    const unsigned int this_num;

    serial_no();
    ~serial_no();
};

unsigned int serial_no::number = 1;

serial_no::serial_no()
    : this_num(number++)
{
    std::cout << "Creating serial # " << this_num << "\n";
}

serial_no::~serial_no()
{
    std::cout << "Destroying serial # " << this_num << "\n";
}


// The Dynamic Map
//
// Maps from strings to serial numbers, dynamcially assigniung serial
// numbers to strings as they are used.
//
// If a client has an active reference to a partcular serial number,
// any later lookups should return the same one. Once no clients have
// any active serial numbers, the next lookup should get a new serial
// number.

class dynamic_map {
    std::map<std::string, std::weak_ptr<serial_no>>  map;

public:
    std::shared_ptr<serial_no> lookup(const std::string &s);
};


std::shared_ptr<serial_no> dynamic_map::lookup(const std::string &s)
// MODIFIES: this
//
// EFFECTS: Returns a shared_ptr to the serial number currently
//          assigned to the string s. If a previous lookup result for
//          a particular string (or any copy of it) still exists, this
//          lookup should return the same result. If this lookup has
//          never been made, or no (copy of) any prior lookup still
//          exists, should return a newly-allocated serial number.
{
    std::shared_ptr<serial_no>  result = nullptr;

    // Look up the weak pointer (creating it if it does not exist)
    std::weak_ptr<serial_no> tentative = map[s];

    // Try to get the underlying serial number (if it exists)
    result = tentative.lock();

    if (!result) {
	// There is no current underlying serial number. Need one.
	// This allocates a serial_no via new(), and then assigned it
	// to be managed by result.
	result = std::make_shared<serial_no>();

	// This stores a weak reference to result in the map at s
	map[s] = result;
    }

    // This passes ownership of the managed serial_no to the caller

    std::cout << "Lookup of " << s << " returns # "
	      << result->this_num << "\n";
    return result;
}


int main()
{
    dynamic_map                m;

    // Instantiate serial #s for P and Q
    std::cout << "First block\n--------------------------\n";
    std::shared_ptr<serial_no> p1 = m.lookup("P");
    std::shared_ptr<serial_no> q1 = m.lookup("Q");
    std::cout << "ends ------------------\n";
    
    {
	// Lookup P, Q, and R. P and Q should get the same ones we've
	// already seen. R should be new.
	
	std::cout << "\nSecond block\n--------------------------\n";
	std::shared_ptr<serial_no> p2 = m.lookup("P");
	std::shared_ptr<serial_no> q2 = m.lookup("Q");
	std::shared_ptr<serial_no> r2 = m.lookup("R");
	std::cout << "ends ------------------\n";

	// Before this scope closes, P and Q have two live references,
	// but R only has one. When this scope closes, the serial
	// number associated with R should be destroyed.
    }

    
    std::cout << "\nThird block\n--------------------------\n";

    // This releases the last reference to Q, and should deallocate Q
    q1 = nullptr;
    
    // This should get the "old" P, but new values for Q and R
    std::shared_ptr<serial_no> p2 = m.lookup("P");
    std::shared_ptr<serial_no> q2 = m.lookup("Q");
    std::shared_ptr<serial_no> r2 = m.lookup("R");
    std::cout << "ends ------------------\n";


    // And this destroys all of them.
    return 0;
}
