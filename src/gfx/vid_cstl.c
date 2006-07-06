/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/* DEFAULT FILTER COEFFICIENTS */

long DurangoVideoFilter[][2] = {
    {0x10000000, 0x00000000},          /*     0, 4096,    0,    0 */
    {0x10008008, 0x00000008},          /*    -8, 4096,    8,    0 */
    {0x10008010, 0x80010011},          /*   -16, 4096,   17,   -1 */
    {0x10008019, 0x8001001A},          /*   -25, 4096,   26,   -1 */
    {0x10008021, 0x80020023},          /*   -33, 4096,   35,   -2 */
    {0x0FFF8029, 0x8003002D},          /*   -41, 4095,   45,   -3 */
    {0x0FFE8031, 0x80030036},          /*   -49, 4094,   54,   -3 */
    {0x0FFC8038, 0x80040040},          /*   -56, 4092,   64,   -4 */
    {0x0FFB8040, 0x8005004A},          /*   -64, 4091,   74,   -5 */
    {0x0FF88047, 0x80050054},          /*   -71, 4088,   84,   -5 */
    {0x0FF6804E, 0x8006005E},          /*   -78, 4086,   94,   -6 */
    {0x0FF48055, 0x80070068},          /*   -85, 4084,  104,   -7 */
    {0x0FF0805C, 0x80070073},          /*   -92, 4080,  115,   -7 */
    {0x0FEE8063, 0x8008007D},          /*   -99, 4078,  125,   -8 */
    {0x0FEA8069, 0x80090088},          /*  -105, 4074,  136,   -9 */
    {0x0FE78070, 0x800A0093},          /*  -112, 4071,  147,  -10 */
    {0x0FE28076, 0x800A009E},          /*  -118, 4066,  158,  -10 */
    {0x0FDD807C, 0x800B00AA},          /*  -124, 4061,  170,  -11 */
    {0x0FD98082, 0x800C00B5},          /*  -130, 4057,  181,  -12 */
    {0x0FD48088, 0x800D00C1},          /*  -136, 4052,  193,  -13 */
    {0x0FCE808E, 0x800D00CD},          /*  -142, 4046,  205,  -13 */
    {0x0FC88093, 0x800E00D9},          /*  -147, 4040,  217,  -14 */
    {0x0FC38099, 0x800F00E5},          /*  -153, 4035,  229,  -15 */
    {0x0FBD809E, 0x801000F1},          /*  -158, 4029,  241,  -16 */
    {0x0FB680A3, 0x801000FD},          /*  -163, 4022,  253,  -16 */
    {0x0FAF80A8, 0x8011010A},          /*  -168, 4015,  266,  -17 */
    {0x0FA880AD, 0x80120117},          /*  -173, 4008,  279,  -18 */
    {0x0FA180B2, 0x80130124},          /*  -178, 4001,  292,  -19 */
    {0x0F9980B6, 0x80140131},          /*  -182, 3993,  305,  -20 */
    {0x0F9280BB, 0x8015013E},          /*  -187, 3986,  318,  -21 */
    {0x0F8880BF, 0x8015014C},          /*  -191, 3976,  332,  -21 */
    {0x0F8080C3, 0x80160159},          /*  -195, 3968,  345,  -22 */
    {0x0F7880C8, 0x80170167},          /*  -200, 3960,  359,  -23 */
    {0x0F6E80CB, 0x80180175},          /*  -203, 3950,  373,  -24 */
    {0x0F6580CF, 0x80190183},          /*  -207, 3941,  387,  -25 */
    {0x0F5C80D3, 0x801A0191},          /*  -211, 3932,  401,  -26 */
    {0x0F5280D7, 0x801B01A0},          /*  -215, 3922,  416,  -27 */
    {0x0F4880DA, 0x801C01AE},          /*  -218, 3912,  430,  -28 */
    {0x0F3D80DD, 0x801D01BD},          /*  -221, 3901,  445,  -29 */
    {0x0F3280E0, 0x801E01CC},          /*  -224, 3890,  460,  -30 */
    {0x0F2880E4, 0x801F01DB},          /*  -228, 3880,  475,  -31 */
    {0x0F1C80E6, 0x802001EA},          /*  -230, 3868,  490,  -32 */
    {0x0F1180E9, 0x802101F9},          /*  -233, 3857,  505,  -33 */
    {0x0F0480EB, 0x80210208},          /*  -235, 3844,  520,  -33 */
    {0x0EFA80EF, 0x80230218},          /*  -239, 3834,  536,  -35 */
    {0x0EEC80F0, 0x80230227},          /*  -240, 3820,  551,  -35 */
    {0x0EE080F3, 0x80240237},          /*  -243, 3808,  567,  -36 */
    {0x0ED380F5, 0x80250247},          /*  -245, 3795,  583,  -37 */
    {0x0EC780F7, 0x80270257},          /*  -247, 3783,  599,  -39 */
    {0x0EB980F9, 0x80280268},          /*  -249, 3769,  616,  -40 */
    {0x0EAC80FB, 0x80290278},          /*  -251, 3756,  632,  -41 */
    {0x0E9E80FD, 0x802A0289},          /*  -253, 3742,  649,  -42 */
    {0x0E9080FE, 0x802B0299},          /*  -254, 3728,  665,  -43 */
    {0x0E838100, 0x802D02AA},          /*  -256, 3715,  682,  -45 */
    {0x0E758102, 0x802E02BB},          /*  -258, 3701,  699,  -46 */
    {0x0E668103, 0x802F02CC},          /*  -259, 3686,  716,  -47 */
    {0x0E568104, 0x803002DE},          /*  -260, 3670,  734,  -48 */
    {0x0E498106, 0x803202EF},          /*  -262, 3657,  751,  -50 */
    {0x0E398107, 0x80330301},          /*  -263, 3641,  769,  -51 */
    {0x0E298108, 0x80340313},          /*  -264, 3625,  787,  -52 */
    {0x0E1A8109, 0x80360325},          /*  -265, 3610,  805,  -54 */
    {0x0E0B810A, 0x80370336},          /*  -266, 3595,  822,  -55 */
    {0x0DFA810A, 0x80380348},          /*  -266, 3578,  840,  -56 */
    {0x0DEA810B, 0x803A035B},          /*  -267, 3562,  859,  -58 */
    {0x0DDA810C, 0x803B036D},          /*  -268, 3546,  877,  -59 */
    {0x0DCA810C, 0x803D037F},          /*  -268, 3530,  895,  -61 */
    {0x0DB7810B, 0x803E0392},          /*  -267, 3511,  914,  -62 */
    {0x0DA7810C, 0x804003A5},          /*  -268, 3495,  933,  -64 */
    {0x0D95810C, 0x804103B8},          /*  -268, 3477,  952,  -65 */
    {0x0D85810C, 0x804303CA},          /*  -268, 3461,  970,  -67 */
    {0x0D73810C, 0x804403DD},          /*  -268, 3443,  989,  -68 */
    {0x0D61810C, 0x804603F1},          /*  -268, 3425, 1009,  -70 */
    {0x0D50810C, 0x80480404},          /*  -268, 3408, 1028,  -72 */
    {0x0D3E810C, 0x80490417},          /*  -268, 3390, 1047,  -73 */
    {0x0D2C810C, 0x804B042B},          /*  -268, 3372, 1067,  -75 */
    {0x0D1B810C, 0x804D043E},          /*  -268, 3355, 1086,  -77 */
    {0x0D07810B, 0x804E0452},          /*  -267, 3335, 1106,  -78 */
    {0x0CF5810B, 0x80500466},          /*  -267, 3317, 1126,  -80 */
    {0x0CE2810A, 0x8052047A},          /*  -266, 3298, 1146,  -82 */
    {0x0CCF810A, 0x8053048E},          /*  -266, 3279, 1166,  -83 */
    {0x0CBC8109, 0x805504A2},          /*  -265, 3260, 1186,  -85 */
    {0x0CA98108, 0x805704B6},          /*  -264, 3241, 1206,  -87 */
    {0x0C968108, 0x805904CB},          /*  -264, 3222, 1227,  -89 */
    {0x0C838107, 0x805B04DF},          /*  -263, 3203, 1247,  -91 */
    {0x0C6F8106, 0x805C04F3},          /*  -262, 3183, 1267,  -92 */
    {0x0C5B8105, 0x805E0508},          /*  -261, 3163, 1288,  -94 */
    {0x0C478104, 0x8060051D},          /*  -260, 3143, 1309,  -96 */
    {0x0C348103, 0x80620531},          /*  -259, 3124, 1329,  -98 */
    {0x0C1F8102, 0x80640547},          /*  -258, 3103, 1351, -100 */
    {0x0C0C8101, 0x8066055B},          /*  -257, 3084, 1371, -102 */
    {0x0BF88100, 0x80680570},          /*  -256, 3064, 1392, -104 */
    {0x0BE380FE, 0x806A0585},          /*  -254, 3043, 1413, -106 */
    {0x0BCF80FD, 0x806C059A},          /*  -253, 3023, 1434, -108 */
    {0x0BBA80FC, 0x806E05B0},          /*  -252, 3002, 1456, -110 */
    {0x0BA480F9, 0x807005C5},          /*  -249, 2980, 1477, -112 */
    {0x0B8F80F8, 0x807205DB},          /*  -248, 2959, 1499, -114 */
    {0x0B7A80F6, 0x807405F0},          /*  -246, 2938, 1520, -116 */
    {0x0B6580F5, 0x80760606},          /*  -245, 2917, 1542, -118 */
    {0x0B4F80F3, 0x8077061B},          /*  -243, 2895, 1563, -119 */
    {0x0B3A80F2, 0x80790631},          /*  -242, 2874, 1585, -121 */
    {0x0B2480F0, 0x807B0647},          /*  -240, 2852, 1607, -123 */
    {0x0B0F80EE, 0x807D065C},          /*  -238, 2831, 1628, -125 */
    {0x0AF980ED, 0x807F0673},          /*  -237, 2809, 1651, -127 */
    {0x0AE480EB, 0x80810688},          /*  -235, 2788, 1672, -129 */
    {0x0ACE80E9, 0x8084069F},          /*  -233, 2766, 1695, -132 */
    {0x0AB980E7, 0x808606B4},          /*  -231, 2745, 1716, -134 */
    {0x0AA380E6, 0x808806CB},          /*  -230, 2723, 1739, -136 */
    {0x0A8D80E4, 0x808A06E1},          /*  -228, 2701, 1761, -138 */
    {0x0A7780E2, 0x808C06F7},          /*  -226, 2679, 1783, -140 */
    {0x0A6180E0, 0x808E070D},          /*  -224, 2657, 1805, -142 */
    {0x0A4B80DE, 0x80910724},          /*  -222, 2635, 1828, -145 */
    {0x0A3580DC, 0x8093073A},          /*  -220, 2613, 1850, -147 */
    {0x0A1F80DA, 0x80950750},          /*  -218, 2591, 1872, -149 */
    {0x0A0880D8, 0x80970767},          /*  -216, 2568, 1895, -151 */
    {0x09F280D6, 0x8099077D},          /*  -214, 2546, 1917, -153 */
    {0x09DD80D4, 0x809C0793},          /*  -212, 2525, 1939, -156 */
    {0x09C680D2, 0x809E07AA},          /*  -210, 2502, 1962, -158 */
    {0x09B080D0, 0x80A007C0},          /*  -208, 2480, 1984, -160 */
    {0x099980CE, 0x80A207D7},          /*  -206, 2457, 2007, -162 */
    {0x098380CB, 0x80A507ED},          /*  -203, 2435, 2029, -165 */
    {0x096C80C9, 0x80A70804},          /*  -201, 2412, 2052, -167 */
    {0x095680C7, 0x80A9081A},          /*  -199, 2390, 2074, -169 */
    {0x094080C5, 0x80AB0830},          /*  -197, 2368, 2096, -171 */
    {0x092980C3, 0x80AE0848},          /*  -195, 2345, 2120, -174 */
    {0x091380C1, 0x80B0085E},          /*  -193, 2323, 2142, -176 */
    {0x08FC80BE, 0x80B20874},          /*  -190, 2300, 2164, -178 */
    {0x08E580BC, 0x80B4088B},          /*  -188, 2277, 2187, -180 */
    {0x08D080BB, 0x80B708A2},          /*  -187, 2256, 2210, -183 */
    {0x08B980B9, 0x80B908B9},          /*  -185, 2233, 2233, -185 */
    {0x08A380B7, 0x80BB08CF},          /*  -183, 2211, 2255, -187 */
    {0x088B80B4, 0x80BC08E5},          /*  -180, 2187, 2277, -188 */
    {0x087480B2, 0x80BE08FC},          /*  -178, 2164, 2300, -190 */
    {0x085E80B0, 0x80C10913},          /*  -176, 2142, 2323, -193 */
    {0x084880AE, 0x80C30929},          /*  -174, 2120, 2345, -195 */
    {0x083080AB, 0x80C50940},          /*  -171, 2096, 2368, -197 */
    {0x081A80A9, 0x80C70956},          /*  -169, 2074, 2390, -199 */
    {0x080480A7, 0x80C9096C},          /*  -167, 2052, 2412, -201 */
    {0x07ED80A5, 0x80CB0983},          /*  -165, 2029, 2435, -203 */
    {0x07D780A2, 0x80CE0999},          /*  -162, 2007, 2457, -206 */
    {0x07C080A0, 0x80D009B0},          /*  -160, 1984, 2480, -208 */
    {0x07AA809E, 0x80D209C6},          /*  -158, 1962, 2502, -210 */
    {0x0793809C, 0x80D409DD},          /*  -156, 1939, 2525, -212 */
    {0x077D8099, 0x80D609F2},          /*  -153, 1917, 2546, -214 */
    {0x07668097, 0x80D80A09},          /*  -151, 1894, 2569, -216 */
    {0x074F8095, 0x80DA0A20},          /*  -149, 1871, 2592, -218 */
    {0x073A8093, 0x80DC0A35},          /*  -147, 1850, 2613, -220 */
    {0x07238091, 0x80DE0A4C},          /*  -145, 1827, 2636, -222 */
    {0x070C808E, 0x80E00A62},          /*  -142, 1804, 2658, -224 */
    {0x06F7808C, 0x80E20A77},          /*  -140, 1783, 2679, -226 */
    {0x06E0808A, 0x80E40A8E},          /*  -138, 1760, 2702, -228 */
    {0x06CA8088, 0x80E60AA4},          /*  -136, 1738, 2724, -230 */
    {0x06B48086, 0x80E70AB9},          /*  -134, 1716, 2745, -231 */
    {0x069E8084, 0x80E90ACF},          /*  -132, 1694, 2767, -233 */
    {0x06878081, 0x80EB0AE5},          /*  -129, 1671, 2789, -235 */
    {0x0672807F, 0x80ED0AFA},          /*  -127, 1650, 2810, -237 */
    {0x065C807D, 0x80EE0B0F},          /*  -125, 1628, 2831, -238 */
    {0x0646807B, 0x80F00B25},          /*  -123, 1606, 2853, -240 */
    {0x06308079, 0x80F20B3B},          /*  -121, 1584, 2875, -242 */
    {0x061A8077, 0x80F30B50},          /*  -119, 1562, 2896, -243 */
    {0x06068076, 0x80F50B65},          /*  -118, 1542, 2917, -245 */
    {0x05F08074, 0x80F60B7A},          /*  -116, 1520, 2938, -246 */
    {0x05DB8072, 0x80F80B8F},          /*  -114, 1499, 2959, -248 */
    {0x05C58070, 0x80F90BA4},          /*  -112, 1477, 2980, -249 */
    {0x05B1806E, 0x80FC0BB9},          /*  -110, 1457, 3001, -252 */
    {0x059B806C, 0x80FD0BCE},          /*  -108, 1435, 3022, -253 */
    {0x0586806A, 0x80FE0BE2},          /*  -106, 1414, 3042, -254 */
    {0x05718068, 0x81000BF7},          /*  -104, 1393, 3063, -256 */
    {0x055C8066, 0x81010C0B},          /*  -102, 1372, 3083, -257 */
    {0x05478064, 0x81020C1F},          /*  -100, 1351, 3103, -258 */
    {0x05328062, 0x81030C33},          /*   -98, 1330, 3123, -259 */
    {0x051D8060, 0x81040C47},          /*   -96, 1309, 3143, -260 */
    {0x0508805E, 0x81050C5B},          /*   -94, 1288, 3163, -261 */
    {0x04F3805C, 0x81060C6F},          /*   -92, 1267, 3183, -262 */
    {0x04E0805B, 0x81070C82},          /*   -91, 1248, 3202, -263 */
    {0x04CB8059, 0x81080C96},          /*   -89, 1227, 3222, -264 */
    {0x04B68057, 0x81080CA9},          /*   -87, 1206, 3241, -264 */
    {0x04A28055, 0x81090CBC},          /*   -85, 1186, 3260, -265 */
    {0x048E8053, 0x810A0CCF},          /*   -83, 1166, 3279, -266 */
    {0x047A8052, 0x810A0CE2},          /*   -82, 1146, 3298, -266 */
    {0x04668050, 0x810B0CF5},          /*   -80, 1126, 3317, -267 */
    {0x0451804E, 0x810B0D08},          /*   -78, 1105, 3336, -267 */
    {0x043E804D, 0x810C0D1B},          /*   -77, 1086, 3355, -268 */
    {0x042B804B, 0x810C0D2C},          /*   -75, 1067, 3372, -268 */
    {0x04178049, 0x810C0D3E},          /*   -73, 1047, 3390, -268 */
    {0x04038048, 0x810C0D51},          /*   -72, 1027, 3409, -268 */
    {0x03F08046, 0x810C0D62},          /*   -70, 1008, 3426, -268 */
    {0x03DD8044, 0x810C0D73},          /*   -68,  989, 3443, -268 */
    {0x03CA8043, 0x810C0D85},          /*   -67,  970, 3461, -268 */
    {0x03B78041, 0x810C0D96},          /*   -65,  951, 3478, -268 */
    {0x03A48040, 0x810C0DA8},          /*   -64,  932, 3496, -268 */
    {0x0391803E, 0x810B0DB8},          /*   -62,  913, 3512, -267 */
    {0x0380803D, 0x810C0DC9},          /*   -61,  896, 3529, -268 */
    {0x036D803B, 0x810C0DDA},          /*   -59,  877, 3546, -268 */
    {0x035B803A, 0x810B0DEA},          /*   -58,  859, 3562, -267 */
    {0x03488038, 0x810A0DFA},          /*   -56,  840, 3578, -266 */
    {0x03368037, 0x810A0E0B},          /*   -55,  822, 3595, -266 */
    {0x03248036, 0x81090E1B},          /*   -54,  804, 3611, -265 */
    {0x03128034, 0x81080E2A},          /*   -52,  786, 3626, -264 */
    {0x03018033, 0x81070E39},          /*   -51,  769, 3641, -263 */
    {0x02EF8032, 0x81060E49},          /*   -50,  751, 3657, -262 */
    {0x02DE8030, 0x81040E56},          /*   -48,  734, 3670, -260 */
    {0x02CC802F, 0x81030E66},          /*   -47,  716, 3686, -259 */
    {0x02BB802E, 0x81020E75},          /*   -46,  699, 3701, -258 */
    {0x02AA802D, 0x81000E83},          /*   -45,  682, 3715, -256 */
    {0x0299802B, 0x80FE0E90},          /*   -43,  665, 3728, -254 */
    {0x0288802A, 0x80FD0E9F},          /*   -42,  648, 3743, -253 */
    {0x02778029, 0x80FB0EAD},          /*   -41,  631, 3757, -251 */
    {0x02678028, 0x80F90EBA},          /*   -40,  615, 3770, -249 */
    {0x02568027, 0x80F70EC8},          /*   -39,  598, 3784, -247 */
    {0x02468025, 0x80F50ED4},          /*   -37,  582, 3796, -245 */
    {0x02368024, 0x80F30EE1},          /*   -36,  566, 3809, -243 */
    {0x02268023, 0x80F00EED},          /*   -35,  550, 3821, -240 */
    {0x02188023, 0x80EF0EFA},          /*   -35,  536, 3834, -239 */
    {0x02078021, 0x80EB0F05},          /*   -33,  519, 3845, -235 */
    {0x01F98021, 0x80E90F11},          /*   -33,  505, 3857, -233 */
    {0x01EA8020, 0x80E60F1C},          /*   -32,  490, 3868, -230 */
    {0x01DC801F, 0x80E40F27},          /*   -31,  476, 3879, -228 */
    {0x01CD801E, 0x80E00F31},          /*   -30,  461, 3889, -224 */
    {0x01BE801D, 0x80DD0F3C},          /*   -29,  446, 3900, -221 */
    {0x01AF801C, 0x80DA0F47},          /*   -28,  431, 3911, -218 */
    {0x01A1801B, 0x80D70F51},          /*   -27,  417, 3921, -215 */
    {0x0192801A, 0x80D30F5B},          /*   -26,  402, 3931, -211 */
    {0x01848019, 0x80CF0F64},          /*   -25,  388, 3940, -207 */
    {0x01768018, 0x80CB0F6D},          /*   -24,  374, 3949, -203 */
    {0x01688017, 0x80C80F77},          /*   -23,  360, 3959, -200 */
    {0x015A8016, 0x80C30F7F},          /*   -22,  346, 3967, -195 */
    {0x014D8015, 0x80BF0F87},          /*   -21,  333, 3975, -191 */
    {0x013F8015, 0x80BB0F91},          /*   -21,  319, 3985, -187 */
    {0x01328014, 0x80B60F98},          /*   -20,  306, 3992, -182 */
    {0x01258013, 0x80B20FA0},          /*   -19,  293, 4000, -178 */
    {0x01188012, 0x80AD0FA7},          /*   -18,  280, 4007, -173 */
    {0x010B8011, 0x80A80FAE},          /*   -17,  267, 4014, -168 */
    {0x00FE8010, 0x80A30FB5},          /*   -16,  254, 4021, -163 */
    {0x00F28010, 0x809E0FBC},          /*   -16,  242, 4028, -158 */
    {0x00E6800F, 0x80990FC2},          /*   -15,  230, 4034, -153 */
    {0x00DA800E, 0x80930FC7},          /*   -14,  218, 4039, -147 */
    {0x00CE800D, 0x808E0FCD},          /*   -13,  206, 4045, -142 */
    {0x00C2800D, 0x80880FD3},          /*   -13,  194, 4051, -136 */
    {0x00B6800C, 0x80820FD8},          /*   -12,  182, 4056, -130 */
    {0x00AB800B, 0x807C0FDC},          /*   -11,  171, 4060, -124 */
    {0x009F800A, 0x80760FE1},          /*   -10,  159, 4065, -118 */
    {0x0094800A, 0x80700FE6},          /*   -10,  148, 4070, -112 */
    {0x00898009, 0x80690FE9},          /*    -9,  137, 4073, -105 */
    {0x007E8008, 0x80630FED},          /*    -8,  126, 4077,  -99 */
    {0x00748007, 0x805C0FEF},          /*    -7,  116, 4079,  -92 */
    {0x00698007, 0x80550FF3},          /*    -7,  105, 4083,  -85 */
    {0x005F8006, 0x804E0FF5},          /*    -6,   95, 4085,  -78 */
    {0x00558005, 0x80470FF7},          /*    -5,   85, 4087,  -71 */
    {0x004B8005, 0x80400FFA},          /*    -5,   75, 4090,  -64 */
    {0x00418004, 0x80380FFB},          /*    -4,   65, 4091,  -56 */
    {0x00378003, 0x80310FFD},          /*    -3,   55, 4093,  -49 */
    {0x002E8003, 0x80290FFE},          /*    -3,   46, 4094,  -41 */
    {0x00238002, 0x80211000},          /*    -2,   35, 4096,  -33 */
    {0x001A8001, 0x80191000},          /*    -1,   26, 4096,  -25 */
    {0x00118001, 0x80101000},          /*    -1,   17, 4096,  -16 */
    {0x00080000, 0x80081000},          /*     0,    8, 4096,   -8 */
};

/* CASTLE PLL TABLE  */

typedef struct tagCASTLEPLL
{
    long frequency;                    /* 16.16 fixed point frequency            */
    unsigned long pll_value;           /* MCP DotPLL Register Upper 32(0x0015)   */
} CASTLEPLLENTRY;

CASTLEPLLENTRY CASTLE_PLLtable[] = {
    {0x00192CCC, 0x0000215D},          /*  25.1750 */
    {0x001C526E, 0x0000216C},          /*  28.3220 */
    {0x001C8F5C, 0x0000218D},          /*  28.5600 */
    {0x001F8000, 0x00003147},          /*  31.5000 */
    {0x00240000, 0x00000057},          /*  36.0000 */
    {0x0025E395, 0x0000219A},          /*  37.8890 */
    {0x00280000, 0x00000045},          /*  40.0000 */
    {0x002B29BA, 0x00000089},          /*  43.1630 */
    {0x002CE666, 0x000010E7},          /*  44.9000 */
    {0x002DB851, 0x00002136},          /*  45.7200 */
    {0x00318000, 0x00003207},          /*  49.5000 */
    {0x00320000, 0x00002187},          /*  50.0000 */
    {0x00384000, 0x00004286},          /*  56.2500 */
    {0x003C10A3, 0x000010E5},          /*  60.0650 */
    {0x00410000, 0x00004214},          /*  65.0000 */
    {0x00442DD2, 0x00001105},          /*  68.1790 */
    {0x004B0000, 0x00003183},          /*  75.0000 */
    {0x004EC000, 0x00004284},          /*  78.7500 */
    {0x00519999, 0x00001104},          /*  81.6000 */
    {0x005E8000, 0x00006363},          /*  94.5000 */
    {0x00618560, 0x00005303},          /*  97.5200 */
    {0x00630000, 0x00002523},          /*  99.0000 */
    {0x00640000, 0x000000D1},          /* 100.0000 */
    {0x00642FDF, 0x00002183},          /* 100.1870 */
    {0x00656B85, 0x00002122},          /* 101.4200 */
    {0x006C0000, 0x00001081},          /* 108.0000 */
    {0x00708000, 0x000013E3},          /* 112.5000 */
    {0x00714F1A, 0x00006201},          /* 113.3090 */
    {0x0077A666, 0x00000041},          /* 119.6500 */
    {0x00806666, 0x00000072},          /* 128.4000 */
    {0x00821999, 0x00006382},          /* 130.1000 */
    {0x00858000, 0x00002182},          /* 133.5000 */
    {0x00870000, 0x000041B1},          /* 135.0000 */
    {0x00906147, 0x00000051},          /* 144.3800 */
    {0x009D8000, 0x000062D1},          /* 157.5000 */
    {0x00A20000, 0x000031A1},          /* 162.0000 */
    {0x00A933F7, 0x00000061},          /* 169.2030 */
    {0x00AF8000, 0x00002151},          /* 175.5000 */
    {0x00BD0000, 0x000052E1},          /* 189.0000 */
    {0x00BEF5C2, 0x00000071},          /* 190.9600 */
    {0x00C60000, 0x00003201},          /* 198.0000 */
    {0x00CA8000, 0x00004291},          /* 202.5000 */
    {0x00E58000, 0x00004170},          /* 229.5000 */
};

#define NUM_CASTLE_FREQUENCIES sizeof(CASTLE_PLLtable)/sizeof(CASTLEPLLENTRY)

/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_reset_video(void)
#else
void
gfx_reset_video(void)
#endif
{
    gfx_set_video_enable(0);
    gfx_select_alpha_region(1);
    gfx_set_alpha_enable(0);
    gfx_select_alpha_region(2);
    gfx_set_alpha_enable(0);

    /* SET REGION 0 AFTER RESET */

    gfx_select_alpha_region(0);
    gfx_set_alpha_enable(0);
}

/*----------------------------------------------------------------------------
 * gfx_set_display_control (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine configures the display output.
 *
 * "sync_polarities" is used to set the polarities of the sync pulses 
 * according to the following mask:
 *
 *     Bit 0: If set to 1, negative horizontal polarity is programmed,
 *            otherwise positive horizontal polarity is programmed.
 *     Bit 1: If set to 1, negative vertical polarity is programmed,
 *            otherwise positive vertical polarity is programmed.
 *
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_display_control(int sync_polarities)
#else
int
gfx_set_display_control(int sync_polarities)
#endif
{
    unsigned long power;
    unsigned long dcfg;

    /* CONFIGURE DISPLAY OUTPUT FROM VIDEO PROCESSOR */

    dcfg = READ_VID32(CASTLE_DISPLAY_CONFIG);
    dcfg &= ~(CASTLE_DCFG_CRT_SYNC_SKW_MASK | CASTLE_DCFG_PWR_SEQ_DLY_MASK |
        CASTLE_DCFG_CRT_HSYNC_POL | CASTLE_DCFG_CRT_VSYNC_POL);

    dcfg |= (CASTLE_DCFG_CRT_SYNC_SKW_INIT |
        CASTLE_DCFG_PWR_SEQ_DLY_INIT | CASTLE_DCFG_GV_PAL_BYP);

    if (PanelEnable) {
        power = READ_VID32(CASTLE_POWER_MANAGEMENT);
        power |= CASTLE_PM_PANEL_POWER_ON;
        WRITE_VID32(CASTLE_POWER_MANAGEMENT, power);
    }

    /* SET APPROPRIATE SYNC POLARITIES */

    if (sync_polarities & 0x1)
        dcfg |= CASTLE_DCFG_CRT_HSYNC_POL;
    if (sync_polarities & 0x2)
        dcfg |= CASTLE_DCFG_CRT_VSYNC_POL;

    WRITE_VID32(CASTLE_DISPLAY_CONFIG, dcfg);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_clock_frequency
 *
 * This routine sets the clock frequency, specified as a 16.16 fixed point
 * value (0x00318000 = 49.5 MHz).  It will set the closest frequency found
 * in the lookup table.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_set_clock_frequency(unsigned long frequency)
#else
void
gfx_set_clock_frequency(unsigned long frequency)
#endif
{
    Q_WORD msr_value;
    unsigned int i, index = 0;
    unsigned long value;
    unsigned long unlock;
    long timeout;
    long min, diff;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
    /* Search the table for the closest frequency (16.16 format). */

    value = CASTLE_PLLtable[0].pll_value;
    min = (long)CASTLE_PLLtable[0].frequency - frequency;
    if (min < 0L)
        min = -min;
    for (i = 1; i < NUM_CASTLE_FREQUENCIES; i++) {
        diff = (long)CASTLE_PLLtable[i].frequency - frequency;
        if (diff < 0L)
            diff = -diff;
        if (diff < min) {
            min = diff;
            index = i;
        }
    }

    /* VERIFY THAT WE ARE NOT WRITING WHAT IS ALREADY IN THE REGISTERS */
    /* The Dot PLL reset bit is tied to VDD for flat panels.  This can */
    /* cause a brief drop in flat panel power, which can cause serious */
    /* glitches on some panels.                                        */

    gfx_msr_read(RC_ID_MCP, MCP_DOTPLL, &msr_value);

    if ((msr_value.low & MCP_DOTPLL_LOCK) &&
        ((msr_value.low & MCP_DOTPLL_HALFPIX) == 0) &&
        (msr_value.high == CASTLE_PLLtable[index].pll_value)) {
        return;
    }

    /* PROGRAM THE SETTINGS WITH THE RESET BIT SET */
    /* Clear the bypass bit to ensure that the programmed */
    /* M, N and P values are being used.                  */

    msr_value.high = CASTLE_PLLtable[index].pll_value;
    msr_value.low |= 0x00000001;
    msr_value.low &= ~MCP_DOTPLL_BYPASS;
    gfx_msr_write(RC_ID_MCP, MCP_DOTPLL, &msr_value);

    /* WAIT FOR THE LOCK BIT */
    /* The official word from LEDA is that the PLL may take up to 100 */
    /* us to lock and the lock bit is unreliable.  We thus add a long */
    /* delay followed by a timed-out poll                             */

    unlock = READ_REG32(DC3_UNLOCK);
    for (timeout = 0; timeout < 1280; timeout++)
        WRITE_REG32(DC3_UNLOCK, unlock);

    for (timeout = 0; timeout < 1000; timeout++) {
        gfx_msr_read(RC_ID_MCP, MCP_DOTPLL, &msr_value);
        if (msr_value.low & MCP_DOTPLL_LOCK)
            break;
    }

    /* CLEAR THE RESET BIT */

    msr_value.low &= 0xFFFFFFFE;
    gfx_msr_write(RC_ID_MCP, MCP_DOTPLL, &msr_value);
}

/*---------------------------------------------------------------------------
 * gfx_set_crt_enable
 * 
 * This routine enables or disables the CRT output from the video processor.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_crt_enable(int enable)
#else
int
gfx_set_crt_enable(int enable)
#endif
{
    unsigned long config, misc;

    config = READ_VID32(CASTLE_DISPLAY_CONFIG);
    misc = READ_VID32(CASTLE_VID_MISC);

    switch (enable) {
    case CRT_DISABLE:                 /* DISABLE EVERYTHING */

        WRITE_VID32(CASTLE_DISPLAY_CONFIG,
            config & ~(CASTLE_DCFG_DIS_EN | CASTLE_DCFG_HSYNC_EN |
                CASTLE_DCFG_VSYNC_EN | CASTLE_DCFG_DAC_BL_EN));
        WRITE_VID32(CASTLE_VID_MISC, misc | CASTLE_DAC_POWER_DOWN);
        break;

    case CRT_ENABLE:                  /* ENABLE CRT DISPLAY, INCLUDING DISPLAY LOGIC */

        WRITE_VID32(CASTLE_DISPLAY_CONFIG,
            config | CASTLE_DCFG_DIS_EN | CASTLE_DCFG_HSYNC_EN |
            CASTLE_DCFG_VSYNC_EN | CASTLE_DCFG_DAC_BL_EN);
        WRITE_VID32(CASTLE_VID_MISC,
            misc & ~CASTLE_DAC_POWER_DOWN & ~CASTLE_ANALOG_POWER_DOWN);
        break;

    case CRT_STANDBY:                 /* HSYNC:OFF VSYNC:ON */

        WRITE_VID32(CASTLE_DISPLAY_CONFIG,
            (config & ~(CASTLE_DCFG_DIS_EN | CASTLE_DCFG_HSYNC_EN |
                    CASTLE_DCFG_DAC_BL_EN)) | CASTLE_DCFG_VSYNC_EN);
        WRITE_VID32(CASTLE_VID_MISC, misc | CASTLE_DAC_POWER_DOWN);
        break;

    case CRT_SUSPEND:                 /* HSYNC:ON VSYNC:OFF */

        WRITE_VID32(CASTLE_DISPLAY_CONFIG,
            (config & ~(CASTLE_DCFG_DIS_EN | CASTLE_DCFG_VSYNC_EN |
                    CASTLE_DCFG_DAC_BL_EN)) | CASTLE_DCFG_HSYNC_EN);
        WRITE_VID32(CASTLE_VID_MISC, misc | CASTLE_DAC_POWER_DOWN);
        break;

    default:
        return (GFX_STATUS_BAD_PARAMETER);
    }
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_enable
 *
 * This routine enables or disables the video overlay functionality.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_enable(int enable)
#else
int
gfx_set_video_enable(int enable)
#endif
{
    unsigned long vcfg;

    /* WAIT FOR VERTICAL BLANK TO START */
    /* Otherwise a glitch can be observed. */

    if (gfx_test_timing_active()) {
        if (!gfx_test_vertical_active()) {
            while (!gfx_test_vertical_active()) ;
        }
        while (gfx_test_vertical_active()) ;
    }

    vcfg = READ_VID32(CASTLE_VIDEO_CONFIG);
    if (enable) {
        /* ENABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
        /* Use private routine to abstract the display controller. */

        gfx_set_display_video_enable(1);

        /* ENABLE DISPLAY FILTER VIDEO OVERLAY */

        vcfg |= CASTLE_VCFG_VID_EN;
        WRITE_VID32(CASTLE_VIDEO_CONFIG, vcfg);
    } else {
        /* DISABLE DISPLAY FILTER VIDEO OVERLAY */

        vcfg &= ~CASTLE_VCFG_VID_EN;
        WRITE_VID32(CASTLE_VIDEO_CONFIG, vcfg);

        /* DISABLE VIDEO OVERLAY FROM DISPLAY CONTROLLER */
        /* Use private routine to abstract the display controller. */

        gfx_set_display_video_enable(0);
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_format
 *
 * Sets input video format type, to one of the YUV formats or to RGB.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_format(unsigned long format)
#else
int
gfx_set_video_format(unsigned long format)
#endif
{
    unsigned long ctrl, vcfg = 0;

    /* SET THE DISPLAY FILTER VIDEO INPUT FORMAT */

    vcfg = READ_VID32(CASTLE_VIDEO_CONFIG);
    ctrl = READ_VID32(CASTLE_VID_ALPHA_CONTROL);
    ctrl &=
        ~(CASTLE_VIDEO_INPUT_IS_RGB | CASTLE_CSC_VIDEO_YUV_TO_RGB |
        CASTLE_CSC_GRAPHICS_RGB_TO_YUV);
    vcfg &= ~(CASTLE_VCFG_VID_INP_FORMAT | CASTLE_VCFG_4_2_0_MODE);
    switch (format) {
    case VIDEO_FORMAT_UYVY:
        vcfg |= CASTLE_VCFG_UYVY_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        break;
    case VIDEO_FORMAT_YUYV:
        vcfg |= CASTLE_VCFG_YUYV_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        break;
    case VIDEO_FORMAT_Y2YU:
        vcfg |= CASTLE_VCFG_Y2YU_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        break;
    case VIDEO_FORMAT_YVYU:
        vcfg |= CASTLE_VCFG_YVYU_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        break;
    case VIDEO_FORMAT_Y0Y1Y2Y3:
        vcfg |= CASTLE_VCFG_UYVY_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        vcfg |= CASTLE_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y3Y2Y1Y0:
        vcfg |= CASTLE_VCFG_Y2YU_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        vcfg |= CASTLE_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y1Y0Y3Y2:
        vcfg |= CASTLE_VCFG_YUYV_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        vcfg |= CASTLE_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y1Y2Y3Y0:
        vcfg |= CASTLE_VCFG_YVYU_FORMAT;
        ctrl |= CASTLE_CSC_VIDEO_YUV_TO_RGB;
        vcfg |= CASTLE_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_RGB:
        ctrl |= CASTLE_VIDEO_INPUT_IS_RGB;
        vcfg |= CASTLE_VCFG_UYVY_FORMAT;
        break;
    case VIDEO_FORMAT_P2M_P2L_P1M_P1L:
        ctrl |= CASTLE_VIDEO_INPUT_IS_RGB;
        vcfg |= CASTLE_VCFG_Y2YU_FORMAT;
        break;
    case VIDEO_FORMAT_P1M_P1L_P2M_P2L:
        ctrl |= CASTLE_VIDEO_INPUT_IS_RGB;
        vcfg |= CASTLE_VCFG_YUYV_FORMAT;
        break;
    case VIDEO_FORMAT_P1M_P2L_P2M_P1L:
        ctrl |= CASTLE_VIDEO_INPUT_IS_RGB;
        vcfg |= CASTLE_VCFG_YVYU_FORMAT;
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    WRITE_VID32(CASTLE_VIDEO_CONFIG, vcfg);
    WRITE_VID32(CASTLE_VID_ALPHA_CONTROL, ctrl);

    /* SET THE VIDEO FORMAT IN THE DISPLAY CONTROLLER      */
    /* Use private routine to abstract display controller. */

    gfx_set_display_video_format(format);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_size
 *
 * This routine specifies the size of the source data.  It is used only 
 * to determine how much data to transfer per frame, and is not used to 
 * calculate the scaling value (that is handled by a separate routine).
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_size(unsigned short width, unsigned short height)
#else
int
gfx_set_video_size(unsigned short width, unsigned short height)
#endif
{
    unsigned long size, vcfg, vid_420, pitch, scale;

    /* SET THE DISPLAY FILTER VIDEO LINE SIZE                            */
    /* Match the DC hardware alignment requirement.  The line size must  */
    /* always be 32-byte aligned.  However, we can manage smaller        */
    /* alignments by decreasing the pitch and clipping the video window. */
    /* The VG will fetch extra data for each line, but the decreased     */
    /* pitch will ensure that it always begins fetching at the start of  */
    /* the video line.                                                   */

    vcfg = READ_VID32(CASTLE_VIDEO_CONFIG);

    vid_420 = vcfg & CASTLE_VCFG_4_2_0_MODE;

    vcfg &=
        ~(CASTLE_VCFG_LINE_SIZE_LOWER_MASK | CASTLE_VCFG_LINE_SIZE_UPPER |
        CASTLE_VCFG_LINE_SIZE_BIT9);

    size = ((width >> 1) + 7) & 0xFFF8;
    pitch = ((width << 1) + 7) & 0xFFF8;

    vcfg |= (size & 0x00FF) << 8;
    if (size & 0x0100)
        vcfg |= CASTLE_VCFG_LINE_SIZE_UPPER;
    if (size & 0x0200)
        vcfg |= CASTLE_VCFG_LINE_SIZE_BIT9;
    WRITE_VID32(CASTLE_VIDEO_CONFIG, vcfg);

    /* WRITE THE VIDEO HEIGHT */

    scale = READ_VID32(CASTLE_VIDEO_SCALER);
    scale = (scale & ~0x7FF) | height;
    WRITE_VID32(CASTLE_VIDEO_SCALER, scale);

    /* SET VIDEO BUFFER LINE SIZE IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    gfx_set_display_video_size(width, height);

    /* SET VIDEO PITCH */
    /* We are only maintaining legacy for 4:2:2 video formats. */
    /* 4:2:0 video in previous chips was inadequate for most   */
    /* common video formats.                                   */

    if (!vid_420)
        gfx_set_video_yuv_pitch(pitch, pitch << 1);

    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_offset
 *
 * This routine sets the starting offset for the video buffer when only 
 * one offset needs to be specified.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_offset(unsigned long offset)
#else
int
gfx_set_video_offset(unsigned long offset)
#endif
{
    /* SAVE VALUE FOR FUTURE CLIPPING OF THE TOP OF THE VIDEO WINDOW */

    gfx_vid_offset = offset;

    /* SET VIDEO BUFFER OFFSET IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    gfx_set_display_video_offset(offset);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_yuv_offsets
 *
 * This routine sets the starting offset for the video buffer when displaying 
 * 4:2:0 video.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_yuv_offsets(unsigned long yoffset, unsigned long uoffset,
    unsigned long voffset)
#else
int
gfx_set_video_yuv_offsets(unsigned long yoffset, unsigned long uoffset,
    unsigned long voffset)
#endif
{
    /* SAVE VALUE FOR FUTURE CLIPPING OF THE TOP OF THE VIDEO WINDOW */

    gfx_vid_offset = yoffset;
    gfx_vid_uoffset = uoffset;
    gfx_vid_voffset = voffset;

    /* SET VIDEO BUFFER OFFSET IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    gfx_set_display_video_yuv_offsets(yoffset, uoffset, voffset);

    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_yuv_pitch
 *
 * This routine sets the byte offset between consecutive scanlines of YUV video data
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch)
#else
int
gfx_set_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch)
#endif
{
    /* SET VIDEO PITCH IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    gfx_set_display_video_yuv_pitch(ypitch, uvpitch);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_scale
 * 
 * This routine sets the scale factor for the video overlay window.  The 
 * size of the source and destination regions are specified in pixels.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_scale(unsigned short srcw, unsigned short srch,
    unsigned short dstw, unsigned short dsth)
#else
int
gfx_set_video_scale(unsigned short srcw, unsigned short srch,
    unsigned short dstw, unsigned short dsth)
#endif
{
    unsigned long xscale, yscale, i;
    unsigned long scale;
    unsigned long temp;

    /* SAVE PARAMETERS */
    /* These are needed for clipping the video window later. Zero */
    /* values indicate that the corresponding scale value should  */
    /* not be changed.                                            */

    if (dstw != 0) {
        gfx_vid_srcw = srcw;
        gfx_vid_dstw = dstw;
    }
    if (dsth != 0) {
        gfx_vid_srch = srch;
        gfx_vid_dsth = dsth;
    }

    /* CALCULATE DISPLAY FILTER SCALE FACTORS */
    /* Zero width and height indicate don't care conditions */

    if (dstw != 0) {
        if (dstw <= srcw)
            xscale = 0x10000;
        else
            xscale = (0x10000 * (unsigned long)srcw) / (unsigned long)dstw;

        WRITE_VID32(CASTLE_VIDEO_XSCALE, xscale);
    }
    if (dsth != 0) {
        if (dsth <= srch)
            yscale = 0x10000;
        else
            yscale = (0x10000 * (unsigned long)srch) / (unsigned long)dsth;

        WRITE_VID32(CASTLE_VIDEO_YSCALE, yscale);
    }

    /* ENABLE OR DISABLE FILTERING */

    temp = READ_VID32(CASTLE_VIDEO_CONFIG);
    if (READ_VID32(CASTLE_VIDEO_XSCALE) == 0x10000 &&
        READ_VID32(CASTLE_VIDEO_YSCALE) == 0x10000) {
        WRITE_VID32(CASTLE_VIDEO_CONFIG, (temp | CASTLE_VCFG_SC_BYP));
    } else {
        scale = READ_VID32(CASTLE_VIDEO_SCALER);

        WRITE_VID32(CASTLE_VIDEO_SCALER, scale & ~CASTLE_SCALE_128_PHASES);
        WRITE_VID32(CASTLE_VIDEO_CONFIG, (temp & ~CASTLE_VCFG_SC_BYP));

        /* PROGRAM THE COEFFICIENTS */

        for (i = 0; i < 256; i++) {
            WRITE_VID32((CASTLE_COEFFICIENT_BASE + (i << 3)),
                DurangoVideoFilter[i][0]);
            WRITE_VID32((CASTLE_COEFFICIENT_BASE + (i << 3) + 4),
                DurangoVideoFilter[i][1]);
        }
    }
    /* CALL ROUTINE TO UPDATE WINDOW POSITION */
    /* This is required because the scale values affect the number of */
    /* source data pixels that need to be clipped, as well as the     */
    /* amount of data that needs to be transferred.                   */

    gfx_set_video_window(gfx_vid_xpos, gfx_vid_ypos, gfx_vid_width,
        gfx_vid_height);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_window
 * 
 * This routine sets the position and size of the video overlay window.  The 
 * x and y positions are specified in screen relative coordinates, and may be
 * negative.  
 * The size of destination region is specified in pixels.  The line size
 * indicates the number of bytes of source data per scanline.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_window(short x, short y, unsigned short w, unsigned short h)
#else
int
gfx_set_video_window(short x, short y, unsigned short w, unsigned short h)
#endif
{
    unsigned long hadjust, vadjust;
    unsigned long xstart, ystart, xend, yend;
    unsigned long lock;

    lock = READ_REG32(DC3_UNLOCK);

    /* SAVE PARAMETERS */
    /* These are needed to call this routine if the scale value changes. */

    gfx_vid_xpos = x;
    gfx_vid_ypos = y;
    gfx_vid_width = w;
    gfx_vid_height = h;

    /* GET ADJUSTMENT VALUES */
    /* Use routines to abstract version of display controller. */

    hadjust = gfx_get_htotal() - gfx_get_hsync_end() - 14l;
    vadjust = gfx_get_vtotal() - gfx_get_vsync_end() + 1l;

    /* LEFT CLIPPING */

    if (x < 0) {
        gfx_set_video_left_crop((unsigned short)(-x));
        xstart = hadjust;
    } else {
        gfx_set_video_left_crop(0);
        xstart = (unsigned long)x + hadjust;
    }

    /* HORIZONTAL END */
    /* End positions in register are non-inclusive (one more than the actual 
     * end) */

    if ((x + w) < gfx_get_hactive())
        xend = (unsigned long)x + (unsigned long)w + hadjust;

    /* RIGHT-CLIPPING */
    else
        xend = (unsigned long)gfx_get_hactive() + hadjust;

    /* VERTICAL START */

    ystart = (unsigned long)y + vadjust;

    /* VERTICAL END */

    if ((y + h) < gfx_get_vactive())
        yend = (unsigned long)y + (unsigned long)h + vadjust;

    /* BOTTOM-CLIPPING */
    else
        yend = (unsigned long)gfx_get_vactive() + vadjust;

    /* SET VIDEO POSITION */

    WRITE_VID32(CASTLE_VIDEO_X_POS, (xend << 16) | xstart);
    WRITE_VID32(CASTLE_VIDEO_Y_POS, (yend << 16) | ystart);

    /* SET COLOR KEY REGION */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_COLOR_KEY_X,
        ((xend - hadjust) << 16) | (xstart - hadjust));
    WRITE_REG32(DC3_COLOR_KEY_Y,
        ((yend - vadjust) << 16) | (ystart - vadjust));
    WRITE_REG32(DC3_UNLOCK, lock);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_left_crop
 * 
 * This routine sets the number of pixels which will be cropped from the
 * beginning of each video line. The video window will begin to display only
 * from the pixel following the cropped pixels, and the cropped pixels
 * will be ignored.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_left_crop(unsigned short x)
#else
int
gfx_set_video_left_crop(unsigned short x)
#endif
{
    unsigned long vcfg, initread;

    vcfg = READ_VID32(CASTLE_VIDEO_CONFIG) & ~CASTLE_VCFG_INIT_READ_MASK;

    /* CLIPPING ON LEFT */
    /* Adjust initial read for scale, checking for divide by zero. All video */
    /* formats must be clipped on a 4-pixel boundary                        */

    if (gfx_vid_dstw) {
        initread = (unsigned long)x *gfx_vid_srcw / gfx_vid_dstw;
    } else
        initread = 0;

    /* SET INITIAL READ ADDRESS */

    vcfg = vcfg | ((initread << 14) & CASTLE_VCFG_INIT_READ_MASK);
    WRITE_VID32(CASTLE_VIDEO_CONFIG, vcfg);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_color_key
 * 
 * This routine specifies the color key value and mask for the video overlay
 * hardware. To disable color key, the color and mask should both be set to 
 * zero. The hardware uses the color key in the following equation:
 *
 * ((source data) & (color key mask)) == ((color key) & (color key mask))
 *
 * If "graphics" is set to TRUE, the source data is graphics, and color key
 * is an RGB value. If "graphics" is set to FALSE, the source data is video,
 * and the color key is an RGB value that is compared to the video data after
 * it has gone through color space conversion (CSC).
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_color_key(unsigned long key, unsigned long mask,
    int graphics)
#else
int
gfx_set_video_color_key(unsigned long key, unsigned long mask, int graphics)
#endif
{
    unsigned long dcfg;

    /* ENABLE OR DISABLE COLOR KEY LOGIC IN THE DISPLAY CONTROLLER */
    /* When chroma key is enabled, the color key logic should be   */
    /* disabled in the VG to allow for per-pixel alpha blending.   */

    gfx_set_display_video_color_key(key, mask, graphics);

    dcfg = READ_VID32(CASTLE_DISPLAY_CONFIG);

    if (graphics)
        dcfg &= ~CASTLE_DCFG_VG_CK;
    else
        dcfg |= CASTLE_DCFG_VG_CK;

    WRITE_VID32(CASTLE_DISPLAY_CONFIG, dcfg);
    WRITE_VID32(CASTLE_VIDEO_COLOR_KEY, key & 0xFFFFFF);
    WRITE_VID32(CASTLE_VIDEO_COLOR_MASK, mask & 0xFFFFFF);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 * 
 * This routine loads the video hardware palette.  If a NULL pointer is 
 * specified, the palette is bypassed (for Castle, this means loading the 
 * palette with identity values). 
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_palette(unsigned long *palette)
#else
int
gfx_set_video_palette(unsigned long *palette)
#endif
{
    unsigned long i, entry;

    /* LOAD CASTLE VIDEO PALETTE */

    WRITE_VID32(CASTLE_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++) {
        if (palette)
            entry = palette[i];
        else
            entry = i | (i << 8) | (i << 16);
        WRITE_VID32(CASTLE_PALETTE_DATA, entry);
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette_entry
 * 
 * This routine loads a single entry of the video hardware palette.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_palette_entry(unsigned long index, unsigned long palette)
#else
int
gfx_set_video_palette_entry(unsigned long index, unsigned long palette)
#endif
{
    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    /* SET A SINGLE ENTRY */

    WRITE_VID32(CASTLE_PALETTE_ADDRESS, index);
    WRITE_VID32(CASTLE_PALETTE_DATA, palette);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_request()
 * 
 * This routine sets the horizontal (pixel) and vertical (line) video request
 * values.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_request(short x, short y)
#else
int
gfx_set_video_request(short x, short y)
#endif
{
    /* SET DISPLAY FILTER VIDEO REQUEST */

    x += gfx_get_htotal() - gfx_get_hsync_end() - 2;
    y += gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    if ((x < 0) || (x > CASTLE_VIDEO_REQUEST_MASK) ||
        (y < 0) || (y > CASTLE_VIDEO_REQUEST_MASK))
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_VID32(CASTLE_VIDEO_REQUEST,
        ((unsigned long)x << CASTLE_VIDEO_X_REQUEST_POS) | ((unsigned long)y
            << CASTLE_VIDEO_Y_REQUEST_POS));
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_cursor()
 * 
 * This routine configures the video hardware cursor.
 * If the "mask"ed bits in the graphics pixel match "key", then either 
 * "color1"or "color2" will be used for this pixel, according to the value of
 * bit number "select_color2" of the graphics pixel.
 *
 * key - 24 bit RGB value
 * mask - 24 bit mask
 * color1, color2 - RGB or YUV, depending on the current color space 
 * conversion select_color2 - value between 0 to 23
 *
 * To disable match, a "mask" and "key" value of 0xffffff should be set,
 * because the graphics pixels incoming to the video processor have maximum 16
 * bits set (0xF8FCF8).
 *
 * This feature is useful for disabling alpha blending of the cursor.
 * Otherwise cursor image would be blurred (or completely invisible if video
 * alpha is maximum value).
 * Note: the cursor pixel replacements take place both inside and outside the
 * video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_cursor(unsigned long key, unsigned long mask,
    unsigned short select_color2, unsigned long color1, unsigned long color2)
#else
int
gfx_set_video_cursor(unsigned long key, unsigned long mask,
    unsigned short select_color2, unsigned long color1, unsigned long color2)
#endif
{
    if (select_color2 > CASTLE_CURSOR_COLOR_BITS)
        return GFX_STATUS_BAD_PARAMETER;
    key =
        (key & CASTLE_COLOR_MASK) | ((unsigned long)select_color2 <<
        CASTLE_CURSOR_COLOR_KEY_OFFSET_POS);
    WRITE_VID32(CASTLE_CURSOR_COLOR_KEY, key);
    WRITE_VID32(CASTLE_CURSOR_COLOR_MASK, mask);
    WRITE_VID32(CASTLE_CURSOR_COLOR_1, color1);
    WRITE_VID32(CASTLE_CURSOR_COLOR_2, color2);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_cursor_enable()
 * 
 * This routine enables or disables the cursor color key mechanism.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_video_cursor_enable(int enable)
#else
int
gfx_set_video_cursor_enable(int enable)
#endif
{
    unsigned long temp = READ_VID32(CASTLE_CURSOR_COLOR_KEY);

    if (enable)
        temp |= CASTLE_CURSOR_COLOR_KEY_ENABLE;
    else
        temp &= ~CASTLE_CURSOR_COLOR_KEY_ENABLE;

    WRITE_VID32(CASTLE_CURSOR_COLOR_KEY, temp);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_enable
 * 
 * This routine enables or disables the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_enable(int enable)
#else
int
gfx_set_alpha_enable(int enable)
#endif
{
    unsigned long address = 0, value = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = CASTLE_ALPHA_CONTROL_1 + ((unsigned long)gfx_alpha_select << 5);
    value = READ_VID32(address);
    if (enable)
        value |= CASTLE_ACTRL_WIN_ENABLE;
    else
        value &= ~(CASTLE_ACTRL_WIN_ENABLE);
    WRITE_VID32(address, value);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_window
 * 
 * This routine sets the size of the currently selected alpha region.
 * Note: "x" and "y" are signed to enable using negative values needed for
 * implementing workarounds of hardware issues.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_window(short x, short y,
    unsigned short width, unsigned short height)
#else
int
gfx_set_alpha_window(short x, short y,
    unsigned short width, unsigned short height)
#endif
{
    unsigned long address = 0;

    /* CHECK FOR CLIPPING */

    if ((x + width) > gfx_get_hactive())
        width = gfx_get_hactive() - x;
    if ((y + height) > gfx_get_vactive())
        height = gfx_get_vactive() - y;

    /* ADJUST POSITIONS */

    x += gfx_get_htotal() - gfx_get_hsync_end() - 2;
    y += gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = CASTLE_ALPHA_XPOS_1 + ((unsigned long)gfx_alpha_select << 5);

    /* END POSITIONS IN REGISTERS ARE NON-INCLUSIVE (ONE MORE THAN ACTUAL END)
     * */

    WRITE_VID32(address, (unsigned long)x |
        ((unsigned long)(x + width) << 16));
    WRITE_VID32(address + 8, (unsigned long)y |
        ((unsigned long)(y + height) << 16));
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_value
 * 
 * This routine sets the alpha value for the currently selected alpha
 * region.  It also specifies an increment/decrement value for fading.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_value(unsigned char alpha, char delta)
#else
int
gfx_set_alpha_value(unsigned char alpha, char delta)
#endif
{
    unsigned long address = 0, value = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = CASTLE_ALPHA_CONTROL_1 + ((unsigned long)gfx_alpha_select << 5);
    value = READ_VID32(address);
    value &= CASTLE_ACTRL_WIN_ENABLE;  /* keep only enable bit */
    value |= (unsigned long)alpha;
    value |= (((unsigned long)delta) & 0xff) << 8;
    value |= CASTLE_ACTRL_LOAD_ALPHA;
    WRITE_VID32(address, value);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_priority
 * 
 * This routine sets the priority of the currently selected alpha region.
 * A higher value indicates a higher priority.
 * Note: Priority of enabled alpha windows must be different.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_priority(int priority)
#else
int
gfx_set_alpha_priority(int priority)
#endif
{
    unsigned long pos = 0, value = 0;

    if (priority > 3)
        return (GFX_STATUS_BAD_PARAMETER);
    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    value = READ_VID32(CASTLE_VID_ALPHA_CONTROL);
    pos = 16 + (gfx_alpha_select << 1);
    value &= ~(0x03l << pos);
    value |= (unsigned long)priority << pos;
    WRITE_VID32(CASTLE_VID_ALPHA_CONTROL, value);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color
 * 
 * This routine sets the color to be displayed inside the currently selected
 * alpha window when there is a color key match (when the alpha color
 * mechanism is enabled).
 * "color" is an RGB value (for RGB blending) or a YUV value (for YUV 
 * blending).
 * In Interlaced YUV blending mode, Y/2 value should be used.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_color(unsigned long color)
#else
int
gfx_set_alpha_color(unsigned long color)
#endif
{
    unsigned long address = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = CASTLE_ALPHA_COLOR_1 + ((unsigned long)gfx_alpha_select << 5);
    WRITE_VID32(address, color);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color_enable
 * 
 * Enable or disable the color mechanism in the alpha window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_alpha_color_enable(int enable)
#else
int
gfx_set_alpha_color_enable(int enable)
#endif
{
    unsigned long color;
    unsigned long address = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = CASTLE_ALPHA_COLOR_1 + ((unsigned long)gfx_alpha_select << 5);
    color = READ_VID32(address);
    if (enable)
        color |= CASTLE_ALPHA_COLOR_ENABLE;
    else
        color &= ~CASTLE_ALPHA_COLOR_ENABLE;
    WRITE_VID32(address, color);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_no_ck_outside_alpha
 * 
 * This function affects where inside the video window color key or chroma
 * key comparison is done:
 * If enable is TRUE, color/chroma key comparison is performed only inside
 * the enabled alpha windows. Outside the (enabled) alpha windows, only video
 * is displayed if color key is used, and only graphics is displayed if chroma
 * key is used.
 * If enable is FALSE, color/chroma key comparison is performed in all the
 * video window area.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_set_no_ck_outside_alpha(int enable)
#else
int
gfx_set_no_ck_outside_alpha(int enable)
#endif
{
    unsigned long value;

    value = READ_VID32(CASTLE_VID_ALPHA_CONTROL);
    if (enable)
        WRITE_VID32(CASTLE_VID_ALPHA_CONTROL,
            value | CASTLE_NO_CK_OUTSIDE_ALPHA);
    else
        WRITE_VID32(CASTLE_VID_ALPHA_CONTROL,
            value & ~CASTLE_NO_CK_OUTSIDE_ALPHA);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_clock_frequency
 *
 * This routine returns the current clock frequency in 16.16 format.
 * It reads the current register value and finds the match in the table.
 * If no match is found, this routine returns 0.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_clock_frequency(void)
#else
unsigned long
gfx_get_clock_frequency(void)
#endif
{
    Q_WORD msr_value;
    unsigned int index;
    unsigned long value, mask = 0x00001FFF;

    /* READ PLL SETTING */

    gfx_msr_read(RC_ID_MCP, MCP_DOTPLL, &msr_value);
    value = msr_value.high & mask;

    /* SEARCH FOR A MATCH */

    for (index = 0; index < NUM_CASTLE_FREQUENCIES; index++) {
        if ((CASTLE_PLLtable[index].pll_value & mask) == value)
            return (CASTLE_PLLtable[index].frequency);
    }
    return (0);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_sync_polarities
 *
 * This routine returns the polarities of the sync pulses:
 *     Bit 0: Set if negative horizontal polarity.
 *     Bit 1: Set if negative vertical polarity.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_sync_polarities(void)
#else
int
gfx_get_sync_polarities(void)
#endif
{
    int polarities = 0;

    if (READ_VID32(CASTLE_DISPLAY_CONFIG) & CASTLE_DCFG_CRT_HSYNC_POL)
        polarities |= 1;
    if (READ_VID32(CASTLE_DISPLAY_CONFIG) & CASTLE_DCFG_CRT_VSYNC_POL)
        polarities |= 2;
    return (polarities);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_palette_entry
 *
 * This routine returns a single palette entry.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_palette_entry(unsigned long index, unsigned long *palette)
#else
int
gfx_get_video_palette_entry(unsigned long index, unsigned long *palette)
#endif
{
    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    /* READ A SINGLE ENTRY */

    WRITE_VID32(CASTLE_PALETTE_ADDRESS, index);
    *palette = READ_VID32(CASTLE_PALETTE_DATA);

    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_enable
 *
 * This routine returns the value "one" if video overlay is currently enabled,
 * otherwise it returns the value "zero".
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_enable(void)
#else
int
gfx_get_video_enable(void)
#endif
{
    if (READ_VID32(CASTLE_VIDEO_CONFIG) & CASTLE_VCFG_VID_EN)
        return (1);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_format
 *
 * This routine returns the current video overlay format.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_format(void)
#else
int
gfx_get_video_format(void)
#endif
{
    unsigned long ctrl, vcfg;

    ctrl = READ_VID32(CASTLE_VID_ALPHA_CONTROL);
    vcfg = READ_VID32(CASTLE_VIDEO_CONFIG);

    if (ctrl & CASTLE_VIDEO_INPUT_IS_RGB) {
        switch (vcfg & CASTLE_VCFG_VID_INP_FORMAT) {
        case CASTLE_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_RGB;
        case CASTLE_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_P2M_P2L_P1M_P1L;
        case CASTLE_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_P1M_P1L_P2M_P2L;
        case CASTLE_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_P1M_P2L_P2M_P1L;
        }
    }

    if (vcfg & CASTLE_VCFG_4_2_0_MODE) {
        switch (vcfg & CASTLE_VCFG_VID_INP_FORMAT) {
        case CASTLE_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_Y0Y1Y2Y3;
        case CASTLE_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_Y3Y2Y1Y0;
        case CASTLE_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_Y1Y0Y3Y2;
        case CASTLE_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_Y1Y2Y3Y0;
        }
    } else {
        switch (vcfg & CASTLE_VCFG_VID_INP_FORMAT) {
        case CASTLE_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_UYVY;
        case CASTLE_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_Y2YU;
        case CASTLE_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_YUYV;
        case CASTLE_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_YVYU;
        }
    }
    return (GFX_STATUS_ERROR);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_src_size
 *
 * This routine returns the size of the source video overlay buffer.  The 
 * return value is (height << 16) | width.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_src_size(void)
#else
unsigned long
gfx_get_video_src_size(void)
#endif
{
    unsigned long width, height;

    /* DETERMINE SOURCE WIDTH FROM THE DISPLAY FILTER VIDEO LINE SIZE */

    width = (READ_VID32(CASTLE_VIDEO_CONFIG) >> 7) & 0x000001FE;
    if (READ_VID32(CASTLE_VIDEO_CONFIG) & CASTLE_VCFG_LINE_SIZE_UPPER)
        width += 512l;
    if (READ_VID32(CASTLE_VIDEO_CONFIG) & CASTLE_VCFG_LINE_SIZE_BIT9)
        width += 1024l;

    /* DETERMINE SOURCE HEIGHT FROM THE DISPLAY FILTER SCALE REGISTER */

    height = READ_VID32(CASTLE_VIDEO_SCALER) & 0x7FF;

    return ((height << 16) | width);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_line_size
 *
 * This routine returns the line size of the source video overlay buffer, in
 * pixels.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_line_size(void)
#else
unsigned long
gfx_get_video_line_size(void)
#endif
{
    unsigned long width = 0;

    /* DETERMINE SOURCE WIDTH FROM THE CASTLE VIDEO LINE SIZE */

    width = (READ_VID32(CASTLE_VIDEO_CONFIG) >> 7) & 0x000001FE;
    if (READ_VID32(CASTLE_VIDEO_CONFIG) & CASTLE_VCFG_LINE_SIZE_UPPER)
        width += 512l;
    return (width);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_xclip
 *
 * This routine returns the number of bytes clipped on the left side of a
 * video overlay line (skipped at beginning).
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_xclip(void)
#else
unsigned long
gfx_get_video_xclip(void)
#endif
{
    unsigned long clip = 0;

    /* DETERMINE SOURCE WIDTH FROM THE CASTLE VIDEO LINE SIZE */

    clip = ((READ_VID32(CASTLE_VIDEO_CONFIG) >> 16) & 0x001FF) << 3;
    return (clip);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_offset
 *
 * This routine returns the current offset for the video overlay buffer.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_offset(void)
#else
unsigned long
gfx_get_video_offset(void)
#endif
{
    return (gfx_get_display_video_offset());
}

/*----------------------------------------------------------------------------
 * gfx_get_video_yuv_offsets
 *
 * This routine returns the current offsets for the video overlay buffer when in 4:2:0.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_video_yuv_offsets(unsigned long *yoffset, unsigned long *uoffset,
    unsigned long *voffset)
#else
void
gfx_get_video_yuv_offsets(unsigned long *yoffset, unsigned long *uoffset,
    unsigned long *voffset)
#endif
{
    gfx_get_display_video_yuv_offsets(yoffset, uoffset, voffset);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_yuv_pitch
 *
 * This routine returns the current pitch values for the video overlay buffer.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch)
#else
void
gfx_get_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch)
#endif
{
    gfx_get_display_video_yuv_pitch(ypitch, uvpitch);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 * 
 * This routine returns the size of the displayed video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_dst_size(void)
#else
unsigned long
gfx_get_video_dst_size(void)
#endif
{
    unsigned long xsize, ysize;

    xsize = READ_VID32(CASTLE_VIDEO_X_POS);
    xsize = ((xsize >> 16) & 0x7FF) - (xsize & 0x7FF);
    ysize = READ_VID32(CASTLE_VIDEO_Y_POS);
    ysize = ((ysize >> 16) & 0x7FF) - (ysize & 0x7FF);
    return ((ysize << 16) | xsize);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 * 
 * This routine returns the position of the video overlay window.  The
 * return value is (ypos << 16) | xpos.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_position(void)
#else
unsigned long
gfx_get_video_position(void)
#endif
{
    unsigned long hadjust, vadjust;
    unsigned long xpos, ypos;

    /* READ HARDWARE POSITION */

    xpos = READ_VID32(CASTLE_VIDEO_X_POS) & 0x000007FF;
    ypos = READ_VID32(CASTLE_VIDEO_Y_POS) & 0x000007FF;

    /* GET ADJUSTMENT VALUES */
    /* Use routines to abstract version of display controller. */

    hadjust =
        (unsigned long)gfx_get_htotal() - (unsigned long)gfx_get_hsync_end() -
        14l;
    vadjust =
        (unsigned long)gfx_get_vtotal() - (unsigned long)gfx_get_vsync_end() +
        1l;
    xpos -= hadjust;
    ypos -= vadjust;
    return ((ypos << 16) | (xpos & 0x0000FFFF));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key
 * 
 * This routine returns the current video color key value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_color_key(void)
#else
unsigned long
gfx_get_video_color_key(void)
#endif
{
    if (READ_VID32(CASTLE_DISPLAY_CONFIG) & CASTLE_DCFG_VG_CK)
        return (READ_VID32(CASTLE_VIDEO_COLOR_KEY));
    else
        return (gfx_get_display_video_color_key());
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 * 
 * This routine returns the current video color mask value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_get_video_color_key_mask(void)
#else
unsigned long
gfx_get_video_color_key_mask(void)
#endif
{
    if (READ_VID32(CASTLE_DISPLAY_CONFIG) & CASTLE_DCFG_VG_CK)
        return (READ_VID32(CASTLE_VIDEO_COLOR_MASK));
    else
        return (gfx_get_display_video_color_key_mask());
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 * 
 * This routine returns 0 for video data compare, 1 for graphics data.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_color_key_src(void)
#else
int
gfx_get_video_color_key_src(void)
#endif
{
    if (READ_VID32(CASTLE_DISPLAY_CONFIG) & CASTLE_DCFG_VG_CK)
        return (0);
    return (1);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_request
 * 
 * This routine returns the horizontal (pixel) and vertical (lines) video
 * request values.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_request(short *x, short *y)
#else
int
gfx_get_video_request(short *x, short *y)
#endif
{
    unsigned long request = 0;

    request = (READ_VID32(CASTLE_VIDEO_REQUEST));
    *x = (short)((request >> CASTLE_VIDEO_X_REQUEST_POS) &
        CASTLE_VIDEO_REQUEST_MASK);
    *y = (short)((request >> CASTLE_VIDEO_Y_REQUEST_POS) &
        CASTLE_VIDEO_REQUEST_MASK);

    *x -= gfx_get_htotal() - gfx_get_hsync_end() - 2;
    *y -= gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_cursor()
 * 
 * This routine configures the video hardware cursor.
 * If the "mask"ed bits in the graphics pixel match "key", then either 
 * "color1" or "color2" will be used for this pixel, according to the value of
 * the bit in offset "select_color2".
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
castle_get_video_cursor(unsigned long *key, unsigned long *mask,
    unsigned short *select_color2, unsigned long *color1,
    unsigned short *color2)
#else
int
gfx_get_video_cursor(unsigned long *key, unsigned long *mask,
    unsigned short *select_color2, unsigned long *color1,
    unsigned short *color2)
#endif
{
    *select_color2 =
        (unsigned short)(READ_VID32(CASTLE_CURSOR_COLOR_KEY) >>
        CASTLE_CURSOR_COLOR_KEY_OFFSET_POS);
    *key = READ_VID32(CASTLE_CURSOR_COLOR_KEY) & CASTLE_COLOR_MASK;
    *mask = READ_VID32(CASTLE_CURSOR_COLOR_MASK) & CASTLE_COLOR_MASK;
    *color1 = READ_VID32(CASTLE_CURSOR_COLOR_1) & CASTLE_COLOR_MASK;
    *color2 =
        (unsigned short)(READ_VID32(CASTLE_CURSOR_COLOR_2) &
        CASTLE_COLOR_MASK);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc
 *
 * This routine returns the hardware CRC value, which is used for automated 
 * testing.  The value is like a checksum, but will change if pixels move
 * locations.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_read_crc(void)
#else
unsigned long
gfx_read_crc(void)
#endif
{
    Q_WORD msr_value;
    unsigned long crc = 0xFFFFFFFF;

    /* DISABLE 32-BIT CRCS */
    /* For GX1.x, this is a reserved bit, and is assumed to be a benign 
     * access */

    gfx_msr_read(RC_ID_DF, CASTLE_MBD_MSR_DIAG_DF, &msr_value);
    msr_value.low &= ~CASTLE_DIAG_32BIT_CRC;
    gfx_msr_write(RC_ID_DF, CASTLE_MBD_MSR_DIAG_DF, &msr_value);

    if (gfx_test_timing_active()) {
        /* WAIT UNTIL ACTIVE DISPLAY */

        while (!gfx_test_vertical_active()) ;

        /* RESET CRC DURING ACTIVE DISPLAY */

        WRITE_VID32(CASTLE_VID_CRC, 0);
        WRITE_VID32(CASTLE_VID_CRC, 1);

        /* WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE */

        while (!gfx_test_vertical_active()) ;
        while (gfx_test_vertical_active()) ;
        while (!gfx_test_vertical_active()) ;
        while (gfx_test_vertical_active()) ;
        while (!gfx_test_vertical_active()) ;
        crc = READ_VID32(CASTLE_VID_CRC) >> 8;
    }
    return (crc);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc32
 *
 * This routine returns the 32-bit hardware CRC value, which is used for 
 * automated testing.  The value is like a checksum, but will change if pixels
 * move
 * locations.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_read_crc32(void)
#else
unsigned long
gfx_read_crc32(void)
#endif
{
    Q_WORD msr_value;
    unsigned long crc = 0xFFFFFFFF;

    /* ENABLE 32-BIT CRCS */
    /* For GX1.x, this is a reserved bit, and is assumed to be a benign 
     * access */

    gfx_msr_read(RC_ID_DF, CASTLE_MBD_MSR_DIAG_DF, &msr_value);
    msr_value.low |= CASTLE_DIAG_32BIT_CRC;
    gfx_msr_write(RC_ID_DF, CASTLE_MBD_MSR_DIAG_DF, &msr_value);

    if (gfx_test_timing_active()) {
        /* WAIT UNTIL ACTIVE DISPLAY */

        while (!gfx_test_vertical_active()) ;

        /* RESET CRC DURING ACTIVE DISPLAY */

        WRITE_VID32(CASTLE_VID_CRC, 0);
        WRITE_VID32(CASTLE_VID_CRC, 1);

        /* WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE */

        while (!gfx_test_vertical_active()) ;
        while (gfx_test_vertical_active()) ;
        while (!gfx_test_vertical_active()) ;
        while (gfx_test_vertical_active()) ;
        while (!gfx_test_vertical_active()) ;
        crc = READ_VID32(CASTLE_VID_CRC32);
    }
    return (crc);
}

/*---------------------------------------------------------------------------
 * gfx_read_window_crc
 *
 * This routine returns the hardware CRC value for a subsection of the display
 * This value is used to debug whole-screen CRC failures.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
castle_read_window_crc(int source, unsigned short x, unsigned short y,
    unsigned short width, unsigned short height, int crc32)
#else
unsigned long
gfx_read_window_crc(int source, unsigned short x, unsigned short y,
    unsigned short width, unsigned short height, int crc32)
#endif
{
    Q_WORD msr_value;
    unsigned long xpos, ypos, crc = 0;
    unsigned long old_fmt = 0;
    unsigned int vsync_active_base, vsync_inactive_base, hsync_active_base;
    unsigned int vsync_active_shift, vsync_inactive_shift, hsync_active_shift;
    unsigned int vsync_bit, hsync_bit, sync_polarities = 0;

    /* CONFIGURE DISPLAY FILTER TO LOAD DATA ONTO LOWER 32-BITS */

    msr_value.high = 0;
    msr_value.low =
        (source == CRC_SOURCE_GFX_DATA) ? (RCDF_MBD_DIAG_EN0 | 0x0000000F)
        : (RCDF_MBD_DIAG_EN0 | 0x0000000B);
    gfx_msr_write(RC_ID_DF, MBD_MSR_DIAG, &msr_value);

    /* CONFIGURE DISPLAY FILTER FOR APPROPRIATE OUTPUT */

    if (source != CRC_SOURCE_GFX_DATA) {
        gfx_msr_read(RC_ID_DF, MBD_MSR_CONFIG, &msr_value);
        old_fmt = msr_value.low;
        msr_value.low &= ~(RCDF_CONFIG_FMT_MASK);
        msr_value.low |=
            ((source ==
                CRC_SOURCE_FP_DATA) ? RCDF_CONFIG_FMT_FP :
            RCDF_CONFIG_FMT_CRT);
        gfx_msr_write(RC_ID_DF, MBD_MSR_CONFIG, &msr_value);
    }

    /* CONFIGURE MCP TO LOAD REGB DATA ONTO UPPER 32-BITS */

    msr_value.low = MCP_MBD_DIAG_EN1 | 0x00050000;
    gfx_msr_write(RC_ID_MCP, MBD_MSR_DIAG, &msr_value);

    /* ENABLE HW CLOCK GATING AND SET MCP CLOCK TO DOT CLOCK */

    msr_value.low = 1l;
    gfx_msr_write(RC_ID_MCP, MBD_MSR_PM, &msr_value);
    msr_value.low = 0;
    gfx_msr_write(RC_ID_MCP, MCP_DBGCLKCTL, &msr_value);
    msr_value.low = 3;
    gfx_msr_write(RC_ID_MCP, MCP_DBGCLKCTL, &msr_value);

    /* DISABLE MCP ACTIONS */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000000;
    gfx_msr_write(RC_ID_MCP, MCP_DIAGCTL, &msr_value);

    /* SET APPROPRIATE BASE ADDRESS */
    /* M-Sets use normal diag bits, while N-Sets use inverted diag bits */
    /* We thus use the M-sets when polling for a high signal and the N  */
    /* sets when polling for a low signal.                              */

    if (source != CRC_SOURCE_GFX_DATA) {
        sync_polarities = gfx_get_sync_polarities();
        vsync_bit = 29;
        hsync_bit = 30;
    } else {
        vsync_bit = 25;
        hsync_bit = 26;
    }

    if (sync_polarities & 1) {
        hsync_active_base = MCP_SETM0CTL;
        hsync_active_shift = 2;
    } else {
        hsync_active_base = MCP_SETN0CTL;
        hsync_active_shift = 1;
    }
    if (sync_polarities & 2) {
        vsync_active_base = MCP_SETM0CTL;
        vsync_inactive_base = MCP_SETN0CTL;
        vsync_active_shift = 2;
        vsync_inactive_shift = 1;
    } else {
        vsync_active_base = MCP_SETN0CTL;
        vsync_inactive_base = MCP_SETM0CTL;
        vsync_active_shift = 1;
        vsync_inactive_shift = 2;
    }

    /* SET STATE TRANSITIONS          */

    /* STATE 0-1 TRANSITION (SET 0)      */
    /* XState = 00 and VSync Inactive    */
    /* Note: DF VSync = Diag Bus Bit 29  */
    /*       VG VSync = Diag Bus Bit 25  */

    msr_value.low = 0x000000A0;
    msr_value.high = 0x00008000 | ((unsigned long)vsync_bit << 16) |
        ((unsigned long)vsync_bit << 21) | ((unsigned long)vsync_bit << 26);
    gfx_msr_write(RC_ID_MCP, vsync_inactive_base, &msr_value);

    /* STATE 1-2 TRANSITION (SET 4)   */
    /* XState = 01 and VSync Active   */

    msr_value.low = 0x000000C0;
    gfx_msr_write(RC_ID_MCP, vsync_active_base + 4, &msr_value);

    /* STATE 2-3 TRANSITION (SET 1)   */
    /* XState = 10 and VSync Inactive */

    msr_value.low = 0x00000120;
    gfx_msr_write(RC_ID_MCP, vsync_inactive_base + 1, &msr_value);

    /* HORIZONTAL COUNTER (SET 5)         */
    /* XState = 10 and HSync Active       */
    /* Notes: DF HSync = Diag Bus Bit 30  */
    /*        VG HSync = Diag Bus Bit 26  */

    msr_value.high = 0x00008000 | ((unsigned long)hsync_bit << 16) |
        ((unsigned long)hsync_bit << 21) | ((unsigned long)hsync_bit << 26);
    msr_value.low = 0x00000120;
    gfx_msr_write(RC_ID_MCP, hsync_active_base + 5, &msr_value);

    /* HORIZONTAL COUNTER RESET (SET 4)     */
    /* XState = 10 and H. Counter = limit   */
    /* Note: H. Counter is lower 16-bits of */
    /*       RegB.                          */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000128;
    gfx_msr_write(RC_ID_MCP, vsync_inactive_base + 4, &msr_value);

    /* CRC TRIGGER (SET 0)   */
    /* Cmp0 <= xpos <  Cmp1  */
    /* Cmp2 <= ypos <  Cmp2  */

    msr_value.high = 0x00000000;
    msr_value.low = 0x10C20120;
    gfx_msr_write(RC_ID_MCP, vsync_active_base, &msr_value);

    /* SET COMPARATOR VALUES */
    /* Note: The VG data outputs from the DF are delayed by one pixel clock. */
    /*       In this mode, we thus add one to horizontal comparator limits. */

    /* COMPARATOR 0                                        */
    /* Lower limit = xpos + (h_blank_pixels - 1) - 3       */
    /* Notes:                                              */
    /*   1. 3 is the pipeline delay for MCP register       */
    /*      data to access the diag bus                    */
    /*   2. h_blank_pixels = HTOTAL - HSYNC_END            */

    xpos = (unsigned long)x + ((unsigned long)gfx_get_htotal() -
        (unsigned long)gfx_get_hsync_end() - 1l) - 3l;
    if (source == CRC_SOURCE_GFX_DATA)
        xpos++;
    msr_value.high = 0x00000000;
    msr_value.low = xpos;
    gfx_msr_write(RC_ID_MCP, MCP_CMPVAL0, &msr_value);

    /* COMPARATOR 1                                          */
    /* Upper limit = xpos + width + (h_blank_pixels - 1) - 3 */

    msr_value.low = xpos + (unsigned long)width;
    gfx_msr_write(RC_ID_MCP, MCP_CMPVAL0 + 2, &msr_value);

    /* COMPARATOR 2                                  */
    /* Lower limit = ypos + v_blank_pixels           */
    /* Notes:                                        */
    /*   1. v_blank_pixels = VTOTAL - VSYNC_END      */

    ypos =
        (unsigned long)y + (unsigned long)gfx_get_vtotal() -
        (unsigned long)gfx_get_vsync_end();
    msr_value.low = ypos << 16;
    gfx_msr_write(RC_ID_MCP, MCP_CMPVAL0 + 4, &msr_value);

    /* COMPARATOR 3                                  */
    /* Upper limit = ypos + height + v_blank_pixels  */

    msr_value.low = (ypos + (unsigned long)height) << 16;
    gfx_msr_write(RC_ID_MCP, MCP_CMPVAL0 + 6, &msr_value);

    /* SET COMPARATOR MASKS */

    /* COMPARATORS 0 AND 1 REFER TO LOWER 16 BITS OF REGB */

    msr_value.high = 0x00000000;
    msr_value.low = 0x0000FFFF;
    gfx_msr_write(RC_ID_MCP, MCP_CMPMASK0, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_CMPMASK0 + 2, &msr_value);

    /* COMPARATORS 2 AND 3 REFER TO UPPER 16 BITS OF REGB */

    msr_value.low = 0xFFFF0000;
    gfx_msr_write(RC_ID_MCP, MCP_CMPMASK0 + 4, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_CMPMASK0 + 6, &msr_value);

    /* SET REGA MASK TO CRC ONLY 24 BITS OF DATA */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00FFFFFF;
    gfx_msr_write(RC_ID_MCP, MCP_REGAMASK, &msr_value);

    /* SET REGB VALUE */
    /* Lower 16 bits use HTOTAL - SYNC TIME - 1 to set the counter rollover 
     * limit. */
    /* Upper 16 bits use 0xFFFF to remove auto-clear behavior.     */

    msr_value.high = 0x00000000;
    msr_value.low = 0xFFFF0000 |
        ((gfx_get_htotal() - (gfx_get_hsync_end() - gfx_get_hsync_start()) -
            1) & 0xFFFF);
    gfx_msr_write(RC_ID_MCP, MCP_REGBVAL, &msr_value);

    /* PROGRAM ACTIONS */

    /* GOTO STATE 01 */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000008 | (1l << vsync_inactive_shift);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 14, &msr_value);

    /* GOTO STATE 10 */

    msr_value.low = 0x00080000 | (1l << (vsync_active_shift + 16));
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 15, &msr_value);

    /* GOTO STATE 11 */

    msr_value.low = 0x00000080 | (1l << (vsync_inactive_shift + 4));
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 16, &msr_value);

    /* CLEAR REGB (COUNTERS)  */
    /* RegB is cleared upon transitioning to state 10              */
    /* RegA is not cleared as the initial value must be 0x00000001 */

    msr_value.low = 0x00080000 | (1l << (vsync_active_shift + 16));
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0, &msr_value);

    /* CRC INTO REGA        */
    /* INCREMENT H. COUNTER */
    /* cmp0 <= xpos < cmp1  */
    /* cmp2 <= ypos < cmp3  */
    /* XState = 10          */

    msr_value.low = 0x00000008 | (1l << vsync_active_shift) |
        0x00800000 | (1l << (hsync_active_shift + 20));
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 1, &msr_value);

    /* INCREMENT V. COUNTER */
    /* V. Counter is incremented when the H. Counter */
    /* rolls over.                                   */

    msr_value.low = 0x00080000 | (1l << (vsync_inactive_shift + 16));
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 2, &msr_value);

    /* CLEAR ALL OTHER ACTIONS */
    /* This prevents side-effects from previous accesses to the MCP */
    /* debug logic.                                                 */
    msr_value.low = 0x00000000;
    msr_value.high = 0x00000000;
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 3, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 4, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 5, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 6, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 7, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 8, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 9, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 10, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 11, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 12, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 13, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 17, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 18, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 19, &msr_value);
    gfx_msr_write(RC_ID_MCP, MCP_ACTION0 + 20, &msr_value);

    /* SET REGA CRC VALUE TO 1 OR 0 */

    if (!crc32)
        msr_value.low = 0x00000001;
    gfx_msr_write(RC_ID_MCP, MCP_REGA, &msr_value);

    /* SET XSTATE TO 0 */

    msr_value.low = 0;
    msr_value.high = 0;
    gfx_msr_write(RC_ID_MCP, MCP_XSTATE, &msr_value);

    /* CONFIGURE DIAG CONTROL */
    /* Set all four comparators to watch the upper diag bus.           */
    /* Set REGA action1 to legacy CRC or 32-bit CRC.                   */
    /* Set REGB action1 to increment lower 16 bits and clear at limit. */
    /* Set REGB action2 to increment upper 16 bits.                    */
    /* Enable all actions.                                             */

    if (crc32)
        msr_value.low = 0x9A820055;
    else
        msr_value.low = 0x9A840055;
    msr_value.high = 0x00000000;
    gfx_msr_write(RC_ID_MCP, MCP_DIAGCTL, &msr_value);

    /* DELAY TWO FRAMES */

    while (!gfx_test_vertical_active()) ;
    while (gfx_test_vertical_active()) ;
    while (!gfx_test_vertical_active()) ;
    while (gfx_test_vertical_active()) ;
    while (!gfx_test_vertical_active()) ;

    /* VERIFY THAT XSTATE = 11 */

    gfx_msr_read(RC_ID_MCP, MCP_XSTATE, &msr_value);
    if ((msr_value.low & 3) == 3) {
        gfx_msr_read(RC_ID_MCP, MCP_REGA, &msr_value);

        crc = msr_value.low;
        if (!crc32)
            crc &= 0xFFFFFF;
    }

    /* DISABLE MCP AND DF DIAG BUS OUTPUTS */

    msr_value.low = 0x00000000;
    msr_value.high = 0x00000000;
    gfx_msr_write(RC_ID_DF, MBD_MSR_DIAG, &msr_value);
    gfx_msr_write(RC_ID_MCP, MBD_MSR_DIAG, &msr_value);

    /* DISABLE MCP ACTIONS */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000000;
    gfx_msr_write(RC_ID_MCP, MCP_DIAGCTL, &msr_value);

    /* RESTORE PREVIOUS OUTPUT FORMAT */

    if (source != CRC_SOURCE_GFX_DATA) {
        gfx_msr_read(RC_ID_DF, MBD_MSR_CONFIG, &msr_value);
        msr_value.low = old_fmt;
        gfx_msr_write(RC_ID_DF, MBD_MSR_CONFIG, &msr_value);
    }
    return crc;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_enable
 * 
 * This routine returns 1 if the selected alpha window is currently 
 * enabled, or 0 if it is currently disabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_alpha_enable(int *enable)
#else
void
gfx_get_alpha_enable(int *enable)
#endif
{
    unsigned long value = 0;

    *enable = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(CASTLE_ALPHA_CONTROL_1 +
            ((unsigned long)gfx_alpha_select << 5));
        if (value & CASTLE_ACTRL_WIN_ENABLE)
            *enable = 1;
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_size
 * 
 * This routine returns the size of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_alpha_size(unsigned short *x, unsigned short *y,
    unsigned short *width, unsigned short *height)
#else
void
gfx_get_alpha_size(unsigned short *x, unsigned short *y,
    unsigned short *width, unsigned short *height)
#endif
{
    unsigned long value = 0;

    *x = 0;
    *y = 0;
    *width = 0;
    *height = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(CASTLE_ALPHA_XPOS_1 +
            ((unsigned long)gfx_alpha_select << 5));
        *x = (unsigned short)(value & 0x000007FF);
        *width = (unsigned short)((value >> 16) & 0x000007FF) - *x;
        value =
            READ_VID32(CASTLE_ALPHA_YPOS_1 +
            ((unsigned long)gfx_alpha_select << 5));
        *y = (unsigned short)(value & 0x000007FF);
        *height = (unsigned short)((value >> 16) & 0x000007FF) - *y;
    }
    *x -= gfx_get_htotal() - gfx_get_hsync_end() - 2;
    *y -= gfx_get_vtotal() - gfx_get_vsync_end() + 1;
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_value
 * 
 * This routine returns the alpha value and increment/decrement value of 
 * the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_alpha_value(unsigned char *alpha, char *delta)
#else
void
gfx_get_alpha_value(unsigned char *alpha, char *delta)
#endif
{
    unsigned long value = 0;

    *alpha = 0;
    *delta = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(CASTLE_ALPHA_CONTROL_1 +
            ((unsigned long)gfx_alpha_select << 5));
        *alpha = (unsigned char)(value & 0x00FF);
        *delta = (char)((value >> 8) & 0x00FF);
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_priority
 * 
 * This routine returns the priority of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_alpha_priority(int *priority)
#else
void
gfx_get_alpha_priority(int *priority)
#endif
{
    unsigned long pos = 0, value = 0;

    *priority = 0;
    if (gfx_alpha_select <= 2) {
        value = READ_VID32(CASTLE_VID_ALPHA_CONTROL);
        pos = 16 + (gfx_alpha_select << 1);
        *priority = (int)((value >> pos) & 3);
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_color
 * 
 * This routine returns the color register value for the currently selected 
 * alpha region.  Bit 24 is set if the color register is enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
castle_get_alpha_color(unsigned long *color)
#else
void
gfx_get_alpha_color(unsigned long *color)
#endif
{
    *color = 0;
    if (gfx_alpha_select <= 2) {
        *color =
            READ_VID32(CASTLE_ALPHA_COLOR_1 +
            ((unsigned long)gfx_alpha_select << 5));
    }
    return;
}

#endif /* GFX_READ_ROUTINES */

/* END OF FILE */
