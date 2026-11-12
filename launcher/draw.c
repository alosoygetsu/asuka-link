/* Rectangles, the baked font and the disc. */
#include "launcher.h"
#include "font_data.inc"

static SDL_Renderer *s_r; /* NULL: measuring only (--check) */
static Uint8 s_alpha = 255;

void draw_init(SDL_Renderer *r)
{
    s_r = r;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
}

void fill_alpha(int x, int y, int w, int h, Uint32 colour, Uint8 alpha)
{
    if (!s_r) {
        return;
    }
    SDL_FRect rect = {(float)x, (float)y, (float)w, (float)h};
    SDL_SetRenderDrawColor(s_r, colour >> 16 & 255, colour >> 8 & 255, colour & 255, alpha);
    SDL_RenderFillRect(s_r, &rect);
}

void fill(int x, int y, int w, int h, Uint32 colour) { fill_alpha(x, y, w, h, colour, s_alpha); }

/* A still picture (the home scene's city), drawn once into a texture and copied every frame after: cache_begin says
   whether to draw it now (true: draw, then cache_end), cache_end puts it on screen. Without a renderer (--check) or a
   texture, it's drawn straight every time */
static SDL_Texture *s_cache;
static bool s_cache_ok;

bool cache_begin(void)
{
    if (!s_r) {
        return true;
    }
    if (s_cache && s_cache_ok) {
        return false;
    }
    if (!s_cache) {
        s_cache = SDL_CreateTexture(s_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H);
        if (!s_cache) {
            return true;
        }
        SDL_SetTextureBlendMode(s_cache, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(s_cache, SDL_SCALEMODE_NEAREST);
    }
    SDL_SetRenderTarget(s_r, s_cache);
    SDL_SetRenderDrawColor(s_r, 0, 0, 0, 0);
    SDL_RenderClear(s_r);
    return true;
}

void cache_end(void)
{
    if (!s_r || !s_cache) {
        return;
    }
    if (SDL_GetRenderTarget(s_r) == s_cache) {
        SDL_SetRenderTarget(s_r, NULL);
        s_cache_ok = true;
    }
    SDL_RenderTexture(s_r, s_cache, NULL, NULL);
}

void cache_lost(void) { s_cache_ok = false; } /* the device lost its textures: drawn again next time */

void outline(int x, int y, int w, int h, Uint32 colour)
{
    fill(x, y, w, 1, colour);
    fill(x, y + h - 1, w, 1, colour);
    fill(x, y, 1, h, colour);
    fill(x + w - 1, y, 1, h, colour);
}

/* Asuka Link's panels: a thick edge, the corners cut */
void panel(int x, int y, int w, int h)
{
    fill_alpha(x, y, w, h, C_PANEL, 230);
    outline(x, y, w, h, C_EDGE);
    outline(x + 1, y + 1, w - 2, h - 2, C_EDGE);
    fill(x, y, 2, 2, C_BG);
    fill(x + w - 2, y + h - 2, 2, 2, C_BG);
}

/* the old screen's soft glow from the top */
void background(int w, int h)
{
    const int step = SDL_max(2, h / 135);
    fill(0, 0, w, h, C_BG);
    for (int y = 0; y < h; y += step) {
        const float k = 1 - (float)y / h;
        fill_alpha(0, y, w, step, C_GLOW, (Uint8)(70 * k * k));
    }
}

static int glyph(Uint32 cp)
{
    static short index[0x180];
    static bool ready;
    if (!ready) {
        for (int i = 0; i < (int)SDL_arraysize(FONT); i++) {
            index[FONT[i].cp] = (short)i;
        }
        ready = true;
    }
    return index[cp < 0x180 ? cp : '?'];
}

/* a line of text: each glyph row's runs of pixels as one rectangle, all of the line sent at once (one pixel a
   rectangle was most of the launcher's time, now that home redraws 20 times a second) */
static int line(int x, int y, int scale, Uint32 colour, const char *s, const char *end)
{
    static SDL_FRect rects[2048];
    int start = x, n = 0;
    while (s < end && *s) {
        const int g = glyph(SDL_StepUTF8(&s, NULL));
        for (int row = 0; s_r && row < FONT_H; row++) {
            int bits = FONT[g].rows[row], column = 0;
            while (bits) {
                if (!(bits & 1)) {
                    bits >>= 1, column++;
                    continue;
                }
                const int from = column;
                while (bits & 1) {
                    bits >>= 1, column++;
                }
                if (n < (int)SDL_arraysize(rects)) {
                    rects[n++] = (SDL_FRect){(float)(x + from * scale), (float)(y + row * scale), (float)((column - from) * scale), (float)scale};
                }
            }
        }
        x += FONT[g].advance * scale;
    }
    if (s_r && n) {
        SDL_SetRenderDrawColor(s_r, colour >> 16 & 255, colour >> 8 & 255, colour & 255, s_alpha);
        SDL_RenderFillRects(s_r, rects, n);
    }
    return x - start;
}

static int span_width(const char *s, const char *end)
{
    SDL_Renderer *kept = s_r;
    s_r = NULL;
    const int width = line(0, 0, 1, 0, s, end);
    s_r = kept;
    return width;
}

int text_width(const char *s, int scale) { return span_width(s, s + SDL_strlen(s)) * scale; }

static void shadow(int x, int y, int scale, const char *s, const char *end)
{
    s_alpha = 160;
    line(x + 1, y + 1, scale, 0, s, end);
    s_alpha = 255;
}

void text(int x, int y, int scale, Uint32 colour, const char *s)
{
    const char *end = s + SDL_strlen(s);
    shadow(x, y, scale, s, end);
    line(x, y, scale, colour, s, end);
}

void text_flat(int x, int y, Uint32 colour, const char *s) { line(x, y, 1, colour, s, s + SDL_strlen(s)); }

void text_sel(int x, int y, bool sel, Uint32 colour, const char *s)
{
    if (sel) {
        text_flat(x, y, C_ON_SELECT, s);
    } else {
        text(x, y, 1, colour, s);
    }
}

void text_centre(int y, int scale, Uint32 colour, const char *s) { text((SCREEN_W - text_width(s, scale)) / 2, y, scale, colour, s); }

void text_right(int right, int y, Uint32 colour, const char *s) { text(right - text_width(s, 1), y, 1, colour, s); }

int wrap(int x, int y, int width, Uint32 colour, const char *s)
{
    int lines = 0;
    while (*s) {
        const char *fit = s, *p = s;
        while (*p) {
            while (*p && *p != ' ') {
                p++;
            }
            if (span_width(s, p) > width && fit != s) {
                break;
            }
            fit = p;
            while (*p == ' ') {
                p++;
            }
        }
        shadow(x, y + lines * LINE_H, 1, s, fit);
        line(x, y + lines * LINE_H, 1, colour, s, fit);
        lines++;
        s = fit;
        while (*s == ' ') {
            s++;
        }
    }
    return lines;
}

void triangle(int x, int y, int half, int dir, Uint32 colour)
{
    for (int i = 0; i <= half; i++) {
        const int column = dir > 0 ? x + i : x + half - i;
        fill(column, y - (half - i), 1, (half - i) * 2 + 1, colour);
    }
}
