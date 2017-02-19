#include <iostream>
#include <string>
#include "utils.hpp"
#include "Controller.hpp"

#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>

namespace mpi = boost::mpi;

//#define DEBUG

#ifdef DEBUG
    #define RANK 0
#endif


int main(int argc, const char* argv[]) {

#ifdef DEBUG
    std::cout << "-- DEBUG, RANK: " << RANK << std::endl;
#endif
    
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    std::cout << "Hello, World! DDS" << std::endl;
    
    core::Controller c;
    std::cout << c.foo() << endl;
    std::cout << c.inline_foo() << endl;
    
#ifndef DEBUG
    mpi::environment env;
    mpi::communicator world;
#endif
    
#ifndef DEBUG
    if (world.rank() == 0) {
#else
    if (RANK == 0){
#endif
        std::string msg, out_msg = "Hello";
        
#ifndef DEBUG
        mpi::request reqs[2];
        reqs[0] = world.isend(1, 0, out_msg);
        reqs[1] = world.irecv(1, 1, msg);
        mpi::wait_all(reqs, reqs + 2);
        std::cout << msg << "!" << std::endl;
#endif
        
    } else {
        
        std::string msg, out_msg = "world";
        
#ifndef DEBUG
        mpi::request reqs[2];
        reqs[0] = world.isend(0, 1, out_msg);
        reqs[1] = world.irecv(0, 0, msg);
        mpi::wait_all(reqs, reqs + 2);
#endif
        std::cout << msg << ", ";
    }
    
    return EXIT_SUCCESS;
}