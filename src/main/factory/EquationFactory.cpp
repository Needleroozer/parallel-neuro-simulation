#if INCLUDE_LUT_SUPPORT
#include <string>
#endif //INCLUDE_LUT_SUPPORT

#include "Factory.h"
#include "../math/LUT.h"

using namespace std;

#if INCLUDE_LUT_SUPPORT
  // global variable for a single lookup table object
  lut::LUT *lutSingleton;
#endif //INCLUDE_LUT_SUPPORT

sequential::ode_system_function * factory::equation::getEquation(po::variables_map &vm) {
#if USE_OPENMP
  const unsigned int num_threads_by_user = vm["num-threads"].as<unsigned int>();
  if (num_threads_by_user) { // Therefore if user specifies 0, this will be false
    omp_set_num_threads(num_threads_by_user);
  }
#endif
#if INCLUDE_LUT_SUPPORT
  #if RISCV
    const bool use_hard_lut = vm.count("use-hard-lut") > 0;
    if (use_hard_lut){
      static lut::HardLUTROM hardLUT;
      lutSingleton = &hardLUT;
      return &ode::hodgkinhuxley_lut::calculateNextState;
    }
  #endif //RISCV

  const bool use_soft_lut = vm.count("use-soft-lut") > 0;
  if (use_soft_lut) {
    const auto &lut_file = vm["use-soft-lut"].as<string>();
    static lut::SoftLUT softLUT(lut_file);
    lutSingleton = &softLUT;
    return &ode::hodgkinhuxley_lut::calculateNextState;
  }
#endif
  return ode::hodgkinhuxley::calculateNextState;
}
