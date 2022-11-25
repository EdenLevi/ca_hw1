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
        Predictor::predictionTable = isGlobalTable ? (new int[2 ^ (historySize)]) : (new int[2 ^
                                                                                             (historySize) * btbSize]);
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

    if(!Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// all local
        Predictor::size = Predictor::btbSize *(1 + Predictor::tagSize + 32 + Predictor::historySize + 2 * (2 ^ Predictor::historySize));
    }
    else if(!Predictor::isGlobalTable && Predictor::isGlobalHist) { /// local table, global history (ohad says can't happen)
        Predictor::size = Predictor::btbSize *(1 + Predictor::tagSize + 32 + 2 * (2 ^ Predictor::historySize)) + Predictor::historySize;
    }
    else if(Predictor::isGlobalTable && !Predictor::isGlobalHist) { /// global table, local history
        Predictor::size = Predictor::btbSize *(1 + Predictor::tagSize + 32 + Predictor::historySize) + 2 * (2 ^ Predictor::historySize);
    }
    else { /// all global
        Predictor::size = Predictor::btbSize *(1 + Predictor::tagSize + 32) + 2 * (2 ^ Predictor::historySize) + Predictor::historySize;
    }

    return 1;
}


/// need to take care of case where history is global ( use it for deciding index )
/// need to take care of case where using share lsb \ mid                               - i think its good
bool BP_predict(uint32_t pc, uint32_t *dst) {
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32 - Predictor::tagSize); /// shift by 2 + log2(btb_size)

    /// LSB <-------------------------------> MSB
    /// pc = 00          log2(btb_size)   tagSize
    /// pc = alignment   btb_index        tag

    /// at this point index = correct BTB line
    if ((Predictor::BTB[index].tag == tag) || (Predictor::isGlobalHist)) { /// thats a hit
        unsigned historyIndex = Predictor::BTB[index].history;

        /// if there is only one fsm table and using Share, need to use XOR
        if ((Predictor::isGlobalTable) && (Predictor::Shared)){
            ///do XOR magic
            unsigned XORIndex = pc >> 2;
            if (Predictor::Shared == 2){
                XORIndex = XORIndex >> 14;
            }
            XORIndex = XORIndex & ((2^Predictor::historySize)-1);
            historyIndex = Predictor::BTB[index].history ^ XORIndex ;
        }


        unsigned i = index * (!Predictor::isGlobalTable); /// if global there is only one prediction table
        unsigned j = historyIndex;
        unsigned n = 2 ^ Predictor::historySize;

        unsigned fsmIndex = i * n + j;
        unsigned prediction = *(Predictor::predictionTable + fsmIndex);

        *dst = (prediction >= 2) ? (Predictor::BTB[index].target) : (pc + 4);
        return (prediction >= 2); /// ST = 3, WT = 2, WNT = 1, SNT = 0
        ///*(Predictor::predictionTable + index*(2^Predictor::historySize)*(!Predictor::isGlobalTable) + historyIndex);
    }

    /// that is a miss
    else {
        *dst = (pc + 4);
        return false;
    }

    return false;
}



/// note: need to take care of XOR based on input (LSB, MSB, etc) - only when fsm is global - isGlobalTable == 1


/// still need to take care of global history table and cases where tag is not equal,
/// meaning local history needs to be deleted

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
    Predictor::br_num++;   /// added this for counting branches. need to add a miss counter in this func aswell
    unsigned index = pc >> 2;
    index = index & (Predictor::btbSize - 1); // masking the pc
    unsigned tag = pc >> (32 - Predictor::tagSize);

    /// Global History Global Table
    if((Predictor::isGlobalHist) && (Predictor::isGlobalTable)){
        unsigned historyIndex = Predictor::BTB[index].history;
        if(Predictor::Shared){ /// need to use XOR to get to the fsm
            unsigned XORIndex = pc >> 2;
            if (Predictor::Shared == 2){
                XORIndex = XORIndex >> 14;
            }
            XORIndex = XORIndex & ((2^Predictor::historySize)-1);
            historyIndex = Predictor::BTB[index].history ^ XORIndex ;
        }
        /// check for flush




        unsigned curr_history = Predictor::BTB[index].history;
        unsigned masked_history = ((curr_history << 1) & ((2^Predictor::historySize)-1));     // set LSB to 1 or 0
        Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & -1); // set LSB to 1 or 0


    }

    /// Global History Local Table
    else if((Predictor::isGlobalHist) && (!Predictor::isGlobalTable)){

    }

    /// Local History Global Table
    else if((!Predictor::isGlobalHist) && (Predictor::isGlobalTable)){

    }

    /// Local History Local Table
    else if((!Predictor::isGlobalHist) && (!Predictor::isGlobalTable)){

    }


    if ((Predictor::BTB[index].tag == tag) || (Predictor::isGlobalHist)) {
        if (!Predictor::isGlobalHist) {
            unsigned curr_history = Predictor::BTB[index].history;
            unsigned masked_history = ((curr_history << 1) & ((2^Predictor::historySize)-1)); ///changed the 2nd term of the & from 15 cause the history size isnt always 4
            Predictor::BTB[index].history = taken ? (masked_history | 1) : (masked_history & -1); // set LSB to 1 or 0
        }                                                                                 ///changed it to & -1 from & 0, cause & 0 will zero it all
        else {
            unsigned curr_history = Predictor::globalHistory;
            unsigned masked_history = ((curr_history << 1) & ((2^Predictor::historySize)-1)); ///changed the 2nd term of the & from 15 cause the history size isnt always 4
            Predictor::globalHistory = taken ? (masked_history | 1) : (masked_history & -1); // set LSB to 1 or 0
        }                                                                                ///changed it to & -1 from & 0, cause & 0 will zero it all

        unsigned historyIndex = Predictor::BTB[index].history;
        unsigned i = index * (!Predictor::isGlobalTable); /// if global there is only one prediction table
        unsigned j = historyIndex;
        unsigned n = 2 ^ Predictor::historySize;
        unsigned fsmIndex = i * n + j;
        int state = *(Predictor::predictionTable + fsmIndex);

        *(Predictor::predictionTable + fsmIndex) = taken ? min(3, state + 1) : max(0, state - 1);


    }
    /// pc is not in the BTB, and also history is not global
    /// so we need to:
    /// change the tag
    /// change the target pc                                - done
    /// zero the history of the BTB line                    - done
    /// move this BTB line's fsm to default value
    else {
        unsigned historyIndex = Predictor::BTB[index].history; /// need to add using share option


        Predictor::BTB[index].target =  targetPc; /// initializing the line
        Predictor::BTB[index].history = 0;

    }
}


void BP_GetStats(SIM_stats *curStats) {
    curStats->br_num = Predictor::br_num; /// number of 'BP_update' calls
    curStats->flush_num = Predictor::flush_num; /// number of 'BP_predict' that caused a flush
    curStats->size = Predictor::size;

    /// MOVED THE SIZE CALC TO THE INIT FUNC!



  /// i dont think that this func needs to delete it right now - it can be called couple of time during the run
//    delete[] Predictor::predictionTable;
//    delete[] Predictor::BTB;
}

