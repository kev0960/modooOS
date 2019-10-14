#include "kmalloc.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

TEST(kmallocTest, SimpleAllocAndFree) {
  kernel_memory_manager.Reset();
  void* mem1 = kmalloc(8);

  kfree(mem1);
  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
}

TEST(kmallocTest, MergeTwoChunksToOne) {
  kernel_memory_manager.Reset();
  void* mem1 = kmalloc(8);
  void* mem2 = kmalloc(8);

  kfree(mem1);
  kfree(mem2);

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
}

TEST(kmallocTest, MergeThreeChunksToOne) {
  kernel_memory_manager.Reset();
  void* mem1 = kmalloc(8);
  void* mem2 = kmalloc(8);
  void* mem3 = kmalloc(8);

  kfree(mem1);
  kfree(mem3);
  kfree(mem2);

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.ShowDebugInfo();
  kernel_memory_manager.DumpMemory();
}
TEST(kmallocTest, DecreaseSize) {
  kernel_memory_manager.Reset();
  for (int i = 50; i >= 1; i--) {
    void* mem = kmalloc(i * 10);
    kfree(mem);
  }

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
}

TEST(kmallocTest, IncreaseSize) {
  kernel_memory_manager.Reset();
  for (int i = 1; i <= 50; i++) {
    void* mem = kmalloc(i * 10);
    kfree(mem);
    EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  }

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.DumpMemory();
}

TEST(kmallocTest, LargeMemory) {
  kernel_memory_manager.Reset();
  for (int i = 1; i <= 10; i++) {
    void* mem = kmalloc(i * 1000000);
    kfree(mem);
    EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  }

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.DumpMemory();
}

TEST(kmallocTest, RandomSmall) {
  kernel_memory_manager.Reset();
  int mem_size[] = {583, 192, 119, 66, 964, 381, 150, 207, 718, 52};
  int actions[] = {2, 1, 5, 0, 5, 3, 1, 6, 2, 7, 6, 8, 7, 8, 4, 3, 9, 9, 0, 4};
  void* mem[10];

  for (int i = 0; i < 10; i++) {
    mem[i] = nullptr;
  }

  for (int i = 0; i < 20; i++) {
    int index = actions[i];
    if (!mem[index]) {
      mem[index] = kmalloc(mem_size[index]);
    } else {
      kfree(mem[index]);
    }
  }
  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.DumpMemory();
}

TEST(kmallocTest, RandomMedium) {
  kernel_memory_manager.Reset();
  constexpr int total_allocs = 100;
  int mem_size[total_allocs] = {
      2077, 6958, 8059, 7871, 663,  1689, 6676, 3084, 3320, 330,  6760, 8774,
      4057, 2645, 8767, 7676, 8584, 7951, 8329, 5610, 1365, 6394, 8896, 7362,
      8076, 1414, 6934, 2768, 3452, 5418, 695,  1874, 4176, 8821, 2987, 5831,
      8273, 4071, 3352, 9791, 2211, 3467, 8398, 8224, 4953, 3277, 8050, 4568,
      5399, 6695, 5983, 3077, 4715, 3675, 2663, 2088, 1384, 9563, 3406, 5823,
      334,  2853, 4995, 939,  2094, 2101, 1484, 160,  4533, 9839, 6289, 6278,
      5045, 3852, 7862, 6963, 4340, 9492, 1910, 380,  8251, 3629, 5251, 4033,
      5008, 6096, 8634, 1549, 6020, 6065, 2115, 2104, 8540, 973,  1855, 9332,
      2228, 3762, 3153, 2273};
  int actions[total_allocs * 2] = {
      78, 73, 65, 74, 14, 77, 86, 17, 8,  12, 5,  10, 15, 72, 93, 76, 55,
      73, 16, 96, 9,  28, 82, 67, 98, 70, 60, 94, 71, 71, 21, 60, 59, 18,
      72, 39, 33, 66, 76, 20, 27, 4,  89, 51, 29, 92, 69, 99, 41, 38, 46,
      25, 6,  55, 61, 13, 61, 81, 5,  45, 88, 75, 47, 31, 29, 43, 87, 28,
      83, 70, 23, 95, 63, 93, 50, 63, 79, 90, 9,  27, 15, 44, 30, 7,  34,
      22, 81, 17, 68, 20, 21, 99, 22, 68, 49, 86, 89, 3,  4,  58, 23, 85,
      24, 6,  64, 37, 57, 18, 58, 0,  10, 56, 25, 77, 19, 97, 47, 0,  16,
      88, 82, 14, 2,  13, 1,  26, 53, 11, 51, 49, 32, 35, 80, 75, 34, 66,
      46, 19, 98, 52, 69, 91, 95, 52, 11, 48, 90, 39, 40, 62, 83, 36, 42,
      24, 43, 97, 48, 32, 37, 87, 54, 3,  44, 7,  96, 53, 12, 40, 78, 38,
      50, 42, 74, 67, 30, 85, 91, 94, 1,  33, 2,  84, 80, 45, 92, 84, 31,
      79, 59, 35, 26, 36, 65, 41, 56, 57, 64, 8,  62, 54};

  void* mem[total_allocs];
  for (int i = 0; i < total_allocs; i++) {
    mem[i] = nullptr;
  }

  for (int i = 0; i < total_allocs * 2; i++) {
    int index = actions[i];
    if (!mem[index]) {
      mem[index] = kmalloc(mem_size[index]);
    } else {
      kfree(mem[index]);
    }
    EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  }

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.DumpMemory();
}

TEST(kmallocTest, RandomLarge) {
  kernel_memory_manager.Reset();
  constexpr int total_allocs = 500;
  int mem_size[total_allocs] = {
      8,    9757, 6167, 4753, 6720, 9689, 2824, 5218, 6716, 4019, 45,   7467,
      4075, 5987, 1395, 6344, 6676, 7132, 9502, 3795, 8659, 341,  9786, 762,
      3796, 1454, 7306, 516,  3930, 5888, 5418, 4561, 5748, 4759, 242,  4876,
      587,  7695, 458,  3948, 4953, 9172, 6426, 1210, 6435, 5503, 3311, 7841,
      8689, 2849, 5439, 3590, 919,  8417, 9858, 3469, 5392, 3888, 1514, 7616,
      619,  4517, 413,  2857, 7366, 5612, 3125, 8873, 1956, 395,  1993, 3179,
      368,  6857, 7831, 7493, 3420, 4145, 5396, 1996, 5888, 4567, 2539, 1781,
      9933, 4548, 7777, 5978, 6706, 6043, 169,  2604, 6373, 9706, 3695, 7197,
      7787, 6793, 4310, 2988, 1292, 4659, 862,  5627, 2446, 532,  159,  1102,
      6028, 6441, 7948, 1341, 4143, 6570, 2400, 9149, 6744, 393,  4944, 8663,
      6611, 9912, 2032, 7371, 661,  7833, 6778, 4404, 5737, 9319, 7963, 5089,
      5250, 8333, 7903, 1694, 8902, 1516, 2233, 6961, 8013, 5536, 1355, 9480,
      2625, 3865, 8356, 7856, 8324, 3577, 3195, 5347, 707,  9951, 9229, 4000,
      6494, 42,   4692, 8115, 8601, 6797, 9654, 1351, 6564, 8273, 268,  4339,
      4881, 3247, 4400, 8178, 6890, 4504, 8442, 66,   6064, 5494, 1506, 6965,
      2808, 6256, 845,  7383, 4565, 7408, 6987, 6794, 8943, 9443, 2885, 3939,
      243,  2581, 1790, 3647, 6172, 8043, 8306, 2145, 9972, 9722, 6324, 7211,
      4783, 5458, 1833, 4770, 493,  7540, 6045, 5072, 3320, 3068, 5896, 2628,
      9176, 2322, 8193, 7564, 9513, 9766, 3182, 7447, 4270, 5950, 8052, 7449,
      7922, 8426, 827,  5292, 1662, 2845, 238,  7275, 1842, 9154, 1767, 6229,
      135,  7506, 4464, 314,  6902, 4978, 9567, 3149, 6721, 5469, 1867, 2711,
      895,  945,  2714, 3936, 8600, 741,  9443, 9358, 8224, 277,  1630, 2331,
      6554, 4753, 1436, 9863, 2519, 4688, 1967, 9544, 4393, 1557, 4701, 5458,
      1611, 5561, 5858, 8728, 7521, 7650, 654,  9670, 8870, 2257, 1301, 2726,
      7768, 3160, 1577, 9296, 7252, 3040, 221,  3892, 1315, 4049, 5436, 4724,
      4266, 1536, 3020, 5580, 7298, 4066, 2514, 6610, 2467, 1138, 3009, 6050,
      1734, 6706, 4290, 4191, 4165, 7022, 8882, 8392, 5009, 3709, 1702, 9797,
      4885, 8809, 396,  9713, 1577, 9422, 3731, 9141, 2759, 2666, 2958, 5971,
      3025, 2465, 2027, 1471, 8286, 3791, 359,  2626, 3295, 928,  1799, 3261,
      3360, 3759, 6811, 9439, 6451, 8748, 824,  7706, 8708, 2756, 5236, 5129,
      7295, 6749, 4299, 2734, 5716, 6144, 5858, 4475, 4667, 8978, 6374, 8110,
      6192, 3422, 8134, 7710, 9587, 3474, 6569, 3692, 8145, 8597, 8016, 4899,
      7016, 8877, 1118, 7436, 102,  341,  6555, 3697, 3034, 2,    8492, 6183,
      1403, 2873, 8601, 2671, 1434, 378,  8229, 7885, 4621, 5241, 139,  572,
      7782, 7491, 2060, 1463, 3130, 7093, 5046, 2065, 5966, 5541, 3449, 5492,
      9791, 6211, 2648, 2838, 3881, 3811, 3005, 6174, 5867, 607,  6652, 3493,
      1138, 5137, 5106, 6192, 6254, 1012, 4185, 4301, 1912, 1361, 5849, 1502,
      8523, 1547, 2760, 5985, 8969, 9469, 1914, 3246, 3963, 3329, 4577, 8982,
      6115, 9653, 6796, 8423, 2835, 2969, 2354, 5191, 2617, 683,  4678, 3957,
      1902, 8942, 6749, 3676, 4102, 373,  8660, 1931, 4171, 1885, 358,  9499,
      574,  4956, 1179, 3270, 2017, 1903, 3333, 8950, 5732, 2119, 7105, 7646,
      6307, 542,  3366, 9072, 6933, 8065, 827,  3315};
  int actions[total_allocs * 2] = {
      137, 186, 233, 151, 482, 87,  230, 304, 41,  340, 242, 448, 447, 30,  299,
      130, 345, 194, 292, 450, 309, 70,  425, 116, 472, 240, 191, 498, 398, 125,
      398, 89,  22,  492, 364, 36,  141, 288, 481, 252, 338, 374, 491, 216, 199,
      476, 224, 426, 336, 474, 211, 271, 35,  425, 477, 369, 10,  276, 406, 175,
      58,  317, 430, 56,  489, 437, 99,  456, 401, 129, 281, 479, 133, 219, 322,
      436, 218, 472, 117, 23,  115, 341, 155, 193, 449, 360, 465, 46,  160, 20,
      467, 160, 261, 379, 207, 443, 381, 147, 274, 127, 246, 388, 184, 1,   22,
      320, 375, 434, 44,  64,  8,   356, 298, 111, 161, 120, 362, 112, 296, 60,
      203, 355, 63,  27,  381, 148, 495, 258, 290, 462, 105, 127, 139, 146, 197,
      103, 412, 88,  368, 265, 204, 491, 417, 285, 318, 241, 108, 258, 328, 261,
      494, 353, 202, 330, 284, 105, 394, 305, 319, 410, 260, 345, 404, 397, 5,
      342, 28,  220, 166, 62,  478, 294, 479, 163, 426, 2,   473, 73,  221, 226,
      49,  382, 281, 3,   217, 456, 85,  75,  359, 17,  168, 344, 222, 383, 128,
      325, 474, 164, 414, 235, 285, 486, 396, 90,  234, 293, 68,  136, 329, 137,
      368, 38,  349, 73,  14,  372, 420, 386, 270, 451, 143, 247, 146, 436, 322,
      444, 363, 384, 147, 488, 340, 77,  400, 466, 299, 461, 454, 457, 397, 408,
      357, 432, 15,  329, 434, 120, 286, 221, 53,  466, 497, 167, 385, 215, 427,
      196, 282, 442, 337, 152, 176, 451, 192, 290, 162, 72,  185, 12,  442, 460,
      163, 323, 94,  252, 154, 25,  401, 431, 157, 6,   271, 47,  109, 26,  373,
      295, 175, 39,  264, 11,  200, 40,  27,  430, 276, 3,   317, 140, 269, 453,
      34,  96,  239, 229, 53,  19,  183, 250, 278, 185, 280, 493, 487, 234, 13,
      365, 365, 254, 238, 107, 498, 495, 292, 57,  124, 89,  50,  179, 350, 313,
      133, 97,  200, 415, 312, 394, 79,  30,  334, 318, 341, 82,  463, 59,  389,
      229, 190, 104, 449, 279, 93,  482, 6,   387, 20,  254, 306, 370, 476, 269,
      209, 279, 142, 435, 7,   155, 231, 295, 364, 74,  366, 455, 64,  404, 433,
      219, 195, 280, 418, 113, 118, 315, 284, 255, 65,  168, 0,   228, 81,  334,
      446, 148, 440, 244, 390, 499, 305, 182, 355, 348, 164, 423, 454, 266, 344,
      232, 80,  43,  172, 181, 243, 321, 54,  18,  320, 469, 471, 192, 248, 174,
      60,  111, 29,  267, 194, 177, 100, 153, 419, 83,  184, 86,  241, 487, 106,
      231, 239, 328, 264, 170, 288, 9,   393, 121, 431, 74,  201, 489, 291, 211,
      272, 477, 247, 439, 169, 166, 79,  36,  303, 43,  382, 21,  204, 273, 392,
      348, 324, 308, 464, 47,  391, 227, 237, 173, 37,  67,  98,  193, 213, 405,
      326, 179, 235, 199, 256, 84,  417, 339, 123, 312, 485, 373, 433, 359, 409,
      31,  253, 257, 51,  360, 42,  52,  56,  369, 443, 467, 307, 126, 471, 399,
      110, 323, 422, 391, 255, 101, 25,  447, 333, 171, 296, 145, 351, 277, 277,
      212, 408, 468, 114, 361, 225, 2,   352, 4,   10,  438, 303, 140, 37,  91,
      422, 376, 358, 86,  267, 313, 232, 161, 106, 272, 473, 98,  459, 114, 32,
      72,  107, 197, 347, 95,  39,  460, 150, 462, 134, 45,  177, 282, 216, 55,
      205, 67,  428, 9,   102, 129, 104, 172, 70,  311, 380, 245, 298, 326, 82,
      50,  62,  379, 289, 300, 189, 4,   149, 480, 310, 95,  236, 13,  427, 178,
      76,  96,  301, 61,  441, 159, 286, 206, 143, 371, 142, 403, 208, 452, 34,
      238, 195, 332, 209, 490, 69,  69,  210, 316, 205, 126, 314, 402, 134, 488,
      23,  243, 331, 339, 49,  335, 119, 445, 354, 420, 149, 178, 392, 5,   176,
      40,  214, 497, 372, 138, 71,  0,   416, 203, 156, 336, 198, 440, 407, 266,
      174, 421, 83,  88,  275, 141, 306, 110, 492, 116, 113, 112, 125, 181, 78,
      173, 493, 435, 461, 68,  301, 207, 244, 424, 121, 429, 206, 406, 249, 480,
      250, 123, 214, 445, 388, 407, 196, 327, 32,  418, 332, 270, 236, 475, 158,
      291, 321, 316, 377, 268, 246, 102, 485, 233, 438, 287, 419, 14,  459, 44,
      97,  409, 337, 54,  226, 338, 248, 51,  465, 218, 90,  132, 490, 260, 165,
      302, 198, 11,  80,  367, 352, 413, 366, 215, 346, 370, 483, 330, 38,  152,
      335, 212, 135, 399, 187, 441, 240, 99,  48,  132, 189, 483, 217, 343, 94,
      358, 131, 251, 103, 385, 91,  387, 188, 357, 275, 202, 424, 225, 77,  100,
      213, 347, 144, 294, 478, 81,  92,  268, 208, 183, 310, 153, 377, 386, 390,
      245, 180, 416, 262, 486, 413, 55,  333, 403, 331, 253, 293, 157, 356, 167,
      130, 367, 346, 343, 458, 315, 380, 45,  151, 52,  444, 421, 124, 180, 353,
      496, 362, 257, 395, 448, 119, 228, 156, 28,  470, 191, 210, 128, 48,  361,
      19,  470, 259, 393, 274, 109, 455, 190, 283, 16,  29,  154, 415, 314, 458,
      429, 131, 283, 58,  262, 319, 363, 494, 412, 309, 171, 186, 411, 428, 169,
      396, 182, 66,  35,  1,   159, 414, 46,  101, 230, 251, 242, 349, 117, 374,
      223, 75,  395, 450, 324, 17,  84,  187, 308, 7,   463, 378, 224, 115, 289,
      201, 400, 384, 327, 85,  499, 350, 162, 165, 66,  452, 8,   61,  265, 18,
      65,  33,  227, 484, 150, 437, 41,  33,  411, 475, 439, 402, 446, 311, 108,
      24,  93,  223, 371, 297, 87,  354, 351, 259, 405, 237, 118, 12,  300, 15,
      342, 139, 376, 16,  378, 389, 26,  453, 136, 42,  71,  496, 302, 383, 57,
      122, 138, 256, 63,  135, 145, 307, 144, 263, 76,  249, 21,  122, 24,  481,
      273, 78,  484, 469, 158, 287, 375, 423, 222, 59,  297, 464, 432, 31,  263,
      457, 410, 220, 468, 325, 170, 188, 92,  304, 278};

  void* mem[total_allocs];
  for (int i = 0; i < total_allocs; i++) {
    mem[i] = nullptr;
  }

  for (int i = 0; i < total_allocs * 2; i++) {
    int index = actions[i];
    if (!mem[index]) {
      mem[index] = kmalloc(mem_size[index]);
    } else {
      kfree(mem[index]);
    }
    EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  }

  EXPECT_TRUE(kernel_memory_manager.SanityCheck());
  kernel_memory_manager.DumpMemory();
}

}  // namespace kernel_test
}  // namespace Kernel
