import CommonConf

import re
from collections import OrderedDict
import numpy as np
import matplotlib.pyplot as pp

def main(dirn, fname):
  queryKindSizes = []
  timePerSolver = OrderedDict()

  with open(dirn+"/"+fname+".dat") as fin:
    lines = fin.readlines()

    for line in lines:
      if line.startswith("#"):
        continue
      (solver, queryKinds, time, deviation, nline) = re.split("[\t]", line)

      queryKinds = int(queryKinds)
      time = float(time)
      deviation = float(deviation)

      if solver in timePerSolver:
        timePerSolver[solver].append((time, deviation))
      else:
        timePerSolver[solver] = [(time, deviation)]
      if len(queryKindSizes) == 0 or queryKinds > queryKindSizes[-1]:
        queryKindSizes.append(queryKinds)

  CommonConf.setupMPPDefaults()
  fmts = CommonConf.getLineFormats()
  fig = pp.figure()
  ax = fig.add_subplot(111)
  ax.set_xscale("log", basex=2)
    
  index = 0
  for solver, timesAndDeviation in timePerSolver.iteritems():
    timesAndDeviationLists = map(list, zip(*timesAndDeviation))
    ax.errorbar(queryKindSizes, timesAndDeviationLists[0],
                yerr=timesAndDeviationLists[1], label=solver,
                marker=fmts[index][0], linestyle=fmts[index][1])
    index = index + 1

  ax.set_xlabel('Number of QueryKinds');
  ax.set_ylabel('Time (sec.)');
  # ax.set_xlim(0, 2100)
  ax.legend(loc='best', fancybox=True)

  pp.savefig(dirn+"/"+fname+".pdf")
  pp.show()

if __name__ == "__main__":
  main("expData", "RunningTimeVsNumQueryKinds")

