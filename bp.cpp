/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

class Predictor {
public:
    static unsigned btbSize;
    static unsigned historySize;
    static unsigned tagSize;
    static unsigned fsmState;
    static bool isGlobalHist;
    static bool isGlobalTable;
    static int Shared;
    static int *predictionTable;
    static int *historyTable;
};

unsigned Predictor::btbSize;
unsigned Predictor::historySize;
unsigned Predictor::tagSize;
unsigned Predictor::fsmState;
bool Predictor::isGlobalHist;
bool Predictor::isGlobalTable;
int Predictor::Shared;
int *Predictor::predictionTable = nullptr;
int *Predictor::historyTable = nullptr;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared) {
    Predictor::btbSize = btbSize;
    Predictor::historySize = historySize;
    Predictor::tagSize = tagSize;
    Predictor::fsmState = fsmState;
    Predictor::isGlobalHist = isGlobalHist;
    Predictor::isGlobalTable = isGlobalTable;
    Predictor::Shared = Shared;
    Predictor::predictionTable = new int[btbSize]; /// ?
    Predictor::historyTable = new int[historySize]; /// ?

    return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst) {
    return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
}

void BP_GetStats(SIM_stats *curStats) {

    delete[] Predictor::predictionTable;
    delete[] Predictor::historyTable;
}

