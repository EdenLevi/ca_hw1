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
    unsigned tag = pc >> (32-Predictor::tagSize); /// shift by 2 + log2(btb_size)

    /// LSB <-------------------------------> MSB
    /// pc = 00          log2(btb_size)   tagSize
    /// pc = alignment   btb_index        tag

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
        /// Predictor::BTB[index].tag = tag;
        /// GOOD QUESTION: do we need to lidros this? if so how do we get the actual address to jump?

        *dst = (pc+4);
        return false;
    }


    return false;
}

/// still need to take care of global history table and cases where tag is not equal,
/// meaning local history needs to be deleted
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32-Predictor::tagSize);

    if(Predictor::BTB[index].tag == tag) {
        unsigned curr_history = Predictor::BTB[index].history;
        unsigned masked_history = ((curr_history << 1) & 15);
        Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & 0); // set LSB to 1 or 0

        unsigned historyIndex = Predictor::BTB[index].history;
        unsigned i = index*(!Predictor::isGlobalTable); /// if global there is only one prediction table
        unsigned j = historyIndex;
        unsigned n = 2^Predictor::historySize;
        unsigned fsmIndex = i*n + j;
        int state = *(Predictor::predictionTable + fsmIndex);

        *(Predictor::predictionTable + fsmIndex) = taken ? min(3, state+1) : max(0, state-1);


    }
    else {

    }
}

void BP_GetStats(SIM_stats *curStats) {
    curStats->br_num = 1; /// number of 'BP_update' calls
    curStats->flush_num = 1; /// number of 'BP_predict' that caused a flush

    /// theoretical memory size required for predictor in bits (including valid bits)
    curStats->size = Predictor::btbSize * (1 + Predictor::tagSize + 32 + Predictor::historySize + 2*(2^Predictor::historySize));
    /// memory_size = entries * (valid_bit + tag_size + target_size + history_Size + 2*2^history_size)

    delete[] Predictor::predictionTable;
    delete[] Predictor::BTB;
}

