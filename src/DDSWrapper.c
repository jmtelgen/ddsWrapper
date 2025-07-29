#include <stdbool.h>
#include "dds.h"
#include <stdio.h>
#include <string.h>
#include <emscripten/emscripten.h>

EMSCRIPTEN_KEEPALIVE int dds_init()
{
    SetResources(40, 1);
    return 0;
}

void translate_current_trick_card(int rb_card, int* dds_suit, int* dds_rank)
{
    if (rb_card < 0)
    {
        dds_suit[0] = 0;
        dds_rank[0] = 0;
        return;
    }

    dds_suit[0] = 3 - (rb_card / 13);       // DDS encoding: 0=spades, 1=hearts, 2=diamonds, 3=clubs
    dds_rank[0] = (rb_card % 13) + 2;       // DDS encoding: 2=2, 3=3 ..., 14=A
}

EMSCRIPTEN_KEEPALIVE int do_dds_solve_board(int contract_bid,
                                            int hand_to_play,
                                            int currentTrick0,
                                            int currentTrick1,
                                            int currentTrick2,
                                            int target,
                                            int solution,
                                            char* pbn_remain_cards,
                                            int* output_array)
{
    // -----------------------------------------------
    // 1. Translate parameters into DDS-speak
    // -----------------------------------------------
    struct dealPBN dpbn;
    dpbn.first = hand_to_play;                      // straight copy

    int rb_trump = contract_bid % 5;
    dpbn.trump = (rb_trump == 4) ? 4 : (3 - rb_trump);  // DDS encoding: 0=spades, 1=hearts, 2=diamonds, 3=clubs, 4=NT

    // current trick
    translate_current_trick_card(currentTrick0, dpbn.currentTrickSuit + 0, dpbn.currentTrickRank + 0);
    translate_current_trick_card(currentTrick1, dpbn.currentTrickSuit + 1, dpbn.currentTrickRank + 1);
    translate_current_trick_card(currentTrick2, dpbn.currentTrickSuit + 2, dpbn.currentTrickRank + 2);

    memcpy(dpbn.remainCards, pbn_remain_cards, strlen(pbn_remain_cards) + 1);

    // -----------------------------------------------
    // 2. Invoke DDS
    // -----------------------------------------------
    struct futureTricks ft;
    /*
        target  solution  description
        -1	    1	    Find the maximum number of tricks for the side to play. Return only one of the optimum cards and its score.
        -1	    2	    Find the maximum number of tricks for the side to play. Return all optimum cards and their scores.
        0	    1	    Return only one of the cards legal to play, with score set to 0.
        0	    2	    Return all cards that legal to play, with score set to 0.
        1 .. 13	1	    If score is -1: Target cannot be reached.
                        If score is 0: In fact no tricks at all can be won.
                        If score is > 0: score will always equal target, even if more tricks can be won.
                        One of the cards achieving the target is returned.
        1 .. 13	2	    Return all cards meeting (at least) the target.
                        If the target cannot be achieved, only one card is returned with the score set as above.
        any	    3	    Return all cards that can be legally played, with their scores in descending order.
    */
    SolveBoardPBN(dpbn, target, solution, 1, &ft, 0);

    // -----------------------------------------------
    // 3. Translate output
    // -----------------------------------------------

    // We return an array of 2N integers, where N is the number of options (legal-to-play cards) in the hand-to-play.
    // Each pair of 2 integers represents (card, result).
    // N.B. these are returned in an arbitrary order. [No particular advantage to sorting them.]

    int* output_ptr = output_array;
    for (int i = 0; i < ft.cards; ++i)
    {
        *output_ptr++ = (3 - ft.suit[i]) * 13 + (ft.rank[i] - 2);
        *output_ptr++ = ft.score[i];

        // also output all equivalent cards
        for (int j = ft.rank[i] - 1; j >= 2; --j)
        {
            if ((ft.equals[i] & (1 << (j - 2))) > 0)
            {
                *output_ptr++ = (3 - ft.suit[i]) * 13 + (j - 2);
                *output_ptr++ = ft.score[i];
            }
        }
    }

    while (output_ptr < output_array + 26)
        *output_ptr++ = -1;

    return ft.nodes;
}

EMSCRIPTEN_KEEPALIVE int do_dds_calc_tables(char* pbn_cards, int* output_array)
{
    // -----------------------------------------------
    // 1. Translate parameters into DDS-speak
    // -----------------------------------------------
    struct ddTableDealPBN dpbn;
    memcpy(dpbn.cards, pbn_cards, strlen(pbn_cards) + 1);

    struct ddTableResults tablep;

    // -----------------------------------------------
    // 2. Invoke DDS CalcDDtablePBN
    // -----------------------------------------------
    int res = CalcDDtablePBN(dpbn, &tablep);
    if (res != RETURN_NO_FAULT)
        return res;

    // -----------------------------------------------
    // 3. Translate output
    // -----------------------------------------------
    // We return a 2D array of 5x4 integers representing the double dummy table
    // Each entry is the number of tricks that can be won by the declarer
    // Rows: 0=NT, 1=Spades, 2=Hearts, 3=Diamonds, 4=Clubs
    // Columns: 0=North, 1=East, 2=South, 3=West

    int* output_ptr = output_array;
    for (int strain = 0; strain < DDS_STRAINS; ++strain)
    {
        for (int hand = 0; hand < DDS_HANDS; ++hand)
        {
            *output_ptr++ = tablep.resTable[strain][hand];
        }
    }

    return RETURN_NO_FAULT;
}

EMSCRIPTEN_KEEPALIVE int do_dds_calc_par(char* pbn_cards, int vulnerable, char* output_par_score, char* output_par_contracts)
{
    // -----------------------------------------------
    // 1. Translate parameters into DDS-speak
    // -----------------------------------------------
    struct ddTableDealPBN dpbn;
    memcpy(dpbn.cards, pbn_cards, strlen(pbn_cards) + 1);

    struct ddTableResults tablep;
    struct parResults presp;

    // -----------------------------------------------
    // 2. Calculate double dummy table first
    // -----------------------------------------------
    int res = CalcDDtablePBN(dpbn, &tablep);
    if (res != RETURN_NO_FAULT)
        return res;

    // -----------------------------------------------
    // 3. Calculate par score and contracts
    // -----------------------------------------------
    res = Par(&tablep, &presp, vulnerable);
    if (res != RETURN_NO_FAULT)
        return res;

    // -----------------------------------------------
    // 4. Copy results to output strings
    // -----------------------------------------------
    // Copy NS par score (index 0)
    strcpy(output_par_score, presp.parScore[0]);
    
    // Copy NS par contracts (index 0)
    strcpy(output_par_contracts, presp.parContractsString[0]);

    return RETURN_NO_FAULT;
}