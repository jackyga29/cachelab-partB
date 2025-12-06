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
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, row, col;
    int temp0, temp1, temp2, temp3;

    // 32x32 matrix case
    if (M == 32 && N == 32) {
        // Use 8x8 blocking
        for (col = 0; col < N; col += 8) {
            for (row = 0; row < M; row += 8) {
                for (i = row; i < row + 8; i++) {
                    for (j = col; j < col + 8; j++) {
                        // Handle diagonal blocks differently to reduce conflicts
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp0 = A[i][j];
                            temp1 = i;
                        }
                    }
                    // Store diagonal element after the inner loop
                    if (row == col) {
                        B[temp1][temp1] = temp0;
                    }
                }
            }
        }
    }
    // 64x64 matrix case
    else if (M == 64 && N == 64) {
        // Use 8x8 blocking but process in 4x4 sub-blocks
        for (col = 0; col < N; col += 8) {
            for (row = 0; row < M; row += 8) {
                // Process first 4 rows
                for (i = row; i < row + 4; i++) {
                    // Load 8 elements from A
                    temp0 = A[i][col];
                    temp1 = A[i][col + 1];
                    temp2 = A[i][col + 2];
                    temp3 = A[i][col + 3];
                    
                    // Store to B transpose position
                    B[col][i] = temp0;
                    B[col + 1][i] = temp1;
                    B[col + 2][i] = temp2;
                    B[col + 3][i] = temp3;
                    
                    // Temporarily store next 4 in upper right of B block
                    B[col][i + 4] = A[i][col + 4];
                    B[col + 1][i + 4] = A[i][col + 5];
                    B[col + 2][i + 4] = A[i][col + 6];
                    B[col + 3][i + 4] = A[i][col + 7];
                }
                
                // Rearrange and complete the transpose
                for (i = col; i < col + 4; i++) {
                    // Save misplaced elements
                    temp0 = B[i][row + 4];
                    temp1 = B[i][row + 5];
                    temp2 = B[i][row + 6];
                    temp3 = B[i][row + 7];
                    
                    // Move them to correct position
                    B[i][row + 4] = A[row + 4][i];
                    B[i][row + 5] = A[row + 5][i];
                    B[i][row + 6] = A[row + 6][i];
                    B[i][row + 7] = A[row + 7][i];
                    
                    // Place saved elements in their transpose position
                    B[i + 4][row] = temp0;
                    B[i + 4][row + 1] = temp1;
                    B[i + 4][row + 2] = temp2;
                    B[i + 4][row + 3] = temp3;
                }
                
                // Complete bottom right sub-block
                for (i = row + 4; i < row + 8; i++) {
                    temp0 = A[i][col + 4];
                    temp1 = A[i][col + 5];
                    temp2 = A[i][col + 6];
                    temp3 = A[i][col + 7];
                    
                    B[col + 4][i] = temp0;
                    B[col + 5][i] = temp1;
                    B[col + 6][i] = temp2;
                    B[col + 7][i] = temp3;
                }
            }
        }
    }
    // 61x67 matrix case
    else {
        // Use 16x16 blocking for irregular size
        for (col = 0; col < M; col += 16) {
            for (row = 0; row < N; row += 16) {
                for (i = row; (i < row + 16) && (i < N); i++) {
                    for (j = col; (j < col + 16) && (j < M); j++) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
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
