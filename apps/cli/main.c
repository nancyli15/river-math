#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rivermath/card.h"
#include "rivermath/equity.h"
#include "rivermath/evaluate.h"
#include "rivermath/random.h"

#define RM_VERSION "1.0.0"

enum { EXIT_BAD_INPUT = 1, EXIT_BAD_USAGE = 2 };

static void usage(FILE *out) {
    fprintf(out,
            "RiverMath %s: Texas Hold'em evaluator and equity calculator\n"
            "\n"
            "usage:\n"
            "  rivermath cards <cards>    parse and show cards, e.g. rivermath cards AhKd\n"
            "  rivermath eval <cards>     score a 5 to 7 card hand, e.g. rivermath eval AhKhQhJhTh2c3d\n"
            "  rivermath equity <player>... [--board <cards>] [--dead <cards>] [--samples N] [--seed N]\n"
            "                             each player is 2 cards (AhAs) or \"random\", 2 to 9 players\n"
            "                             e.g. rivermath equity AhAs KdKc --board 2h7c9s\n"
            "  rivermath bench            measure 7-card hands/sec for each evaluator\n"
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

static double seconds_now(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
}

/* ---- equity ------------------------------------------------------------------ */

enum { EQUITY_BAR_WIDTH = 30 };

/* Parse a whole-number option value. Returns false (with a message) if it isn't one. */
static bool parse_count(const char *option, const char *text, uint64_t *out) {
    char *end;
    *out = strtoull(text, &end, 10);
    if (*text == '\0' || *end != '\0' || *text == '-') {
        fprintf(stderr, "error: %s needs a whole number, got \"%s\"\n", option, text);
        return false;
    }
    return true;
}

/* Fill input from the command line. Returns false (with a message) on bad input. */
static bool parse_equity_args(int num_args, char **args, rm_equity_input *input) {
    for (int i = 0; i < num_args; i++) {
        const char *arg = args[i];
        bool is_option = strncmp(arg, "--", 2) == 0;
        if (is_option && i + 1 >= num_args) {
            fprintf(stderr, "error: %s needs a value\n", arg);
            return false;
        }
        if (strcmp(arg, "--board") == 0) {
            if (parse_card_args(1, &args[++i], &input->board) < 0) return false;
        } else if (strcmp(arg, "--dead") == 0) {
            if (parse_card_args(1, &args[++i], &input->dead_cards) < 0) return false;
        } else if (strcmp(arg, "--samples") == 0) {
            if (!parse_count(arg, args[++i], &input->num_samples)) return false;
        } else if (strcmp(arg, "--seed") == 0) {
            if (!parse_count(arg, args[++i], &input->seed)) return false;
        } else if (is_option) {
            fprintf(stderr, "error: unknown option %s\n", arg);
            return false;
        } else if (input->num_players == RM_MAX_PLAYERS) {
            fprintf(stderr, "error: at most %d players\n", RM_MAX_PLAYERS);
            return false;
        } else if (strcmp(arg, "random") == 0) {
            input->hole_cards[input->num_players++] = 0;
        } else {
            if (parse_card_args(1, &args[i], &input->hole_cards[input->num_players]) < 0)
                return false;
            input->num_players++;
        }
    }
    return true;
}

static void print_equity_bar(double equity) {
    int width = (int)(equity * EQUITY_BAR_WIDTH + 0.5);
    for (int i = 0; i < width; i++) printf("█");
}

static int command_equity(int num_args, char **args) {
    rm_equity_input input = {0};
    if (!parse_equity_args(num_args, args, &input)) return EXIT_BAD_INPUT;

    rm_lookup_table_bytes(); /* build the lookup tables once, outside the timing */
    rm_equity_result result;
    double start = seconds_now();
    int error = rm_equity(&input, &result);
    double elapsed = seconds_now() - start;
    if (error) {
        fprintf(stderr, "error: %s\n", rm_equity_error_message(error));
        return EXIT_BAD_INPUT;
    }

    char board_text[16];
    rm_format_cards(input.board, board_text, sizeof board_text);
    printf("Board: %s   ", input.board ? board_text : "(none)");
    if (result.was_exact)
        printf("Mode: exact (%llu boards)", (unsigned long long)result.num_deals);
    else
        printf("Mode: Monte Carlo (%llu samples, seed %llu)", (unsigned long long)result.num_deals,
               (unsigned long long)input.seed);
    printf("   %.0f ms\n\n", elapsed * 1e3);

    printf("Player  Hand      Win      Tie   Equity\n");
    for (int player = 0; player < input.num_players; player++) {
        char hand_text[8];
        rm_format_cards(input.hole_cards[player], hand_text, sizeof hand_text);
        printf("  %d     %-6s %6.2f%%  %5.2f%%  %6.2f%%  ", player + 1,
               input.hole_cards[player] ? hand_text : "random", result.win[player] * 100,
               result.tie[player] * 100, result.equity[player] * 100);
        print_equity_bar(result.equity[player]);
        printf("\n");
    }
    return 0;
}

/* ---- bench ------------------------------------------------------------------ */

enum { BENCH_NUM_HANDS = 1 << 20, BENCH_NUM_ROUNDS = 5, BENCH_SEED = 12345 };

static int compare_doubles(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/*
 * Median time (seconds) to evaluate every hand once. The sum of all hand
 * values is written to *checksum so the compiler can't skip the work.
 */
static double time_evaluator(rm_hand_value (*evaluate)(rm_cards), const rm_cards *hands,
                             uint64_t *checksum) {
    double round_seconds[BENCH_NUM_ROUNDS];
    for (int round = -1; round < BENCH_NUM_ROUNDS; round++) { /* round -1 is a warm-up */
        uint64_t sum = 0;
        double start = seconds_now();
        for (int i = 0; i < BENCH_NUM_HANDS; i++) sum += evaluate(hands[i]);
        double elapsed = seconds_now() - start;
        if (round >= 0) round_seconds[round] = elapsed;
        *checksum = sum;
    }
    qsort(round_seconds, BENCH_NUM_ROUNDS, sizeof round_seconds[0], compare_doubles);
    return round_seconds[BENCH_NUM_ROUNDS / 2];
}

static int command_bench(void) {
    rm_cards *hands = malloc(BENCH_NUM_HANDS * sizeof *hands);
    if (!hands) {
        fprintf(stderr, "error: out of memory\n");
        return EXIT_BAD_INPUT;
    }
    rm_random random;
    rm_random_seed(&random, BENCH_SEED);
    for (int i = 0; i < BENCH_NUM_HANDS; i++)
        hands[i] = rm_deal_random_cards(&random, RM_FULL_DECK, 7);

    double build_start = seconds_now();
    size_t table_bytes = rm_lookup_table_bytes(); /* builds the tables outside the timing */
    double build_seconds = seconds_now() - build_start;

    struct {
        const char *name;
        rm_hand_value (*evaluate)(rm_cards);
    } evaluators[] = {
        {"naive", rm_evaluate_naive},
        {"bitwise", rm_evaluate_bitwise},
        {"lookup", rm_evaluate_lookup},
    };
    enum { NUM_EVALUATORS = sizeof evaluators / sizeof evaluators[0] };

    printf("%d random 7-card hands, median of %d runs, single thread\n\n", BENCH_NUM_HANDS,
           BENCH_NUM_ROUNDS);
    printf("%-10s %16s %12s %10s\n", "evaluator", "hands/sec", "ns/hand", "vs naive");

    double naive_seconds = 0;
    uint64_t first_checksum = 0;
    bool checksums_match = true;
    for (int i = 0; i < NUM_EVALUATORS; i++) {
        uint64_t checksum;
        double seconds = time_evaluator(evaluators[i].evaluate, hands, &checksum);
        if (i == 0) {
            naive_seconds = seconds;
            first_checksum = checksum;
        }
        if (checksum != first_checksum) checksums_match = false;
        printf("%-10s %14.1f M %12.1f %9.1fx\n", evaluators[i].name,
               BENCH_NUM_HANDS / seconds / 1e6, seconds / BENCH_NUM_HANDS * 1e9,
               naive_seconds / seconds);
    }
    printf("\nlookup tables: %.0f KB, built in %.0f ms\n", table_bytes / 1024.0,
           build_seconds * 1e3);
    printf("all evaluators agree on these hands: %s\n", checksums_match ? "yes" : "NO");
    free(hands);
    return checksums_match ? 0 : EXIT_BAD_INPUT;
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
    if (strcmp(command, "equity") == 0) return command_equity(num_args, args);
    if (strcmp(command, "bench") == 0) return command_bench();

    fprintf(stderr, "error: unknown command \"%s\"\n\n", command);
    usage(stderr);
    return EXIT_BAD_USAGE;
}
