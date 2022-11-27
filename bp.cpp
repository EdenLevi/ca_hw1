/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include <iostream>
#include "bp_api.h"
#include <cmath>
#include <bitset>

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
        Predictor::predictionTable = isGlobalTable ? (new int[int(pow(2, historySize))]) : (new int[int(
                pow(2, historySize) * btbSize)]);
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

    /// calc memory needed for the data
    /// theoretical memory size required for predictor in bits (including valid bits)
    /// memory_size = entries * (valid_bit + tag_size + target_size + history_Size + 2*2^history_size)
    /// note: need to take into consideration the type of machine (global/local, etc)

    if (!Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// all local
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + Predictor::historySize +
                                                2 * int(pow(2, Predictor::historySize)));
    } else if (!Predictor::isGlobalTable &&
               Predictor::isGlobalHist) { /// local table, global history (ohad says can't happen)
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + 2 * int(pow(2, Predictor::historySize))) +
                          Predictor::historySize;
    } else if (Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// global table, local history
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30 + Predictor::historySize) +
                          2 * int(pow(2, Predictor::historySize));
    } else { /// all global
        Predictor::size = Predictor::btbSize * (1 + Predictor::tagSize + 30) + 2 * int(pow(2, Predictor::historySize)) +
                          Predictor::historySize;
    }
    /// need to change 2 ^ x to pow(2,x)
    return 1;
}


bool BP_predict(uint32_t pc, uint32_t *dst) {
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32 - Predictor::tagSize); /// shift by 2 + log2(btb_size)

    /// LSB <-------------------------------> MSB
    /// pc = 00          log2(btb_size)   tagSize
    /// pc = alignment   btb_index        tag


    /// Global History Global Table
    if ((Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {

        if (Predictor::BTB[index].tag == tag) {
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
            //    cout <<"the pc is " << pc << " and the prediction is " << prediction << "\n";
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Global History Local Table
    else if ((Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == tag) {
            unsigned historyIndex = Predictor::globalHistory;

            unsigned prediction = *(Predictor::predictionTable + historyIndex
                                    + index * int(pow(2, Predictor::historySize)));
            //    cout <<"the pc is " << pc << " and the prediction is " << prediction << "\n";
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Local History Global Table (probably doesnt exist)
    else if ((!Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == tag) {
            unsigned historyIndex = Predictor::BTB[index].history;
            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int(pow(2, Predictor::historySize) - 1);
                historyIndex = Predictor::globalHistory ^ XORIndex;
            }

            unsigned prediction = *(Predictor::predictionTable + historyIndex);
            //    cout <<"the pc is " << pc << " and the prediction is " << prediction << "\n";
            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        } else {
            *dst = (pc + 4);
            return false;
        }
    }

        /// Local History Local Table
    else if ((!Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        if (Predictor::BTB[index].tag == tag) {
            unsigned historyIndex = Predictor::BTB[index].history;

            unsigned prediction = *(Predictor::predictionTable +
                                    historyIndex +
                                    index * int(pow(2, Predictor::historySize)));

            *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
            return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        }
        else {
            *dst = (pc + 4);
            return false;
        }
    }

    return false;
}



/// still need to take care of global history table and cases where tag is not equal,
/// meaning local history needs to be deleted


void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {

    if (taken) {
        if (pred_dst != targetPc) {
            Predictor::flush_num++;
        }
    } else {
        if (pred_dst != (pc + 4)) {
            Predictor::flush_num++;
        }
    }

    Predictor::br_num++;   /// added this for counting branches. need to add a miss counter in this func aswell
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32 - Predictor::tagSize);

    int print_flag = 0;
    if(print_flag){
        /// print the fsm
        cout << index << '\n';
        for (int i = 0;
             i <  ((int(pow(2, Predictor::historySize) + (!Predictor::isGlobalTable) * Predictor::btbSize))); i++) {
            std::bitset<32> x(*(Predictor::predictionTable + i));
            cout << x << '\n';
        }
        cout << "--------------------------------\n";
    }
    /// Global History Global Table
    if ((Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {
        /// print the fsm table
        //cout << "Printing fsm states!" << '\n';
        for (int i = 0; i < int(pow(2, Predictor::historySize)); i++) {
            std::bitset<32> x(*(Predictor::predictionTable + i));
            //    cout << x << '\n';
        }

        /// find the previous fsm
        unsigned historyIndex = Predictor::globalHistory;
        if (Predictor::Shared) { /// need to use XOR to get to the fsm
        //    cout << Predictor::Shared <<" \n";
            unsigned XORIndex = pc >> 2;
            if (Predictor::Shared == 2) {
                XORIndex = XORIndex >> 14;
            }
            XORIndex = XORIndex & int((pow(2, Predictor::historySize)) - 1);
          //  cout << XORIndex << '\n';
            historyIndex = Predictor::globalHistory ^ XORIndex;
        }

        /// insert tag and target to the BTB
        Predictor::BTB[index].tag = tag;
        Predictor::BTB[index].target = targetPc;


        int state = *(Predictor::predictionTable + historyIndex); /// this is the predicted behaviour
        //if ((taken) ^ (state >= 2)) {  /// there is a flush if taken (actual behaviour) is different then prediction
        //Predictor::flush_num = Predictor::flush_num + 1;
        //}
        *(Predictor::predictionTable + historyIndex) = taken ? min(3, state + 1) : max(0, state - 1);
        unsigned curr_history = Predictor::globalHistory;
        unsigned masked_history = ((curr_history << 1) &
                                   ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
        Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0


    }

        /// Global History Local Table
    else if ((Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        unsigned historyIndex = Predictor::globalHistory;

        /// its a hit
        if (Predictor::BTB[index].tag == tag) {
            int state = *(Predictor::predictionTable + historyIndex +
                          index * int(pow(2, Predictor::historySize))); /// this is the predicted behaviour
            //if ((taken) ^ (state >= 2)) {  /// there is a flush if taken (actual behaviour) is different then prediction
            //Predictor::flush_num = Predictor::flush_num + 1;
            //}

            *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))) =
                    taken ? min(3, state + 1) : max(0, state - 1);
            unsigned curr_history = Predictor::globalHistory;
            unsigned masked_history = ((curr_history << 1) &
                                       ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
            Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0
        }

            /// its a miss
        else {
            /// insert tag and target to the BTB
            Predictor::BTB[index].tag = tag;
            Predictor::BTB[index].target = targetPc;

            /// set the proper fsm table to default state IF its a miss
            int maxFsm = (int(pow(2, Predictor::historySize)));
            for (int i = index; i < index + maxFsm; i++) {
                *(Predictor::predictionTable + i) = Predictor::fsmState;
            }
        }
    }

        /// Local History Global Table
    else if ((!Predictor::isGlobalHist) && (Predictor::isGlobalTable)) {
        /// its a hit
        if (Predictor::BTB[index].tag == tag) {
            unsigned historyIndex = Predictor::BTB[index].history;

            if (Predictor::Shared) { /// need to use XOR to get to the fsm
                unsigned XORIndex = pc >> 2;
                if (Predictor::Shared == 2) {
                    XORIndex = XORIndex >> 14;
                }
                XORIndex = XORIndex & int((pow(2, Predictor::historySize) - 1));
                historyIndex = Predictor::globalHistory ^ XORIndex;
            }

            int state = *(Predictor::predictionTable + historyIndex); /// this is the predicted behaviour
            //if ((taken) ^ (state >= 2)) {  /// there is a flush if taken (actual behaviour) is different then prediction
                //Predictor::flush_num = Predictor::flush_num + 1;
            //}

            *(Predictor::predictionTable + historyIndex) = taken ? min(3, state + 1) : max(0, state - 1);
            unsigned curr_history = Predictor::BTB[index].history; // its the same as historyIndex
            unsigned masked_history = ((curr_history << 1) &
                                       ((int(pow(2, Predictor::historySize)) - 1)));  // set LSB to 1 or 0
            Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0
        }

            /// its a miss
        else {
            Predictor::BTB[index].tag = tag;
            Predictor::BTB[index].target = targetPc;
            Predictor::BTB[index].history = 0;
        }
    }

        /// Local History Local Table
    else if ((!Predictor::isGlobalHist) && (!Predictor::isGlobalTable)) {
        /// its a hit
        if (Predictor::BTB[index].tag == tag) {
            unsigned historyIndex = Predictor::BTB[index].history;
            int state = *(Predictor::predictionTable + historyIndex +
                          index * int(pow(2, Predictor::historySize))); /// this is the predicted behaviour
            //if ((taken) ^ (state >= 2)) {  /// there is a flush if taken (actual behaviour) is different then prediction
            //Predictor::flush_num = Predictor::flush_num + 1;
            //}
            *(Predictor::predictionTable + historyIndex + index * int(pow(2, Predictor::historySize))) =
                    taken ? min(3, state + 1) : max(0, state - 1);
            unsigned curr_history = Predictor::BTB[index].history;
            unsigned masked_history = ((curr_history << 1) &
                                       ((int(pow(2, Predictor::historySize)) - 1)));     // set LSB to 1 or 0
            Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & -2); // set LSB to 1 or 0

        }

            /// its a miss
        else {
            Predictor::BTB[index].tag = tag;
            Predictor::BTB[index].target = targetPc;
            Predictor::BTB[index].history = 0;

            /// set the proper fsm table to default state IF its a miss
            int maxFsm = (int(pow(2, Predictor::historySize)));
            for (int i = index; i < index + maxFsm; i++) {
                *(Predictor::predictionTable + i) = Predictor::fsmState;
            }
        }
    }
}


void BP_GetStats(SIM_stats *curStats) {
    curStats->br_num = Predictor::br_num; /// number of 'BP_update' calls
    curStats->flush_num = Predictor::flush_num; /// number of 'BP_predict' that caused a flush
    curStats->size = Predictor::size;

    /// MOVED THE SIZE CALC TO THE INIT FUNC!



    /// i dont think that this func needs to delete it right now - it can be called couple of time during the run
    delete[] Predictor::predictionTable;
    delete[] Predictor::BTB;
}

