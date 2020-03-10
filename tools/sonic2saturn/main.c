//Convert Genesis tilemap format (PCCY XAAA AAAA AAAA)
//to Saturn (YXP000000CCCCCCC0AAAAAAAAAAAAAAA)

#include <stdio.h>
#include <stdint.h>

#define GENESIS_PRIORITY (0x8000)
#define GENESIS_PALETTE (0x6000)
#define GENESIS_YFLIP (0x1000)
#define GENESIS_XFLIP (0x800)
#define GENESIS_TILENUM (0x7ff)

#define SATURN_PRIORITY (0x20000000)
#define SATURN_XFLIP (0x40000000)
#define SATURN_YFLIP (0x80000000)

uint16_t byteswap(uint16_t word) {
    uint8_t low = (uint8_t)(word & 0x00ff);
    uint8_t high = (uint8_t)((word & 0xff00) >> 8);
    return (low << 8) | high;
}

uint32_t convert(uint16_t original) {
    int tilenum = original & GENESIS_TILENUM;
    int palette = (original & GENESIS_PALETTE) >> 13;
    uint32_t newtile = tilenum;
    newtile |= (palette << 16);
    if (original & GENESIS_PRIORITY) {
        newtile |= SATURN_PRIORITY;
    }
    if (original & GENESIS_XFLIP) {
        newtile |= SATURN_XFLIP;
    }
    if (original & GENESIS_YFLIP) {
        newtile |= SATURN_YFLIP;
    }
    return newtile;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("Usage: sonic2saturn [in filename].bin [out filename].bin\n");
    }
    FILE *input = fopen(argv[1], "rb");
    FILE *output = fopen(argv[2], "wb");
    uint16_t curr_word;
    uint32_t new_long;
    fseek(input, 0, SEEK_END);
    int size = ftell(input);
    rewind(input);

    for (int i = 0; i < (size >> 1); i++) {
        fread(&curr_word, sizeof(uint16_t), 1, input);
        curr_word = byteswap(curr_word);
        new_long = __builtin_bswap32(convert(curr_word));
        fwrite(&new_long, sizeof(uint32_t), 1, output);
    }
    fclose(input);
    fclose(output);
    return 0;
}
