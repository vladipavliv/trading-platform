/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

// boost_exceptions.cpp
#include <boost/throw_exception.hpp>
#include <cstdlib>
#include <iostream>

#ifdef BOOST_NO_EXCEPTIONS
// Custom implementation of boost::throw_exception
void boost::throw_exception(const std::exception &e) {
  std::cerr << "Boost exception occurred: " << e.what() << std::endl;
  std::abort();
}

void boost::throw_exception(const std::exception &e, const boost::source_location &loc) {
  std::cerr << "Boost exception occurred: " << e.what() << std::endl;
  std::cerr << "At: " << loc.file_name() << ":" << loc.line() << std::endl;
  std::abort();
}
#endif