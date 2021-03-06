#ifndef PRECICE_NO_MPI
#include "mpi.h"
#endif
#include "tarch/multicore/BooleanSemaphore.h"
#include "utils/assertion.hpp"
#include "logging/Logger.hpp"


#include <limits>
#include <chrono>


int        tarch::multicore::BooleanSemaphore::_pauseCounter(1);
const int  tarch::multicore::BooleanSemaphore::_pauseBeforeYield(32);
const int  tarch::multicore::BooleanSemaphore::_counterThresholdForWarning(std::numeric_limits<int>::max() - 20);


tarch::multicore::BooleanSemaphore::BooleanSemaphore():
  _mutex() {
}


tarch::multicore::BooleanSemaphore::~BooleanSemaphore() {
}


void tarch::multicore::BooleanSemaphore::enterCriticalSection() {
  _mutex.lock();
}


void tarch::multicore::BooleanSemaphore::leaveCriticalSection() {
  _mutex.unlock();
}


void tarch::multicore::BooleanSemaphore::sendCurrentTaskToBack(const std::string& methodTrace) {
  static precice::logging::Logger  _log( "tarch::multicore::BooleanSemaphore" );
  if (_pauseCounter < _pauseBeforeYield) {
    cobra::this_thread::sleep_for(std::chrono::seconds(_pauseCounter));
    _pauseCounter*=2;
  }
  else {
    if (_pauseCounter>_counterThresholdForWarning && _pauseCounter != std::numeric_limits<int>::max()) {
      _pauseCounter = std::numeric_limits<int>::max();
      preciceWarning( "sendCurrentTaskToBack(string)", "probably running into deadlock or inefficient behaviour in " << methodTrace );
    }
    else {
      _pauseCounter++;
    }
    cobra::this_thread::yield();
  }
}


void tarch::multicore::BooleanSemaphore::continueWithTask() {
  _pauseCounter = 1;
}
