// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "xmath/xmath.h"
#include <math.h>

#include <ns_api.h>
#include <ns_priv.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31

int32_t half_hanning[NS_WINDOW_LENGTH / 2] = {
	13996827, 27993059, 41988102, 55981362, 69972243, 83960152, 97944494, 111924676, 
	125900102, 139870180, 153834316, 167791917, 181742389, 195685141, 209619580, 223545114, 
	237461151, 251367100, 265262371, 279146373, 293018516, 306878212, 320724870, 334557904, 
	348376724, 362180746, 375969381, 389742044, 403498150, 417237114, 430958354, 444661286, 
	458345328, 472009898, 485654416, 499278303, 512880980, 526461868, 540020391, 553555974, 
	567068040, 580556016, 594019328, 607457406, 620869678, 634255574, 647614526, 660945965, 
	674249327, 687524045, 700769556, 713985296, 727170706, 740325223, 753448290, 766539350, 
	779597845, 792623221, 805614925, 818572406, 831495111, 844382493, 857234004, 870049099, 
	882827231, 895567860, 908270443, 920934441, 933559316, 946144532, 958689554, 971193848, 
	983656885, 996078134, 1008457067, 1020793159, 1033085887, 1045334726, 1057539158, 1069698663, 
	1081812726, 1093880831, 1105902466, 1117877120, 1129804285, 1141683453, 1153514121, 1165295785, 
	1177027945, 1188710102, 1200341761, 1211922427, 1223451608, 1234928815, 1246353559, 1257725356, 
	1269043723, 1280308178, 1291518243, 1302673442, 1313773300, 1324817348, 1335805114, 1346736133, 
	1357609940, 1368426073, 1379184073, 1389883482, 1400523846, 1411104714, 1421625635, 1432086162, 
	1442485852, 1452824262, 1463100954, 1473315490, 1483467437, 1493556363, 1503581841, 1513543443, 
	1523440747, 1533273332, 1543040781, 1552742678, 1562378612, 1571948173, 1581450955, 1590886554, 
	1600254569, 1609554602, 1618786258, 1627949145, 1637042873, 1646067057, 1655021312, 1663905259, 
	1672718521, 1681460722, 1690131491, 1698730460, 1707257264, 1715711540, 1724092930, 1732401077, 
	1740635628, 1748796234, 1756882547, 1764894225, 1772830926, 1780692314, 1788478055, 1796187818, 
	1803821276, 1811378104, 1818857981, 1826260589, 1833585614, 1840832745, 1848001674, 1855092097, 
	1862103711, 1869036220, 1875889328, 1882662745, 1889356183, 1895969358, 1902501988, 1908953796, 
	1915324508, 1921613853, 1927821565, 1933947379, 1939991035, 1945952277, 1951830851, 1957626507, 
	1963339000, 1968968086, 1974513527, 1979975086, 1985352533, 1990645637, 1995854176, 2000977926, 
	2006016672, 2010970198, 2015838294, 2020620753, 2025317372, 2029927952, 2034452297, 2038890215, 
	2043241516, 2047506017, 2051683535, 2055773894, 2059776920, 2063692443, 2067520296, 2071260317, 
	2074912347, 2078476230, 2081951816, 2085338956, 2088637508, 2091847329, 2094968286, 2098000244, 
	2100943075, 2103796653, 2106560859, 2109235574, 2111820684, 2114316080, 2116721656, 2119037310, 
	2121262942, 2123398460, 2125443771, 2127398790, 2129263432, 2131037620, 2132721276, 2134314331, 
	2135816716, 2137228367, 2138549225, 2139779233, 2140918339, 2141966495, 2142923656, 2143789781, 
	2144564834, 2145248782, 2145841596, 2146343250, 2146753723, 2147072999, 2147301062, 2147437904, 
	};

TEST_GROUP_RUNNER(ns_priv_apply_window){
    RUN_TEST_CASE(ns_priv_apply_window, case0);
}

TEST_GROUP(ns_priv_apply_window);
TEST_SETUP(ns_priv_apply_window) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_apply_window) {}

TEST(ns_priv_apply_window, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    int32_t second_half[NS_WINDOW_LENGTH /2];

    int32_t frame_int[NS_PROC_FRAME_LENGTH];
    float_s32_t frame_fl;

    bfp_s32_t window;
    bfp_s32_t rev_window;

    double expected[NS_PROC_FRAME_LENGTH];
    double actual;

    float_s32_t t;


    for(int v = 0; v < (NS_WINDOW_LENGTH / 2); v++){
        second_half[v] = half_hanning[(NS_WINDOW_LENGTH / 2) - v];
    }
    bfp_s32_init(&window, half_hanning, EXP, NS_WINDOW_LENGTH / 2, 0);
    bfp_s32_init(&rev_window, second_half, EXP, NS_WINDOW_LENGTH / 2, 0);

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < NS_PROC_FRAME_LENGTH; v++){
            frame_int[v] = pseudo_rand_int(&seed, INT_MIN, INT_MAX);
            frame_fl.mant = frame_int[v];
            frame_fl.exp = EXP;
            expected[v] = float_s32_to_double(frame_fl);
        }

        for(int v = 0; v < (NS_WINDOW_LENGTH / 2); v++){
            t.mant = half_hanning[v];
            t.exp = EXP;
            expected[v] *= float_s32_to_double(t);

            t.mant = second_half[v];
            t.exp = EXP;
            expected[v + (NS_WINDOW_LENGTH / 2)] *= float_s32_to_double(t);
        }

        for(int v = 0; v < (NS_PROC_FRAME_LENGTH - NS_WINDOW_LENGTH); v++){
            expected[v + NS_WINDOW_LENGTH] = 0;
        }

        bfp_s32_t tmp;
        bfp_s32_init(&tmp, frame_int, EXP, NS_PROC_FRAME_LENGTH, 1);
        ns_priv_apply_window(&tmp, &window, &rev_window, NS_PROC_FRAME_LENGTH, NS_WINDOW_LENGTH);

        double abs_diff = 0;
        int id = 0;

        for (int v = 0; v < NS_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = tmp.data[v];
            act_fl.exp = tmp.exp;
            actual = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/(expected[id] + ldexp(1, -40)));
        double thresh = ldexp(1, -26);
        TEST_ASSERT(rel_error < thresh);
    }
}