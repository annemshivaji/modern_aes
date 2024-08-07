#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define BLOCK_LEN 16
#define KEY_LEN 16
#define ROUND_KEYS_SIZE 176

typedef uint8_t state_t[4][4];

// S-box for substitution
static const uint8_t sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB,
    0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4,
    0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71,
    0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
    0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6,
    0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB,
    0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45,
    0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44,
    0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A,
    0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49,
    0x06, 0x24, 0x5C, 0xC6, 0xD0, 0xE3, 0x1F, 0xD3, 0x3C, 0x30, 0xA9, 0x1E, 0xB5, 0xB4, 0x8D,
    0x9B, 0x9C, 0xB0, 0x07, 0xBD, 0xF8, 0x79, 0x2D, 0xFF, 0xC2, 0xA6, 0x9E, 0x1C, 0x74, 0xB9,
    0xD5, 0x84, 0x4B, 0xE8, 0xCE, 0x6D, 0xA1, 0xE4, 0x62, 0x2E, 0x41, 0xAB, 0xF6, 0x80, 0x8C,
    0x0F, 0xBF, 0x71, 0xDA, 0x54, 0x5A, 0x7A, 0x9F, 0xE7, 0x6F, 0x2C, 0x9D, 0xC9, 0xD8, 0x6B
};

// Inverse S-box
static const uint8_t rbox[256] = {
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7,
    0xFB, 0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE,
    0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42,
    0xFC, 0xF4, 0xF7, 0xF2, 0xF9, 0xDF, 0xF6, 0x9D, 0xF0, 0x6D, 0x6C, 0xB0, 0xD3, 0x6E, 0x7F,
    0x9A, 0xB9, 0xDA, 0xB5, 0x1F, 0xB6, 0x99, 0xA7, 0xD4, 0xB0, 0xB1, 0xF6, 0x64, 0x86, 0x4F,
    0x3C, 0xE2, 0x58, 0xA6, 0xC6, 0x5B, 0xE4, 0xC0, 0x9D, 0x1E, 0x1D, 0xA1, 0x89, 0x1C, 0x52,
    0x4E, 0x61, 0x57, 0x6F, 0x5D, 0x0C, 0xB6, 0x0F, 0xA0, 0x62, 0x1E, 0xB5, 0xD0, 0x27, 0x6B,
    0x00, 0x66, 0x82, 0x84, 0xE1, 0x39, 0x9B, 0x2D, 0xD2, 0x2B, 0xA8, 0x65, 0x7E, 0xB7, 0x76
};

// Randomly generated round constants for demonstration purposes
static uint8_t rcon[10] = {0x6A, 0x3C, 0x7D, 0xE1, 0x91, 0xF4, 0xBD, 0xD8, 0x47, 0x5B};

// Key expansion function
void keyexpansion(uint8_t *expandedkey, const uint8_t *key) {
    uint8_t temp[4];
    uint8_t i, j;
    uint8_t k = 0;

    memcpy(expandedkey, key, KEY_LEN);

    for (i = KEY_LEN; i < ROUND_KEYS_SIZE; i += 4) {
        memcpy(temp, &expandedkey[i - 4], 4);

        if (i % KEY_LEN == 0) {
            uint8_t first = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = first;

            for (j = 0; j < 4; ++j) {
                temp[j] = sbox[temp[j]];
            }

            temp[0] ^= rcon[k++];
        }

        for (j = 0; j < 4; ++j) {
            expandedkey[i + j] = expandedkey[i + j - KEY_LEN] ^ temp[j];
        }
    }
}


// SubBytes transformation
void subbytes(state_t *state) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            (*state)[i][j] = sbox[(*state)[i][j]];
        }
    }
}

// ShiftRows transformation (Right Shift)
void shiftrows(state_t *state) {
    uint8_t temp;
    // Shift row 1 right by 1
    temp = (*state)[1][3];
    for (int i = 3; i > 0; --i) {
        (*state)[1][i] = (*state)[1][i - 1];
    }
    (*state)[1][0] = temp;
    // Shift row 2 right by 2
    uint8_t temp1 = (*state)[2][3];
    uint8_t temp2 = (*state)[2][2];
    (*state)[2][3] = (*state)[2][1];
    (*state)[2][2] = (*state)[2][0];
    (*state)[2][1] = temp1;
    (*state)[2][0] = temp2;
    // Shift row 3 right by 3
    temp = (*state)[3][0];
    for (int i = 0; i < 3; ++i) {
        (*state)[3][i] = (*state)[3][i + 1];
    }
    (*state)[3][3] = temp;
}

// MixColumns transformation
static const uint8_t mix[4][4] = {
    {0x02, 0x03, 0x01, 0x01},
    {0x01, 0x02, 0x03, 0x01},
    {0x01, 0x01, 0x02, 0x03},
    {0x03, 0x01, 0x01, 0x02}
};

uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
            p ^= a;
        }
        uint8_t high_bit_set = a & 0x80;
        a <<= 1;
        if (high_bit_set) {
            a ^= 0x1B;
        }
        b >>= 1;
    }
    return p;
}

void mixcolumns(state_t *state) {
    uint8_t temp[4];
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            temp[i] = (*state)[i][j];
        }
        for (int i = 0; i < 4; ++i) {
            (*state)[i][j] = gmul(mix[i][0], temp[0]) ^ gmul(mix[i][1], temp[1]) ^ gmul(mix[i][2], temp[2]) ^ gmul(mix[i][3], temp[3]);
        }
    }
}

// AddRoundKey transformation
void addroundkey(state_t *state, uint8_t *key) {
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            (*state)[i][j] ^= key[j * 4 + i];
        }
    }
}

// Convert block to state array
void block2state(const uint8_t *block, state_t *state) {
    for (int i = 0; i < 16; ++i) {
        (*state)[i % 4][i / 4] = block[i];
    }
}

// Convert state array to block
void state2block(const state_t *state, uint8_t *block) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            block[i + 4 * j] = (*state)[i][j];
        }
    }
}

// Print state for debugging
void printstate(const state_t *state) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%02X ", (*state)[i][j]);
        }
        printf("\n");
    }
}

// Initialization function
int init() {
    uint8_t block[BLOCK_LEN] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };

    uint8_t key[KEY_LEN] = {
        0x24, 0x75, 0xA2, 0xB3,
        0x34, 0x75, 0x56, 0x88,
        0x31, 0xE2, 0x12, 0x00,
        0x13, 0xAA, 0x54, 0x87
    };

    uint8_t expandedkey[ROUND_KEYS_SIZE];
    state_t state;

    // Initialize round constants
    srand(time(NULL));  // Seed random number generator
    for (int i = 0; i < 10; ++i) {
        rcon[i] = rand() % 256;
    }

    // Perform key expansion
    keyexpansion(expandedkey, key);

    // Transform block to state
    block2state(block, &state);

    // AES operations
    subbytes(&state);
    shiftrows(&state);
    mixcolumns(&state);
    addroundkey(&state, expandedkey);

    // Print state
    printstate(&state);

    return 0;
}

int main() {
    init();
    return 0;
}
