/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master's Thesis
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


#include <stdlib.h>
#include <stdarg.h>

#include "cgp.h"


/**
 * Loads chromosome from given file stored in CGP-viewer compatible format
 * @param chr
 * @param fp
 * @return 0 on success, -1 on file format error, -2 on incompatible CGP config
 */
int cgp_load_chr_compat(ga_chr_t chr, FILE *fp)
{
    int inputs, outputs, cols, rows, func_inputs, func_outputs, func_count;
    int count;
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    count = fscanf(fp, "{%u, %u, %u, %u, %u, %u, %u}", &inputs, &outputs,
        &cols, &rows, &func_inputs, &func_outputs, &func_count);


    if (count != 7) return -1;
    if (func_inputs != CGP_FUNC_INPUTS) return -2;
    if (func_outputs != 1) return -2;
    if (func_count != CGP_FUNC_COUNT) return -2;

    genome->cols = cols;
    genome->rows = rows;
    genome->inputs_count = inputs;
    genome->outputs_count = outputs;

    // nodes


    for (int i = 0; i < cgp_nodes_count(genome); i++) {
        cgp_node_t *n = &genome->nodes[i];

        int nodeid;
        count = fscanf(fp, "([%u] %u, %u, %u)",
            &nodeid, &n->inputs[0], &n->inputs[1], &n->function);
        n->is_constant = false;
        if (count != 4) return -1;
        if (nodeid != CGP_INPUTS + i) return -1;
    }

    // primary outputs


    count = fscanf(fp, "(");
    for (int i = 0; i < genome->outputs_count; i++) {
        if (i > 0) {
            count = fscanf(fp, ",");
        }
        count = fscanf(fp, "%u", &genome->outputs[i]);
        if (count != 1) return -1;
    }
    count = fscanf(fp, ")\n");

    cgp_find_active_blocks(chr);

    return 0;
}
