/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include <iostream>
#include "bp_api.h"
#include <cmath>
#include <bitset>

using namespace std;

class BTB_line {
public:
    int tag = -1;
    int target = -1;
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
    static unsigned globalHistory;
    static unsigned flush_num;
    static unsigned br_num;
    static unsigned size;
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
unsigned Predictor::globalHistory = 0;
unsigned Predictor::flush_num = 0;
unsigned Predictor::br_num = 0;
unsigned Predictor::size = 0;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared) {
    Predictor::btbSize = btbSize;
    Predictor::historySize = historySize;
    Predictor::tagSize = tagSize;
    Predictor::fsmState = fsmState;
    Predictor::isGlobalHist = isGlobalHist;
    Predictor::isGlobalTable = isGlobalTable;
    Predictor::Shared = Shared;  /// 0 = not using, 1 = using lsb, 2 = using mid

    try {
        Predictor::predictionTable = isGlobalTable ? (new int[int(pow(2, historySize))]) : (new int[int(pow(2, historySize) * btbSize)]);
        /// init the fsm to the given default state
        int maxFsm = isGlobalTable ? (int(pow(2, historySize))) : (int(pow(2, historySize) * btbSize));
        for (int i = 0; i < maxFsm; i++) {
            *(Predictor::predictionTable + i) = fsmState;
        }

        Predictor::BTB = new BTB_line[btbSize];
    }
    catch (const std::bad_alloc &e) {
        delete[] Predictor::predictionTable;
        delete[] Predictor::BTB;
        return -1;
    }

    /// calculate memory size:
    if (!Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// all local
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + Predictor::historySize + 2 * int(pow(2, Predictor::historySize)));
    }
    else if (!Predictor::isGlobalTable && Predictor::isGlobalHist) { /// local table, global history (ohad says can't happen)
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + 2 * int(pow(2, Predictor::historySize))) + Predictor::historySize;
    }
    else if (Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// global table, local history
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + Predictor::historySize) + 2 * int(pow(2, Predictor::historySize));
    }
    else { /// all global
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30) + 2 * int(pow(2, Predictor::historySize)) + Predictor::historySize;
    }
    return 1;
}


bool BP_predict(uint32_t pc, uint32_t *dst) {
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    //unsigned tag = pc >> (32 - Predictor::tagSize); /// shift by 2 + log2(btb_size);
    unsigned tag = pc >> (2 + int(log2(Predictor::btbSize)));
    tag = tag & (int(pow(2, Predictor::tagSize)) - 1); /// wonder if true




    /// LSB <-------------------------------> MSB
    /// pc = 00          log2(btb_size)   tagSize
    /// pc = alignment   btb_index        tag

    /// Global History Global Table
    if ((Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {

        if (Predictor::BTB[index].tag == (int)tag) {
            unsigned historyIndex = Predictor::globalHistory;

            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int(pow(2, Predictor::historySize) - 1);
                historyIndex = Predictor::globalHistory ^ XORIndex;
            }

            unsigned prediction = *(Predictor::predictionTable + historyIndex);
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Global History Local Table
    else if ((Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == (int)tag) {
            unsigned historyIndex = Predictor::globalHistory;

            unsigned prediction = *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize)));
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Local History Global Table
    else if ((!Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == (int)tag) {
            unsigned historyIndex = Predictor::BTB[index].history;
            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int(pow(2, Predictor::historySize) - 1);
                historyIndex = Predictor::BTB[index].history ^ XORIndex;
            }

            unsigned prediction = *(Predictor::predictionTable + historyIndex);
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Local History Local Table
    else if ((!Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == (int)tag) {
            unsigned historyIndex = Predictor::BTB[index].history;

            int fsmIndex = historyIndex + index * int(pow(2, Predictor::historySize));
            unsigned prediction = *(Predictor::predictionTable + fsmIndex);

            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

    return false;
}


void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {


    /// take care of flush cases:
    if (taken) {
        if (pred_dst != targetPc) {
            Predictor::flush_num++;
        }
    } else {
        if (pred_dst != (pc + 4)) {
            Predictor::flush_num++;
        }
    }
    Predictor::br_num++; /// count branches

    /// calculate index and tag:
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (2 + int(log2(Predictor::btbSize)));
    tag = tag & (int(pow(2, Predictor::tagSize)) - 1); /// wonder if true


    /// Global History Global Table
    if ((Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {

        /// find the previous fsm
        unsigned historyIndex = Predictor::globalHistory;
        if (Predictor::Shared) { /// need to use XOR to get to the fsm
            unsigned XORIndex = pc >> 2;
            if (Predictor::Shared == 2) {
                XORIndex = XORIndex >> 14;
            }
            XORIndex = XORIndex & int((pow(2, Predictor::historySize)) - 1);
            historyIndex = Predictor::globalHistory ^ XORIndex;
        }

        int state = *(Predictor::predictionTable + historyIndex); /// this is the predicted behaviour

        *(Predictor::predictionTable + historyIndex) = taken ? min(3, state + 1) : max(0, state - 1);
        unsigned curr_history = Predictor::globalHistory;
        unsigned masked_history = ((curr_history << 1) & ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
        Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0


    }

        /// Global History Local Table
    else if ((Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        unsigned historyIndex = Predictor::globalHistory;

        /// its a hit
        if (Predictor::BTB[index].tag == (int)tag) {
            /// update fsm: if taken decrease by 1 (minimum is 0), else increase by 1 (maximum is 3)
            int state = *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))); /// this is the predicted behaviour
            *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))) = taken ? min(3, state + 1) : max(0, state - 1);
        }

            /// its a miss
        else {
            /// clear the fsm table
            for (int i = (int)index * (int(pow(2, Predictor::historySize))); i < (int)((index + 1) * (int(pow(2, Predictor::historySize)))); i++) {
                *(Predictor::predictionTable + i) = (int)Predictor::fsmState;
            }
            ///////////////////////////////////////////////
            /// update fsm table based on current iteration
            ///////////////////////////////////////////////
            int state = *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))); /// this is the predicted behaviour
            *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))) = taken ? min(3, state + 1) : max(0, state - 1);
        }
        unsigned curr_history = Predictor::globalHistory; /// does it need to be outside?
        unsigned masked_history = ((curr_history << 1) & ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
        Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0
    }
        /// Local History Global Table
    else if ((!Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {
        /// its a hit
        if (Predictor::BTB[index].tag == (int)tag) {
            unsigned historyIndex = Predictor::BTB[index].history;

            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int((pow(2, Predictor::historySize) - 1));
                historyIndex = Predictor::BTB[index].history ^ XORIndex;
            }

            /// update fsm: if taken decrease by 1 (minimum is 0), else increase by 1 (maximum is 3)
            int state = *(Predictor::predictionTable + historyIndex); /// this is the predicted behaviour
            *(Predictor::predictionTable + historyIndex) = taken ? min(3, state + 1) : max(0, state - 1);

            /// update history: shift by 1 and set LSB to 1/0
            unsigned curr_history = Predictor::BTB[index].history;
            unsigned masked_history = ((curr_history << 1) & ((int(pow(2, Predictor::historySize)) - 1)));
            Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & -2);
        }

            /// its a miss
        else {
            ////////////////////////////////////////////////////////////////////////
            /////  TESTING IF WE NEED TO UPDATE FSM ON INSERT OF NEW BRANCH    /////
            ////////////////////////////////////////////////////////////////////////
            Predictor::BTB[index].history = 0;

            unsigned historyIndex = Predictor::BTB[index].history;

            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int((pow(2, Predictor::historySize) - 1));
                historyIndex = Predictor::BTB[index].history ^ XORIndex;
            }

            /// update fsm: if taken decrease by 1 (minimum is 0), else increase by 1 (maximum is 3)
            int state = *(Predictor::predictionTable + historyIndex); /// this is the predicted behaviour
            *(Predictor::predictionTable + historyIndex) = taken ? min(3, state + 1) : max(0, state - 1);
            Predictor::BTB[index].history = taken;
            ////////////////////////////////////////////////////////////////////////
            /////                              END                             /////
            ////////////////////////////////////////////////////////////////////////


            /// clear local history
        }

    }

        /// Local History Local Table
    else if ((!Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        /// its a hit
        if (Predictor::BTB[index].tag == (int)tag) {
            int fsmIndex = Predictor::BTB[index].history + index * int(pow(2, Predictor::historySize));
            int state = *(Predictor::predictionTable + fsmIndex); /// this is the predicted behaviour

            *(Predictor::predictionTable + fsmIndex) = taken ? min(3, state + 1) : max(0, state - 1);
            unsigned curr_history = Predictor::BTB[index].history;
            unsigned masked_history = ((curr_history << 1) & ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
            Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0

        }

            /// its a miss
        else {
            /// clear local table
            for (int i = index * (int(pow(2, Predictor::historySize))); i < (int)((index + 1) * (int(pow(2, Predictor::historySize)))); i++) {
                *(Predictor::predictionTable + i) = (int)Predictor::fsmState;
            }


            ////////////////////////////////////////////////////////////////////////
            /////  TESTING IF WE NEED TO UPDATE FSM ON INSERT OF NEW BRANCH    /////
            ////////////////////////////////////////////////////////////////////////
            Predictor::BTB[index].history = 0;

            int fsmIndex = Predictor::BTB[index].history + index * int(pow(2, Predictor::historySize));
            int state = *(Predictor::predictionTable + fsmIndex); /// this is the predicted behaviour

            *(Predictor::predictionTable + fsmIndex) = taken ? min(3, state + 1) : max(0, state - 1);
            Predictor::BTB[index].history = taken;
            ////////////////////////////////////////////////////////////////////////
            /////                              END                             /////
            ////////////////////////////////////////////////////////////////////////

            /// clear local history
            //Predictor::BTB[index].history = 0;
        }
    }

    /// always update tag and target to the latest branch predicted
    Predictor::BTB[index].tag = (int)tag;
    Predictor::BTB[index].target = (int)targetPc;
}


void BP_GetStats(SIM_stats *curStats) {
    curStats->br_num = Predictor::br_num; /// number of 'BP_update' calls
    curStats->flush_num = Predictor::flush_num; /// number of 'BP_predict' that caused a flush
    curStats->size = Predictor::size; /// memory size which was calculated on init

    delete[] Predictor::predictionTable;
    delete[] Predictor::BTB;
}
