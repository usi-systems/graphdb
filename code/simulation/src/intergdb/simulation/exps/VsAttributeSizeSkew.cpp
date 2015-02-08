#include <intergdb/simulation/Experiments.h>
#include <intergdb/expcommon/ExperimentalData.h>
#include <intergdb/simulation/SimulationConf.h>
#include <intergdb/simulation/Constants.h>
#include <intergdb/util/RunningStat.h>
#include <intergdb/util/AutoTimer.h>
#include <intergdb/common/Cost.h>
#include <intergdb/common/SchemaStats.h>

#include <iostream>
#include <random>
#include <vector>
#include <intergdb/optimizer/Solver.h>
#include <intergdb/optimizer/SolverFactory.h>

using namespace std;
using namespace intergdb;
using namespace intergdb::common;
using namespace intergdb::expcommon;
using namespace intergdb::simulation;
using namespace intergdb::optimizer;

void VsAttributeSizeSkew::makeQueryIOExp(ExperimentalData * exp) {
  exp->setDescription("Query IO Vs. AttributeSizeSkew");
  exp->addField("solver");
  exp->addField("attributeSizeSkew");
  exp->addField("io");
  exp->addField("deviation");
  exp->setKeepValues(false);
}

void VsAttributeSizeSkew::makeStorageExp(ExperimentalData * exp) {
  exp->setDescription("Storage Overhead Vs. AttributeSizeSkew");
  exp->addField("solver");
  exp->addField("attributeSizeSkew");
  exp->addField("storage");
  exp->addField("deviation");
  exp->setKeepValues(false);
}

void VsAttributeSizeSkew::makeRunningTimeExp(ExperimentalData * exp) {
    exp->setDescription("Running Time Vs. AttributeSizeSkew");
    exp->addField("solver");
    exp->addField("attributeSizeSkew");
    exp->addField("time");
    exp->addField("deviation");
    exp->setKeepValues(false);
}

void VsAttributeSizeSkew::process() 
{
  cerr << "This is an experiment with name: " 
    << this->getClassName() << endl;
  
  SimulationConf simConf;
  SchemaStats stats;
  Cost cost(stats);
  util::AutoTimer timer;  
  
  ExperimentalData queryIOExp("QueryIOVsAttributeSizeSkew");
  ExperimentalData runningTimeExp("RunningTimeVsAttributeSizeSkew");
  ExperimentalData storageExp("StorageOverheadVsAttributeSizeSkew");

  auto expData = { &queryIOExp, &runningTimeExp, &storageExp };

  makeQueryIOExp(&queryIOExp);
  makeRunningTimeExp(&runningTimeExp);
  makeStorageExp(&storageExp);

  for (auto exp : expData) {
      exp->open();
  }

  auto solvers = {       
      SolverFactory::instance().makeSinglePartition(), 
      SolverFactory::instance().makePartitionPerAttribute(),
      SolverFactory::instance().makeOptimalOverlapping(), 
      SolverFactory::instance().makeOptimalNonOverlapping(), 
      SolverFactory::instance().makeHeuristicOverlapping(),  
      SolverFactory::instance().makeHeuristicNonOverlapping() 
  };
  auto attributeSizeSkews = {0.1, 0.2, 0.3, 0.4, 0.50, 0.6, 0.7, 0.8, 0.9 }; 
  

  double total = solvers.size() * attributeSizeSkews.size()  * numRuns;
  double completed = 0;

  vector<util::RunningStat> io;
  vector<util::RunningStat> storage;
  vector<util::RunningStat> times;
  vector<std::string> names;


  for (auto solver : solvers) {
      io.push_back(util::RunningStat());
      storage.push_back(util::RunningStat());
      times.push_back(util::RunningStat());
      names.push_back(solver->getClassName());
      vector<std::string> names;  
  }

  int j;
  for (double attributeSizeSkew : attributeSizeSkews) {
      for (int i = 0; i < numRuns; i++) {
          simConf.setAttributeSizeZipfParam(attributeSizeSkew);
          auto workloadAndStats = simConf.getQueryWorkloadAndStats();
          QueryWorkload workload = workloadAndStats.first;
          stats = workloadAndStats.second;
          j = 0;
          for (auto solver : solvers) {              
              timer.start();
              Partitioning partitioning = solver->solve(workload, storageOverheadThreshold, stats); 
              timer.stop();                            
              io.at(j).push(cost.getIOCost(partitioning, workload));
              storage.at(j).push(cost.getStorageOverhead(partitioning, workload));   
              times.at(j).push(timer.getRealTimeInSeconds());                    
              j++;
              cerr << ".";
              completed++;
          }
      }

      int j = 0;
      for (auto solver : solvers) {  
   
          runningTimeExp.addRecord();
          runningTimeExp.setFieldValue("solver", solver->getClassName());
          runningTimeExp.setFieldValue("attributeSizeSkew", attributeSizeSkew);
          runningTimeExp.setFieldValue("time", times.at(j).getMean());
          runningTimeExp.setFieldValue("deviation", times.at(j).getStandardDeviation());
          times.at(j).clear();
          
          queryIOExp.addRecord();
          queryIOExp.setFieldValue("solver", solver->getClassName());
          queryIOExp.setFieldValue("attributeSizeSkew", attributeSizeSkew);        
          queryIOExp.setFieldValue("io", io.at(j).getMean());
          queryIOExp.setFieldValue("deviation", io.at(j).getStandardDeviation());
          io.at(j).clear();
          
          storageExp.addRecord();
          storageExp.setFieldValue("solver", solver->getClassName());
          storageExp.setFieldValue("attributeSizeSkew",attributeSizeSkew);
          storageExp.setFieldValue("storage", storage.at(j).getMean());    
          storageExp.setFieldValue("deviation", storage.at(j).getStandardDeviation());               
          storage.at(j).clear();

          j++;
      }
      cerr << " (" << (completed / total) * 100 << "%)" << endl;           
  }


  for (auto exp : expData) {
      exp->close();
  }

};
