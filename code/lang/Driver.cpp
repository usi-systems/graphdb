#include <cctype>
#include <fstream>
#include <cassert>
#include <iostream>

#include "driver.h"

using namespace intergdb;
using namespace intergdb::lang;

Driver::~Driver(){ 
   delete(scanner);
   scanner = nullptr;
   delete(parser);
   parser = nullptr;
}

void 
Driver::parse( const char *filename )
{
   assert( filename != nullptr );
   std::ifstream in_file( filename );
   if( ! in_file.good() ) exit( EXIT_FAILURE );
   
   delete(scanner);
   try
   {
      scanner = new Scanner( &in_file );
   }
   catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate scanner: (" <<
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
   
   delete(parser); 
   try
   {
      parser = new Parser( (*scanner) /* scanner */, 
                                  (*this) /* driver */ );
   }
   catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate parser: (" << 
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
   const int accept( 0 );
   if( parser->parse() != accept )
   {
      std::cerr << "Parse failed!!\n";
   }
}

void 
Driver::printVertex()
{ 
    std::cout << "vertex" << std::endl;
}

void 
Driver::printEdge()
{ 
    std::cout << "edge" << std::endl;
}
