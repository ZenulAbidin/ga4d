#include "text.h"

#include <GLFW/glfw3.h>

#include <ctype.h>
#include <string.h>

static void glyph_rows(char c, unsigned char rows[7])
{
    static const unsigned char blank[7] = { 0, 0, 0, 0, 0, 0, 0 };
    const unsigned char *src = blank;

    c = (char)toupper((unsigned char)c);

    switch (c) {
    case 'A': { static const unsigned char r[7] = { 14, 17, 17, 31, 17, 17, 17 }; src = r; } break;
    case 'B': { static const unsigned char r[7] = { 30, 17, 17, 30, 17, 17, 30 }; src = r; } break;
    case 'C': { static const unsigned char r[7] = { 14, 17, 16, 16, 16, 17, 14 }; src = r; } break;
    case 'D': { static const unsigned char r[7] = { 30, 17, 17, 17, 17, 17, 30 }; src = r; } break;
    case 'E': { static const unsigned char r[7] = { 31, 16, 16, 30, 16, 16, 31 }; src = r; } break;
    case 'F': { static const unsigned char r[7] = { 31, 16, 16, 30, 16, 16, 16 }; src = r; } break;
    case 'G': { static const unsigned char r[7] = { 14, 17, 16, 23, 17, 17, 14 }; src = r; } break;
    case 'H': { static const unsigned char r[7] = { 17, 17, 17, 31, 17, 17, 17 }; src = r; } break;
    case 'I': { static const unsigned char r[7] = { 31, 4, 4, 4, 4, 4, 31 }; src = r; } break;
    case 'J': { static const unsigned char r[7] = { 1, 1, 1, 1, 17, 17, 14 }; src = r; } break;
    case 'K': { static const unsigned char r[7] = { 17, 18, 20, 24, 20, 18, 17 }; src = r; } break;
    case 'L': { static const unsigned char r[7] = { 16, 16, 16, 16, 16, 16, 31 }; src = r; } break;
    case 'M': { static const unsigned char r[7] = { 17, 27, 21, 21, 17, 17, 17 }; src = r; } break;
    case 'N': { static const unsigned char r[7] = { 17, 25, 21, 19, 17, 17, 17 }; src = r; } break;
    case 'O': { static const unsigned char r[7] = { 14, 17, 17, 17, 17, 17, 14 }; src = r; } break;
    case 'P': { static const unsigned char r[7] = { 30, 17, 17, 30, 16, 16, 16 }; src = r; } break;
    case 'Q': { static const unsigned char r[7] = { 14, 17, 17, 17, 21, 18, 13 }; src = r; } break;
    case 'R': { static const unsigned char r[7] = { 30, 17, 17, 30, 20, 18, 17 }; src = r; } break;
    case 'S': { static const unsigned char r[7] = { 15, 16, 16, 14, 1, 1, 30 }; src = r; } break;
    case 'T': { static const unsigned char r[7] = { 31, 4, 4, 4, 4, 4, 4 }; src = r; } break;
    case 'U': { static const unsigned char r[7] = { 17, 17, 17, 17, 17, 17, 14 }; src = r; } break;
    case 'V': { static const unsigned char r[7] = { 17, 17, 17, 17, 17, 10, 4 }; src = r; } break;
    case 'W': { static const unsigned char r[7] = { 17, 17, 17, 21, 21, 27, 17 }; src = r; } break;
    case 'X': { static const unsigned char r[7] = { 17, 17, 10, 4, 10, 17, 17 }; src = r; } break;
    case 'Y': { static const unsigned char r[7] = { 17, 17, 10, 4, 4, 4, 4 }; src = r; } break;
    case 'Z': { static const unsigned char r[7] = { 31, 1, 2, 4, 8, 16, 31 }; src = r; } break;
    case '0': { static const unsigned char r[7] = { 14, 17, 19, 21, 25, 17, 14 }; src = r; } break;
    case '1': { static const unsigned char r[7] = { 4, 12, 4, 4, 4, 4, 14 }; src = r; } break;
    case '2': { static const unsigned char r[7] = { 14, 17, 1, 2, 4, 8, 31 }; src = r; } break;
    case '3': { static const unsigned char r[7] = { 30, 1, 1, 14, 1, 1, 30 }; src = r; } break;
    case '4': { static const unsigned char r[7] = { 2, 6, 10, 18, 31, 2, 2 }; src = r; } break;
    case '5': { static const unsigned char r[7] = { 31, 16, 16, 30, 1, 1, 30 }; src = r; } break;
    case '6': { static const unsigned char r[7] = { 14, 16, 16, 30, 17, 17, 14 }; src = r; } break;
    case '7': { static const unsigned char r[7] = { 31, 1, 2, 4, 8, 8, 8 }; src = r; } break;
    case '8': { static const unsigned char r[7] = { 14, 17, 17, 14, 17, 17, 14 }; src = r; } break;
    case '9': { static const unsigned char r[7] = { 14, 17, 17, 15, 1, 1, 14 }; src = r; } break;
    case '-': { static const unsigned char r[7] = { 0, 0, 0, 31, 0, 0, 0 }; src = r; } break;
    case '+': { static const unsigned char r[7] = { 0, 4, 4, 31, 4, 4, 0 }; src = r; } break;
    case '=': { static const unsigned char r[7] = { 0, 0, 31, 0, 31, 0, 0 }; src = r; } break;
    case '.': { static const unsigned char r[7] = { 0, 0, 0, 0, 0, 12, 12 }; src = r; } break;
    case ',': { static const unsigned char r[7] = { 0, 0, 0, 0, 0, 12, 8 }; src = r; } break;
    case ':': { static const unsigned char r[7] = { 0, 12, 12, 0, 12, 12, 0 }; src = r; } break;
    case '/': { static const unsigned char r[7] = { 1, 1, 2, 4, 8, 16, 16 }; src = r; } break;
    case '[': { static const unsigned char r[7] = { 14, 8, 8, 8, 8, 8, 14 }; src = r; } break;
    case ']': { static const unsigned char r[7] = { 14, 2, 2, 2, 2, 2, 14 }; src = r; } break;
    case '(': { static const unsigned char r[7] = { 2, 4, 8, 8, 8, 4, 2 }; src = r; } break;
    case ')': { static const unsigned char r[7] = { 8, 4, 2, 2, 2, 4, 8 }; src = r; } break;
    case '<': { static const unsigned char r[7] = { 2, 4, 8, 16, 8, 4, 2 }; src = r; } break;
    case '>': { static const unsigned char r[7] = { 8, 4, 2, 1, 2, 4, 8 }; src = r; } break;
    case '_': { static const unsigned char r[7] = { 0, 0, 0, 0, 0, 0, 31 }; src = r; } break;
    default:
        if (c != ' ') {
            static const unsigned char unknown[7] = { 31, 17, 2, 4, 4, 0, 4 };
            src = unknown;
        }
        break;
    }

    memcpy(rows, src, 7);
}

void draw_text_quads(float x, float y, float scale, const char *text, Color color)
{
    float cursor = x;
    size_t i;

    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    for (i = 0; text[i] != '\0'; ++i) {
        unsigned char rows[7];
        int row;
        int col;

        if (text[i] == '\n') {
            y += 9.0f * scale;
            cursor = x;
            continue;
        }

        glyph_rows(text[i], rows);
        for (row = 0; row < 7; ++row) {
            for (col = 0; col < 5; ++col) {
                if (rows[row] & (1u << (4 - col))) {
                    float x0 = cursor + (float)col * scale;
                    float y0 = y + (float)row * scale;
                    float x1 = x0 + scale;
                    float y1 = y0 + scale;
                    glVertex2f(x0, y0);
                    glVertex2f(x1, y0);
                    glVertex2f(x1, y1);
                    glVertex2f(x0, y1);
                }
            }
        }
        cursor += 6.0f * scale;
    }
    glEnd();
}
