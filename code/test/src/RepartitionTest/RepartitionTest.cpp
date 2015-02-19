#include <intergdb/core/InteractionGraph.h>

#include "gtest/gtest.h"
#include <boost/filesystem.hpp>

#include <iostream>
#include <memory>

using namespace std;
using namespace intergdb::core;

class RepartitionTest : public ::testing::Test
{
public:
    RepartitionTest() {}
protected:
    virtual void SetUp()
    {
        auto storageDir = boost::filesystem::unique_path("/tmp/mydb_%%%%");
        conf.reset(new Conf("testDB", storageDir.string(),
            {{"v", DataType::STRING}}, // vertex schema
            {{"a", DataType::STRING}, {"b", DataType::STRING}})); // edge schema
        if (boost::filesystem::exists(conf->getStorageDir()))
            boost::filesystem::remove_all(conf->getStorageDir());
        boost::filesystem::create_directories(conf->getStorageDir());
        graph.reset(new InteractionGraph(*conf));
    }
    virtual void TearDown()
    {
        boost::filesystem::remove_all(graph->getConf().getStorageDir());
    }
protected:
    std::unique_ptr<Conf> conf;
    std::unique_ptr<InteractionGraph> graph;
};

static void test_graph(InteractionGraph * graph)
{
    {
        IntervalQuery iq(5.0, 10.0);
        InteractionGraph::VertexIterator iqIt = graph->processIntervalQuery(iq);
        EXPECT_EQ(iqIt.isValid(), true);
        EXPECT_EQ(iqIt.getVertexData()->getStringAttribute("v"), "v2");
        EXPECT_EQ(2, iqIt.getVertexId());
        iqIt.next();
        EXPECT_EQ(iqIt.isValid(), true);
        EXPECT_EQ(iqIt.getVertexData()->getStringAttribute("v"), "v4");
        EXPECT_EQ(4, iqIt.getVertexId());
        iqIt.next();
        EXPECT_EQ(iqIt.isValid(), false);
    }

    {
        FocusedIntervalQuery fiq(2, 5.0, 10.0, {"a"});
        InteractionGraph::EdgeIterator fiqIt = graph->processFocusedIntervalQuery(fiq);
        EXPECT_EQ(fiqIt.isValid(), true);
        EXPECT_EQ(fiqIt.getEdgeData()->getFields().size(), 1);
        EXPECT_EQ(fiqIt.getEdgeData()->getStringAttribute("a"), "a-data");
        EXPECT_EQ(fiqIt.getToVertex(), 4);
        EXPECT_EQ(fiqIt.getTime(), 7);
        fiqIt.next();
        EXPECT_EQ(fiqIt.isValid(), false);
    }

    {
        FocusedIntervalQuery fiq(2, 5.0, 10.0, {"b"});
        InteractionGraph::EdgeIterator fiqIt = graph->processFocusedIntervalQuery(fiq);
        EXPECT_EQ(fiqIt.isValid(), true);
        EXPECT_EQ(fiqIt.getEdgeData()->getFields().size(), 1);
        EXPECT_EQ(fiqIt.getEdgeData()->getStringAttribute("b"), "b-data");
        EXPECT_EQ(fiqIt.getToVertex(), 4);
        EXPECT_EQ(fiqIt.getTime(), 7);
        fiqIt.next();
        EXPECT_EQ(fiqIt.isValid(), false);
    }

    {
        FocusedIntervalQuery fiq(2, 5.0, 10.0, {"a", "b"});
        InteractionGraph::EdgeIterator fiqIt = graph->processFocusedIntervalQuery(fiq);
        EXPECT_EQ(fiqIt.isValid(), true);
        EXPECT_EQ(fiqIt.getEdgeData()->getFields().size(), 2);
        EXPECT_EQ(fiqIt.getEdgeData()->getStringAttribute("a"), "a-data");
        EXPECT_EQ(fiqIt.getEdgeData()->getStringAttribute("b"), "b-data");
        EXPECT_EQ(fiqIt.getToVertex(), 4);
        EXPECT_EQ(fiqIt.getTime(), 7);
        fiqIt.next();
        EXPECT_EQ(fiqIt.isValid(), false);
    }
}

TEST_F(RepartitionTest, WriteReadTest)
{
    graph->createVertex(2, "v2");
    graph->createVertex(4, "v4");
    Timestamp ts = 7.0;
    graph->addEdge(2, 4, ts, "a-data", "b-data");
    graph->flush();

    graph.reset(nullptr);
    graph.reset(new InteractionGraph(*conf));

    // base test
    test_graph(graph.get());

    {
        auto & partIndex = graph->getPartitionIndex();
        auto origParting = partIndex.getTimeSlicedPartitioning(Timestamp(0.0));
        TimeSlicedPartitioning newParting{}; // -inf to inf
        newParting.getPartitioning() = { {"a"}, {"b"} };
        partIndex.replaceTimeSlicedPartitioning(origParting, {newParting});
        // with {"a"}, {"b"}
        test_graph(graph.get());
    }

    {
        auto & partIndex = graph->getPartitionIndex();
        auto origParting = partIndex.getTimeSlicedPartitioning(Timestamp(0.0));
        TimeSlicedPartitioning newParting{}; // -inf to inf
        newParting.getPartitioning() = { {"a", "b"} };
        partIndex.replaceTimeSlicedPartitioning(origParting, {newParting});
        // with {"a", "b"} again
        test_graph(graph.get());
    }

}




