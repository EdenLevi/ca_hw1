# Branch Predictor simulator
This program is a simulator for a 2-level branch predictor. 
The configuration of the predictor will be defined at the beginning of the simulation run using parameters. 

Parameter  | Available Parameters
------------- | -------------
Branch Target Buffer (BTB) size  | 1, 2, 4, 8, 16, 32
History register size (in Bits)  | 1 to 8
Size of the Tag field in the BTB (in Bits) | 0 to (30 - log2[BTB Size])
Starting state of the Bimodal State machines | 0 to 3 (0 = SNT, 1 = WNT, 2 = WT, 3 = ST)
History mode | local_history, global_history
State machines Tables | local_tables, global_tables
Usage of Lshare/Gshare (only relevant with global_tables) | not_using_share, using_share_mid, using_share_lsb




