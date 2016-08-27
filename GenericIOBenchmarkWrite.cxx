/*
 *                    Copyright (C) 2015, UChicago Argonne, LLC
 *                               All Rights Reserved
 * 
 *                               Generic IO (ANL-15-066)
 *                     Hal Finkel, Argonne National Laboratory
 * 
 *                              OPEN SOURCE LICENSE
 * 
 * Under the terms of Contract No. DE-AC02-06CH11357 with UChicago Argonne,
 * LLC, the U.S. Government retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 * 
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 * 
 *   3. Neither the names of UChicago Argonne, LLC or the Department of Energy
 *      nor the names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 * 
 * *****************************************************************************
 * 
 *                                  DISCLAIMER
 * THE SOFTWARE IS SUPPLIED “AS IS” WITHOUT WARRANTY OF ANY KIND.  NEITHER THE
 * UNTED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT OF ENERGY, NOR
 * UCHICAGO ARGONNE, LLC, NOR ANY OF THEIR EMPLOYEES, MAKES ANY WARRANTY,
 * EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE
 * ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, DATA, APPARATUS,
 * PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE
 * PRIVATELY OWNED RIGHTS.
 * 
 * *****************************************************************************
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <string>
#include <cassert>

#include "GenericIO.h"

#define POSVEL_T float
#define ID_T int64_t
#define MASK_T uint16_t

using namespace std;
using namespace gio;

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int commRank, commRanks;
  MPI_Comm_rank(MPI_COMM_WORLD, &commRank);
  MPI_Comm_size(MPI_COMM_WORLD, &commRanks);

  int a = 1;
  if (argc > 1 && string(argv[a]) == "-c") {
    GenericIO::setDefaultShouldCompress(true);
    --argc;
    ++a;
  }

  if(argc != 4) {
    fprintf(stderr,"USAGE: %s [-c] <mpiioName> <NP> <seed>\n", argv[0]);
    exit(-1);
  }

  GenericIO::setNaturalDefaultPartition();

  char *mpiioName = argv[a++];
  size_t Np = atoi(argv[a++])/commRanks;
  int seed = atoi(argv[a++]);

  srand48(seed + commRank);

  // Add a 2% variance to make things a bit more realistic.
  Np = double(Np)*(1.0 + (drand48() - 0.5)*0.02);

  vector<POSVEL_T> xx, yy, zz, vx, vy, vz, phi;
  vector<ID_T> id;
  vector<MASK_T> mask;

  assert(sizeof(ID_T) == 8);

  unsigned Method = GenericIO::FileIOPOSIX;
  const char *EnvStr = getenv("GENERICIO_USE_MPIIO");
  if (EnvStr && string(EnvStr) == "1")
    Method = GenericIO::FileIOMPI;

  { // scope GIO
  GenericIO GIO(
    MPI_COMM_WORLD,
    mpiioName, Method);
  GIO.setNumElems(Np);

  int CoordFlagsX = GenericIO::VarIsPhysCoordX;
  int CoordFlagsY = GenericIO::VarIsPhysCoordY;
  int CoordFlagsZ = GenericIO::VarIsPhysCoordZ;
  GIO.setPhysOrigin(0.0);
  GIO.setPhysScale(256.0);

  xx.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  yy.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  zz.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  vx.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  vy.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  vz.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  phi.resize(Np + GIO.requestedExtraSpace()/sizeof(POSVEL_T));
  id.resize(Np + GIO.requestedExtraSpace()/sizeof(ID_T));
  mask.resize(Np + GIO.requestedExtraSpace()/sizeof(MASK_T));

  std::fill(xx.begin(), xx.end(), 25);
  std::fill(yy.begin(), yy.end(), 25);
  std::fill(zz.begin(), zz.end(), 25);
  std::fill(vx.begin(), vx.end(), 25);
  std::fill(vy.begin(), vy.end(), 25);
  std::fill(vz.begin(), vz.end(), 25);
  std::fill(phi.begin(), phi.end(), 25);
  std::fill(id.begin(), id.end(), 25);
  std::fill(mask.begin(), mask.end(), 25);

  GIO.addVariable("x", xx, CoordFlagsX | GenericIO::VarHasExtraSpace);
  GIO.addVariable("y", yy, CoordFlagsY | GenericIO::VarHasExtraSpace);
  GIO.addVariable("z", zz, CoordFlagsZ | GenericIO::VarHasExtraSpace);
  GIO.addVariable("vx", vx, GenericIO::VarHasExtraSpace);
  GIO.addVariable("vy", vy, GenericIO::VarHasExtraSpace);
  GIO.addVariable("vz", vz, GenericIO::VarHasExtraSpace);
  GIO.addVariable("phi", phi, GenericIO::VarHasExtraSpace);
  GIO.addVariable("id", id, GenericIO::VarHasExtraSpace);
  GIO.addVariable("mask", mask, GenericIO::VarHasExtraSpace);

  GIO.write();
  } // destroy GIO prior to calling MPI_Finalize

  xx.resize(Np);
  yy.resize(Np);
  zz.resize(Np);
  vx.resize(Np);
  vy.resize(Np);
  vz.resize(Np);
  phi.resize(Np);
  id.resize(Np);
  mask.resize(Np);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();

  return 0;
}

