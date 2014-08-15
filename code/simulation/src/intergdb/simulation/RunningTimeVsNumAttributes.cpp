#include <intergdb/simulation/Experiments.h>
#include <intergdb/simulation/ExperimentalData.h>
#include <intergdb/simulation/SimulationConf.h>

#include <intergdb/util/AutoTimer.h>

#include <intergdb/common/Cost.h>

#include <iostream>
#include <random>
#include <vector>

#include <Solver.h>
#include <SolverFactory.h>

using namespace std;
using namespace intergdb;
using namespace intergdb::common;
using namespace intergdb::simulation;
using namespace intergdb::util;
using namespace intergdb::optimizer;

void RunningTimeVsNumAttributes::process() 
{
  cerr << "This is a sample experiment with name: " 
    << this->getClassName() << endl;
  
  SimulationConf simConf;
  Cost cost;
  AutoTimer timer;

  QueryWorkload workload = simConf.getQueryWorkload();
  double storageOverheadThreshold = 0.0;
  cerr << workload.toString() << endl;

  mt19937 rgen;
  uniform_real_distribution<> udist(0, 10);

  ExperimentalData exp(getClassName());
  exp.setDescription("This experiment compares the running time io vs. the number of attributes for each of the partitioning methods.");

  exp.addField("solver");
  exp.addField("attributes");
  exp.addField("time");

  exp.setKeepValues(false);
  exp.open();

  for (auto solver : { SolverFactory::instance().makeSinglePartition(), 
              SolverFactory::instance().makePartitionPerAttribute()  }) {
//              SolverFactory::instance().makeOptimalOverlapping(),
//              SolverFactory::instance().makeOptimalNonOverlapping() }) {
      cerr << "Running with solver: "
           << solver->getClassName() << endl;

      timer.start();
      solver->solve(workload, storageOverheadThreshold); 
      timer.stop();
      exp.addRecord();
      exp.setFieldValue("solver", solver->getClassName());
      exp.setFieldValue("attributes", workload.getAttributes().size());
      exp.setFieldValue("time", timer.getRealTimeInSeconds());
  }

  exp.close();
};
