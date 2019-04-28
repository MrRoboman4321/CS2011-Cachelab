/*
 * Cachelab -- CS 2011
 * 4/28/19
 * Eli Benevedes - eabenevedes@wpi.edu
 * Patrick Eaton - pweaton@wpi.edu
*/

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
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
    if(M == 32 && N == 32) {
        //If the array is 32x32, iterate through 8 by 8 blocks in row major order
        for(int block_row = 0; block_row < 4; block_row++) {
            for(int block_col = 0; block_col < 4; block_col++) {
                //Iterate through each of the blocks in row major order
                for (int row = 0; row < 8; row++) {
                    for (int col = 0; col < 8; col++) {
                        //Transpose data from array A to array B
                        int copy_a = A[8 * block_row + row][8 * block_col + col];
                        B[8 * block_col + col][8 * block_row + row] = copy_a;
                    }
                }
            }
        }
    } else if(M == 64 && N == 64) {
        //If the array is 64x64, iterate through 4 by 4 blocks in row major order
        //If we were to loop over 8x8 blocks like we did for the 32x32 matrix, we would run into issues with collisions
        // due to the set id's wrapping within each of the 8x8 blocks.
        for(int block_row = 0; block_row < 16; block_row++) {
            for (int block_col = 0; block_col < 16; block_col++) {
                //Iterate through each of the blocks in row major order
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        //Transpose data from array A to array B
                        int copy_a = A[4 * block_row + row][4 * block_col + col];
                        B[4 * block_col + col][4 * block_row + row] = copy_a;
                    }
                }
            }
        }
    } else {
        //For an arbitrary length array, we split the array up into blocks again with width 8
        // (to preserve temporal locality), but with dynamic height. We look to minimize the number of leftover rows
        // after dividing the array up into chunks, to minimize the amount of extra rows we need to copy over after
        // block copies.
        int block_height = 0;

        if(M % 8 == 0) {
            block_height = 8;
        } else if (M % 7 < M % 6 && M % 7 < M % 5 && M % 7 < M % 4) {
            block_height = 7;
        } else if (M % 6 < M % 7 && M % 6 < M % 5 && M % 6 < M % 4) {
            block_height = 6;
        } else if (M % 5 < M % 7 && M % 5 < M % 6 && M % 5 < M % 4) {
            block_height = 5;
        } else {
            block_height = 4;
        }

        int block_width = 8;

        //The block counts are the max number of blocks that can fit within an NxM array.
        int block_rows = (N - (N % block_height))/block_height;
        int block_cols = (M - (M % block_width))/block_width;

        //Do standard block transposes
        for(int block_row = 0; block_row < block_rows; block_row++) {
            for(int block_col = 0; block_col < block_cols; block_col++) {
                for(int row = 0; row < block_height; row++) {
                    for(int col = 0; col < block_width; col++) {
                        B[block_width*block_col + col][block_height*block_row + row] = A[block_height*block_row + row][block_width*block_col + col];
                    }
                }
            }
        }

        //This section transposes the leftover columns to the right of the blocks.
        for(int row = 0; row < N; row++) {
            for(int col = block_cols * block_width; col < M; col++) {
                B[col][row] = A[row][col];
            }
        }

        //This section transposes the leftover rows below the blocks.
        for(int row = block_rows * block_height; row < N; row++) {
            for(int col = 0; col < block_cols * block_width; col++) {
                B[col][row] = A[row][col];
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

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

