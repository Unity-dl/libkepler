#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "numtest.h"

struct numtest_args {
    uint64_t first, last;
    int random, random_seed;

    char * const *tests;
    int num_tests;
};

struct numtest_ctx {
    const char *test_case_name;
    uint64_t seed;

    int asserts_passed;
    int asserts_failed;

    uint64_t cases_passed;
    uint64_t cases_failed;

    int tests_run;
};

void numtest_assert_failed(
    struct numtest_ctx *ctx,
    const char *file, int line, const char *function,
    const char *msg,
    va_list va)
{
    FILE *out = stdout;
    fprintf(out, "%s(%lu): ASSERT FAILED (%s:%d %s)  \n\t",
            ctx->test_case_name,
            ctx->seed,
            file,
            line,
            function);
    vfprintf(out, msg, va);
    fprintf(out, "\n");
}

void numtest_assert(
    int cond,
    struct numtest_ctx *ctx,
    const char *file, int line, const char *function,
    const char *msg, ...) {

    if(cond) {
        ctx->asserts_passed += 1;
        return;
    }

    ctx->asserts_failed += 1;

    va_list va;
    va_start(va, msg);
    numtest_assert_failed(ctx, file, line, function, msg, va);
    va_end(va);
}

static double test_pattern_1d(uint64_t seed) {
    if(seed < 2)
        return (double)seed;

    seed -= 1;

    int level = 64 - __builtin_clzl(seed);
    uint64_t numer = 1 + (seed - (1ull << (level - 1))) * 2;
    uint64_t denom = 1ull << level;

    return numer / (double)denom;
}

static void test_pattern(uint64_t seed, int dim, double *params) {
    if(dim == 1) {
        params[0] = test_pattern_1d(seed);
        return;
    }

    uint64_t seeds[dim];
    for(int i = 0; i < dim; ++i)
        seeds[i] = 0;

    // interpret seed as Morton code
    for(int i = 0; i < 64 && (1ull << i) <= seed; ++i)
        if(seed & (1ull << i))
            seeds[i % dim] |= 1ull << (i / dim);

    for(int i = 0; i < dim; ++i)
        params[i] = test_pattern_1d(seeds[i]);
}

static bool numtest_run_tests(const struct numtest_args *args) {
    struct numtest_ctx ctx = { 0, 0, 0, 0, 0, 0, 0 };

    for(const struct numtest_case *test_case = numtest_cases + 0;
        test_case->name != 0;
        ++test_case) {
        bool skip = args->num_tests != 0;
        for(int i = 0; i < args->num_tests; ++i)
            if(strcmp(test_case->name, args->tests[i]) == 0)
                skip = false;
        if(skip)
            continue;

        ctx.test_case_name = test_case->name;

        uint64_t num = 1 << 30; // TODO: reasonable default?

        uint64_t first = args->first;
        uint64_t last = args->last != 0 ? args->last : num;

        // TODO: split range for parallel execution!

        for(uint64_t xxx = first; xxx < last; ++xxx) {
            uint64_t seed = xxx; // TODO: random and random seed!

            double params[test_case->num_params];
            test_pattern(seed, test_case->num_params, params);

            // TODO: optionally add noise

            ctx.seed = seed;
            ctx.asserts_passed = ctx.asserts_failed = 0;
            test_case->func(params, test_case->num_params, test_case->extra_args, &ctx);

            if(ctx.asserts_failed == 0)
                ctx.cases_passed += 1;
            else
                ctx.cases_failed += 1;
        }

        ctx.tests_run += 1;
    }

    printf("TESTS %s  %lu%%  (%d tests, %lu cases pass, %lu cases fail)\n",
        ctx.cases_failed == 0 ? "PASS" : "FAIL" ,
        100 * ctx.cases_passed / (ctx.cases_failed + ctx.cases_passed),
        ctx.tests_run, ctx.cases_passed, ctx.cases_failed
        );

    return ctx.cases_failed == 0;
}

#include <getopt.h>
#include <stdlib.h>
#include <time.h>

static void usage() {
    printf("*** USAGE ***\n");
    exit(EXIT_FAILURE);
}

static struct numtest_args parse_args(int argc, char * const argv[]) {
    const struct option long_options[] = {
        {"first", required_argument, 0, 0 },
        {"last", required_argument, 0, 0 },
        {"random", optional_argument, 0, 0 },
        { 0, 0, 0, 0}
    };

    const char *short_options = "f:l:r::";

    struct numtest_args args = { 0, 0, 0, 0, 0, 0 };

    while(1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);

        if(c == -1)
            break;

        if((c == 0 && strcmp(long_options[option_index].name, "first") == 0) ||
            c == 'f') {
            if(!optarg || sscanf(optarg, "%lu", &args.first) != 1)
                usage();
        } else if((c == 0 && strcmp(long_options[option_index].name, "last") == 0) ||
            c == 'l') {
            if(!optarg || sscanf(optarg, "%lu", &args.last) != 1)
                usage();
        } else if((c == 0 && strcmp(long_options[option_index].name, "random") == 0) ||
            c == 'r') {
            args.random = 1;
            if(optarg && sscanf(optarg, "%u", &args.random_seed) != 1)
                usage();
        } else {
            usage();
        }
    }

    args.tests = argv + optind;
    args.num_tests = argc - optind;

    if(args.random && args.random_seed == 0)
        args.random_seed = time(NULL);

    return args;
}

int main(int argc, char *argv[]) {
    struct numtest_args args = parse_args(argc, argv);

    bool result = numtest_run_tests(&args);

    return result ? 0 : -1;
}

#if 0
void dummy_test(double *params, int num_params, void *extra_args, struct numtest_ctx* test_ctx) {
    (void)extra_args;

    for(int i = 0; i < num_params; ++i)
        ASSERT_RANGEF(params[i], 0.0, 1.0, "Parameter %d not within range: %lf\n", i, params[i]);

    /*
    for(int i = 0; i < num_params; ++i)
        printf("%2.4f\t", params[i]);
    printf("\n");
    */
}

const struct numtest_case numtest_cases[] = {
    { "dummy_test", dummy_test, 3, 0 },
    { "dummy_test2", dummy_test, 2, 0 },
    { 0, 0, 0, 0 }
};
#endif
