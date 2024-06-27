//Wav file normalizer - Assembly Version
//By Jose Campos
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

extern short average_vol(short samples[], int sample_size);

extern short ref_vol(short avg_vals[], int size);

extern void amp_factor(short avg_vols[], int size, short ref_val, int amp_facts[]);

extern void apply_amp_fact(short sample_block[], int size, int amp_fact);

int main()
{
    //Current sample read
    short sample;;

    //Holder variables to read meta data
    char item[200];
    short short_var;
    int int_var;

    //Counter variables
    int i, processed_bytes;
    int current_chunk = 0;

    //Offset for start of block sample
    int block_offset = 0;

    //Flag for while loops
    int go = 1;

    //Average vol in current block
    short avg_vol_l;
    short avg_vol_r;

    //Average volumes per sample block
    short avg_vols[14000];

    //Index for average vols array
    int avg_idx = 0;

    //Reference volume
    short ref_val;

    //Amplification factors
    int amp_facts[14000];

    //Amplification factors array index
    int amp_fact_idx = 0;

    //Sample data pointer
    short* psamples;

    //Left and right data pointers
    short* psamples_left;
    short* psamples_right;

    //Size of sample arrays
    int num_samples;
    int num_sub_samples;

    //Left and right write indexes
    int l_samples_idx = 0;
    int r_samples_idx = 0;

    //Chunk Sizes
    int Chunk_size;
    int subchunk_size;

    //File pointers
    FILE* audio_file;
    FILE* audio_w_file;

    //Clock variables
    LARGE_INTEGER frequency, start1, end1, start2,
        end2, start3, end3, start4, end4,
        start5, end5, start6, end6;
    double elapsedTime1, elapsedTime2, elapsedTime3,
        elapsedTime4, elapsedTime5, elapsedTime6;

    //Reference point
    QueryPerformanceFrequency(&frequency);

    //Read audio file
    audio_file = fopen("KidA.wav", "rb");

    //Create audio file to write to
    audio_w_file = fopen("KidA_norm_asm.wav", "wb");

    if (audio_file == NULL)
    {
        printf("Error when reading file.\n");
        return(1);
    }

    if (audio_w_file == NULL)
    {
        printf("Error when creating file.\n");
        return(1);
    }

    /* Chunk ID: 4-byte string */
    fread(item, 4, 1, audio_file);
    item[4] = 0;

    //Write into new file
    fwrite(item, 4, 1, audio_w_file);

    /* Chunk size: 32-bit int */
    fread(&Chunk_size, 4, 1, audio_file);
    //Write into new file
    fwrite(&Chunk_size, 4, 1, audio_w_file);

    /* Chunk format: 4-byte string */
    fread(item, 4, 1, audio_file);
    item[4] = 0;
    //Write into new file
    fwrite(item, 4, 1, audio_w_file);
    processed_bytes = 4;

    /* Now we will process sub-chunks */
    while (processed_bytes < Chunk_size)
    {
        /* Subchunk ID: 4-byte string */
        fread(item, 4, 1, audio_file);
        item[4] = 0;
        //Write into new file
        fwrite(item, 4, 1, audio_w_file);

        //Increase processed bytes by 4
        processed_bytes += 4;

        /* Subchunk size: 32-bit int */
        fread(&subchunk_size, 4, 1, audio_file);
        //Write into new file
        fwrite(&subchunk_size, 4, 1, audio_w_file);

        //Increase processed bytes by 4
        processed_bytes += 4;

        /* Process subchunk data */
        if (strcmp(item, "fmt ") == 0)
        {
            /* Audio format: 2-byte integer */
            fread(&short_var, 2, 1, audio_file);
            //Write into new file
            fwrite(&short_var, 2, 1, audio_w_file);

            /* Number of channels: 2-byte integer */
            fread(&short_var, 2, 1, audio_file);
            //Write into new file
            fwrite(&short_var, 2, 1, audio_w_file);

            /* Sample rate: 4-byte integer */
            fread(&int_var, 4, 1, audio_file);
            //Write into new file
            fwrite(&int_var, 4, 1, audio_w_file);

            /* Byte rate: 4-byte integer */
            fread(&int_var, 4, 1, audio_file);
            //Write into new file
            fwrite(&int_var, 4, 1, audio_w_file);

            /* Block alignment: 2-byte integer */
            fread(&short_var, 2, 1, audio_file);
            //Write into new file
            fwrite(&short_var, 2, 1, audio_w_file);

            /* Bits per sample: 2-byte integer */
            fread(&short_var, 2, 1, audio_file);
            //Write into new file
            fwrite(&short_var, 2, 1, audio_w_file);
        }
        else if (strcmp(item, "data") == 0)
        {

            /* This is where we read the audio samples data */
            /* Read samples in one shot
             * Note that samples come interleaved for the two channels (left,right,left,right,...)
             */
            psamples = (short*)malloc(subchunk_size);
            fread(psamples, subchunk_size, 1, audio_file);

            //Get size of array
            num_samples = subchunk_size / sizeof(short);  // Calculate the number of short samples

            //Allocate memory for left and right sections of psamples
            //Add a little over a half in case more memory is needed
            psamples_left = (short*)malloc((subchunk_size / 2) + 1);
            psamples_right = (short*)malloc((subchunk_size / 2) + 1);

            //Divide samples into left and right channels
            for (i = 0; i < num_samples; i++)
            {
                //If sample index is even
                if ((i % 2) == 0)
                {
                    //Append to left samples array
                    psamples_left[l_samples_idx] = psamples[i];
                    l_samples_idx += 1;
                }
                //If sample position is odd
                else
                {
                    //Append to right samples array
                    psamples_right[r_samples_idx] = psamples[i];
                    r_samples_idx += 1;
                }
            }

            //Store size of sub_arrays
            num_sub_samples = l_samples_idx;

            //Free original array
            free(psamples);

            //Loop to calculate average volumes
            while (go == 1)
            {
                //Calculate avg vol per block
                //Less than 4410 samples in block
                if (num_sub_samples - block_offset < 4410)
                {
                    //Measure execution time of average_vol function with array size < 4410
                    //Initial time stamp
                    QueryPerformanceCounter(&start2);

                    //Compute avg_vol of left block
                    avg_vol_l = average_vol(psamples_left + block_offset, (num_sub_samples % 4410));

                    //Final time stamp
                    QueryPerformanceCounter(&end2);

                    //Compute avg_vol of right block
                    avg_vol_r = average_vol(psamples_right + block_offset, (num_sub_samples % 4410));

                    //Calculate time elapsed
                    elapsedTime2 = (double)(end2.QuadPart - start2.QuadPart) / frequency.QuadPart;

                    //End loop
                    go = 0;
                }
                //4410 samples in block
                else
                {
                    //Measure function performance once
                    if (block_offset == 4410)
                    {
                        //Measure execution time of average_vol function with array size of 4410
                        //Initial time stamp
                        QueryPerformanceCounter(&start1);

                        //Compute avg_vol of left block
                        avg_vol_l = average_vol(psamples_left + block_offset, 4410);

                        //Final time stamp
                        QueryPerformanceCounter(&end1);

                        //Calculate time elapsed
                        elapsedTime1 = (double)(end1.QuadPart - start1.QuadPart) / frequency.QuadPart;
                    }
                    //Execute normally next iterations
                    else
                    {
                        //Compute avg_vol of left block
                        avg_vol_l = average_vol(psamples_left + block_offset, 4410);
                    }

                    //Compute avg_vol of right block
                    avg_vol_r = average_vol(psamples_right + block_offset, 4410);
                }

                //Write avg vols into array 
                //Left volumes are in even positions
                //Right volumes are in odd positoins

                //Write left average volume
                avg_vols[avg_idx] = avg_vol_l;
                avg_idx += 1;

                //Write right average volume
                avg_vols[avg_idx] = avg_vol_r;
                avg_idx += 1;

                //Move head of array
                block_offset += 4410;
            }

            //Measure execution time of ref_vol function
            //Initial time stamp
            QueryPerformanceCounter(&start3);

            //Find max avg vol
            ref_val = ref_vol(avg_vols, avg_idx);

            //Final time stamp
            QueryPerformanceCounter(&end3);

            //Calculate time elapsed
            elapsedTime3 = (double)(end3.QuadPart - start3.QuadPart) / frequency.QuadPart;

            //Measure execution time of amp_factor function
            //Initial time stamp
            QueryPerformanceCounter(&start4);

            //Find amplification factor for each block
            amp_factor(avg_vols, avg_idx, ref_val, amp_facts);

            //Final time stamp
            QueryPerformanceCounter(&end4);

            //Calculate time elapsed
            elapsedTime4 = (double)(end4.QuadPart - start4.QuadPart) / frequency.QuadPart;

            //Reset offset of sample block
            block_offset = 0;

            //Reset loop flag
            go = 1;

            //Loop to apply amplification factors
            while (go == 1)
            {

                //Calculate avg vol per block
                //Less than 4410 samples in block
                if (num_sub_samples - block_offset < 4410)
                {

                    //Measure execution time of apply_amp_fact function with array size < 4410
                    //Initial time stamp
                    QueryPerformanceCounter(&start6);

                    //Normalize left block
                    apply_amp_fact(psamples_left + block_offset, (num_sub_samples % 4410), amp_facts[amp_fact_idx]);

                    //Final time stamp
                    QueryPerformanceCounter(&end6);

                    //Calculate time elapsed
                    elapsedTime6 = (double)(end6.QuadPart - start6.QuadPart) / frequency.QuadPart;

                    //Incease index in amplification factors' index
                    amp_fact_idx += 1;

                    //Normalize right block
                    apply_amp_fact(psamples_right + block_offset, (num_sub_samples % 4410), amp_facts[amp_fact_idx]);

                    //Incease index in amplification factors' index
                    amp_fact_idx += 1;


                    //End loop
                    go = 0;
                }
                //4410 samples in block
                else
                {
                    //Measure function performance once
                    if (block_offset == 4410)
                    {
                        //Measure execution time of average_vol function with array size of 4410
                        //Initial time stamp
                        QueryPerformanceCounter(&start5);

                        //Compute avg_vol of left block
                        apply_amp_fact(psamples_left + block_offset, 4410, amp_facts[amp_fact_idx]);

                        //Final time stamp
                        QueryPerformanceCounter(&end5);

                        //Calculate time elapsed
                        elapsedTime5 = (double)(end5.QuadPart - start5.QuadPart) / frequency.QuadPart;
                    }
                    //Execute normally next iterations
                    else
                    {
                        //Compute avg_vol of left block
                        apply_amp_fact(psamples_left + block_offset, 4410, amp_facts[amp_fact_idx]);
                    }

                    //Incease index in amplification factors' index
                    amp_fact_idx += 1;

                    //Compute avg_vol of right block
                    apply_amp_fact(psamples_right + block_offset, 4410, amp_facts[amp_fact_idx]);

                    //Incease index in amplification factors' index
                    amp_fact_idx += 1;
                }

                //Move head of array
                block_offset += 4410;
            }

            //Loop to write normalized samples into new file
            for (i = 0; i < num_sub_samples; i++)
            {
                //Write left and write samples in current index position
                fwrite(&(psamples_left[i]), 2, 1, audio_w_file);
                fwrite(&(psamples_right[i]), 2, 1, audio_w_file);
            }
            //Free left and right sample arrays
            free(psamples_left);
            free(psamples_right);

            //Print Results
            printf("Printing C Version Results:\n");
            printf("Execution time for average_vol with array == 4410: %.8f seconds\n", elapsedTime1);
            printf("Execution time for average_vol with array < 4410: %.8f seconds\n", elapsedTime2);
            printf("Execution time for ref_vol: %.8f seconds\n", elapsedTime3);
            printf("Execution time for amp_factor: %.8f seconds\n", elapsedTime4);
            printf("Execution time for apply_amp_fact with array == 4410: %.8f seconds\n", elapsedTime5);
            printf("Execution time for apply_amp_fact with array == 4410: %.8f seconds\n", elapsedTime6);
        }
        else
        {
            /* Unknown subchunk*/
            fread(item, subchunk_size, 1, audio_file);
            fwrite(item, subchunk_size, 1, audio_w_file);
        }

        processed_bytes += subchunk_size;
    }
    //Close read and write files
    fclose(audio_file);
    fclose(audio_w_file);
    return(0);
}