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


#pragma once

#include <stdio.h>
#include <sys/time.h>

#include "cgp.h"
#include "image.h"
#include "files.h"
#include "config.h"
#include "archive.h"
#include "baldwin.h"
#include "fitness.h"
#include "predictors.h"


#define FITNESS_FMT "%.10g"

#define SECTION_CGP "cgp"
#define SECTION_PRED "prd"
#define SECTION_SYS "sys"
#define SECTION_BALDWIN "bwn"

/* predictors log to second column */
#define PRED_INDENT "                                                        "


/* standard console outputing */

#define LOG_F(fp, ...) { LOG_THREAD_IDENT((fp)); fprintf((fp), __VA_ARGS__); fprintf((fp), "\n"); }
#define SLOWLOG_F(fp, ...) { LOG_THREAD_IDENT((fp)); fprintf((fp), __VA_ARGS__); fprintf((fp), "\n"); }

#define LOG(...) LOG_F(stdout, __VA_ARGS__)
#define SLOWLOG(...) SLOWLOG_F(stdout, __VA_ARGS__)

#ifdef _OPENMP
    #include <omp.h>
    #define LOG_THREAD_IDENT(stream) { fprintf((stream), "thread %02d: ", omp_get_thread_num()); }
#else
    #define LOG_THREAD_IDENT(stream)
#endif


/**
 * Load current time as start time
 */
void log_init_time();


/**
 * Creates log directories
 * @param  dir
 */
int log_create_dirs(const char *dir);


/**
 * Logs current CGP progress
 * @param  cgp_population
 */
void log_cgp_progress(FILE *fp, ga_pop_t cgp_population, long cgp_evals);


/**
 * Logs that CGP has finished
 * @param  cgp_population
 */
void log_cgp_finished(FILE *fp, ga_pop_t cgp_population);


/**
 * Logs current predictors progress
 * @param  pred_population
 * @param  pred_archive
 */
void log_pred_progress(FILE *fp, ga_pop_t pred_population,
    archive_t pred_archive, bool indent);


/**
 * Logs that CGP best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_cgp_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best);


/**
 * Logs CGP history entry
 * @param history
 */
void log_bw_history_entry(FILE *fp, bw_history_entry_t *history);


/**
 * Logs that CGP was moved to archive
 * @param predicted
 * @param real
 */
void log_cgp_archived(FILE *fp, ga_fitness_t predicted, ga_fitness_t real);


/**
 * Logs that predictors best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_pred_change(FILE *fp, ga_fitness_t previous_best,
    ga_fitness_t new_best, pred_genome_t new_genome, bool indent);


/**
 * Logs best circuit to file.
 */
void log_cgp_circuit(FILE *fp, int generation, ga_chr_t circuit);


/**
 * Logs final summary
 */
void log_final_summary(FILE *fp, int generation, ga_fitness_t best_fitness,
    long cgp_evals);


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(const char *dir, img_image_t original);


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(const char *dir, img_image_t noisy);


/**
 * Saves image filtered by best filter to results directory
 * @param dir results directory
 * @param cgp_population
 * @param noisy
 */
void save_filtered_image(const char *dir, ga_pop_t cgp_population, img_image_t noisy);


/**
 * Saves best found image to results directory
 * @param best_circuit
 * @param noisy
 */
void save_best_image(const char *dir, ga_chr_t best_circuit, img_image_t noisy);


/**
 * Saves configuration to results directory
 */
void save_config(const char *dir, config_t *config);


/**
 * Open specified file for writing. Caller is responsible for closing.
 * @param  dir
 * @param  file
 * @param  log_start whether to insert initial log message
 * @return
 */
FILE *open_log_file(const char *dir, const char *file, bool log_start);


/**
 * Initializes CGP history CSV file and writes header to it
 * @param  dir
 * @param  file
 * @return fp
 */
FILE *init_cgp_history_file(const char *dir, const char *file);


/**
 * Log CGP history entry in CSV format
 * @param fp
 * @param hist
 * @param cgp_evals
 * @param pred_length
 * @param pred_used_length
 * @param best_ever
 */
void log_cgp_history(FILE *fp, bw_history_entry_t *hist, long cgp_evals,
    int pred_length, int pred_used_length, ga_fitness_t best_ever);


/**
 * Log that predictors' length has changed
 * @param fp
 * @param old_length
 * @param new_length
 */
void log_predictors_length_change(FILE *fp, int old_length, int new_length);


/**
 * Logs spent time
 * @param fp
 * @param usertime_start
 * @param usertime_end
 * @param wallclock_start
 * @param wallclock_end
 */
void log_time(FILE *fp);


/**
 * Logs magic baldwin string
 * @param fp
 */
void log_baldwin_magic_string(FILE *fp);
