/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master Thesis
 * 2014/2015
 *
 * Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>
 *
 * Faculty of Information Technologies
 * Brno University of Technology
 * http://www.fit.vutbr.cz/
 *
 * Started on 28/07/2014.
 *      _       _
 *   __(.)=   =(.)__
 *   \___)     (___/
 */


#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>

#ifdef _OPENMP
    #include <omp.h>
#endif

#include "config.h"
#include "cpu.h"
#include "cgp_config.h"
#include "logging.h"  /* for FITNESS_FMT */


#define OPT_MAX_GENERATIONS 'g'
#define OPT_TARGET_PSNR 't'
#define OPT_TARGET_FITNESS 'f'

#define OPT_ALGORITHM 'a'
#define OPT_RANDOM_SEED 'r'

#define OPT_ORIGINAL 'i'
#define OPT_NOISY 'n'

#define OPT_VAULT_ENABLE 'v'
#define OPT_VAULT_INTERVAL 'w'

#define OPT_LOG_DIR 'l'
#define OPT_LOG_INTERVAL 'k'

#define OPT_CGP_MUTATE 'm'
#define OPT_CGP_POPSIZE 'p'
#define OPT_CGP_ARCSIZE 's'

#define OPT_PRED_SIZE 'S'
#define OPT_PRED_MUTATE 'M'
#define OPT_PRED_POPSIZE 'P'
#define OPT_PRED_TYPE 'T'

#define OPT_HELP 'h'

#define OPT_BW_INTERVAL 'b'
#define OPT_BW_INACCURACY_TOLERANCE 1000
#define OPT_BW_INACCURACY_COEF      1001
#define OPT_BW_ZERO_EPSILON         1002
#define OPT_BW_SLOW_THRESHOLD       1003
#define OPT_BW_ZERO_COEF            1004
#define OPT_BW_DECREASE_COEF        1005
#define OPT_BW_INCREASE_SLOW_COEF   1006
#define OPT_BW_INCREASE_FAST_COEF   1007

#define OPT_BW_PRED_INITIAL_SIZE 'I'
#define OPT_BW_PRED_MIN_SIZE 'N'


static struct option long_options[] =
{
    {"help", no_argument, 0, OPT_HELP},

    {"max-generations", required_argument, 0, OPT_MAX_GENERATIONS},
    {"target-psnr", required_argument, 0, OPT_TARGET_PSNR},
    {"target-fitness", required_argument, 0, OPT_TARGET_FITNESS},

    /* Algorithm mode */
    {"algorithm", required_argument, 0, OPT_ALGORITHM},

    /* PRNG seed */
    {"random-seed", required_argument, 0, OPT_RANDOM_SEED},

    /* Input images */
    {"original", required_argument, 0, OPT_ORIGINAL},
    {"noisy", required_argument, 0, OPT_NOISY},

    /* Vault */
    {"vault", no_argument, 0, OPT_VAULT_ENABLE},
    {"vault-interval", required_argument, 0, OPT_VAULT_INTERVAL},

    /* Logging */
    {"log-dir", required_argument, 0, OPT_LOG_DIR},
    {"log-interval", required_argument, 0, OPT_LOG_INTERVAL},

    /* CGP */
    {"cgp-mutate", required_argument, 0, OPT_CGP_MUTATE},
    {"cgp-population-size", required_argument, 0, OPT_CGP_POPSIZE},
    {"cgp-archive-size", required_argument, 0, OPT_CGP_ARCSIZE},

    /* Predictors */
    {"pred-size", required_argument, 0, OPT_PRED_SIZE},
    {"pred-mutate", required_argument, 0, OPT_PRED_MUTATE},
    {"pred-population-size", required_argument, 0, OPT_PRED_POPSIZE},
    {"pred-type", required_argument, 0, OPT_PRED_TYPE},

    /* Baldwin */
    {"bw-interval", required_argument, 0, OPT_BW_INTERVAL},
    {"bw-inac-tol", required_argument, 0, OPT_BW_INACCURACY_TOLERANCE},
    {"bw-inac-coef", required_argument, 0, OPT_BW_INACCURACY_COEF},
    {"bw-zero-eps", required_argument, 0, OPT_BW_ZERO_EPSILON},
    {"bw-slow-thr", required_argument, 0, OPT_BW_SLOW_THRESHOLD},
    {"bw-zero-coef", required_argument, 0, OPT_BW_ZERO_COEF},
    {"bw-decr-coef", required_argument, 0, OPT_BW_DECREASE_COEF},
    {"bw-slow-coef", required_argument, 0, OPT_BW_INCREASE_SLOW_COEF},
    {"bw-fast-coef", required_argument, 0, OPT_BW_INCREASE_FAST_COEF},

    {"bw-pred-initial-size", required_argument, 0, OPT_BW_PRED_INITIAL_SIZE},
    {"bw-pred-min-size", required_argument, 0, OPT_BW_PRED_MIN_SIZE},

    {0, 0, 0, 0}
};

static const char *short_options = "hg:t:f:a:r:i:n:vw:l:k:m:p:s:S:M:P:T:b:I:N:";


#define CHECK_FILENAME_LENGTH do { \
    if (strlen(optarg) > MAX_FILENAME_LENGTH - 1) { \
        fprintf(stderr, "Option %s is too long (limit: %d chars)\n", \
            long_options[option_index].name, \
            MAX_FILENAME_LENGTH - 1); \
        return 1; \
    } \
} while(0);

#define PARSE_INT(dst) do { \
    char *endptr; \
    (dst) = strtol(optarg, &endptr, 10); \
    if (*endptr != '\0' && endptr != optarg && errno != ERANGE) { \
        fprintf(stderr, "Option %s requires valid integer\n", \
            long_options[option_index].name); \
        return 1; \
    } \
} while(0);

#define PARSE_UNSIGNED_INT(dst) do { \
    char *endptr; \
    (dst) = strtoul(optarg, &endptr, 10); \
    if (*endptr != '\0' && endptr != optarg && errno != ERANGE) { \
        fprintf(stderr, "Option %s requires valid unsigned integer\n", \
            long_options[option_index].name); \
        return 1; \
    } \
} while(0);

#define PARSE_DOUBLE(dst) do { \
    char *endptr; \
    (dst) = strtod(optarg, &endptr); \
    if (*endptr != '\0' && endptr != optarg && errno != ERANGE) { \
        fprintf(stderr, "Option %s requires valid float\n", \
            long_options[option_index].name); \
        return 1; \
    } \
} while(0);


/**
 * Load configuration from command line
 */
config_retval_t config_load_args(int argc, char **argv, config_t *cfg)
{
    assert(cfg != NULL);

    bool algorithm_specified = false;
    bool pred_type_specified = false;

    while (1) {
        int option_index;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == - 1) break;

        double target_psnr;

        switch (c) {
            case OPT_HELP:
                print_help();
                return cfg_help;

            case OPT_MAX_GENERATIONS:
                PARSE_INT(cfg->max_generations);
                break;

            case OPT_TARGET_PSNR:
                PARSE_DOUBLE(target_psnr);
                cfg->target_fitness = pow(10, (target_psnr / 10));
                break;

            case OPT_TARGET_FITNESS:
                PARSE_DOUBLE(cfg->target_fitness);
                break;

            case OPT_ALGORITHM:
                if (strcmp(optarg, "cgp") == 0) {
                    cfg->algorithm = simple_cgp;
                } else if (strcmp(optarg, "predictors") == 0) {
                    cfg->algorithm = predictors;
                } else if (strcmp(optarg, "baldwin") == 0) {
                    cfg->algorithm = baldwin;
                    if (pred_type_specified && cfg->pred_genome_type != repeated) {
                        fprintf(stderr, "Cannot combine baldwin and permuted genotype.\n");
                        return cfg_err;
                    }
                } else {
                    fprintf(stderr, "Invalid algorithm (options: cgp, predictors, baldwin)\n");
                    return cfg_err;
                }
                algorithm_specified = true;
                break;

            case OPT_RANDOM_SEED:
                PARSE_UNSIGNED_INT(cfg->random_seed);
                break;

            case OPT_ORIGINAL:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->input_image, optarg, MAX_FILENAME_LENGTH);
                break;

            case OPT_NOISY:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->noisy_image, optarg, MAX_FILENAME_LENGTH);
                break;

            case OPT_VAULT_ENABLE:
                cfg->vault_enabled = true;
                break;

            case OPT_VAULT_INTERVAL:
                PARSE_INT(cfg->vault_interval);
                break;

            case OPT_LOG_INTERVAL:
                PARSE_INT(cfg->log_interval);
                break;

            case OPT_LOG_DIR:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->log_dir, optarg, MAX_FILENAME_LENGTH);
                break;

            case OPT_CGP_MUTATE:
                PARSE_INT(cfg->cgp_mutate_genes);
                break;

            case OPT_CGP_POPSIZE:
                PARSE_INT(cfg->cgp_population_size);
                break;

            case OPT_CGP_ARCSIZE:
                PARSE_INT(cfg->cgp_archive_size);
                break;

            case OPT_PRED_SIZE:
                PARSE_DOUBLE(cfg->pred_size);
                if (cfg->pred_size > 1) {
                    cfg->pred_size /= 100.0;
                }
                break;

            case OPT_BW_PRED_INITIAL_SIZE:
                PARSE_DOUBLE(cfg->pred_initial_size);
                if (cfg->pred_initial_size > 1) {
                    cfg->pred_initial_size /= 100.0;
                }
                break;

            case OPT_BW_PRED_MIN_SIZE:
                PARSE_DOUBLE(cfg->pred_min_size);
                if (cfg->pred_min_size > 1) {
                    cfg->pred_min_size /= 100.0;
                }
                break;

            case OPT_PRED_MUTATE:
                PARSE_DOUBLE(cfg->pred_mutation_rate);
                if (cfg->pred_mutation_rate > 1) {
                    cfg->pred_mutation_rate /= 100.0;
                }
                break;

            case OPT_PRED_POPSIZE:
                PARSE_INT(cfg->pred_population_size);
                break;

            case OPT_PRED_TYPE:
                if (strcmp(optarg, "permuted") == 0) {
                    cfg->pred_genome_type = permuted;
                    if (algorithm_specified && cfg->algorithm == baldwin) {
                        fprintf(stderr, "Cannot combine baldwin and permuted genotype.\n");
                        return cfg_err;
                    }
                } else if (strcmp(optarg, "repeated") == 0) {
                    cfg->pred_genome_type = repeated;
                } else {
                    fprintf(stderr, "Invalid predictor type (options: permuted, repeated)\n");
                    return cfg_err;
                }
                pred_type_specified = true;
                break;

            case OPT_BW_INTERVAL:
                PARSE_INT(cfg->bw_interval);
                break;

            case OPT_BW_INACCURACY_TOLERANCE:
                PARSE_DOUBLE(cfg->bw_config.inaccuracy_tolerance);
                break;

            case OPT_BW_INACCURACY_COEF:
                PARSE_DOUBLE(cfg->bw_config.inaccuracy_coef);
                break;

            case OPT_BW_ZERO_EPSILON:
                PARSE_DOUBLE(cfg->bw_config.zero_epsilon);
                break;

            case OPT_BW_SLOW_THRESHOLD:
                PARSE_DOUBLE(cfg->bw_config.slow_threshold);
                break;

            case OPT_BW_ZERO_COEF:
                PARSE_DOUBLE(cfg->bw_config.zero_coef);
                break;

            case OPT_BW_DECREASE_COEF:
                PARSE_DOUBLE(cfg->bw_config.decrease_coef);
                break;

            case OPT_BW_INCREASE_SLOW_COEF:
                PARSE_DOUBLE(cfg->bw_config.increase_slow_coef);
                break;

            case OPT_BW_INCREASE_FAST_COEF:
                PARSE_DOUBLE(cfg->bw_config.increase_fast_coef);
                break;

            default:
                return cfg_err;

        }
    }

    return cfg_ok;
}


/**
 * Load configuration from XML file
 */
int config_load_file(FILE *file, config_t *cfg);


/**
 * Load configuration from XML file
 */
void config_save_file(FILE *file, config_t *cfg)
{
    time_t now = time(NULL);
    char timestr[200];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S %z", localtime(&now));

    fprintf(file, "# Configuration dump (%s)\n", timestr);
    fprintf(file, "\n");
    fprintf(file, "original: %s\n", cfg->input_image);
    fprintf(file, "noisy: %s\n", cfg->noisy_image);
    fprintf(file, "algorithm: %s\n", config_algorithm_names[cfg->algorithm]);
    fprintf(file, "random-seed: %u\n", cfg->random_seed);
    fprintf(file, "max-generations: %d\n", cfg->max_generations);
    fprintf(file, "target_fitness: " FITNESS_FMT "\n", cfg->target_fitness);
    fprintf(file, "\n");
    fprintf(file, "vault: %s\n", cfg->vault_enabled? "yes" : "no");
    fprintf(file, "vault-interval: %d\n", cfg->vault_interval);
    fprintf(file, "\n");
    fprintf(file, "log-dir: %s\n", cfg->log_dir);
    fprintf(file, "log-interval: %d\n", cfg->log_interval);
    fprintf(file, "\n");
    fprintf(file, "cgp-mutate: %d\n", cfg->cgp_mutate_genes);
    fprintf(file, "cgp-population-size: %d\n", cfg->cgp_population_size);
    fprintf(file, "cgp-archive-size: %d\n", cfg->cgp_archive_size);
    fprintf(file, "\n");
    fprintf(file, "pred-size: %.5g\n", cfg->pred_size);
    fprintf(file, "pred-mutate: %.5g\n", cfg->pred_mutation_rate);
    fprintf(file, "pred-population-size: %d\n", cfg->pred_population_size);
    fprintf(file, "pred-type: %s\n", cfg->pred_genome_type == permuted? "permuted" : "repeated");
    fprintf(file, "\n");
    fprintf(file, "bw-interval: %d\n", cfg->bw_interval);
    fprintf(file, "bw-inac-tol: %.5g\n", cfg->bw_config.inaccuracy_tolerance);
    fprintf(file, "bw-inac-coef: %.5g\n", cfg->bw_config.inaccuracy_coef);
    fprintf(file, "bw-zero-eps: %.5g\n", cfg->bw_config.zero_epsilon);
    fprintf(file, "bw-slow-thr: %.5g\n", cfg->bw_config.slow_threshold);
    fprintf(file, "bw-zero-coef: %.5g\n", cfg->bw_config.zero_coef);
    fprintf(file, "bw-decr-coef: %.5g\n", cfg->bw_config.decrease_coef);
    fprintf(file, "bw-slow-coef: %.5g\n", cfg->bw_config.increase_slow_coef);
    fprintf(file, "bw-fast-coef: %.5g\n", cfg->bw_config.increase_fast_coef);
    fprintf(file, "bw-pred-initial-size: %.5g\n", cfg->pred_initial_size);
    fprintf(file, "bw-pred-min-size: %.5g\n", cfg->pred_min_size);
    fprintf(file, "# baldwin-len=%d,zeroeps=%g,slowthr=%g,steady=%g,dec=%g,slow=%g,fast=%g,inthr=%g,incoef=%g\n",
        BW_HISTORY_LENGTH,
        cfg->bw_config.zero_epsilon,
        cfg->bw_config.slow_threshold,
        cfg->bw_config.zero_coef,
        cfg->bw_config.decrease_coef,
        cfg->bw_config.increase_slow_coef,
        cfg->bw_config.increase_fast_coef,
        cfg->bw_config.inaccuracy_tolerance,
        cfg->bw_config.inaccuracy_coef
    );
    fprintf(file, "\n");
    fprintf(file, "# Compiler flags\n");
    fprintf(file, "# CGP_COLS: %d\n", CGP_COLS);
    fprintf(file, "# CGP_ROWS: %d\n", CGP_ROWS);
    fprintf(file, "# CGP_INPUTS: %d\n", CGP_INPUTS);
    fprintf(file, "# CGP_OUTPUTS: %d\n", CGP_OUTPUTS);
    fprintf(file, "# CGP_LBACK: %d\n", CGP_LBACK);
    #ifdef CGP_LIMIT_FUNCS
        fprintf(file, "# CGP_LIMIT_FUNCS: yes\n");
    #else
        fprintf(file, "# CGP_LIMIT_FUNCS: no\n");
    #endif
    fprintf(file, "#\n");
    fprintf(file, "# System\n");
    #ifdef _OPENMP
        fprintf(file, "# OpenMP: yes\n");
        fprintf(file, "# OpenMP CPUs: %d\n", omp_get_num_procs());
        fprintf(file, "# OpenMP max threads: %d\n", omp_get_max_threads());
    #else
        fprintf(file, "# OpenMP: no\n");
    #endif
    #ifdef AVX2
        fprintf(file, "# AVX2: yes\n");
    #else
        fprintf(file, "# AVX2: no\n");
    #endif
    fprintf(file, "# AVX2 supported by CPU/OS: %s\n", can_use_intel_core_4th_gen_features()? "yes" : "no");
    #ifdef SSE2
        fprintf(file, "# SSE2: yes\n");
    #else
        fprintf(file, "# SSE2: no\n");
    #endif
    fprintf(file, "# SSE2 supported by CPU/OS: %s\n", can_use_sse2()? "yes" : "no");
}
