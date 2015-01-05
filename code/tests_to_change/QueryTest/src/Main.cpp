#include <intergdb/core/InteractionGraph.h>

#include <cstdlib>
#include <iostream>

#include <boost/filesystem.hpp>

using namespace std;
using namespace intergdb::core;

int main()
{
    Conf conf("test", "/tmp/qt_igdb", 
        {{"vertex-label", DataType::STRING}}, 
        {{"a", DataType::STRING}, {"b", DataType::STRING}});
    bool newDB = !boost::filesystem::exists(conf.getStorageDir());
    boost::filesystem::create_directories(conf.getStorageDir());   
    InteractionGraph graph(conf);

    if (newDB) {  
        graph.createVertex(2, "v2");
        graph.createVertex(4, "v4");
        Timestamp ts = 7.0;
        graph.addEdge(2, 4, ts, "a-data" "b-data");
        graph.flush();
    }

    IntervalQuery q1(5.0, 10.0);
    FocusedIntervalQuery q2(2, 5.0, 10.0, {"a"});
    
    InteractionGraph::VertexIterator iqIt = graph.processIntervalQuery(q1);
    while(iqIt.isValid()) {
        cout << *iqIt.getVertexData() << endl; 
        cout << iqIt.getVertexId() << endl; 
        iqIt.next();
    }
    InteractionGraph::EdgeIterator fiqIt = graph.processFocusedIntervalQuery(q2);
    while(fiqIt.isValid()) {
        cout << *fiqIt.getEdgeData() << endl;  
        cout << fiqIt.getToVertex() << endl; 
        cout << fiqIt.getTime() << endl; 
        fiqIt.next();
    }
    return EXIT_SUCCESS;
}


