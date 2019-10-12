
#include "treeNode.h"
#include "oda.h"
#include "octUtils.h"
#include "hcurvedata.h"

#include <vector>

#include <stdio.h>
#include <iostream>

int main(int argc, char * argv[])
{
  MPI_Init(&argc, &argv);

  MPI_Comm comm = MPI_COMM_WORLD;

  int rProc, nProc;
  MPI_Comm_rank(comm, &rProc);
  MPI_Comm_size(comm, &nProc);


  using C = unsigned int;
  constexpr unsigned int dim = 4;

  _InitializeHcurve(dim);


  const unsigned int lev = 1;
  const unsigned int eleOrder = 1;

  std::vector<ot::TreeNode<C, dim>> treePart;
  ot::createRegularOctree(treePart, 1, comm);

  ot::DA<dim> octDA(treePart.data(), treePart.size(), comm, 1);

  fprintf(stderr, "%*s[%d] Local size = %llu, global size = %llu\n",
      40*rProc, "\0", rProc,
      octDA.getLocalNodalSz(), octDA.getGlobalNodeSz());

  _DestroyHcurve();

  MPI_Finalize();

  return 0;
}