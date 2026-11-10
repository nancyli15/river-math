#include <stdio.h>
#include <string.h>

#include "rivermath/card.h"
#include "rivermath/evaluate.h"

#define RM_VERSION "0.1.0"

enum { EXIT_BAD_INPUT = 1, EXIT_BAD_USAGE = 2 };

static void usage(FILE *out) {
    fprintf(out,
            "RiverMath %s: Texas Hold'em evaluator and equity calculator\n"
            "\n"
            "usage:\n"
            "  rivermath cards <cards>    parse and show cards, e.g. rivermath cards AhKd\n"
            "  rivermath eval <cards>     score a 5 to 7 card hand, e.g. rivermath eval AhKhQhJhTh2c3d\n"
            "  rivermath --version\n"
            "  rivermath --help\n"
            "\n"
            "Cards are rank + suit: ranks 23456789TJQKA, suits c d h s.\n",
            RM_VERSION);
}

/*
 * Join the arguments with spaces (so "Ah Kd" and "AhKd" both work) and parse
 * them. Prints an error and returns a negative number on failure, otherwise
 * returns the number of cards.
 */
static int parse_card_args(int num_args, char **args, rm_cards *cards) {
    char input[512] = "";
    for (int i = 0; i < num_args; i++) {
        if (strlen(input) + strlen(args[i]) + 2 > sizeof input) {
            fprintf(stderr, "error: input too long\n");
            return RM_ERR_SYNTAX;
        }
        if (i > 0) strcat(input, " ");
        strcat(input, args[i]);
    }

    int num_cards = rm_parse_cards(input, cards);
    if (num_cards == RM_ERR_DUPLICATE)
        fprintf(stderr, "error: duplicate card in \"%s\"\n", input);
    else if (num_cards < 0)
        fprintf(stderr, "error: invalid cards \"%s\" (expected e.g. AhKd)\n", input);
    return num_cards;
}

static int command_cards(int num_args, char **args) {
    rm_cards cards;
    int num_cards = parse_card_args(num_args, args, &cards);
    if (num_cards < 0) return EXIT_BAD_INPUT;

    char text[RM_NUM_CARDS * 2 + 1];
    rm_format_cards(cards, text, sizeof text);
    printf("Cards:   %s\n", text);
    printf("Count:   %d\n", num_cards);
    printf("Bitmask: 0x%016llx\n", (unsigned long long)cards);
    return 0;
}

static int command_eval(int num_args, char **args) {
    rm_cards hand;
    int num_cards = parse_card_args(num_args, args, &hand);
    if (num_cards < 0) return EXIT_BAD_INPUT;
    if (num_cards < 5 || num_cards > 7) {
        fprintf(stderr, "error: eval needs 5 to 7 cards, got %d\n", num_cards);
        return EXIT_BAD_INPUT;
    }

    rm_hand_value value = rm_evaluate_bitwise(hand);
    char cards_text[16], description[64];
    rm_format_cards(hand, cards_text, sizeof cards_text);
    rm_describe_hand_value(value, description, sizeof description);
    printf("Hand:   %s\n", cards_text);
    printf("Result: %s\n", description);
    printf("Value:  0x%06x (bigger is better)\n", value);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return EXIT_BAD_USAGE;
    }
    const char *command = argv[1];
    int num_args = argc - 2;
    char **args = argv + 2;

    if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        usage(stdout);
        return 0;
    }
    if (strcmp(command, "--version") == 0) {
        printf("rivermath %s\n", RM_VERSION);
        return 0;
    }
    if (strcmp(command, "cards") == 0) return command_cards(num_args, args);
    if (strcmp(command, "eval") == 0) return command_eval(num_args, args);

    fprintf(stderr, "error: unknown command \"%s\"\n\n", command);
    usage(stderr);
    return EXIT_BAD_USAGE;
}
