#include <stdio.h>
#include <string.h>

#include "rivermath/card.h"

#define RM_VERSION "0.0.1"

static void usage(FILE *out) {
    fprintf(out,
            "RiverMath %s: Texas Hold'em evaluator and equity calculator\n"
            "\n"
            "usage:\n"
            "  rivermath cards <cards>    parse and show cards, e.g. rivermath cards AhKd\n"
            "  rivermath --version\n"
            "  rivermath --help\n",
            RM_VERSION);
}

static int command_cards(int num_args, char **args) {
    if (num_args < 1) {
        fprintf(stderr, "error: no cards given\n");
        return 2;
    }

    /* Join all remaining args so "rivermath cards Ah Kd" also works. */
    char input[512] = "";
    for (int i = 0; i < num_args; i++) {
        if (strlen(input) + strlen(args[i]) + 2 > sizeof input) {
            fprintf(stderr, "error: input too long\n");
            return 2;
        }
        if (i > 0) strcat(input, " ");
        strcat(input, args[i]);
    }

    rm_cards cards;
    int num_cards = rm_parse_cards(input, &cards);
    if (num_cards == RM_ERR_DUPLICATE) {
        fprintf(stderr, "error: duplicate card in \"%s\"\n", input);
        return 1;
    }
    if (num_cards < 0) {
        fprintf(stderr, "error: invalid cards \"%s\" (expected e.g. AhKd, ranks 2-9TJQKA, suits cdhs)\n",
                input);
        return 1;
    }

    char text[RM_NUM_CARDS * 2 + 1];
    rm_format_cards(cards, text, sizeof text);
    printf("Cards:   %s\n", text);
    printf("Count:   %d\n", num_cards);
    printf("Bitmask: 0x%016llx\n", (unsigned long long)cards);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }
    const char *command = argv[1];
    if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        usage(stdout);
        return 0;
    }
    if (strcmp(command, "--version") == 0) {
        printf("rivermath %s\n", RM_VERSION);
        return 0;
    }
    if (strcmp(command, "cards") == 0) return command_cards(argc - 2, argv + 2);

    fprintf(stderr, "error: unknown command \"%s\"\n\n", command);
    usage(stderr);
    return 2;
}
