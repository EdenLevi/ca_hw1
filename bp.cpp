/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include <iostream>
#include "bp_api.h"
using namespace std;

class BTB_line {
public:
    unsigned tag = 0;
    unsigned target = 0;
    unsigned history = 0;
};

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
    static BTB_line *BTB;
};

unsigned Predictor::btbSize = 0;
unsigned Predictor::historySize = 0;
unsigned Predictor::tagSize = 0;
unsigned Predictor::fsmState = 0;
bool Predictor::isGlobalHist = false;
bool Predictor::isGlobalTable = false;
int Predictor::Shared = 0;
int *Predictor::predictionTable = nullptr;
BTB_line *Predictor::BTB = nullptr;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared) {
    Predictor::btbSize = btbSize;
    Predictor::historySize = historySize;
    Predictor::tagSize = tagSize;
    Predictor::fsmState = fsmState;
    Predictor::isGlobalHist = isGlobalHist;
    Predictor::isGlobalTable = isGlobalTable;
    Predictor::Shared = Shared;

    try {
        Predictor::predictionTable = isGlobalTable ? (new int[2 ^ (historySize)]) : (new int[2 ^(historySize) * btbSize]);
        Predictor::BTB = new BTB_line[btbSize];
    }
    catch (const std::bad_alloc& e) {
        delete[] Predictor::predictionTable;
        delete[] Predictor::BTB;
        return -1;
    }

    return 1;
}

bool BP_predict(uint32_t pc, uint32_t *dst) {
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32-Predictor::tagSize);

    /// at this point index = correct BTB line
    if(Predictor::BTB[index].tag == tag) {
        unsigned historyIndex = Predictor::BTB[index].history;

        unsigned i = index*(!Predictor::isGlobalTable); /// if global there is only one prediction table
        unsigned j = historyIndex;
        unsigned n = 2^Predictor::historySize;

        unsigned fsmIndex = i*n + j;
        unsigned prediction = *(Predictor::predictionTable + fsmIndex);

        *dst = (prediction>=2) ? (Predictor::BTB[index].target) : (pc+4);
        return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        ///*(Predictor::predictionTable + index*(2^Predictor::historySize)*(!Predictor::isGlobalTable) + historyIndex);

    }
    else {

        // drisa
        Predictor::BTB[index].tag = tag;
        /// GOOD QUESTION: do we need to lidros this? if so how do we get the actual address to jump?

        *dst = (pc+4);
        return false;
    }


    return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
}

void BP_GetStats(SIM_stats *curStats) {

    delete[] Predictor::predictionTable;
    delete[] Predictor::BTB;
}

