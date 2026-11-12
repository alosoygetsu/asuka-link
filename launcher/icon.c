/* Asuka Link's icon on the menu and the language flags, drawn from numbers: no image files. The icon's pixels come from
   tools/asuka_icon.py, the same as the .exe's (platform/dw3.ico). */
#include "launcher.h"
#include "icon_data.inc"

void art_icon(int x, int y)
{
    for (int j = 0; j < 32; j++) {
        for (int i = 0; i < 32; i++) {
            const char c = ICON[j][i];
            if (c != ' ') {
                fill(x + i, y + j, 1, 1, c == 'e' ? C_EDGE : c == 'c' ? C_LINK : c == 'y' ? C_GOLD : C_BG);
            }
        }
    }
}

enum { FLAG_W = 16, FLAG_H = 10 };

/* a flag (en, es, fr, de, it) of FLAG_W x FLAG_H with a thin edge */
void art_flag(int x, int y, int lang)
{
    for (int j = 0; j < FLAG_H; j++) {
        for (int i = 0; i < FLAG_W; i++) {
            Uint32 c = C_EDGE;
            const int third = i * 3 / FLAG_W;
            if (lang == 1) {
                c = j < 3 || j > 6 ? 0xC60B1E : 0xFFC400;
            } else if (lang == 2 || lang == 4) {
                c = third == 0 ? (lang == 2 ? 0x0055A4 : 0x009246) : third == 1 ? 0xF4F4F4 : (lang == 2 ? 0xEF4135 : 0xCE2B37);
            } else if (lang == 3) {
                c = j < 4 ? 0x151515 : j < 7 ? 0xDD0000 : 0xFFCE00;
            } else { /* the Union Jack, as simply as it goes in 16x10 */
                const int d1 = SDL_abs(i * 10 - j * 16 - 5), d2 = SDL_abs((FLAG_W - 1 - i) * 10 - j * 16 - 5);
                const bool cross_w = SDL_abs(2 * i - 15) <= 4 || SDL_abs(2 * j - 9) <= 3, cross_r = SDL_abs(2 * i - 15) <= 1 || SDL_abs(2 * j - 9) <= 1;
                c = cross_r ? 0xC8102E : cross_w ? 0xFFFFFF : d1 < 9 || d2 < 9 ? (d1 < 4 || d2 < 4 ? 0xC8102E : 0xFFFFFF) : 0x1F3C88;
            }
            fill(x + i, y + j, 1, 1, c);
        }
    }
    outline(x - 1, y - 1, FLAG_W + 2, FLAG_H + 2, C_EDGE);
}
