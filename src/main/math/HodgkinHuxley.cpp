#include "ODE.h"
#include "VITAMIN.h"
#include <mpi.h>

using namespace config;

void ode::hodgkinhuxley::calculateNextState(const storage_type &xs, storage_type &dxdts, double t) {
  static HodgkinHuxleyEquation equationInstance;
  return equationInstance.calculateNextState(xs, dxdts, t);
}

ode::hodgkinhuxley::HodgkinHuxleyEquation::HodgkinHuxleyEquation() {
  this->pc = &(ProgramConfig::getInstance());

  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

  sendData = (double*) malloc(4*sizeof(double)); //FIXME - Memory leak and a hack
}

// Out-of-class initialization is necessary for static variables
bool ode::hodgkinhuxley::HodgkinHuxleyEquation::runIsSpeculative = false;

void ode::hodgkinhuxley::HodgkinHuxleyEquation::setSpeculative(bool speculative)
{
  runIsSpeculative = speculative;
}

// //FIXME - this should iterate over the neurons in this rank and work on each of their synapses
// This will work because this rank is responsible for every synapse leaving one of its neurons
void ode::hodgkinhuxley::HodgkinHuxleyEquation::broadcastIsynsValues(
      double *arrP, double *arrM, double *arrG, SynapseConstants *allSynapses, double t)
                                         // ProtobufRepeatedInt32 &ownSynapses, int numOfOwnSynapses) 
{
  //FIXME: THIS IS A HACK!!!!
  int numOfOwnSynapses = 1;
  int synapseIndex = 0;

  // TODO Investigate the formula expressed here. It's not the same as in the paper
  //#pragma omp parallel for reduction(+:result)
  for (int i = 0; i < numOfOwnSynapses; i++) {
    //const int synapseIndex = ownSynapses[i];
    const SynapseConstants &s = allSynapses[i]; //allSynapses[synapseIndex];
    const double Esyn = s.esyn;
    const double gbarsyng = s.gbarsyng;
    const double gbarsyns = s.gbarsyns;
    const double cGraded = s.cGraded;
    const double tauDecay = s.tauDecay;
    const double tauRise = s.tauRise;
    const double P = arrP[synapseIndex];
    const double M = arrM[synapseIndex];
    const double g = arrG[synapseIndex];
    const double P3 = pow(P, 3);
    double isyng = gbarsyng * P3 / (cGraded + P3);
    // TODO Investigate: magic number t0
    const double t0 = 0;
    const double tPeak = t0 + (tauDecay * tauRise * log(tauDecay/tauRise)) / (tauDecay - tauRise);
    // TODO Investigate: why
    const double fsyns = 1 / (exp(-(tPeak - t0)/tauDecay) + exp(-(tPeak - t0)/tauRise));
    const double isyns = M * gbarsyns * g * fsyns;
    
    double xiyi = isyns + isyng;

    sendData[0] = (runIsSpeculative) ? 1.0 : 0.0;
    sendData[1] = t;
    sendData[2] = xiyi;
    sendData[3] = Esyn*xiyi;

    //FIXME: HACK
    if (mpiRank == 0)
    { 
      MPI_Isend(sendData, 4, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, &sendRequest); //tagged w/ synapse #1
    }
    else if (mpiRank == 1)
    {
      MPI_Isend(sendData, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &sendRequest); //tagged w/ synapse #0
    }

    // std::cout << "Rank " << mpiRank << " sent a packet" << std::endl;
  }
}

void ode::hodgkinhuxley::HodgkinHuxleyEquation::calculateNextState(const storage_type &x, storage_type &dxdt, double t) {
  ProgramConfig &c = *pc;
  double *arrV = c.getVArray(const_cast<storage_type &>(x));
  double *arrMk2 = c.getMk2Array(const_cast<storage_type &>(x));
  double *arrMp = c.getMpArray(const_cast<storage_type &>(x));
  double *arrMna = c.getMnaArray(const_cast<storage_type &>(x));
  double *arrHna = c.getHnaArray(const_cast<storage_type &>(x));
  double *arrMcaf = c.getMcafArray(const_cast<storage_type &>(x));
  double *arrHcaf = c.getHcafArray(const_cast<storage_type &>(x));
  double *arrMcas = c.getMcasArray(const_cast<storage_type &>(x));
  double *arrHcas = c.getHcasArray(const_cast<storage_type &>(x));
  double *arrMk1 = c.getMk1Array(const_cast<storage_type &>(x));
  double *arrHk1 = c.getHk1Array(const_cast<storage_type &>(x));
  double *arrMka = c.getMkaArray(const_cast<storage_type &>(x));
  double *arrHka = c.getHkaArray(const_cast<storage_type &>(x));
  double *arrMkf = c.getMkfArray(const_cast<storage_type &>(x));
  double *arrMh = c.getMhArray(const_cast<storage_type &>(x));
  double *arrA = c.getAArray(const_cast<storage_type &>(x));
  double *arrP = c.getPArray(const_cast<storage_type &>(x));
  double *arrM = c.getMArray(const_cast<storage_type &>(x));
  double *arrG = c.getGArray(const_cast<storage_type &>(x));
  double *arrH = c.getHArray(const_cast<storage_type &>(x));

  double *arrdVdt = c.getVArray(dxdt);
  double *arrdMk2dt = c.getMk2Array(dxdt);
  double *arrdMpdt = c.getMpArray(dxdt);
  double *arrdMnadt = c.getMnaArray(dxdt);
  double *arrdHnadt = c.getHnaArray(dxdt);
  double *arrdMcafdt = c.getMcafArray(dxdt);
  double *arrdHcafdt = c.getHcafArray(dxdt);
  double *arrdMcasdt = c.getMcasArray(dxdt);
  double *arrdHcasdt = c.getHcasArray(dxdt);
  double *arrdMk1dt = c.getMk1Array(dxdt);
  double *arrdHk1dt = c.getHk1Array(dxdt);
  double *arrdMkadt = c.getMkaArray(dxdt);
  double *arrdHkadt = c.getHkaArray(dxdt);
  double *arrdMkfdt = c.getMkfArray(dxdt);
  double *arrdMhdt = c.getMhArray(dxdt);
  double *arrdAdt = c.getAArray(dxdt);
  double *arrdPdt = c.getPArray(dxdt);
  double *arrdMdt = c.getMArray(dxdt);
  double *arrdGdt = c.getGArray(dxdt);
  double *arrdHdt = c.getHArray(dxdt);

  broadcastIsynsValues(arrP, arrM, arrG, c.getAllSynapseConstants(), t);
  if (!runIsSpeculative)
  {
    vitamins.clearSpeculativeDataOlderThan(t); // This is not that great
  }
  // std::cout << "Rank " << mpiRank << " is starting at t=" << t << std::endl;

  const int numOfNeurons = c.numOfNeurons;
  const int numOfSynapses = c.numOfSynapses;

  std::queue<Neuron> neurons;

  //FIXME: HACK
  Neuron testNeuron;
  testNeuron.localIndex = 0;
  if (mpiRank == 0)
    testNeuron.pendingSynapses.push(0); // need to receive synapse 0
  else if (mpiRank == 1)
    testNeuron.pendingSynapses.push(1);

  neurons.push(testNeuron);

  while (!neurons.empty())
  {
    Neuron neuron = neurons.front(); neurons.pop(); // This is a pop operation now
    for (unsigned int i=0; i<neuron.pendingSynapses.size(); i++)
    {
      int globalSynapse = neuron.pendingSynapses.front();
      neuron.pendingSynapses.pop();
      
      int flag = 0;
      if (mpiRank == 0)
        MPI_Iprobe(1, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE); //syn0 from rank 1
      else if (mpiRank == 1)
        MPI_Iprobe(0, 1, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE); //syn1 from rank 0

      if (flag)
      {
        // std::cout << "Rank " << mpiRank << " has a pending packet" << std::endl;
        double recvData[4];
        if (mpiRank == 0)
          MPI_Recv(recvData, 4, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        else if (mpiRank == 1)
          MPI_Recv(recvData, 4, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        vitamins.addPacket(new vitamin::VITAMINPacket((recvData[0] == 1), recvData[1], recvData[2], recvData[3]));
        // std::cout << "Rank " << mpiRank << " saved a packet" << std::endl;
      }

      if (vitamins.haveDataForTime(t))
      {
        neuron.isynsAcc += vitamins.calculateIsyns(t, arrV[neuron.localIndex]);
        // std::cout << "Rank " << mpiRank << " finished synapse at t=" << t << std::endl;
      }
      else // put this synapse back in the queue
      {
        neuron.pendingSynapses.push(globalSynapse);
      }

    }

    if (neuron.pendingSynapses.empty())
    {
      int i = neuron.localIndex;
      const NeuronConstants &n = c.getNeuronConstantAt(i);
      const double V = arrV[i];

      // Calculate dVdt
      arrdVdt[i] = -(ina(n.gbarna, arrMna[i], arrHna[i], V, n.ena) +
                     ip(n.gbarp, arrMp[i], V, n.ena) +
                     icaf(n.gbarcaf, arrMcaf[i], arrHcaf[i], V, n.eca) +
                     icas(n.gbarcas, arrMcas[i], arrHcas[i], V, n.eca) +
                     ik1(n.gbark1, arrMk1[i], arrHk1[i], V, n.ek) +
                     ik2(n.gbark2, arrMk2[i], V, n.ek) +
                     ika(n.gbarka, arrMka[i], arrHka[i], V, n.ek) +
                     ikf(n.gbarkf, arrMkf[i], V, n.ek) +
                     ih(n.gbarh, arrMh[i], V, n.eh) +
                     il(n.gbarl, V, n.el) +
                     neuron.isynsAcc
      ) / n.capacitance;

      // Calculate dMk2dt
      arrdMk2dt[i] = dMk2dt(V, arrMk2[i]);
      // Calculate dMpdt
      arrdMpdt[i] = dMpdt(V, arrMp[i]);
      // Calculate dMnadt
      arrdMnadt[i] = dMnadt(V, arrMna[i]);
      // Calculate dHnadt
      arrdHnadt[i] = dHnadt(V, arrHna[i]);
      // Calculate dMcafdt
      arrdMcafdt[i] = dMcafdt(V, arrMcaf[i]);
      // Calculate dHcafdt
      arrdHcafdt[i] = dHcafdt(V, arrHcaf[i]);
      // Calculate dMcasdt
      arrdMcasdt[i] = dMcasdt(V, arrMcas[i]);
      // Calculate dHcasdt
      arrdHcasdt[i] = dHcasdt(V, arrHcas[i]);
      // Calculate dMk1dt
      arrdMk1dt[i] = dMk1dt(V, arrMk1[i]);
      // Calculate dHk1dt
      arrdHk1dt[i] = dHk1dt(V, arrHk1[i]);
      // Calculate dMkadt
      arrdMkadt[i] = dMkadt(V, arrMka[i]);
      // Calculate dHkadt
      arrdHkadt[i] = dHkadt(V, arrHka[i]);
      // Calculate dMkfdt
      arrdMkfdt[i] = dMkfdt(V, arrMkf[i]);
      // Calculate dMhdt
      arrdMhdt[i] = dMhdt(V, arrMh[i]);
    }
    else // This neuron still has work to be done, so put it back on the queue
    {
      neurons.push(neuron);
    }
  }

// #if USE_OPENMP
//   #pragma omp parallel for default(shared)
// #endif
//   for (int i = 0; i < numOfNeurons; i++) {
//     using namespace std;
//     const NeuronConstants &n = c.getNeuronConstantAt(i);
//     const double V = arrV[i];

//     // Calculate dVdt
//     arrdVdt[i] = -(ina(n.gbarna, arrMna[i], arrHna[i], V, n.ena) +
//                    ip(n.gbarp, arrMp[i], V, n.ena) +
//                    icaf(n.gbarcaf, arrMcaf[i], arrHcaf[i], V, n.eca) +
//                    icas(n.gbarcas, arrMcas[i], arrHcas[i], V, n.eca) +
//                    ik1(n.gbark1, arrMk1[i], arrHk1[i], V, n.ek) +
//                    ik2(n.gbark2, arrMk2[i], V, n.ek) +
//                    ika(n.gbarka, arrMka[i], arrHka[i], V, n.ek) +
//                    ikf(n.gbarkf, arrMkf[i], V, n.ek) +
//                    ih(n.gbarh, arrMh[i], V, n.eh) +
//                    il(n.gbarl, V, n.el) +
//                    isyns(V, arrP, arrM, arrG, c.getAllSynapseConstants(), *(n.incoming), n.incoming->size())
//     ) / n.capacitance;

//     // Calculate dMk2dt
//     arrdMk2dt[i] = dMk2dt(V, arrMk2[i]);
//     // Calculate dMpdt
//     arrdMpdt[i] = dMpdt(V, arrMp[i]);
//     // Calculate dMnadt
//     arrdMnadt[i] = dMnadt(V, arrMna[i]);
//     // Calculate dHnadt
//     arrdHnadt[i] = dHnadt(V, arrHna[i]);
//     // Calculate dMcafdt
//     arrdMcafdt[i] = dMcafdt(V, arrMcaf[i]);
//     // Calculate dHcafdt
//     arrdHcafdt[i] = dHcafdt(V, arrHcaf[i]);
//     // Calculate dMcasdt
//     arrdMcasdt[i] = dMcasdt(V, arrMcas[i]);
//     // Calculate dHcasdt
//     arrdHcasdt[i] = dHcasdt(V, arrHcas[i]);
//     // Calculate dMk1dt
//     arrdMk1dt[i] = dMk1dt(V, arrMk1[i]);
//     // Calculate dHk1dt
//     arrdHk1dt[i] = dHk1dt(V, arrHk1[i]);
//     // Calculate dMkadt
//     arrdMkadt[i] = dMkadt(V, arrMka[i]);
//     // Calculate dHkadt
//     arrdHkadt[i] = dHkadt(V, arrHka[i]);
//     // Calculate dMkfdt
//     arrdMkfdt[i] = dMkfdt(V, arrMkf[i]);
//     // Calculate dMhdt
//     arrdMhdt[i] = dMhdt(V, arrMh[i]);
//   }
#if USE_OPENMP
  #pragma omp parallel for default(shared)
#endif
  for (int j = 0; j < numOfSynapses; j++) {
    using namespace std;
    const SynapseConstants &s = c.getSynapseConstantAt(j);
    const int sourceNeuronIndex = s.source;
    const NeuronConstants &n = c.getNeuronConstantAt(sourceNeuronIndex);
    const double V = arrV[sourceNeuronIndex];

    // Calculate dPdt
    // TODO Cache the result from loop above to save time
    arrdPdt[j] = ica(
      icaf(n.gbarcaf, arrMcaf[sourceNeuronIndex], arrHcaf[sourceNeuronIndex], V, n.eca),
      icas(n.gbarcas, arrMcas[sourceNeuronIndex], arrHcas[sourceNeuronIndex], V, n.eca),
      arrA[j]
    ) - s.buffering * arrP[j];

    // Calculate dAdt
    arrdAdt[j] = (1.0e-10 / (1 + exp(-100.0 * (V + 0.02))) - arrA[j]) / 0.2;
    // Calculate dMdt
    arrdMdt[j] = (0.1 + 0.9 / (1 + exp(-1000.0 * (V + 0.04))) - arrM[j]) / 0.2;
    // Calculate dGdt
    arrdGdt[j] = -arrG[j] / s.tauDecay + arrH[j];
    // Calculate dHdt
    arrdHdt[j] = -arrH[j] / s.tauRise + (V > s.thresholdV ? s.h0 : 0);
  }

  MPI_Wait(&sendRequest, MPI_STATUS_IGNORE);
  setSpeculative(true);
  // std::cout << "Rank " << mpiRank << " finished a system function call" << std::endl;
}
