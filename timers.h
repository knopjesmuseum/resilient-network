#define MAX_TIMERS 5

int addTimer(unsigned long ms, void (*func)());
void removeTimer(int id);
void removeAllTimers();
void handleTimers();

typedef struct Timer {
  int id;
  unsigned long start;
  unsigned long duration;
  void (*callback)();
};

Timer timers[MAX_TIMERS];
int numtimers = 0;

int addTimer(unsigned long ms, void (*func)()) {
  // create new timer
  if (numtimers<MAX_TIMERS) {
    // get new timer id
    int newid = 0;
    boolean flag = true;
    while(flag) {
      flag = false;
      for (int i=0; i<numtimers; i++) if (timers[i].id == newid) {
        newid++;
        flag = true;
      }
    }
    timers[numtimers].id = newid;
    timers[numtimers].start = millis();
    timers[numtimers].duration = ms;
    timers[numtimers].callback = func;
    numtimers++;
    return newid;
  }
  else return -1;
}

void removeTimer(int id) {
  boolean flag = false;
  for (int i=0; i<numtimers; i++) {
    if (timers[i].id == id) {
      flag = true;
      numtimers--;
    }
    if (flag) timers[i] = timers[i+1];
  }
}

void removeAllTimers() {
  numtimers = 0;
}

void handleTimers() {
  void (*func)();
  unsigned long currenttime = millis();
  // walk through all timers
  for (int i=0; i<numtimers; i++) {
    // check if timer expired
    if (currenttime - timers[i].start >= timers[i].duration) {
      // copy callback pointer to temporary variable
      func = timers[i].callback;
      // remove timer
      removeTimer(timers[i].id);
      // invoke callback
      (*func)();
    }
  }
}

