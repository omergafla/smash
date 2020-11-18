#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
  cout << "smash: got ctrl-Z" <<endl;

}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  cout << "smash: got ctrl-C omer was here" <<endl;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

