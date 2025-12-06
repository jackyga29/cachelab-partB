/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 * 
 * Jaqueline Gallegos Alcala (jgallegosalcala29@unm.edu)
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for the cache lab.
 *
 * It must have description exactly "Transpose submission" so that
 * test-trans can identify it as the official version.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    /* At most 12 local int variables in this function */
    int i, j, k, l;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    /* 1) 32 x 32: simple 8x8 blocking with diagonal handling */
    if (M == 32 && N == 32) {
        for (i = 0; i < 32; i += 8) {
            for (j = 0; j < 32; j += 8) {

                if (i != j) {
                    /* Off-diagonal blocks: straightforward 8x8 copy */
                    for (k = i; k < i + 8; ++k) {
                        for (l = j; l < j + 8; ++l) {
                            B[l][k] = A[k][l];
                        }
                    }
                } else {
                    /* Diagonal blocks: delay writing diagonal elements
                     * to reduce conflict misses.
                     */
                    for (k = i; k < i + 8; ++k) {
                        for (l = j; l < j + 8; ++l) {
                            if (k != l) {
                                B[l][k] = A[k][l];
                            } else {
                                a0 = A[k][l];  /* value on the diagonal */
                                a1 = k;        /* index on the diagonal */
                            }
                        }
                        B[a1][a1] = a0;
                    }
                }
            }
        }
    }

    /* 2) 64 x 64: 8x8 blocking with 4x4 sub-block reorganization */
    else if (M == 64 && N == 64) {
        for (i = 0; i < 64; i += 8) {
            for (j = 0; j < 64; j += 8) {

                if (i == j) {
                    /* Diagonal 8x8 block: similar to the 32x32 case. */
                    for (k = i; k < i + 8; ++k) {
                        for (l = j; l < j + 8; ++l) {
                            if (k != l) {
                                B[l][k] = A[k][l];
                            } else {
                                a0 = A[k][l];
                                a1 = k;
                            }
                        }
                        B[a1][a1] = a0;
                    }
                } else {
                    /* Off-diagonal 8x8 block split into 4x4 sub-blocks.
                     *
                     * Layout of the block:
                     *   A block: rows i..i+7, cols j..j+7
                     *   We treat it as:
                     *     [ A0 | A1 ]
                     *     [ A2 | A3 ]
                     *   where each A* is 4x4
                     */

                    /* Step 1: top 4 rows of A (A0 and A1) */
                    for (k = 0; k < 4; ++k) {
                        a0 = A[i + k][j + 0];
                        a1 = A[i + k][j + 1];
                        a2 = A[i + k][j + 2];
                        a3 = A[i + k][j + 3];
                        a4 = A[i + k][j + 4];
                        a5 = A[i + k][j + 5];
                        a6 = A[i + k][j + 6];
                        a7 = A[i + k][j + 7];

                        /* Write A0ᵀ into upper-left of B-block */
                        B[j + 0][i + k] = a0;
                        B[j + 1][i + k] = a1;
                        B[j + 2][i + k] = a2;
                        B[j + 3][i + k] = a3;

                        /* Temporarily store A1ᵀ in upper-right of B-block */
                        B[j + 0][i + k + 4] = a4;
                        B[j + 1][i + k + 4] = a5;
                        B[j + 2][i + k + 4] = a6;
                        B[j + 3][i + k + 4] = a7;
                    }

                    /* Step 2: left-bottom 4x4 (A2) and swap with the
                     * temporary top-right 4x4 of B (which holds A1ᵀ).
                     */
                    for (k = 0; k < 4; ++k) {
                        a0 = A[i + 4][j + k];
                        a1 = A[i + 5][j + k];
                        a2 = A[i + 6][j + k];
                        a3 = A[i + 7][j + k];

                        /* Values from B that currently store A1ᵀ */
                        a4 = B[j + k][i + 4];
                        a5 = B[j + k][i + 5];
                        a6 = B[j + k][i + 6];
                        a7 = B[j + k][i + 7];

                        /* Put A2ᵀ into the upper-right positions */
                        B[j + k][i + 4] = a0;
                        B[j + k][i + 5] = a1;
                        B[j + k][i + 6] = a2;
                        B[j + k][i + 7] = a3;

                        /* Move the old A1ᵀ values down into lower-left */
                        B[j + k + 4][i + 0] = a4;
                        B[j + k + 4][i + 1] = a5;
                        B[j + k + 4][i + 2] = a6;
                        B[j + k + 4][i + 3] = a7;
                    }

                    /* Step 3: bottom-right 4x4 (A3) into lower-right of B-block */
                    for (k = 4; k < 8; ++k) {
                        a0 = A[i + k][j + 4];
                        a1 = A[i + k][j + 5];
                        a2 = A[i + k][j + 6];
                        a3 = A[i + k][j + 7];

                        B[j + 4][i + k] = a0;
                        B[j + 5][i + k] = a1;
                        B[j + 6][i + k] = a2;
                        B[j + 7][i + k] = a3;
                    }
                }
            }
        }
    }

    /* 3) General case (e.g., 61 x 67): blocked transpose with 16x16 tiles */
    else {
        for (i = 0; i < N; i += 16) {
            for (j = 0; j < M; j += 16) {
                for (k = i; k < N && k < i + 16; ++k) {
                    for (l = j; l < M && l < j + 16; ++l) {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 * The driver also uses this to check correctness.
 */
char trans_desc[] = "Simple row-wise transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            B[j][i] = A[i][j];
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver, so they can be accessed and
 *     tested from there.
 */
void registerFunctions()
{
    /* Register the optimized transpose */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register a simple baseline transpose for debugging/profiling */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of A.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
