//
//  fontes.h
//  Librairies
//
//  Created by Michel Gros on 31.03.21.
//
//

#ifndef FONTES_H
#define FONTES_H

typedef struct {
	char           *Name;         /* The source font name             */
	int             Quantity;     /* Number of chars in font          */
	int             Height;       /* Height of the characters         */
	const GLubyte **Characters;   /* The characters mapping           */
	float           xorig, yorig; /* Relative origin of the character */
} SFG_Font;

static const GLubyte Fixed8x13_Character_000[] = {  8,  0,  0,  0,170,  0,130,  0,130,  0,130,  0,170,  0,  0};
static const GLubyte Fixed8x13_Character_001[] = {  8,  0,  0,  0,  0, 16, 56,124,254,124, 56, 16,  0,  0,  0};
static const GLubyte Fixed8x13_Character_002[] = {  8,  0,170, 85,170, 85,170, 85,170, 85,170, 85,170, 85,170};
static const GLubyte Fixed8x13_Character_003[] = {  8,  0,  0,  0,  4,  4,  4,  4,174,160,224,160,160,  0,  0};
static const GLubyte Fixed8x13_Character_004[] = {  8,  0,  0,  0,  8,  8, 12,  8,142,128,192,128,224,  0,  0};
static const GLubyte Fixed8x13_Character_005[] = {  8,  0,  0,  0, 10, 10, 12, 10,108,128,128,128, 96,  0,  0};
static const GLubyte Fixed8x13_Character_006[] = {  8,  0,  0,  0,  8,  8, 12,  8,238,128,128,128,128,  0,  0};
static const GLubyte Fixed8x13_Character_007[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0, 24, 36, 36, 24,  0,  0};
static const GLubyte Fixed8x13_Character_008[] = {  8,  0,  0,  0,  0,124,  0, 16, 16,124, 16, 16,  0,  0,  0};
static const GLubyte Fixed8x13_Character_009[] = {  8,  0,  0,  0, 14,  8,  8,  8,168,160,160,160,192,  0,  0};
static const GLubyte Fixed8x13_Character_010[] = {  8,  0,  0,  0,  4,  4,  4,  4, 46, 80, 80,136,136,  0,  0};
static const GLubyte Fixed8x13_Character_011[] = {  8,  0,  0,  0,  0,  0,  0,  0,240, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_012[] = {  8,  0, 16, 16, 16, 16, 16, 16,240,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_013[] = {  8,  0, 16, 16, 16, 16, 16, 16, 31,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_014[] = {  8,  0,  0,  0,  0,  0,  0,  0, 31, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_015[] = {  8,  0, 16, 16, 16, 16, 16, 16,255, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_016[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255};
static const GLubyte Fixed8x13_Character_017[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0};
static const GLubyte Fixed8x13_Character_018[] = {  8,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_019[] = {  8,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_020[] = {  8,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_021[] = {  8,  0, 16, 16, 16, 16, 16, 16, 31, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_022[] = {  8,  0, 16, 16, 16, 16, 16, 16,240, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_023[] = {  8,  0,  0,  0,  0,  0,  0,  0,255, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_024[] = {  8,  0, 16, 16, 16, 16, 16, 16,255,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_025[] = {  8,  0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static const GLubyte Fixed8x13_Character_026[] = {  8,  0,  0,  0,254,  0, 14, 48,192, 48, 14,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_027[] = {  8,  0,  0,  0,254,  0,224, 24,  6, 24,224,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_028[] = {  8,  0,  0,  0, 68, 68, 68, 68, 68,254,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_029[] = {  8,  0,  0,  0, 32, 32,126, 16,  8,126,  4,  4,  0,  0,  0};
static const GLubyte Fixed8x13_Character_030[] = {  8,  0,  0,  0,220, 98, 32, 32, 32,112, 32, 34, 28,  0,  0};
static const GLubyte Fixed8x13_Character_031[] = {  8,  0,  0,  0,  0,  0,  0,  0, 24,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_032[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_033[] = {  8,  0,  0,  0, 16,  0, 16, 16, 16, 16, 16, 16, 16,  0,  0};
static const GLubyte Fixed8x13_Character_034[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0, 36, 36, 36,  0,  0};
static const GLubyte Fixed8x13_Character_035[] = {  8,  0,  0,  0,  0, 36, 36,126, 36,126, 36, 36,  0,  0,  0};
static const GLubyte Fixed8x13_Character_036[] = {  8,  0,  0,  0, 16,120, 20, 20, 56, 80, 80, 60, 16,  0,  0};
static const GLubyte Fixed8x13_Character_037[] = {  8,  0,  0,  0, 68, 42, 36, 16,  8,  8, 36, 82, 34,  0,  0};
static const GLubyte Fixed8x13_Character_038[] = {  8,  0,  0,  0, 58, 68, 74, 48, 72, 72, 48,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_039[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0, 64, 48, 56,  0,  0};
static const GLubyte Fixed8x13_Character_040[] = {  8,  0,  0,  0,  4,  8,  8, 16, 16, 16,  8,  8,  4,  0,  0};
static const GLubyte Fixed8x13_Character_041[] = {  8,  0,  0,  0, 32, 16, 16,  8,  8,  8, 16, 16, 32,  0,  0};
static const GLubyte Fixed8x13_Character_042[] = {  8,  0,  0,  0,  0,  0, 36, 24,126, 24, 36,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_043[] = {  8,  0,  0,  0,  0,  0, 16, 16,124, 16, 16,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_044[] = {  8,  0,  0, 64, 48, 56,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_045[] = {  8,  0,  0,  0,  0,  0,  0,  0,126,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_046[] = {  8,  0,  0, 16, 56, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_047[] = {  8,  0,  0,  0,128,128, 64, 32, 16,  8,  4,  2,  2,  0,  0};
static const GLubyte Fixed8x13_Character_048[] = {  8,  0,  0,  0, 24, 36, 66, 66, 66, 66, 66, 36, 24,  0,  0};
static const GLubyte Fixed8x13_Character_049[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16, 80, 48, 16,  0,  0};
static const GLubyte Fixed8x13_Character_050[] = {  8,  0,  0,  0,126, 64, 32, 24,  4,  2, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_051[] = {  8,  0,  0,  0, 60, 66,  2,  2, 28,  8,  4,  2,126,  0,  0};
static const GLubyte Fixed8x13_Character_052[] = {  8,  0,  0,  0,  4,  4,126, 68, 68, 36, 20, 12,  4,  0,  0};
static const GLubyte Fixed8x13_Character_053[] = {  8,  0,  0,  0, 60, 66,  2,  2, 98, 92, 64, 64,126,  0,  0};
static const GLubyte Fixed8x13_Character_054[] = {  8,  0,  0,  0, 60, 66, 66, 98, 92, 64, 64, 32, 28,  0,  0};
static const GLubyte Fixed8x13_Character_055[] = {  8,  0,  0,  0, 32, 32, 16, 16,  8,  8,  4,  2,126,  0,  0};
static const GLubyte Fixed8x13_Character_056[] = {  8,  0,  0,  0, 60, 66, 66, 66, 60, 66, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_057[] = {  8,  0,  0,  0, 56,  4,  2,  2, 58, 70, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_058[] = {  8,  0,  0, 16, 56, 16,  0,  0, 16, 56, 16,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_059[] = {  8,  0,  0, 64, 48, 56,  0,  0, 16, 56, 16,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_060[] = {  8,  0,  0,  0,  2,  4,  8, 16, 32, 16,  8,  4,  2,  0,  0};
static const GLubyte Fixed8x13_Character_061[] = {  8,  0,  0,  0,  0,  0,126,  0,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_062[] = {  8,  0,  0,  0, 64, 32, 16,  8,  4,  8, 16, 32, 64,  0,  0};
static const GLubyte Fixed8x13_Character_063[] = {  8,  0,  0,  0,  8,  0,  8,  8,  4,  2, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_064[] = {  8,  0,  0,  0, 60, 64, 74, 86, 82, 78, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_065[] = {  8,  0,  0,  0, 66, 66, 66,126, 66, 66, 66, 36, 24,  0,  0};
static const GLubyte Fixed8x13_Character_066[] = {  8,  0,  0,  0,252, 66, 66, 66,124, 66, 66, 66,252,  0,  0};
static const GLubyte Fixed8x13_Character_067[] = {  8,  0,  0,  0, 60, 66, 64, 64, 64, 64, 64, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_068[] = {  8,  0,  0,  0,252, 66, 66, 66, 66, 66, 66, 66,252,  0,  0};
static const GLubyte Fixed8x13_Character_069[] = {  8,  0,  0,  0,126, 64, 64, 64,120, 64, 64, 64,126,  0,  0};
static const GLubyte Fixed8x13_Character_070[] = {  8,  0,  0,  0, 64, 64, 64, 64,120, 64, 64, 64,126,  0,  0};
static const GLubyte Fixed8x13_Character_071[] = {  8,  0,  0,  0, 58, 70, 66, 78, 64, 64, 64, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_072[] = {  8,  0,  0,  0, 66, 66, 66, 66,126, 66, 66, 66, 66,  0,  0};
static const GLubyte Fixed8x13_Character_073[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16, 16, 16,124,  0,  0};
static const GLubyte Fixed8x13_Character_074[] = {  8,  0,  0,  0, 56, 68,  4,  4,  4,  4,  4,  4, 31,  0,  0};
static const GLubyte Fixed8x13_Character_075[] = {  8,  0,  0,  0, 66, 68, 72, 80, 96, 80, 72, 68, 66,  0,  0};
static const GLubyte Fixed8x13_Character_076[] = {  8,  0,  0,  0,126, 64, 64, 64, 64, 64, 64, 64, 64,  0,  0};
static const GLubyte Fixed8x13_Character_077[] = {  8,  0,  0,  0,130,130,130,146,146,170,198,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_078[] = {  8,  0,  0,  0, 66, 66, 66, 70, 74, 82, 98, 66, 66,  0,  0};
static const GLubyte Fixed8x13_Character_079[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_080[] = {  8,  0,  0,  0, 64, 64, 64, 64,124, 66, 66, 66,124,  0,  0};
static const GLubyte Fixed8x13_Character_081[] = {  8,  0,  0,  2, 60, 74, 82, 66, 66, 66, 66, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_082[] = {  8,  0,  0,  0, 66, 68, 72, 80,124, 66, 66, 66,124,  0,  0};
static const GLubyte Fixed8x13_Character_083[] = {  8,  0,  0,  0, 60, 66,  2,  2, 60, 64, 64, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_084[] = {  8,  0,  0,  0, 16, 16, 16, 16, 16, 16, 16, 16,254,  0,  0};
static const GLubyte Fixed8x13_Character_085[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66, 66, 66,  0,  0};
static const GLubyte Fixed8x13_Character_086[] = {  8,  0,  0,  0, 16, 40, 40, 40, 68, 68, 68,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_087[] = {  8,  0,  0,  0, 68,170,146,146,146,130,130,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_088[] = {  8,  0,  0,  0,130,130, 68, 40, 16, 40, 68,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_089[] = {  8,  0,  0,  0, 16, 16, 16, 16, 16, 40, 68,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_090[] = {  8,  0,  0,  0,126, 64, 64, 32, 16,  8,  4,  2,126,  0,  0};
static const GLubyte Fixed8x13_Character_091[] = {  8,  0,  0,  0, 60, 32, 32, 32, 32, 32, 32, 32, 60,  0,  0};
static const GLubyte Fixed8x13_Character_092[] = {  8,  0,  0,  0,  2,  2,  4,  8, 16, 32, 64,128,128,  0,  0};
static const GLubyte Fixed8x13_Character_093[] = {  8,  0,  0,  0,120,  8,  8,  8,  8,  8,  8,  8,120,  0,  0};
static const GLubyte Fixed8x13_Character_094[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0, 68, 40, 16,  0,  0};
static const GLubyte Fixed8x13_Character_095[] = {  8,  0,  0,254,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_096[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4, 24, 56,  0,  0};
static const GLubyte Fixed8x13_Character_097[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_098[] = {  8,  0,  0,  0, 92, 98, 66, 66, 98, 92, 64, 64, 64,  0,  0};
static const GLubyte Fixed8x13_Character_099[] = {  8,  0,  0,  0, 60, 66, 64, 64, 66, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_100[] = {  8,  0,  0,  0, 58, 70, 66, 66, 70, 58,  2,  2,  2,  0,  0};
static const GLubyte Fixed8x13_Character_101[] = {  8,  0,  0,  0, 60, 66, 64,126, 66, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_102[] = {  8,  0,  0,  0, 32, 32, 32, 32,124, 32, 32, 34, 28,  0,  0};
static const GLubyte Fixed8x13_Character_103[] = {  8,  0, 60, 66, 60, 64, 56, 68, 68, 58,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_104[] = {  8,  0,  0,  0, 66, 66, 66, 66, 98, 92, 64, 64, 64,  0,  0};
static const GLubyte Fixed8x13_Character_105[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 48,  0, 16,  0,  0,  0};
static const GLubyte Fixed8x13_Character_106[] = {  8,  0, 56, 68, 68,  4,  4,  4,  4, 12,  0,  4,  0,  0,  0};
static const GLubyte Fixed8x13_Character_107[] = {  8,  0,  0,  0, 66, 68, 72,112, 72, 68, 64, 64, 64,  0,  0};
static const GLubyte Fixed8x13_Character_108[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16, 16, 16, 48,  0,  0};
static const GLubyte Fixed8x13_Character_109[] = {  8,  0,  0,  0,130,146,146,146,146,236,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_110[] = {  8,  0,  0,  0, 66, 66, 66, 66, 98, 92,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_111[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_112[] = {  8,  0, 64, 64, 64, 92, 98, 66, 98, 92,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_113[] = {  8,  0,  2,  2,  2, 58, 70, 66, 70, 58,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_114[] = {  8,  0,  0,  0, 32, 32, 32, 32, 34, 92,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_115[] = {  8,  0,  0,  0, 60, 66, 12, 48, 66, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_116[] = {  8,  0,  0,  0, 28, 34, 32, 32, 32,124, 32, 32,  0,  0,  0};
static const GLubyte Fixed8x13_Character_117[] = {  8,  0,  0,  0, 58, 68, 68, 68, 68, 68,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_118[] = {  8,  0,  0,  0, 16, 40, 40, 68, 68, 68,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_119[] = {  8,  0,  0,  0, 68,170,146,146,130,130,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_120[] = {  8,  0,  0,  0, 66, 36, 24, 24, 36, 66,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_121[] = {  8,  0, 60, 66,  2, 58, 70, 66, 66, 66,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_122[] = {  8,  0,  0,  0,126, 32, 16,  8,  4,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_123[] = {  8,  0,  0,  0, 14, 16, 16,  8, 48,  8, 16, 16, 14,  0,  0};
static const GLubyte Fixed8x13_Character_124[] = {  8,  0,  0,  0, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0};
static const GLubyte Fixed8x13_Character_125[] = {  8,  0,  0,  0,112,  8,  8, 16, 12, 16,  8,  8,112,  0,  0};
static const GLubyte Fixed8x13_Character_126[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0, 72, 84, 36,  0,  0};
static const GLubyte Fixed8x13_Character_127[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_128[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_129[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_130[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_131[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_132[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_133[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_134[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_135[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_136[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_137[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_138[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_139[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_140[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_141[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_142[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_143[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_144[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_145[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_146[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_147[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_148[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_149[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_150[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_151[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_152[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_153[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_154[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_155[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_156[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_157[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_158[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_159[] = {  9,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,130,  0,  0,  0,170,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_160[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_161[] = {  8,  0,  0,  0, 16, 16, 16, 16, 16, 16, 16,  0, 16,  0,  0};
static const GLubyte Fixed8x13_Character_162[] = {  8,  0,  0,  0,  0, 16, 56, 84, 80, 80, 84, 56, 16,  0,  0};
static const GLubyte Fixed8x13_Character_163[] = {  8,  0,  0,  0,220, 98, 32, 32, 32,112, 32, 34, 28,  0,  0};
static const GLubyte Fixed8x13_Character_164[] = {  8,  0,  0,  0,  0, 66, 60, 36, 36, 60, 66,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_165[] = {  8,  0,  0,  0, 16, 16,124, 16,124, 40, 68,130,130,  0,  0};
static const GLubyte Fixed8x13_Character_166[] = {  8,  0,  0,  0, 16, 16, 16, 16,  0, 16, 16, 16, 16,  0,  0};
static const GLubyte Fixed8x13_Character_167[] = {  8,  0,  0,  0, 24, 36,  4, 24, 36, 36, 24, 32, 36, 24,  0};
static const GLubyte Fixed8x13_Character_168[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,108,  0,  0};
static const GLubyte Fixed8x13_Character_169[] = {  8,  0,  0,  0,  0, 56, 68,146,170,162,170,146, 68, 56,  0};
static const GLubyte Fixed8x13_Character_170[] = {  8,  0,  0,  0,  0,  0,124,  0, 60, 68, 60,  4, 56,  0,  0};
static const GLubyte Fixed8x13_Character_171[] = {  8,  0,  0,  0,  0, 18, 36, 72,144, 72, 36, 18,  0,  0,  0};
static const GLubyte Fixed8x13_Character_172[] = {  8,  0,  0,  0,  0,  2,  2,  2,126,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_173[] = {  8,  0,  0,  0,  0,  0,  0,  0, 60,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_174[] = {  8,  0,  0,  0,  0, 56, 68,170,178,170,170,146, 68, 56,  0};
static const GLubyte Fixed8x13_Character_175[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,126,  0,  0};
static const GLubyte Fixed8x13_Character_176[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0, 24, 36, 36, 24,  0,  0};
static const GLubyte Fixed8x13_Character_177[] = {  8,  0,  0,  0,  0,124,  0, 16, 16,124, 16, 16,  0,  0,  0};
static const GLubyte Fixed8x13_Character_178[] = {  8,  0,  0,  0,  0,  0,  0,  0,120, 64, 48,  8, 72, 48,  0};
static const GLubyte Fixed8x13_Character_179[] = {  8,  0,  0,  0,  0,  0,  0,  0, 48, 72,  8, 16, 72, 48,  0};
static const GLubyte Fixed8x13_Character_180[] = {  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_181[] = {  8,  0,  0, 64, 90,102, 66, 66, 66, 66,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_182[] = {  8,  0,  0,  0, 20, 20, 20, 20, 52,116,116,116, 62,  0,  0};
static const GLubyte Fixed8x13_Character_183[] = {  8,  0,  0,  0,  0,  0,  0,  0, 24,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_184[] = {  8,  0, 24,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_185[] = {  8,  0,  0,  0,  0,  0,  0,  0,112, 32, 32, 32, 96, 32,  0};
static const GLubyte Fixed8x13_Character_186[] = {  8,  0,  0,  0,  0,  0,  0,120,  0, 48, 72, 72, 48,  0,  0};
static const GLubyte Fixed8x13_Character_187[] = {  8,  0,  0,  0,  0,144, 72, 36, 18, 36, 72,144,  0,  0,  0};
static const GLubyte Fixed8x13_Character_188[] = {  8,  0,  0,  0,  6, 26, 18, 10,230, 66, 64, 64,192, 64,  0};
static const GLubyte Fixed8x13_Character_189[] = {  8,  0,  0,  0, 30, 16, 12,  2,242, 76, 64, 64,192, 64,  0};
static const GLubyte Fixed8x13_Character_190[] = {  8,  0,  0,  0,  6, 26, 18, 10,102,146, 16, 32,144, 96,  0};
static const GLubyte Fixed8x13_Character_191[] = {  8,  0,  0,  0, 60, 66, 66, 64, 32, 16, 16,  0, 16,  0,  0};
static const GLubyte Fixed8x13_Character_192[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24,  0,  8, 16,  0};
static const GLubyte Fixed8x13_Character_193[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_194[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_195[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24,  0, 76, 50,  0};
static const GLubyte Fixed8x13_Character_196[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_197[] = {  8,  0,  0,  0, 66, 66,126, 66, 66, 36, 24, 24, 36, 24,  0};
static const GLubyte Fixed8x13_Character_198[] = {  8,  0,  0,  0,158,144,144,240,156,144,144,144,110,  0,  0};
static const GLubyte Fixed8x13_Character_199[] = {  8,  0, 16,  8, 60, 66, 64, 64, 64, 64, 64, 66, 60,  0,  0};
static const GLubyte Fixed8x13_Character_200[] = {  8,  0,  0,  0,126, 64, 64,120, 64, 64,126,  0,  8, 16,  0};
static const GLubyte Fixed8x13_Character_201[] = {  8,  0,  0,  0,126, 64, 64,120, 64, 64,126,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_202[] = {  8,  0,  0,  0,126, 64, 64,120, 64, 64,126,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_203[] = {  8,  0,  0,  0,126, 64, 64,120, 64, 64,126,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_204[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16,124,  0, 16, 32,  0};
static const GLubyte Fixed8x13_Character_205[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16,124,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_206[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16,124,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_207[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 16,124,  0, 40, 40,  0};
static const GLubyte Fixed8x13_Character_208[] = {  8,  0,  0,  0,120, 68, 66, 66,226, 66, 66, 68,120,  0,  0};
static const GLubyte Fixed8x13_Character_209[] = {  8,  0,  0,  0,130,134,138,146,162,194,130,  0,152,100,  0};
static const GLubyte Fixed8x13_Character_210[] = {  8,  0,  0,  0,124,130,130,130,130,130,124,  0, 16, 32,  0};
static const GLubyte Fixed8x13_Character_211[] = {  8,  0,  0,  0,124,130,130,130,130,130,124,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_212[] = {  8,  0,  0,  0,124,130,130,130,130,130,124,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_213[] = {  8,  0,  0,  0,124,130,130,130,130,130,124,  0,152,100,  0};
static const GLubyte Fixed8x13_Character_214[] = {  8,  0,  0,  0,124,130,130,130,130,130,124,  0, 40, 40,  0};
static const GLubyte Fixed8x13_Character_215[] = {  8,  0,  0,  0,  0, 66, 36, 24, 24, 36, 66,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_216[] = {  8,  0,  0, 64, 60, 98, 82, 82, 82, 74, 74, 70, 60,  2,  0};
static const GLubyte Fixed8x13_Character_217[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66,  0,  8, 16,  0};
static const GLubyte Fixed8x13_Character_218[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_219[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_220[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 66, 66,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_221[] = {  8,  0,  0,  0, 16, 16, 16, 16, 40, 68, 68,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_222[] = {  8,  0,  0,  0, 64, 64, 64,124, 66, 66, 66,124, 64,  0,  0};
static const GLubyte Fixed8x13_Character_223[] = {  8,  0,  0,  0, 92, 66, 66, 76, 80, 72, 68, 68, 56,  0,  0};
static const GLubyte Fixed8x13_Character_224[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0,  8, 16,  0};
static const GLubyte Fixed8x13_Character_225[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0,  8,  4,  0};
static const GLubyte Fixed8x13_Character_226[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_227[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0, 76, 50,  0};
static const GLubyte Fixed8x13_Character_228[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_229[] = {  8,  0,  0,  0, 58, 70, 66, 62,  2, 60,  0, 24, 36, 24,  0};
static const GLubyte Fixed8x13_Character_230[] = {  8,  0,  0,  0,108,146,144,124, 18,108,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_231[] = {  8,  0, 16,  8, 60, 66, 64, 64, 66, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_232[] = {  8,  0,  0,  0, 60, 66, 64,126, 66, 60,  0,  0,  8, 16,  0};
static const GLubyte Fixed8x13_Character_233[] = {  8,  0,  0,  0, 60, 66, 64,126, 66, 60,  0,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_234[] = {  8,  0,  0,  0, 60, 66, 64,126, 66, 60,  0,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_235[] = {  8,  0,  0,  0, 60, 66, 64,126, 66, 60,  0,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_236[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 48,  0,  0, 16, 32,  0};
static const GLubyte Fixed8x13_Character_237[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 48,  0,  0, 32, 16,  0};
static const GLubyte Fixed8x13_Character_238[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 48,  0,  0, 72, 48,  0};
static const GLubyte Fixed8x13_Character_239[] = {  8,  0,  0,  0,124, 16, 16, 16, 16, 48,  0,  0, 40, 40,  0};
static const GLubyte Fixed8x13_Character_240[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  4, 40, 24, 36,  0};
static const GLubyte Fixed8x13_Character_241[] = {  8,  0,  0,  0, 66, 66, 66, 66, 98, 92,  0,  0, 76, 50,  0};
static const GLubyte Fixed8x13_Character_242[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0, 16, 32,  0};
static const GLubyte Fixed8x13_Character_243[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_244[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_245[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0, 76, 50,  0};
static const GLubyte Fixed8x13_Character_246[] = {  8,  0,  0,  0, 60, 66, 66, 66, 66, 60,  0,  0, 36, 36,  0};
static const GLubyte Fixed8x13_Character_247[] = {  8,  0,  0,  0,  0, 16, 16,  0,124,  0, 16, 16,  0,  0,  0};
static const GLubyte Fixed8x13_Character_248[] = {  8,  0,  0, 64, 60, 98, 82, 74, 70, 60,  2,  0,  0,  0,  0};
static const GLubyte Fixed8x13_Character_249[] = {  8,  0,  0,  0, 58, 68, 68, 68, 68, 68,  0,  0, 16, 32,  0};
static const GLubyte Fixed8x13_Character_250[] = {  8,  0,  0,  0, 58, 68, 68, 68, 68, 68,  0,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_251[] = {  8,  0,  0,  0, 58, 68, 68, 68, 68, 68,  0,  0, 36, 24,  0};
static const GLubyte Fixed8x13_Character_252[] = {  8,  0,  0,  0, 58, 68, 68, 68, 68, 68,  0,  0, 40, 40,  0};
static const GLubyte Fixed8x13_Character_253[] = {  8,  0, 60, 66,  2, 58, 70, 66, 66, 66,  0,  0, 16,  8,  0};
static const GLubyte Fixed8x13_Character_254[] = {  8,  0, 64, 64, 92, 98, 66, 66, 98, 92, 64, 64,  0,  0,  0};
static const GLubyte Fixed8x13_Character_255[] = {  8,  0, 60, 66,  2, 58, 70, 66, 66, 66,  0,  0, 36, 36,  0};

/* The font characters mapping: */
static const GLubyte* Fixed8x13_Character_Map[] = {Fixed8x13_Character_000,Fixed8x13_Character_001,Fixed8x13_Character_002,Fixed8x13_Character_003,Fixed8x13_Character_004,Fixed8x13_Character_005,Fixed8x13_Character_006,Fixed8x13_Character_007,Fixed8x13_Character_008,Fixed8x13_Character_009,Fixed8x13_Character_010,Fixed8x13_Character_011,Fixed8x13_Character_012,Fixed8x13_Character_013,Fixed8x13_Character_014,Fixed8x13_Character_015,
	Fixed8x13_Character_016,Fixed8x13_Character_017,Fixed8x13_Character_018,Fixed8x13_Character_019,Fixed8x13_Character_020,Fixed8x13_Character_021,Fixed8x13_Character_022,Fixed8x13_Character_023,Fixed8x13_Character_024,Fixed8x13_Character_025,Fixed8x13_Character_026,Fixed8x13_Character_027,Fixed8x13_Character_028,Fixed8x13_Character_029,Fixed8x13_Character_030,Fixed8x13_Character_031,
	Fixed8x13_Character_032,Fixed8x13_Character_033,Fixed8x13_Character_034,Fixed8x13_Character_035,Fixed8x13_Character_036,Fixed8x13_Character_037,Fixed8x13_Character_038,Fixed8x13_Character_039,Fixed8x13_Character_040,Fixed8x13_Character_041,Fixed8x13_Character_042,Fixed8x13_Character_043,Fixed8x13_Character_044,Fixed8x13_Character_045,Fixed8x13_Character_046,Fixed8x13_Character_047,
	Fixed8x13_Character_048,Fixed8x13_Character_049,Fixed8x13_Character_050,Fixed8x13_Character_051,Fixed8x13_Character_052,Fixed8x13_Character_053,Fixed8x13_Character_054,Fixed8x13_Character_055,Fixed8x13_Character_056,Fixed8x13_Character_057,Fixed8x13_Character_058,Fixed8x13_Character_059,Fixed8x13_Character_060,Fixed8x13_Character_061,Fixed8x13_Character_062,Fixed8x13_Character_063,
	Fixed8x13_Character_064,Fixed8x13_Character_065,Fixed8x13_Character_066,Fixed8x13_Character_067,Fixed8x13_Character_068,Fixed8x13_Character_069,Fixed8x13_Character_070,Fixed8x13_Character_071,Fixed8x13_Character_072,Fixed8x13_Character_073,Fixed8x13_Character_074,Fixed8x13_Character_075,Fixed8x13_Character_076,Fixed8x13_Character_077,Fixed8x13_Character_078,Fixed8x13_Character_079,
	Fixed8x13_Character_080,Fixed8x13_Character_081,Fixed8x13_Character_082,Fixed8x13_Character_083,Fixed8x13_Character_084,Fixed8x13_Character_085,Fixed8x13_Character_086,Fixed8x13_Character_087,Fixed8x13_Character_088,Fixed8x13_Character_089,Fixed8x13_Character_090,Fixed8x13_Character_091,Fixed8x13_Character_092,Fixed8x13_Character_093,Fixed8x13_Character_094,Fixed8x13_Character_095,
	Fixed8x13_Character_096,Fixed8x13_Character_097,Fixed8x13_Character_098,Fixed8x13_Character_099,Fixed8x13_Character_100,Fixed8x13_Character_101,Fixed8x13_Character_102,Fixed8x13_Character_103,Fixed8x13_Character_104,Fixed8x13_Character_105,Fixed8x13_Character_106,Fixed8x13_Character_107,Fixed8x13_Character_108,Fixed8x13_Character_109,Fixed8x13_Character_110,Fixed8x13_Character_111,
	Fixed8x13_Character_112,Fixed8x13_Character_113,Fixed8x13_Character_114,Fixed8x13_Character_115,Fixed8x13_Character_116,Fixed8x13_Character_117,Fixed8x13_Character_118,Fixed8x13_Character_119,Fixed8x13_Character_120,Fixed8x13_Character_121,Fixed8x13_Character_122,Fixed8x13_Character_123,Fixed8x13_Character_124,Fixed8x13_Character_125,Fixed8x13_Character_126,Fixed8x13_Character_032,
	Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,
	Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,Fixed8x13_Character_032,
	Fixed8x13_Character_160,Fixed8x13_Character_161,Fixed8x13_Character_162,Fixed8x13_Character_163,Fixed8x13_Character_164,Fixed8x13_Character_165,Fixed8x13_Character_166,Fixed8x13_Character_167,Fixed8x13_Character_168,Fixed8x13_Character_169,Fixed8x13_Character_170,Fixed8x13_Character_171,Fixed8x13_Character_172,Fixed8x13_Character_173,Fixed8x13_Character_174,Fixed8x13_Character_175,
	Fixed8x13_Character_176,Fixed8x13_Character_177,Fixed8x13_Character_178,Fixed8x13_Character_179,Fixed8x13_Character_180,Fixed8x13_Character_181,Fixed8x13_Character_182,Fixed8x13_Character_183,Fixed8x13_Character_184,Fixed8x13_Character_185,Fixed8x13_Character_186,Fixed8x13_Character_187,Fixed8x13_Character_188,Fixed8x13_Character_189,Fixed8x13_Character_190,Fixed8x13_Character_191,
	Fixed8x13_Character_192,Fixed8x13_Character_193,Fixed8x13_Character_194,Fixed8x13_Character_195,Fixed8x13_Character_196,Fixed8x13_Character_197,Fixed8x13_Character_198,Fixed8x13_Character_199,Fixed8x13_Character_200,Fixed8x13_Character_201,Fixed8x13_Character_202,Fixed8x13_Character_203,Fixed8x13_Character_204,Fixed8x13_Character_205,Fixed8x13_Character_206,Fixed8x13_Character_207,
	Fixed8x13_Character_208,Fixed8x13_Character_209,Fixed8x13_Character_210,Fixed8x13_Character_211,Fixed8x13_Character_212,Fixed8x13_Character_213,Fixed8x13_Character_214,Fixed8x13_Character_215,Fixed8x13_Character_216,Fixed8x13_Character_217,Fixed8x13_Character_218,Fixed8x13_Character_219,Fixed8x13_Character_220,Fixed8x13_Character_221,Fixed8x13_Character_222,Fixed8x13_Character_223,
	Fixed8x13_Character_224,Fixed8x13_Character_225,Fixed8x13_Character_226,Fixed8x13_Character_227,Fixed8x13_Character_228,Fixed8x13_Character_229,Fixed8x13_Character_230,Fixed8x13_Character_231,Fixed8x13_Character_232,Fixed8x13_Character_233,Fixed8x13_Character_234,Fixed8x13_Character_235,Fixed8x13_Character_236,Fixed8x13_Character_237,Fixed8x13_Character_238,Fixed8x13_Character_239,
	Fixed8x13_Character_240,Fixed8x13_Character_241,Fixed8x13_Character_242,Fixed8x13_Character_243,Fixed8x13_Character_244,Fixed8x13_Character_245,Fixed8x13_Character_246,Fixed8x13_Character_247,Fixed8x13_Character_248,Fixed8x13_Character_249,Fixed8x13_Character_250,Fixed8x13_Character_251,Fixed8x13_Character_252,Fixed8x13_Character_253,Fixed8x13_Character_254,Fixed8x13_Character_255,NULL};

/* The font structure: */
static SFG_Font fgFontFixed8x13 = { "-misc-fixed-medium-r-normal--13-120-75-75-C-80-iso8859-1", 256, 14, Fixed8x13_Character_Map, 0, 3 };

static const GLubyte Fixed9x15_Character_000[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_001[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0, 28,  0, 62,  0,127,  0,255,128,127,  0, 62,  0, 28,  0,  8,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_002[] = {  9,  0,  0,170,128, 85,  0,170,128, 85,  0,170,128, 85,  0,170,128, 85,  0,170,128, 85,  0,170,128, 85,  0,170,128, 85,  0,170,128};
static const GLubyte Fixed9x15_Character_003[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  4,  0,  4,  0,  4,  0, 31,  0,  0,  0, 72,  0, 72,  0,120,  0, 72,  0, 72,  0,  0,  0};
static const GLubyte Fixed9x15_Character_004[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0, 14,  0, 72,  0, 79,  0, 64,  0,112,  0, 64,  0,120,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_005[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0,  9,  0, 14,  0,  9,  0, 14,  0,  0,  0, 56,  0, 64,  0, 64,  0, 64,  0, 56,  0,  0,  0};
static const GLubyte Fixed9x15_Character_006[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0, 14,  0,  8,  0, 15,  0,  0,  0,120,  0, 64,  0, 64,  0, 64,  0, 64,  0,  0,  0};
static const GLubyte Fixed9x15_Character_007[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 18,  0, 18,  0, 12,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_008[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  8,  0,  8,  0,  8,  0,127,  0,  8,  0,  8,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_009[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0, 68,  0, 76,  0, 84,  0,100,  0, 68,  0,  0,  0};
static const GLubyte Fixed9x15_Character_010[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  4,  0,  4,  0,  4,  0, 31,  0,  0,  0, 16,  0, 40,  0, 40,  0, 68,  0, 68,  0,  0,  0};
static const GLubyte Fixed9x15_Character_011[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,248,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_012[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,248,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_013[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 15,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_014[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15,128,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_015[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,255,128,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_016[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,128};
static const GLubyte Fixed9x15_Character_017[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,128,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_018[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_019[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,255,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_020[] = {  9,  0,  0,255,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_021[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 15,128,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_022[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,248,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_023[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,128,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_024[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,255,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_025[] = {  9,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0};
static const GLubyte Fixed9x15_Character_026[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  3,  0, 28,  0, 96,  0, 28,  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_027[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0, 96,  0, 28,  0,  3,  0, 28,  0, 96,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_028[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 34,  0, 34,  0, 34,  0, 34,  0, 34,  0, 34,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_029[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  0, 16,  0,127,  0,  8,  0,127,  0,  4,  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_030[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 46,  0, 81,  0, 48,  0, 16,  0, 16,  0,124,  0, 16,  0, 16,  0, 17,  0, 14,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_031[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_032[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_033[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_034[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 18,  0, 18,  0, 18,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_035[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 36,  0, 36,  0,126,  0, 36,  0, 36,  0,126,  0, 36,  0, 36,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_036[] = {  9,  0,  0,  0,  0,  0,  0,  8,  0, 62,  0, 73,  0,  9,  0,  9,  0, 10,  0, 28,  0, 40,  0, 72,  0, 73,  0, 62,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_037[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 66,  0, 37,  0, 37,  0, 18,  0,  8,  0,  8,  0, 36,  0, 82,  0, 82,  0, 33,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_038[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 49,  0, 74,  0, 68,  0, 74,  0, 49,  0, 48,  0, 72,  0, 72,  0, 72,  0, 48,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_039[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  0,  8,  0,  4,  0,  6,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_040[] = {  9,  0,  0,  0,  0,  0,  0,  4,  0,  8,  0,  8,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0,  8,  0,  8,  0,  4,  0,  0,  0};
static const GLubyte Fixed9x15_Character_041[] = {  9,  0,  0,  0,  0,  0,  0, 16,  0,  8,  0,  8,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  8,  0,  8,  0, 16,  0,  0,  0};
static const GLubyte Fixed9x15_Character_042[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0, 73,  0, 42,  0, 28,  0, 42,  0, 73,  0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_043[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,127,  0,  8,  0,  8,  0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_044[] = {  9,  0,  0,  8,  0,  4,  0,  4,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_045[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_046[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_047[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 64,  0, 32,  0, 32,  0, 16,  0,  8,  0,  8,  0,  4,  0,  2,  0,  2,  0,  1,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_048[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 28,  0, 34,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_049[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 72,  0, 40,  0, 24,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_050[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 64,  0, 32,  0, 16,  0,  8,  0,  4,  0,  2,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_051[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0,  1,  0,  1,  0,  1,  0, 14,  0,  4,  0,  2,  0,  1,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_052[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  2,  0,  2,  0,127,  0, 66,  0, 34,  0, 18,  0, 10,  0,  6,  0,  2,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_053[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0,  1,  0,  1,  0,  1,  0, 97,  0, 94,  0, 64,  0, 64,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_054[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0, 64,  0, 64,  0, 32,  0, 30,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_055[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 32,  0, 32,  0, 16,  0, 16,  0,  8,  0,  4,  0,  2,  0,  1,  0,  1,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_056[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 28,  0, 34,  0, 65,  0, 65,  0, 34,  0, 28,  0, 34,  0, 65,  0, 34,  0, 28,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_057[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,  2,  0,  1,  0,  1,  0, 61,  0, 67,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_058[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_059[] = {  9,  0,  0,  8,  0,  4,  0,  4,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_060[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  4,  0,  8,  0, 16,  0, 32,  0, 32,  0, 16,  0,  8,  0,  4,  0,  2,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_061[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_062[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 32,  0, 16,  0,  8,  0,  4,  0,  2,  0,  2,  0,  4,  0,  8,  0, 16,  0, 32,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_063[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0,  0,  8,  0,  8,  0,  4,  0,  2,  0,  1,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_064[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0, 77,  0, 83,  0, 81,  0, 79,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_065[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 65,  0, 34,  0, 20,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_066[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,126,  0, 33,  0, 33,  0, 33,  0, 33,  0,126,  0, 33,  0, 33,  0, 33,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_067[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_068[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,126,  0, 33,  0, 33,  0, 33,  0, 33,  0, 33,  0, 33,  0, 33,  0, 33,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_069[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0, 32,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_070[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0, 32,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_071[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 71,  0, 64,  0, 64,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_072[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_073[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_074[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0, 66,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0, 15,128,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_075[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 66,  0, 68,  0, 72,  0, 80,  0,112,  0, 72,  0, 68,  0, 66,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_076[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_077[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 73,  0, 73,  0, 85,  0, 85,  0, 99,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_078[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 67,  0, 69,  0, 73,  0, 81,  0, 97,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_079[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_080[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0,126,  0, 65,  0, 65,  0, 65,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_081[] = {  9,  0,  0,  0,  0,  3,  0,  4,  0, 62,  0, 73,  0, 81,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_082[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 66,  0, 68,  0, 72,  0,126,  0, 65,  0, 65,  0, 65,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_083[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0,  1,  0,  6,  0, 56,  0, 64,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_084[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_085[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_086[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0, 20,  0, 20,  0, 20,  0, 34,  0, 34,  0, 34,  0, 65,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_087[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 34,  0, 85,  0, 73,  0, 73,  0, 73,  0, 73,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_088[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 34,  0, 20,  0,  8,  0,  8,  0, 20,  0, 34,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_089[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 20,  0, 34,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_090[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 64,  0, 64,  0, 32,  0, 16,  0,  8,  0,  4,  0,  2,  0,  1,  0,127,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_091[] = {  9,  0,  0,  0,  0,  0,  0, 30,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 16,  0, 30,  0,  0,  0};
static const GLubyte Fixed9x15_Character_092[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  2,  0,  2,  0,  4,  0,  8,  0,  8,  0, 16,  0, 32,  0, 32,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_093[] = {  9,  0,  0,  0,  0,  0,  0, 60,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0,  4,  0, 60,  0,  0,  0};
static const GLubyte Fixed9x15_Character_094[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 34,  0, 20,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_095[] = {  9,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_096[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  8,  0, 16,  0, 48,  0,  0,  0};
static const GLubyte Fixed9x15_Character_097[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_098[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 94,  0, 97,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0, 64,  0, 64,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_099[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 64,  0, 64,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_100[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 65,  0, 65,  0, 67,  0, 61,  0,  1,  0,  1,  0,  1,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_101[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0,127,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_102[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 16,  0, 16,  0, 16,  0, 16,  0,124,  0, 16,  0, 16,  0, 17,  0, 17,  0, 14,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_103[] = {  9,  0,  0, 62,  0, 65,  0, 65,  0, 62,  0, 64,  0, 60,  0, 66,  0, 66,  0, 66,  0, 61,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_104[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0, 64,  0, 64,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_105[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0,  0,  0, 24,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_106[] = {  9,  0,  0, 60,  0, 66,  0, 66,  0, 66,  0,  2,  0,  2,  0,  2,  0,  2,  0,  2,  0, 14,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_107[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 70,  0, 88,  0, 96,  0, 88,  0, 70,  0, 65,  0, 64,  0, 64,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_108[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_109[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 73,  0, 73,  0, 73,  0, 73,  0, 73,  0,118,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_110[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_111[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_112[] = {  9,  0,  0, 64,  0, 64,  0, 64,  0, 94,  0, 97,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_113[] = {  9,  0,  0,  1,  0,  1,  0,  1,  0, 61,  0, 67,  0, 65,  0, 65,  0, 65,  0, 67,  0, 61,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_114[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 32,  0, 32,  0, 32,  0, 32,  0, 33,  0, 49,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_115[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0,  1,  0, 62,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_116[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 14,  0, 17,  0, 16,  0, 16,  0, 16,  0, 16,  0,126,  0, 16,  0, 16,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_117[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_118[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0, 20,  0, 20,  0, 34,  0, 34,  0, 65,  0, 65,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_119[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 34,  0, 85,  0, 73,  0, 73,  0, 73,  0, 65,  0, 65,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_120[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 34,  0, 20,  0,  8,  0, 20,  0, 34,  0, 65,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_121[] = {  9,  0,  0, 60,  0, 66,  0,  2,  0, 58,  0, 70,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_122[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 16,  0,  8,  0,  4,  0,  2,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_123[] = {  9,  0,  0,  0,  0,  0,  0,  7,  0,  8,  0,  8,  0,  8,  0,  4,  0, 24,  0, 24,  0,  4,  0,  8,  0,  8,  0,  8,  0,  7,  0,  0,  0};
static const GLubyte Fixed9x15_Character_124[] = {  9,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_125[] = {  9,  0,  0,  0,  0,  0,  0,112,  0,  8,  0,  8,  0,  8,  0, 16,  0, 12,  0, 12,  0, 16,  0,  8,  0,  8,  0,  8,  0,112,  0,  0,  0};
static const GLubyte Fixed9x15_Character_126[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70,  0, 73,  0, 49,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_127[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_128[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_129[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_130[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_131[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_132[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_133[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_134[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_135[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_136[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_137[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_138[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_139[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_140[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_141[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_142[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_143[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_144[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_145[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_146[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_147[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_148[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_149[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_150[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_151[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_152[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_153[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_154[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_155[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_156[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_157[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_158[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_159[] = { 10,  0,  0,  0,  0,  0,  0,  0,  0, 91,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0, 65,  0, 64,  0,  1,  0,109,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_160[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_161[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0,  0,  0,  8,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_162[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 32,  0, 60,  0, 82,  0, 80,  0, 72,  0, 74,  0, 60,  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_163[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 46,  0, 81,  0, 48,  0, 16,  0, 16,  0,124,  0, 16,  0, 16,  0, 17,  0, 14,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_164[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 62,  0, 34,  0, 34,  0, 62,  0, 65,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_165[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0, 62,  0,  8,  0, 62,  0, 20,  0, 34,  0, 65,  0, 65,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_166[] = {  9,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_167[] = {  9,  0,  0,  0,  0,  0,  0, 24,  0, 36,  0,  4,  0, 24,  0, 36,  0, 36,  0, 36,  0, 24,  0, 32,  0, 36,  0, 24,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_168[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_169[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0, 66,  0,153,  0,165,  0,161,  0,165,  0,153,  0, 66,  0, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_170[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,124,  0,  0,  0, 60,  0, 72,  0, 56,  0, 72,  0, 48,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_171[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0, 18,  0, 36,  0, 72,  0, 72,  0, 36,  0, 18,  0,  9,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_172[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  2,  0,  2,  0,126,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_173[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_174[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0, 66,  0,165,  0,169,  0,185,  0,165,  0,185,  0, 66,  0, 60,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_175[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,126,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_176[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 18,  0, 18,  0, 12,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_177[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  8,  0,  8,  0,  8,  0,127,  0,  8,  0,  8,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_178[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,120,  0, 64,  0, 48,  0,  8,  0, 72,  0, 48,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_179[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 48,  0, 72,  0,  8,  0, 16,  0, 72,  0, 48,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_180[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  0,  8,  0,  4,  0,  0,  0};
static const GLubyte Fixed9x15_Character_181[] = {  9,  0,  0,  0,  0, 64,  0, 64,  0, 93,  0, 99,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_182[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  5,  0,  5,  0,  5,  0,  5,  0, 61,  0, 69,  0, 69,  0, 69,  0, 63,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_183[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_184[] = {  9,  0,  0, 24,  0, 36,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_185[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,112,  0, 32,  0, 32,  0, 32,  0, 96,  0, 32,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_186[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,124,  0,  0,  0, 56,  0, 68,  0, 68,  0, 56,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_187[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 72,  0, 36,  0, 18,  0,  9,  0,  9,  0, 18,  0, 36,  0, 72,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_188[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0, 13,  0,  9,  0,  5,  0,115,  0, 33,  0, 32,  0, 32,  0, 96,  0, 32,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_189[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  8,  0,  6,  0,  1,  0,121,  0, 38,  0, 32,  0, 32,  0, 96,  0, 32,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_190[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0, 13,  0,  9,  0,  5,  0, 51,  0, 73,  0,  8,  0, 16,  0, 72,  0, 48,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_191[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 64,  0, 32,  0, 16,  0,  8,  0,  8,  0,  0,  0,  8,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_192[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0,  8,  0, 16,  0, 32,  0};
static const GLubyte Fixed9x15_Character_193[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_194[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0, 34,  0, 20,  0,  8,  0};
static const GLubyte Fixed9x15_Character_195[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0, 78,  0, 49,  0,  0,  0};
static const GLubyte Fixed9x15_Character_196[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 28,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_197[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0,127,  0, 65,  0, 65,  0, 34,  0, 20,  0, 28,  0, 34,  0, 28,  0,  0,  0};
static const GLubyte Fixed9x15_Character_198[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 79,  0, 72,  0, 72,  0, 72,  0,126,  0, 72,  0, 72,  0, 72,  0, 72,  0, 55,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_199[] = {  9,  0,  0, 24,  0, 36,  0, 12,  0, 62,  0, 65,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_200[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0,127,  0,  0,  0,  8,  0, 16,  0, 32,  0};
static const GLubyte Fixed9x15_Character_201[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0,127,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_202[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0,127,  0,  0,  0, 34,  0, 20,  0,  8,  0};
static const GLubyte Fixed9x15_Character_203[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,127,  0, 32,  0, 32,  0, 32,  0, 60,  0, 32,  0, 32,  0,127,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_204[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 62,  0,  0,  0,  8,  0, 16,  0, 32,  0};
static const GLubyte Fixed9x15_Character_205[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 62,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_206[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 62,  0,  0,  0, 34,  0, 20,  0,  8,  0};
static const GLubyte Fixed9x15_Character_207[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 62,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_208[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,124,  0, 33,  0, 33,  0, 33,  0, 33,  0,225,  0, 33,  0, 33,  0, 33,  0,124,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_209[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 67,  0, 69,  0, 73,  0, 73,  0, 81,  0, 97,  0, 65,  0,  0,  0, 78,  0, 49,  0,  0,  0};
static const GLubyte Fixed9x15_Character_210[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0, 16,  0, 32,  0};
static const GLubyte Fixed9x15_Character_211[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_212[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 20,  0,  8,  0};
static const GLubyte Fixed9x15_Character_213[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 78,  0, 49,  0,  0,  0};
static const GLubyte Fixed9x15_Character_214[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_215[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 34,  0, 20,  0,  8,  0, 20,  0, 34,  0, 65,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_216[] = {  9,  0,  0,  0,  0,  0,  0, 64,  0, 62,  0, 97,  0, 81,  0, 81,  0, 73,  0, 73,  0, 69,  0, 69,  0, 67,  0, 62,  0,  1,  0,  0,  0};
static const GLubyte Fixed9x15_Character_217[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  8,  0, 16,  0, 32,  0};
static const GLubyte Fixed9x15_Character_218[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_219[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0, 34,  0, 20,  0,  8,  0};
static const GLubyte Fixed9x15_Character_220[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0,  0,  0, 34,  0, 34,  0,  0,  0};
static const GLubyte Fixed9x15_Character_221[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  8,  0,  8,  0,  8,  0, 20,  0, 34,  0, 65,  0, 65,  0,  0,  0,  8,  0,  4,  0,  2,  0};
static const GLubyte Fixed9x15_Character_222[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 64,  0, 64,  0, 64,  0,126,  0, 65,  0, 65,  0, 65,  0,126,  0, 64,  0, 64,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_223[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 44,  0, 34,  0, 34,  0, 34,  0, 36,  0,104,  0, 36,  0, 34,  0, 34,  0, 28,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_224[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0,  4,  0,  8,  0, 16,  0,  0,  0};
static const GLubyte Fixed9x15_Character_225[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0,  8,  0,  4,  0,  2,  0,  0,  0};
static const GLubyte Fixed9x15_Character_226[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0, 34,  0, 20,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_227[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0, 38,  0, 25,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_228[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0, 34,  0, 34,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_229[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 67,  0, 65,  0, 63,  0,  1,  0,  1,  0, 62,  0,  0,  0, 12,  0, 18,  0, 12,  0,  0,  0};
static const GLubyte Fixed9x15_Character_230[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 55,  0, 73,  0, 72,  0, 62,  0,  9,  0, 73,  0, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_231[] = {  9,  0,  0, 24,  0, 36,  0, 12,  0, 62,  0, 65,  0, 64,  0, 64,  0, 64,  0, 65,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_232[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0,127,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0, 16,  0, 32,  0,  0,  0};
static const GLubyte Fixed9x15_Character_233[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0,127,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0,  4,  0,  2,  0,  0,  0};
static const GLubyte Fixed9x15_Character_234[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0,127,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 20,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_235[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 64,  0, 64,  0,127,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 34,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_236[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0,  8,  0, 16,  0, 32,  0,  0,  0};
static const GLubyte Fixed9x15_Character_237[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0, 16,  0,  8,  0,  4,  0,  0,  0};
static const GLubyte Fixed9x15_Character_238[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0, 68,  0, 40,  0, 16,  0,  0,  0};
static const GLubyte Fixed9x15_Character_239[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  8,  0,  8,  0,  8,  0,  8,  0,  8,  0, 56,  0,  0,  0, 36,  0, 36,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_240[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  4,  0, 40,  0, 24,  0, 36,  0,  0,  0};
static const GLubyte Fixed9x15_Character_241[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 97,  0, 94,  0,  0,  0, 78,  0, 49,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_242[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0, 16,  0, 32,  0,  0,  0};
static const GLubyte Fixed9x15_Character_243[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0,  8,  0,  4,  0,  2,  0,  0,  0};
static const GLubyte Fixed9x15_Character_244[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 20,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_245[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 78,  0, 49,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_246[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0, 65,  0, 65,  0, 65,  0, 65,  0, 65,  0, 62,  0,  0,  0, 34,  0, 34,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_247[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0, 28,  0,  8,  0,  0,  0,127,  0,  0,  0,  8,  0, 28,  0,  8,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_248[] = {  9,  0,  0,  0,  0,  0,  0, 64,  0, 62,  0, 81,  0, 81,  0, 73,  0, 69,  0, 69,  0, 62,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_249[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0,  8,  0, 16,  0, 32,  0,  0,  0};
static const GLubyte Fixed9x15_Character_250[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0,  8,  0,  4,  0,  2,  0,  0,  0};
static const GLubyte Fixed9x15_Character_251[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0, 34,  0, 20,  0,  8,  0,  0,  0};
static const GLubyte Fixed9x15_Character_252[] = {  9,  0,  0,  0,  0,  0,  0,  0,  0, 61,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0, 36,  0, 36,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_253[] = {  9,  0,  0, 60,  0, 66,  0,  2,  0, 58,  0, 70,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0, 16,  0,  8,  0,  4,  0,  0,  0};
static const GLubyte Fixed9x15_Character_254[] = {  9,  0,  0, 64,  0, 64,  0, 64,  0, 94,  0, 97,  0, 65,  0, 65,  0, 97,  0, 94,  0, 64,  0, 64,  0, 64,  0,  0,  0,  0,  0,  0,  0};
static const GLubyte Fixed9x15_Character_255[] = {  9,  0,  0, 60,  0, 66,  0,  2,  0, 58,  0, 70,  0, 66,  0, 66,  0, 66,  0, 66,  0, 66,  0,  0,  0, 36,  0, 36,  0,  0,  0,  0,  0};

/* The font characters mapping: */
static const GLubyte* Fixed9x15_Character_Map[] = {Fixed9x15_Character_000,Fixed9x15_Character_001,Fixed9x15_Character_002,Fixed9x15_Character_003,Fixed9x15_Character_004,Fixed9x15_Character_005,Fixed9x15_Character_006,Fixed9x15_Character_007,Fixed9x15_Character_008,Fixed9x15_Character_009,Fixed9x15_Character_010,Fixed9x15_Character_011,Fixed9x15_Character_012,Fixed9x15_Character_013,Fixed9x15_Character_014,Fixed9x15_Character_015,
	Fixed9x15_Character_016,Fixed9x15_Character_017,Fixed9x15_Character_018,Fixed9x15_Character_019,Fixed9x15_Character_020,Fixed9x15_Character_021,Fixed9x15_Character_022,Fixed9x15_Character_023,Fixed9x15_Character_024,Fixed9x15_Character_025,Fixed9x15_Character_026,Fixed9x15_Character_027,Fixed9x15_Character_028,Fixed9x15_Character_029,Fixed9x15_Character_030,Fixed9x15_Character_031,
	Fixed9x15_Character_032,Fixed9x15_Character_033,Fixed9x15_Character_034,Fixed9x15_Character_035,Fixed9x15_Character_036,Fixed9x15_Character_037,Fixed9x15_Character_038,Fixed9x15_Character_039,Fixed9x15_Character_040,Fixed9x15_Character_041,Fixed9x15_Character_042,Fixed9x15_Character_043,Fixed9x15_Character_044,Fixed9x15_Character_045,Fixed9x15_Character_046,Fixed9x15_Character_047,
	Fixed9x15_Character_048,Fixed9x15_Character_049,Fixed9x15_Character_050,Fixed9x15_Character_051,Fixed9x15_Character_052,Fixed9x15_Character_053,Fixed9x15_Character_054,Fixed9x15_Character_055,Fixed9x15_Character_056,Fixed9x15_Character_057,Fixed9x15_Character_058,Fixed9x15_Character_059,Fixed9x15_Character_060,Fixed9x15_Character_061,Fixed9x15_Character_062,Fixed9x15_Character_063,
	Fixed9x15_Character_064,Fixed9x15_Character_065,Fixed9x15_Character_066,Fixed9x15_Character_067,Fixed9x15_Character_068,Fixed9x15_Character_069,Fixed9x15_Character_070,Fixed9x15_Character_071,Fixed9x15_Character_072,Fixed9x15_Character_073,Fixed9x15_Character_074,Fixed9x15_Character_075,Fixed9x15_Character_076,Fixed9x15_Character_077,Fixed9x15_Character_078,Fixed9x15_Character_079,
	Fixed9x15_Character_080,Fixed9x15_Character_081,Fixed9x15_Character_082,Fixed9x15_Character_083,Fixed9x15_Character_084,Fixed9x15_Character_085,Fixed9x15_Character_086,Fixed9x15_Character_087,Fixed9x15_Character_088,Fixed9x15_Character_089,Fixed9x15_Character_090,Fixed9x15_Character_091,Fixed9x15_Character_092,Fixed9x15_Character_093,Fixed9x15_Character_094,Fixed9x15_Character_095,
	Fixed9x15_Character_096,Fixed9x15_Character_097,Fixed9x15_Character_098,Fixed9x15_Character_099,Fixed9x15_Character_100,Fixed9x15_Character_101,Fixed9x15_Character_102,Fixed9x15_Character_103,Fixed9x15_Character_104,Fixed9x15_Character_105,Fixed9x15_Character_106,Fixed9x15_Character_107,Fixed9x15_Character_108,Fixed9x15_Character_109,Fixed9x15_Character_110,Fixed9x15_Character_111,
	Fixed9x15_Character_112,Fixed9x15_Character_113,Fixed9x15_Character_114,Fixed9x15_Character_115,Fixed9x15_Character_116,Fixed9x15_Character_117,Fixed9x15_Character_118,Fixed9x15_Character_119,Fixed9x15_Character_120,Fixed9x15_Character_121,Fixed9x15_Character_122,Fixed9x15_Character_123,Fixed9x15_Character_124,Fixed9x15_Character_125,Fixed9x15_Character_126,Fixed9x15_Character_032,
	Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,
	Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,Fixed9x15_Character_032,
	Fixed9x15_Character_160,Fixed9x15_Character_161,Fixed9x15_Character_162,Fixed9x15_Character_163,Fixed9x15_Character_164,Fixed9x15_Character_165,Fixed9x15_Character_166,Fixed9x15_Character_167,Fixed9x15_Character_168,Fixed9x15_Character_169,Fixed9x15_Character_170,Fixed9x15_Character_171,Fixed9x15_Character_172,Fixed9x15_Character_173,Fixed9x15_Character_174,Fixed9x15_Character_175,
	Fixed9x15_Character_176,Fixed9x15_Character_177,Fixed9x15_Character_178,Fixed9x15_Character_179,Fixed9x15_Character_180,Fixed9x15_Character_181,Fixed9x15_Character_182,Fixed9x15_Character_183,Fixed9x15_Character_184,Fixed9x15_Character_185,Fixed9x15_Character_186,Fixed9x15_Character_187,Fixed9x15_Character_188,Fixed9x15_Character_189,Fixed9x15_Character_190,Fixed9x15_Character_191,
	Fixed9x15_Character_192,Fixed9x15_Character_193,Fixed9x15_Character_194,Fixed9x15_Character_195,Fixed9x15_Character_196,Fixed9x15_Character_197,Fixed9x15_Character_198,Fixed9x15_Character_199,Fixed9x15_Character_200,Fixed9x15_Character_201,Fixed9x15_Character_202,Fixed9x15_Character_203,Fixed9x15_Character_204,Fixed9x15_Character_205,Fixed9x15_Character_206,Fixed9x15_Character_207,
	Fixed9x15_Character_208,Fixed9x15_Character_209,Fixed9x15_Character_210,Fixed9x15_Character_211,Fixed9x15_Character_212,Fixed9x15_Character_213,Fixed9x15_Character_214,Fixed9x15_Character_215,Fixed9x15_Character_216,Fixed9x15_Character_217,Fixed9x15_Character_218,Fixed9x15_Character_219,Fixed9x15_Character_220,Fixed9x15_Character_221,Fixed9x15_Character_222,Fixed9x15_Character_223,
	Fixed9x15_Character_224,Fixed9x15_Character_225,Fixed9x15_Character_226,Fixed9x15_Character_227,Fixed9x15_Character_228,Fixed9x15_Character_229,Fixed9x15_Character_230,Fixed9x15_Character_231,Fixed9x15_Character_232,Fixed9x15_Character_233,Fixed9x15_Character_234,Fixed9x15_Character_235,Fixed9x15_Character_236,Fixed9x15_Character_237,Fixed9x15_Character_238,Fixed9x15_Character_239,
	Fixed9x15_Character_240,Fixed9x15_Character_241,Fixed9x15_Character_242,Fixed9x15_Character_243,Fixed9x15_Character_244,Fixed9x15_Character_245,Fixed9x15_Character_246,Fixed9x15_Character_247,Fixed9x15_Character_248,Fixed9x15_Character_249,Fixed9x15_Character_250,Fixed9x15_Character_251,Fixed9x15_Character_252,Fixed9x15_Character_253,Fixed9x15_Character_254,Fixed9x15_Character_255,NULL};

/* The font structure: */
static SFG_Font fgFontFixed9x15 = { "-misc-fixed-medium-r-normal--15-140-75-75-C-90-iso8859-1", 256, 16, Fixed9x15_Character_Map, 0, 4 };


#endif /* FONTES_H */
