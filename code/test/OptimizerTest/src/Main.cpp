
#include <cstdlib>
#include <iostream>
#include <numeric>

#include <intergdb/common/QueryWorkload.h>
#include <intergdb/common/Query.h>

#include <solver.h>

using namespace std;
using namespace intergdb::common;


void createWorkLoad(QueryWorkload * workload)
{

    for (size_t i=0, iu=4; i<iu; ++i)  { 
        workload->addAttribute(Attribute(i, 8));    
    }
    auto const & attributes = workload->getAttributes();
    vector<size_t> attributeIndices(attributes.size());
    iota(attributeIndices.begin(), attributeIndices.end(), 0);
    for (size_t j=0; j<2; ++j) {
        Query query;
        query.addAttribute(attributes[attributeIndices[j]]);
        query.setFrequency(0.5);
        workload->addQuery(query);
    }
}


int main()
{
    cerr << "This is a test program for the solver" << endl;

    QueryWorkload workload; 
    Solver s;

    createWorkLoad(&workload);

    cerr << workload.toString() << endl;

    s.solve(&workload);
    return 0;    
}
 


