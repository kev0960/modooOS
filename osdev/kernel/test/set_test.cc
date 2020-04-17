#include "../std/set.h"
#include "../std/map.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

TEST(TreeTest, SimpleInsert) {
  std::BinaryTreeNode<int> node1(2);
  std::BinaryTreeNode<int> node2(1);
  std::BinaryTreeNode<int> node3(3);

  EXPECT_TRUE(node1.NumChild() == 0);

  node1.SetLeft(&node2);
  EXPECT_TRUE(node1.NumChild() == 1);

  node1.SetRight(&node3);
  EXPECT_TRUE(node1.NumChild() == 2);

  node1.RemoveChild(&node2);
  EXPECT_TRUE(node1.Left() == nullptr);
  EXPECT_TRUE(node1.NumChild() == 1);

  EXPECT_EQ(node1.GetOnlyChild(), &node3);

  EXPECT_EQ(node1.AddChild(&node2), nullptr);
  EXPECT_EQ(node1.Left(), &node2);
}

template <typename T>
bool CheckExist(std::set<T>& s, T* data, int num_data) {
  for (int i = 0; i < num_data; i++) {
    if (s.count(data[i]) == 0) {
      kprintf("Not found! %d \n", data[i]);
      return false;
    }
  }
  return true;
}

TEST(SetTest, SimpleInsert) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(3);
  s.insert(6);
  s.insert(5);
  s.insert(4);
  s.insert(8);

  int data[] = {1, 2, 3, 4, 5, 6, 8};
  int i = 0;
  for (auto itr = s.begin(); itr != s.end(); ++itr, ++i) {
    EXPECT_EQ(*itr, data[i]);
  }

  for (int i = 0; i < 7; i++) {
    EXPECT_TRUE(s.count(data[i]) == 1);
  }
  EXPECT_TRUE(s.count(7) == 0);
  EXPECT_TRUE(s.count(0) == 0);
}

TEST(SetTest, SimpleEraseTest) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(3);

  s.erase(1);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 1);
  EXPECT_TRUE(s.count(3) == 1);

  s.erase(2);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 0);
  EXPECT_TRUE(s.count(3) == 1);

  s.erase(3);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 0);
  EXPECT_TRUE(s.count(3) == 0);
}

//           2
//        1       4
//            3      10
//                 6    12
//                   7
TEST(SetTest, EraseTest) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(4);
  s.insert(3);
  s.insert(10);
  s.insert(6);
  s.insert(12);
  s.insert(7);

  int data1[] = {1, 2, 3, 4, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data1, 8));

  s.erase(4);

  int data2[] = {1, 2, 3, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data2, 7));

  s.erase(2);
  int data3[] = {1, 3, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data3, 6));

  s.erase(6);
  int data4[] = {1, 3, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data4, 5));

  s.erase(1);
  int data5[] = {3, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data5, 4));

  s.erase(12);
  int data6[] = {3, 7, 10};
  EXPECT_TRUE(CheckExist(s, data6, 3));
}

static int A_dest_cnt = 0;

struct A {
  bool operator<(const A& a) const { return data < a.data; }
  bool operator>(const A& a) const { return data > a.data; }
  bool operator==(const A& a) const { return data == a.data; }

  A(int data) : data(data) {}
  ~A() { A_dest_cnt++; }

  int data;
};

TEST(SetTest, RemoveAllOnDestruction) {
  {
    std::set<A> s;
    s.insert(A(3));
    s.insert(A(1));
    s.insert(A(5));
    s.insert(A(4));
    s.insert(A(2));
  }

  // Dest when copy constructing, and then when s gets destroyed.
  EXPECT_EQ(A_dest_cnt, 10);
}

void SanityCheck(const std::set<int>& s, bool* is_in, size_t num) {
  for (size_t i = 0; i < num; i++) {
    if (is_in[i]) {
      EXPECT_EQ(s.count(i), 1u);
      EXPECT_EQ(*s.find(i), (int)i);
    } else {
      EXPECT_EQ(s.count(i), 0u);
      EXPECT_EQ(s.find(i), s.end());
    }
  }
}

TEST(SetTest, RandomInsertAndRemoveTestSmall) {
  int data[] = {9, 3, 1, 3, 1, 4, 6, 2, 7, 7, 2, 5, 0, 5, 4, 8, 6, 0, 9, 8};
  bool is_in[10] = {0};

  std::set<int> s;
  for (int i = 0; i < 20; i++) {
    if (is_in[data[i]]) {
      is_in[data[i]] = false;
      s.erase(data[i]);

      SanityCheck(s, is_in, 10);
    } else {
      is_in[data[i]] = true;
      s.insert(data[i]);
    }
  }
}

TEST(SetTest, RandomInsertAndRemoveTestMedium) {
  int data[] = {64, 8,  86, 85, 1,  68, 21, 29, 78, 74, 67, 12, 16, 20, 18, 56,
                17, 77, 42, 7,  46, 43, 91, 28, 19, 22, 70, 91, 93, 23, 53, 93,
                26, 61, 94, 71, 44, 30, 95, 59, 37, 69, 3,  62, 69, 99, 47, 13,
                78, 33, 50, 22, 52, 37, 96, 23, 9,  14, 58, 88, 60, 28, 39, 83,
                4,  5,  24, 60, 8,  47, 27, 0,  53, 40, 57, 92, 2,  51, 39, 34,
                80, 64, 89, 4,  36, 38, 38, 54, 90, 6,  61, 33, 27, 35, 49, 59,
                79, 44, 48, 35, 84, 11, 25, 29, 95, 32, 20, 79, 85, 98, 17, 46,
                36, 10, 89, 65, 21, 34, 97, 66, 76, 40, 9,  67, 2,  84, 87, 49,
                86, 88, 3,  0,  73, 32, 68, 45, 18, 56, 45, 50, 26, 90, 55, 41,
                65, 81, 1,  55, 77, 14, 15, 92, 25, 31, 16, 81, 76, 57, 99, 71,
                12, 10, 75, 54, 63, 13, 51, 83, 72, 5,  70, 6,  43, 63, 80, 75,
                96, 82, 74, 42, 41, 73, 66, 97, 98, 82, 24, 15, 72, 52, 30, 19,
                48, 94, 62, 31, 7,  58, 11, 87};

  bool is_in[100] = {0};

  std::set<int> s;
  for (int i = 0; i < 200; i++) {
    if (is_in[data[i]]) {
      is_in[data[i]] = false;
      s.erase(data[i]);

      SanityCheck(s, is_in, 100);
    } else {
      is_in[data[i]] = true;
      s.insert(data[i]);
    }
  }
}

TEST(SetTest, RandomInsertAndRemoveTestLarge) {
  int data[] = {
      791, 279, 48,  494, 962, 97,  230, 886, 438, 372, 418, 189, 164, 101, 861,
      196, 553, 18,  379, 366, 665, 187, 568, 534, 369, 493, 383, 226, 15,  464,
      141, 76,  25,  343, 963, 136, 900, 796, 413, 817, 146, 893, 236, 175, 800,
      366, 983, 836, 304, 898, 974, 962, 355, 682, 472, 770, 319, 479, 967, 843,
      189, 507, 693, 422, 598, 32,  471, 984, 185, 994, 466, 420, 686, 961, 748,
      144, 776, 265, 970, 198, 978, 658, 56,  745, 984, 173, 172, 63,  739, 206,
      669, 133, 587, 891, 761, 374, 398, 531, 680, 876, 797, 132, 493, 93,  447,
      448, 453, 242, 352, 311, 585, 810, 606, 8,   668, 448, 685, 826, 874, 700,
      621, 317, 938, 552, 700, 397, 234, 625, 546, 657, 628, 219, 373, 690, 642,
      307, 790, 35,  729, 56,  686, 241, 243, 572, 518, 766, 917, 481, 541, 66,
      334, 887, 463, 238, 336, 390, 559, 759, 253, 451, 53,  326, 278, 98,  489,
      443, 477, 55,  218, 561, 538, 581, 111, 294, 51,  937, 582, 790, 804, 177,
      392, 529, 704, 636, 282, 776, 154, 872, 17,  719, 960, 426, 858, 259, 955,
      129, 270, 347, 640, 57,  110, 482, 385, 787, 751, 441, 58,  147, 312, 785,
      951, 794, 75,  547, 321, 702, 948, 871, 896, 892, 593, 940, 936, 180, 505,
      543, 63,  225, 993, 946, 367, 825, 820, 566, 256, 843, 286, 277, 402, 178,
      38,  899, 155, 725, 845, 854, 470, 470, 514, 557, 633, 813, 19,  117, 508,
      855, 85,  616, 854, 902, 28,  603, 43,  783, 224, 298, 38,  569, 760, 84,
      826, 892, 868, 485, 713, 755, 204, 949, 433, 212, 919, 586, 612, 995, 210,
      26,  478, 332, 441, 844, 89,  215, 858, 874, 792, 868, 555, 699, 545, 426,
      217, 181, 681, 487, 197, 79,  197, 162, 81,  152, 937, 981, 388, 675, 5,
      391, 726, 510, 384, 86,  481, 199, 964, 460, 644, 934, 396, 611, 109, 674,
      667, 193, 356, 241, 494, 576, 957, 333, 399, 103, 698, 430, 418, 439, 417,
      963, 674, 143, 400, 881, 914, 37,  695, 521, 316, 408, 3,   737, 477, 672,
      110, 353, 192, 371, 659, 302, 7,   257, 883, 619, 767, 357, 495, 459, 848,
      539, 503, 768, 65,  421, 894, 818, 125, 525, 942, 474, 910, 449, 643, 570,
      772, 820, 476, 199, 652, 5,   262, 408, 362, 7,   722, 730, 300, 33,  736,
      504, 990, 734, 207, 276, 989, 269, 979, 689, 113, 951, 920, 135, 952, 758,
      74,  946, 665, 473, 620, 923, 254, 465, 334, 572, 710, 143, 188, 116, 386,
      67,  445, 633, 636, 83,  999, 203, 175, 471, 80,  604, 897, 0,   975, 589,
      294, 746, 255, 83,  609, 539, 601, 389, 314, 570, 394, 222, 500, 310, 678,
      921, 765, 667, 789, 588, 617, 560, 54,  383, 134, 964, 728, 880, 156, 129,
      246, 2,   885, 976, 353, 528, 656, 630, 895, 308, 497, 249, 769, 380, 156,
      800, 232, 967, 204, 835, 522, 338, 111, 29,  423, 831, 11,  457, 346, 44,
      122, 663, 958, 573, 525, 371, 668, 723, 245, 486, 306, 478, 452, 641, 977,
      213, 809, 605, 935, 244, 434, 307, 805, 77,  130, 257, 717, 836, 825, 703,
      125, 398, 610, 337, 280, 523, 181, 271, 906, 839, 506, 23,  121, 567, 176,
      602, 930, 402, 265, 558, 602, 837, 278, 513, 840, 184, 414, 394, 425, 626,
      9,   727, 390, 533, 791, 648, 512, 511, 244, 432, 657, 596, 851, 285, 303,
      645, 108, 214, 838, 866, 883, 911, 855, 749, 405, 81,  312, 211, 975, 759,
      455, 672, 61,  151, 998, 727, 703, 435, 64,  575, 847, 502, 809, 90,  684,
      158, 666, 102, 913, 877, 382, 629, 446, 921, 43,  327, 590, 675, 345, 576,
      320, 862, 631, 696, 720, 530, 745, 339, 594, 464, 264, 18,  236, 933, 807,
      68,  416, 202, 37,  638, 638, 275, 841, 348, 379, 518, 348, 734, 526, 331,
      595, 985, 429, 950, 137, 615, 327, 715, 118, 973, 669, 499, 331, 954, 62,
      332, 697, 287, 248, 351, 311, 349, 550, 284, 419, 78,  466, 176, 557, 122,
      922, 985, 792, 40,  260, 68,  618, 459, 770, 827, 689, 591, 852, 687, 969,
      316, 221, 295, 46,  380, 75,  737, 284, 982, 606, 88,  140, 521, 875, 721,
      126, 592, 354, 107, 740, 454, 170, 492, 612, 720, 325, 806, 971, 458, 753,
      310, 445, 65,  714, 428, 821, 235, 870, 846, 384, 397, 237, 163, 895, 781,
      492, 613, 596, 578, 60,  643, 365, 34,  857, 424, 647, 40,  780, 655, 679,
      160, 760, 642, 680, 617, 568, 157, 846, 267, 496, 879, 301, 779, 959, 443,
      859, 944, 956, 534, 330, 757, 931, 219, 66,  171, 232, 859, 654, 634, 106,
      270, 819, 711, 295, 773, 637, 999, 876, 177, 96,  375, 216, 741, 673, 338,
      651, 367, 875, 387, 736, 803, 39,  52,  378, 101, 793, 908, 259, 627, 559,
      646, 229, 815, 635, 440, 403, 532, 375, 335, 393, 24,  655, 288, 847, 227,
      233, 924, 915, 891, 648, 475, 650, 363, 952, 500, 604, 239, 532, 31,  191,
      480, 229, 550, 210, 304, 109, 87,  447, 274, 660, 508, 832, 423, 972, 728,
      142, 696, 702, 150, 437, 203, 929, 829, 877, 869, 777, 706, 165, 341, 566,
      928, 29,  634, 428, 662, 442, 714, 201, 28,  483, 912, 535, 614, 939, 524,
      374, 162, 365, 685, 811, 656, 687, 444, 504, 112, 272, 263, 299, 533, 469,
      888, 41,  767, 912, 4,   688, 519, 355, 618, 607, 544, 352, 182, 565, 4,
      13,  719, 562, 857, 878, 84,  961, 610, 773, 395, 424, 292, 542, 556, 16,
      651, 530, 660, 431, 747, 782, 50,  987, 645, 786, 31,  427, 678, 128, 637,
      813, 664, 888, 548, 301, 404, 456, 706, 933, 104, 306, 654, 1,   138, 282,
      336, 904, 849, 878, 670, 39,  775, 616, 163, 475, 482, 862, 458, 838, 870,
      155, 11,  673, 317, 480, 593, 157, 30,  299, 716, 400, 103, 145, 938, 403,
      890, 811, 986, 802, 799, 450, 436, 99,  527, 205, 905, 625, 220, 364, 153,
      902, 461, 768, 309, 903, 917, 212, 884, 924, 260, 8,   523, 966, 829, 95,
      624, 339, 932, 558, 141, 388, 27,  677, 889, 308, 839, 59,  169, 699, 769,
      715, 716, 998, 815, 583, 812, 415, 213, 783, 342, 10,  283, 499, 293, 491,
      32,  798, 690, 345, 641, 899, 439, 810, 754, 919, 80,  486, 758, 887, 231,
      483, 598, 867, 411, 268, 943, 738, 923, 966, 119, 953, 918, 64,  552, 139,
      406, 95,  909, 100, 266, 406, 411, 600, 167, 619, 818, 359, 405, 735, 178,
      240, 72,  725, 692, 216, 223, 733, 522, 832, 174, 688, 871, 676, 171, 58,
      608, 289, 697, 89,  454, 44,  149, 47,  925, 647, 145, 321, 956, 627, 652,
      849, 778, 827, 96,  45,  434, 965, 279, 994, 132, 605, 357, 252, 936, 142,
      724, 251, 378, 194, 328, 718, 765, 661, 614, 824, 370, 487, 930, 624, 385,
      644, 990, 361, 795, 119, 286, 290, 131, 169, 799, 205, 909, 569, 712, 209,
      356, 873, 341, 267, 903, 944, 574, 852, 460, 653, 303, 730, 112, 897, 170,
      179, 300, 988, 302, 574, 350, 202, 915, 376, 750, 10,  775, 409, 146, 231,
      431, 748, 90,  484, 971, 26,  881, 620, 788, 126, 578, 742, 363, 412, 968,
      772, 551, 161, 118, 757, 50,  916, 900, 276, 764, 823, 671, 49,  777, 192,
      456, 831, 694, 601, 161, 305, 816, 70,  328, 692, 863, 514, 315, 691, 168,
      381, 797, 630, 779, 949, 255, 349, 21,  537, 250, 646, 20,  20,  382, 635,
      682, 221, 188, 939, 679, 337, 718, 901, 693, 529, 69,  599, 61,  607, 133,
      516, 683, 484, 451, 623, 658, 781, 571, 372, 970, 664, 17,  833, 134, 150,
      77,  305, 87,  344, 16,  281, 581, 666, 415, 774, 928, 92,  801, 733, 275,
      873, 830, 208, 368, 1,   988, 180, 977, 23,  113, 707, 108, 545, 747, 218,
      817, 510, 172, 661, 711, 296, 911, 996, 36,  368, 845, 107, 886, 865, 560,
      42,  513, 71,  287, 739, 271, 360, 266, 588, 823, 85,  97,  14,  632, 164,
      519, 989, 88,  35,  457, 467, 24,  99,  455, 782, 615, 437, 546, 726, 217,
      863, 446, 764, 193, 834, 354, 333, 245, 536, 323, 535, 427, 283, 717, 830,
      904, 784, 281, 226, 82,  344, 324, 468, 438, 945, 527, 488, 115, 850, 463,
      732, 22,  315, 609, 907, 254, 46,  597, 556, 806, 889, 488, 575, 544, 183,
      22,  867, 322, 524, 731, 982, 144, 640, 837, 869, 709, 528, 496, 329, 793,
      298, 127, 537, 117, 511, 186, 25,  239, 653, 540, 580, 395, 860, 671, 135,
      495, 808, 350, 223, 972, 128, 540, 452, 469, 369, 233, 190, 124, 291, 73,
      473, 743, 206, 49,  729, 762, 784, 932, 515, 453, 943, 183, 639, 131, 420,
      224, 613, 94,  440, 864, 577, 721, 429, 425, 106, 898, 462, 885, 763, 381,
      553, 707, 713, 501, 73,  577, 631, 579, 738, 571, 600, 821, 154, 709, 490,
      913, 215, 376, 621, 766, 724, 973, 512, 723, 698, 746, 787, 76,  140, 597,
      467, 801, 579, 554, 708, 822, 57,  893, 102, 450, 498, 21,  277, 360, 880,
      756, 182, 247, 12,  476, 541, 623, 955, 449, 649, 940, 14,  542, 608, 732,
      753, 323, 105, 158, 586, 722, 927, 860, 0,   925, 506, 243, 430, 173, 683,
      491, 159, 261, 54,  931, 12,  650, 186, 105, 563, 526, 866, 325, 905, 209,
      991, 220, 622, 828, 996, 3,   749, 691, 662, 882, 735, 412, 237, 313, 754,
      861, 362, 159, 965, 752, 6,   195, 228, 701, 761, 228, 137, 835, 947, 422,
      252, 41,  319, 926, 399, 273, 320, 591, 603, 242, 377, 910, 547, 563, 585,
      230, 520, 114, 808, 976, 584, 194, 824, 185, 981, 639, 997, 318, 359, 554,
      969, 166, 123, 958, 324, 992, 91,  59,  983, 152, 13,  291, 841, 851, 987,
      573, 763, 991, 285, 594, 472, 100, 916, 33,  234, 139, 626, 589, 272, 149,
      507, 165, 908, 914, 548, 997, 130, 27,  740, 611, 115, 489, 124, 853, 444,
      918, 264, 387, 71,  957, 386, 60,  580, 78,  329, 248, 992, 9,   565, 410,
      247, 322, 465, 980, 755, 934, 515, 343, 840, 853, 622, 116, 250, 945, 979,
      82,  93,  834, 771, 120, 743, 127, 166, 844, 198, 922, 786, 432, 160, 401,
      208, 274, 705, 6,   676, 628, 812, 433, 326, 404, 79,  238, 993, 258, 670,
      474, 785, 517, 246, 959, 200, 485, 190, 136, 505, 114, 442, 335, 752, 401,
      479, 55,  273, 92,  750, 98,  196, 842, 249, 414, 848, 708, 856, 421, 814,
      896, 330, 389, 47,  649, 148, 468, 42,  584, 200, 788, 920, 490, 503, 19,
      516, 701, 227, 742, 663, 704, 941, 684, 417, 174, 340, 509, 926, 814, 396,
      778, 796, 347, 555, 91,  789, 269, 497, 590, 805, 253, 392, 795, 804, 86,
      974, 677, 292, 948, 774, 53,  30,  567, 15,  802, 251, 995, 351, 582, 587,
      980, 509, 517, 551, 632, 744, 413, 629, 72,  816, 167, 410, 564, 36,  694,
      262, 929, 599, 222, 393, 261, 34,  947, 419, 953, 45,  941, 290, 187, 373,
      258, 67,  595, 207, 289, 74,  549, 520, 225, 120, 235, 461, 313, 756, 268,
      256, 498, 501, 705, 907, 293, 842, 807, 531, 138, 364, 502, 950, 104, 798,
      94,  51,  901, 762, 803, 318, 48,  850, 744, 121, 890, 377, 69,  407, 884,
      549, 856, 543, 62,  191, 297, 935, 695, 731, 536, 978, 263, 879, 148, 562,
      592, 391, 153, 822, 659, 370, 942, 710, 52,  986, 712, 346, 123, 409, 882,
      70,  751, 342, 561, 894, 240, 462, 865, 968, 564, 214, 201, 314, 927, 771,
      179, 361, 538, 819, 741, 297, 780, 828, 681, 280, 358, 288, 583, 864, 184,
      794, 151, 2,   954, 211, 147, 340, 168, 309, 436, 407, 358, 906, 416, 872,
      833, 195, 435, 960, 296};

  bool is_in[1000] = {0};

  std::set<int> s;
  for (int i = 0; i < 2000; i++) {
    if (is_in[data[i]]) {
      is_in[data[i]] = false;
      s.erase(data[i]);

      SanityCheck(s, is_in, 1000);
    } else {
      is_in[data[i]] = true;
      s.insert(data[i]);
    }
  }

  // Check iterator works properly.
  for (int i = 0; i < 2000; i++) {
    s.insert(data[i]);
  }

  int i = 0;
  for (auto itr = s.begin(); itr != s.end(); ++itr, ++i) {
    EXPECT_EQ(*itr, i);
  }
}

void MapSanityCheck(std::map<int, int>& m, int* state, int num) {
  for (int i = 0; i < num; i++) {
    if (state[i] == 0 || state[i] == 3) {
      EXPECT_TRUE(m.find(i) == m.end());
    } else if (state[i] == 1) {
      EXPECT_TRUE(m.find(i)->second == i + 1);
    } else if (state[i] == 2) {
      EXPECT_TRUE(m.find(i)->second == 2 * (i + 1));
    }
  }
}

TEST(MapTest, MapIteratorTest) {
  std::map<int, int> m;
  const int num = 10;
  int data[] = {9, 4, 7, 0, 6, 3, 1, 0, 8, 6, 9, 9, 2, 5, 1,
                8, 1, 6, 5, 7, 4, 3, 2, 7, 8, 3, 4, 5, 0, 2};
  int state[num] = {0};

  for (int i = 0; i < 3 * num; i++) {
    if (state[data[i]] == 0) {
      m[data[i]] = data[i] + 1;
    } else if (state[data[i]] == 1) {
      m[data[i]] *= 2;
    } else if (state[data[i]] == 2) {
      m[data[i]] += 2;
    }

    state[data[i]]++;
  }

  int i = 0;
  for (auto itr = m.begin(); itr != m.end(); ++itr, ++i) {
    EXPECT_EQ(itr->first, i);
    EXPECT_EQ(itr->second, (i + 1) * 2 + 2);

    itr->second++;
  }

  i = 0;
  for (auto itr = m.begin(); itr != m.end(); ++itr, ++i) {
    EXPECT_EQ(itr->first, i);
    EXPECT_EQ(itr->second, (i + 1) * 2 + 3);
  }
}

TEST(MapTest, RandomMapTestSmall) {
  std::map<int, int> m;

  const int num = 10;

  int data[] = {9, 4, 7, 0, 6, 3, 1, 0, 8, 6, 9, 9, 2, 5, 1,
                8, 1, 6, 5, 7, 4, 3, 2, 7, 8, 3, 4, 5, 0, 2};
  int state[num] = {0};

  for (int i = 0; i < 3 * num; i++) {
    if (state[data[i]] == 0) {
      m[data[i]] = data[i] + 1;
    } else if (state[data[i]] == 1) {
      m[data[i]] *= 2;
    } else {
      m.erase(data[i]);
    }
    state[data[i]]++;
    MapSanityCheck(m, state, num);
  }
}

TEST(MapTest, RandomMapTestMedium) {
  std::map<int, int> m;

  const int num = 100;

  int data[] = {
      29, 84, 39, 99, 90, 48, 86, 85, 28, 1,  36, 17, 31, 10, 56, 12, 49, 7,
      23, 5,  48, 43, 43, 8,  30, 59, 25, 79, 41, 12, 75, 70, 72, 15, 52, 20,
      31, 2,  3,  99, 26, 37, 95, 21, 61, 64, 67, 76, 26, 77, 11, 74, 68, 22,
      96, 65, 88, 89, 84, 75, 3,  50, 66, 49, 57, 32, 82, 13, 97, 54, 76, 44,
      63, 57, 95, 50, 47, 60, 15, 38, 28, 12, 6,  62, 46, 83, 56, 63, 4,  63,
      40, 38, 74, 92, 13, 52, 67, 52, 62, 87, 67, 82, 37, 45, 41, 85, 18, 42,
      42, 86, 50, 69, 35, 54, 26, 33, 88, 35, 44, 75, 10, 39, 79, 73, 97, 24,
      38, 81, 51, 5,  85, 14, 0,  68, 64, 45, 55, 9,  93, 25, 82, 81, 53, 4,
      32, 80, 87, 83, 11, 97, 58, 21, 94, 46, 47, 58, 69, 53, 84, 13, 70, 41,
      98, 91, 16, 96, 16, 21, 2,  28, 47, 78, 73, 42, 91, 4,  44, 66, 86, 78,
      83, 95, 5,  81, 22, 34, 62, 89, 24, 60, 65, 77, 40, 99, 92, 98, 23, 51,
      87, 71, 45, 15, 40, 1,  90, 57, 6,  14, 19, 18, 17, 55, 43, 46, 24, 31,
      59, 20, 78, 27, 27, 76, 33, 58, 89, 98, 7,  19, 36, 6,  53, 54, 18, 68,
      96, 30, 73, 3,  9,  49, 72, 36, 51, 10, 19, 48, 29, 11, 55, 37, 90, 17,
      0,  59, 30, 80, 34, 61, 71, 77, 64, 34, 65, 25, 27, 20, 39, 91, 92, 60,
      0,  79, 69, 70, 14, 88, 2,  1,  32, 94, 94, 61, 8,  29, 16, 93, 8,  23,
      66, 9,  71, 7,  74, 80, 56, 35, 72, 33, 93, 22};
  int state[num] = {0};

  for (int i = 0; i < 3 * num; i++) {
    if (state[data[i]] == 0) {
      m[data[i]] = data[i] + 1;
    } else if (state[data[i]] == 1) {
      m[data[i]] *= 2;
    } else {
      m.erase(data[i]);
    }
    state[data[i]]++;
    MapSanityCheck(m, state, num);
  }
}

TEST(MapTest, RandomMapTestLarge) {
  std::map<int, int> m;

  const int num = 500;

  int data[] = {
      70,  370, 352, 232, 52,  247, 249, 358, 320, 132, 327, 250, 254, 384,
      127, 203, 315, 210, 445, 176, 12,  166, 76,  199, 202, 440, 222, 487,
      267, 172, 304, 48,  112, 443, 26,  24,  326, 317, 273, 67,  91,  397,
      335, 454, 186, 333, 119, 85,  165, 168, 256, 198, 26,  272, 137, 204,
      305, 238, 20,  133, 348, 373, 62,  380, 280, 160, 255, 316, 265, 419,
      197, 430, 439, 10,  72,  96,  159, 282, 441, 497, 65,  211, 250, 473,
      129, 437, 429, 207, 276, 129, 98,  9,   433, 108, 23,  419, 55,  240,
      318, 125, 472, 91,  289, 329, 99,  112, 215, 138, 226, 406, 182, 103,
      231, 421, 171, 231, 58,  13,  456, 79,  330, 335, 51,  482, 239, 22,
      377, 141, 245, 49,  319, 390, 9,   53,  324, 454, 39,  409, 254, 330,
      259, 291, 356, 150, 222, 202, 470, 24,  150, 124, 221, 306, 65,  181,
      313, 281, 190, 118, 443, 315, 213, 467, 411, 23,  173, 496, 312, 212,
      236, 16,  27,  191, 318, 57,  104, 205, 149, 324, 117, 371, 286, 89,
      184, 215, 144, 134, 355, 316, 485, 371, 217, 410, 196, 149, 342, 290,
      441, 430, 395, 439, 420, 91,  190, 285, 389, 327, 19,  241, 300, 164,
      435, 18,  464, 112, 346, 287, 387, 345, 294, 152, 142, 299, 498, 195,
      233, 312, 471, 221, 60,  176, 332, 424, 158, 409, 35,  462, 219, 443,
      191, 320, 258, 485, 241, 404, 463, 281, 177, 300, 317, 2,   61,  120,
      469, 491, 455, 495, 407, 257, 4,   73,  237, 295, 474, 299, 285, 127,
      313, 371, 247, 19,  226, 346, 478, 423, 397, 122, 210, 268, 252, 392,
      157, 195, 301, 362, 136, 291, 97,  205, 383, 57,  106, 252, 141, 262,
      456, 17,  128, 348, 364, 434, 361, 214, 343, 496, 100, 257, 395, 438,
      219, 182, 150, 428, 294, 56,  367, 54,  138, 335, 459, 392, 282, 319,
      78,  140, 230, 400, 482, 481, 173, 434, 89,  401, 446, 147, 497, 428,
      201, 238, 296, 165, 8,   277, 95,  145, 268, 308, 340, 81,  94,  157,
      42,  78,  442, 292, 295, 311, 100, 419, 243, 339, 123, 388, 279, 310,
      497, 22,  298, 99,  251, 42,  84,  278, 317, 357, 74,  7,   60,  452,
      273, 239, 336, 494, 186, 86,  278, 54,  410, 107, 278, 59,  410, 143,
      365, 59,  64,  261, 27,  162, 135, 464, 281, 57,  283, 431, 476, 272,
      207, 77,  378, 175, 304, 55,  178, 187, 350, 417, 390, 425, 160, 461,
      3,   374, 15,  12,  489, 11,  15,  405, 48,  135, 442, 288, 429, 405,
      363, 439, 110, 298, 428, 350, 328, 159, 234, 490, 469, 0,   0,   122,
      21,  228, 143, 132, 218, 309, 193, 461, 392, 357, 432, 141, 227, 279,
      125, 22,  499, 123, 99,  228, 406, 83,  109, 447, 377, 225, 218, 73,
      154, 457, 67,  260, 405, 43,  90,  494, 86,  342, 70,  478, 460, 297,
      359, 373, 154, 454, 262, 430, 177, 292, 211, 124, 379, 235, 208, 399,
      363, 438, 253, 61,  352, 45,  20,  140, 322, 116, 227, 437, 189, 77,
      380, 185, 115, 148, 8,   157, 450, 368, 383, 1,   206, 308, 400, 249,
      156, 184, 190, 78,  6,   366, 267, 285, 30,  343, 402, 220, 16,  367,
      213, 440, 274, 152, 189, 243, 398, 146, 325, 12,  220, 130, 257, 462,
      293, 247, 131, 394, 10,  417, 137, 216, 426, 152, 198, 413, 123, 47,
      5,   421, 236, 449, 475, 73,  471, 310, 178, 9,   356, 110, 476, 399,
      360, 342, 179, 411, 170, 98,  379, 209, 21,  161, 302, 80,  334, 33,
      89,  11,  139, 475, 431, 485, 321, 118, 455, 184, 26,  323, 246, 331,
      376, 232, 164, 423, 161, 101, 261, 242, 220, 28,  66,  491, 200, 271,
      450, 115, 52,  361, 97,  16,  263, 93,  426, 153, 113, 323, 259, 133,
      113, 261, 349, 148, 451, 355, 449, 472, 404, 356, 459, 266, 44,  251,
      49,  384, 477, 396, 416, 297, 320, 200, 465, 280, 436, 87,  174, 56,
      111, 41,  388, 153, 287, 229, 435, 449, 87,  140, 389, 167, 7,   423,
      328, 244, 269, 398, 50,  301, 338, 347, 256, 126, 351, 484, 253, 365,
      328, 116, 394, 63,  473, 477, 70,  120, 444, 412, 396, 470, 199, 480,
      201, 244, 180, 29,  368, 64,  213, 422, 130, 286, 316, 468, 139, 307,
      453, 487, 236, 178, 494, 118, 34,  156, 493, 411, 412, 403, 294, 376,
      284, 104, 131, 334, 307, 81,  387, 181, 301, 92,  125, 195, 196, 154,
      418, 42,  311, 169, 313, 193, 432, 353, 234, 478, 100, 486, 14,  55,
      258, 307, 310, 464, 228, 288, 408, 132, 448, 225, 172, 153, 242, 274,
      265, 326, 58,  408, 107, 175, 93,  283, 463, 75,  156, 341, 30,  441,
      217, 327, 341, 369, 360, 36,  461, 223, 207, 398, 128, 216, 103, 82,
      414, 309, 187, 339, 75,  139, 422, 403, 373, 448, 40,  254, 484, 163,
      269, 306, 333, 393, 146, 20,  401, 192, 309, 114, 283, 68,  394, 434,
      5,   351, 173, 114, 128, 352, 386, 447, 200, 36,  427, 355, 369, 267,
      380, 175, 451, 214, 41,  372, 108, 4,   71,  452, 104, 117, 303, 223,
      435, 489, 229, 188, 312, 384, 252, 450, 106, 389, 433, 155, 275, 375,
      359, 452, 337, 19,  208, 83,  32,  418, 292, 275, 75,  18,  350, 29,
      331, 289, 458, 397, 325, 490, 88,  263, 311, 145, 222, 14,  23,  202,
      135, 259, 241, 166, 358, 383, 480, 147, 86,  351, 17,  71,  451, 297,
      80,  218, 413, 287, 62,  11,  372, 102, 382, 491, 63,  146, 388, 280,
      426, 90,  246, 391, 336, 296, 477, 318, 498, 126, 238, 38,  481, 264,
      25,  366, 393, 119, 385, 462, 264, 151, 438, 463, 445, 483, 72,  399,
      265, 64,  13,  192, 60,  212, 74,  176, 409, 305, 235, 183, 377, 6,
      243, 14,  161, 120, 117, 255, 142, 188, 446, 87,  242, 277, 27,  189,
      206, 77,  293, 164, 38,  263, 229, 102, 391, 45,  52,  458, 2,   466,
      460, 338, 194, 105, 322, 142, 97,  393, 174, 170, 482, 420, 466, 105,
      366, 223, 134, 111, 225, 144, 334, 401, 358, 163, 1,   116, 343, 332,
      361, 136, 133, 305, 353, 138, 145, 39,  186, 79,  205, 337, 209, 136,
      370, 162, 493, 245, 96,  375, 406, 299, 31,  436, 197, 459, 81,  96,
      56,  245, 308, 325, 386, 191, 227, 162, 470, 483, 74,  493, 33,  468,
      36,  79,  181, 271, 338, 214, 469, 53,  276, 93,  345, 264, 303, 249,
      382, 111, 83,  37,  110, 359, 232, 174, 379, 421, 476, 171, 114, 53,
      425, 203, 272, 82,  34,  431, 160, 286, 127, 49,  489, 374, 2,   98,
      457, 224, 206, 262, 10,  6,   378, 4,   480, 155, 465, 270, 341, 134,
      109, 177, 330, 302, 233, 246, 354, 3,   50,  240, 211, 109, 349, 260,
      54,  340, 147, 499, 276, 416, 95,  402, 30,  210, 3,   69,  475, 179,
      219, 248, 167, 427, 0,   381, 115, 201, 76,  487, 386, 230, 102, 234,
      496, 486, 284, 374, 183, 455, 270, 50,  121, 108, 215, 28,  372, 62,
      367, 412, 444, 266, 273, 415, 25,  103, 46,  437, 427, 324, 24,  457,
      274, 194, 329, 378, 124, 244, 364, 226, 360, 13,  323, 260, 122, 51,
      217, 375, 304, 418, 279, 151, 349, 251, 208, 7,   204, 203, 460, 436,
      344, 403, 365, 94,  269, 479, 194, 407, 172, 420, 498, 197, 363, 369,
      199, 106, 159, 1,   473, 385, 385, 488, 354, 345, 40,  417, 105, 148,
      425, 76,  31,  346, 35,  486, 413, 347, 59,  400, 467, 290, 440, 43,
      187, 336, 314, 170, 25,  45,  216, 368, 467, 158, 224, 212, 69,  466,
      8,   329, 290, 298, 248, 415, 484, 340, 275, 155, 395, 396, 88,  446,
      119, 180, 370, 453, 101, 479, 169, 306, 165, 492, 137, 163, 66,  179,
      90,  339, 221, 126, 284, 382, 71,  144, 63,  61,  171, 235, 302, 481,
      495, 121, 188, 92,  166, 46,  429, 240, 321, 422, 391, 48,  256, 458,
      333, 364, 43,  255, 448, 28,  300, 233, 495, 69,  196, 424, 17,  5,
      289, 268, 58,  248, 85,  46,  314, 354, 293, 258, 230, 130, 344, 444,
      277, 66,  314, 390, 84,  472, 319, 85,  183, 445, 44,  192, 15,  321,
      499, 95,  32,  488, 209, 92,  347, 84,  72,  433, 415, 34,  414, 40,
      296, 456, 465, 414, 158, 332, 131, 348, 291, 44,  270, 282, 322, 151,
      35,  18,  129, 376, 474, 182, 381, 68,  295, 492, 65,  29,  381, 344,
      407, 432, 424, 47,  488, 82,  168, 315, 442, 408, 149, 253, 107, 331,
      453, 185, 33,  37,  402, 471, 387, 479, 204, 490, 169, 468, 51,  224,
      303, 67,  357, 37,  362, 326, 239, 337, 237, 167, 483, 180, 271, 416,
      185, 168, 193, 21,  404, 198, 113, 447, 250, 47,  39,  237, 41,  38,
      266, 80,  492, 288, 121, 31,  68,  101, 231, 474, 88,  362, 94,  32,
      143, 353};

  int state[num] = {0};

  for (int i = 0; i < 3 * num; i++) {
    if (state[data[i]] == 0) {
      m[data[i]] = data[i] + 1;
    } else if (state[data[i]] == 1) {
      m[data[i]] *= 2;
    } else {
      m.erase(data[i]);
    }
    state[data[i]]++;
    MapSanityCheck(m, state, num);
  }
}

}  // namespace kernel_test
}  // namespace Kernel