#include <intergdb/common/Cost.h>
#include <intergdb/common/SchemaStats.h>
#include <intergdb/core/InteractionGraph.h>
#include <intergdb/optimizer/Solver.h>
#include <intergdb/optimizer/SolverFactory.h>
#include <intergdb/simulation/ExperimentalData.h>
#include <intergdb/simulation/Experiments.h>
#include <intergdb/simulation/ExpSetupHelper.h>
#include <intergdb/simulation/SimulationConf.h>
#include <intergdb/util/AutoTimer.h>
#include <intergdb/util/RunningStat.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <random>
#include <vector>

using namespace std;
using namespace intergdb;
using namespace intergdb::core;
using namespace intergdb::common;
using namespace intergdb::optimizer;
using namespace intergdb::simulation;

void VsNumQueryTemplates::setUp()
{
    cout << " VsNumQueryTemplates::setUp()..." << endl;
    
     string dbDirPath = "data";
     string expName = str(boost::format("tweetDB%08d") % blockSize_);
     string pathAndName =
         str(boost::format("data/tweetDB%08d") % blockSize_);

    // Create tweetDB if its not there
    if( !(boost::filesystem::exists(pathAndName))) {
        boost::filesystem::create_directory(pathAndName);
    }
    
    // Clean up anything that is in the directory
    boost::filesystem::path path_to_remove(pathAndName);
    for (boost::filesystem::directory_iterator end_dir_it,
             it(path_to_remove); it!=end_dir_it; ++it)
    {
        remove_all(it->path());
    }
    
    // Create the graph conf
    Conf conf = ExpSetupHelper::createGraphConf(dbDirPath, expName);
    conf.setBlockSize(blockSize_);
    conf.setBlockBufferSize(blockBufferSize_);

    // Create a graph 
    graph_.reset(new InteractionGraph(conf));
   
    // // Create a vertex just so we can populate it with
    // // the same function.
    std::vector< std::unique_ptr<core::InteractionGraph> > graphs;
    graphs.push_back(std::move(graph_));

    cout << " populateGraphFromTweets..." << endl;
    ExpSetupHelper::populateGraphFromTweets(
        "data/tweets", graphs, tsStart_, tsEnd_, vertices_);
    cout << " done." << endl;

    std::cout << "start " << tsStart_ << std::endl;
    std::cout << "stop " << tsEnd_ << std::endl;

}

void VsNumQueryTemplates::makeEdgeIOCountExp(ExperimentalData * exp)
{
    exp->setDescription("Query IO Vs. EdgeIOCount");
    exp->addField("solver");
    exp->addField("numQueryTemplates");
    exp->addField("edgeIO");
    exp->addField("deviation");
    exp->setKeepValues(false);
}

void VsNumQueryTemplates::makeEdgeWriteIOCountExp(ExperimentalData * exp)
{
    exp->setDescription("Storage Overhead Vs. EdgeWriteIOCount");
    exp->addField("solver");
    exp->addField("numQueryTemplates");
    exp->addField("edgeWriteIO");
    exp->addField("deviation");
    exp->setKeepValues(false);
}

void VsNumQueryTemplates::makeEdgeReadIOCountExp(ExperimentalData * exp) {
    exp->setDescription("Running Time Vs. EdgeReadIOCount");
    exp->addField("solver");
    exp->addField("numQueryTemplates");
    exp->addField("edgeReadIO");
    exp->addField("deviation");
    exp->setKeepValues(false);
}

 void VsNumQueryTemplates::runWorkload(
     InteractionGraph * graph,
    std::vector<core::FocusedIntervalQuery> & queries,
    std::vector<int> indices)
{
    int count = 0;
    int sizes = 0;
    for (int i : indices) {
        std::cout << queries[i].toString() << std::endl;
        for (auto iqIt = graph->processFocusedIntervalQuery(queries[i]);
             iqIt.isValid(); iqIt.next()) {
            sizes += iqIt.getEdgeData()->getFields().size();
            count += 1;
        }
    }
    assert (count != 0);
    assert (sizes != 0);
 }


std::vector<int> VsNumQueryTemplates::genWorkload(size_t numQueryTypes)
{
    util::ZipfRand queryGen_(queryZipfParam_, numQueryTypes);
    unsigned seed = time(NULL);
    queryGen_.setSeed(seed++);
    vector<int> indices;
    for (int i = 0; i < numQueries_; ++i) {
        if (numQueryTypes > 1)
            indices.push_back(queryGen_.getRandomValue());
        else
            indices.push_back(0);
    }
    return indices;
}

void VsNumQueryTemplates::process()
{
    SimulationConf simConf;
    double storageOverheadThreshold = 1.0;

    simConf.setAttributeCount(
        graph_->getConf().getEdgeSchema().getAttributes().size());

    // std::vector<std::vector<core::FocusedIntervalQuery>> queries;
    // std::vector<std::vector<int>> indicies;
    // std::vector<SchemaStats> stats;
    // std::vector<QueryWorkload> workloads;

    // std::cout << "Generating workload..." << std::endl;
    // for (int i=0; i < numRuns_; i++) {
    //     std::cout << "    " << i << "/" << numRuns_ << std::endl;
    //     std::vector<core::FocusedIntervalQuery> qs =
    //         simConf.getQueries(graphs_[0].get(), tsStart_, tsEnd_, vertices_);
    //     std::vector<int> inds = genWorkload(qs.size()-1);
    //     runWorkload(graphs_[0].get(),qs, inds);
    //     SchemaStats ss = graphs_[0]->getSchemaStats();
    //     std::map<BucketId,common::QueryWorkload> ws =
    //         graphs_[0]->getWorkloads();
    //     // Make sure everything is in one bucket
    //     assert(ws.size() == 1);
    //     QueryWorkload w = ws.begin()->second;

    //     // (queries, indices, stats, workload)
    //     queries.push_back(qs);
    //     indicies.push_back(inds);
    //     stats.push_back(ss);
    //     workloads.push_back(w);

    //     graphs_[0]->resetWorkloads();
    // }
    // std::cout << "done." << std::endl;

    ExperimentalData edgeIOCountExp("EdgeIOCountVsNumQueryTemplates");
    ExperimentalData edgeWriteIOCountExp("EdgeWriteIOCountVsNumQueryTemplates");
    ExperimentalData edgeReadIOCountExp("EdgeReadIOCountVsNumQueryTemplates");

    auto expData =
        { &edgeIOCountExp, &edgeWriteIOCountExp, &edgeReadIOCountExp };

    makeEdgeIOCountExp(&edgeIOCountExp);
    makeEdgeWriteIOCountExp(&edgeWriteIOCountExp);
    makeEdgeReadIOCountExp(&edgeReadIOCountExp);

    for (auto exp : expData)
        exp->open();

    vector<util::RunningStat> edgeIO;
    vector<util::RunningStat> edgeWriteIO;
    vector<util::RunningStat> edgeReadIO;
    vector<std::string> names;
    vector< shared_ptr<Solver> > solvers =
    {
        SolverFactory::instance().makeSinglePartition(),
        SolverFactory::instance().makeOptimalNonOverlapping(),
        SolverFactory::instance().makeHeuristicNonOverlapping()
    };

    for (auto solver : solvers) {
        edgeIO.push_back(util::RunningStat());
        edgeWriteIO.push_back(util::RunningStat());
        edgeReadIO.push_back(util::RunningStat());
        names.push_back(solver->getClassName());
    }

    int solverIndex;
    size_t prevEdgeIOCount;
    size_t prevEdgeReadIOCount;
    size_t prevEdgeWriteIOCount;
    
    std::cout << "Running experiments..." << std::endl;
  
    int queryTemplatesIndex = -1;

    SchemaStats stats = graph_->getSchemaStats();
    QueryWorkload workload;

    for (int i = 0; i < numRuns_; i++) {
        for (auto numQueryTemplates : queryTemplatesSizes_) {
            queryTemplatesIndex++;        
            simConf.setQueryTypeCount(numQueryTemplates);

            // generate a different workload with numQueryTemplates
            std::vector<core::FocusedIntervalQuery> queries =
                simConf.getQueries(graph_.get(), tsStart_, tsEnd_, vertices_);


            solverIndex = -1;
            for (auto solver : solvers) {
                solverIndex++;
                auto & partIndex = graph_->getPartitionIndex();
                auto origParting =
                    partIndex.getTimeSlicedPartitioning(Timestamp(0.0));
                intergdb::common::Partitioning solverSolution =
                      solver->solve(workload, storageOverheadThreshold, stats);
                std::vector<int> indicies = genWorkload(queries.size()-1);
                graph_->resetWorkloads();

                runWorkload(graph_.get(), queries, indicies);
                SchemaStats ss = graph_->getSchemaStats();
                std::map<BucketId,common::QueryWorkload> ws =
                    graph_->getWorkloads();
                // Make sure everything is in one bucket
                assert(ws.size() == 1);
                QueryWorkload workload = ws.begin()->second;

                std::cout << "Workload: "
                          << workload.toString() << std::endl;
                std::cout << "Summary size: "
                          << workload.getQuerySummaries().size() << std::endl;

                std::cout << "Solver: " <<  solver->getClassName() << std::endl;

                std::cout << solverSolution.toString() << std::endl;
                TimeSlicedPartitioning newParting{}; // -inf to inf
                newParting.getPartitioning() = solverSolution.toStringSet();
                partIndex.replaceTimeSlicedPartitioning(
                    origParting, {newParting});

                prevEdgeIOCount = graph_->getEdgeIOCount();
                            prevEdgeReadIOCount = graph_->getEdgeReadIOCount();
                prevEdgeWriteIOCount = graph_->getEdgeWriteIOCount();

                runWorkload(graph_.get(),queries, indicies);


                std::cout <<
                    graph_->getEdgeIOCount() - prevEdgeIOCount << std::endl;
                std::cout <<
                    graph_->getEdgeReadIOCount() - prevEdgeReadIOCount
                    << std::endl;
                std::cout <<
                    graph_->getEdgeWriteIOCount() - prevEdgeWriteIOCount
                    << std::endl;

                edgeIO[solverIndex].push(
                    graph_->getEdgeIOCount() - prevEdgeIOCount);
                edgeReadIO[solverIndex].push(
                    graph_->getEdgeReadIOCount() - prevEdgeReadIOCount);
                edgeWriteIO[solverIndex].push(
                    graph_->getEdgeWriteIOCount() - prevEdgeWriteIOCount);
                
            }
        }


        for (int solverIndex = 0; solverIndex < solvers.size(); solverIndex++)
        {

            edgeIOCountExp.addRecord();
            edgeIOCountExp.setFieldValue(
                "solver", solvers[solverIndex]->getClassName());
            edgeIOCountExp.setFieldValue(
                "numQueryTemplates",
                boost::lexical_cast<std::string>(queryTemplatesSizes_[queryTemplatesIndex]));
            edgeIOCountExp.setFieldValue(
                "edgeIO", edgeIO[solverIndex].getMean());
            edgeIOCountExp.setFieldValue(
                "deviation", edgeIO[solverIndex].getStandardDeviation());
            edgeIO[solverIndex].clear();

            edgeWriteIOCountExp.addRecord();
            edgeWriteIOCountExp.setFieldValue(
                "solver", solvers[solverIndex]->getClassName());
            edgeWriteIOCountExp.setFieldValue("numQueryTemplates",
                boost::lexical_cast<std::string>(queryTemplatesSizes_[queryTemplatesIndex]));
            edgeWriteIOCountExp.setFieldValue(
                "edgeWriteIO", edgeWriteIO[solverIndex].getMean());
            edgeWriteIOCountExp.setFieldValue(
                "deviation", edgeWriteIO[solverIndex].getStandardDeviation());
            edgeWriteIO[solverIndex].clear();

            edgeReadIOCountExp.addRecord();
            edgeReadIOCountExp.setFieldValue(
                "solver", solvers[solverIndex]->getClassName());
            edgeReadIOCountExp.setFieldValue(
                "numQueryTemplates",
                boost::lexical_cast<std::string>(queryTemplatesSizes_[queryTemplatesIndex]));
            edgeReadIOCountExp.setFieldValue(
                "edgeReadIO", edgeReadIO[solverIndex].getMean());
            edgeReadIOCountExp.setFieldValue(
                "deviation", edgeReadIO[solverIndex].getStandardDeviation());
            edgeReadIO[solverIndex].clear();
        }
    }

    // std::cout << "done." << std::endl;

    for (auto exp : expData)
        exp->close();
};
