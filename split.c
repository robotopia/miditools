#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#define  MAX_STRLEN  256

int main (int argc, char *argv[]) {
    // Some very basic documentation
    if (argc < 2) {
        fprintf(stderr, "usage: split [input MIDI file] [output MIDI base name]\n");
        fprintf(stderr, "  (Track 1 is assumed to be a control track)\n");
        exit(1);
    }

    midihdr mhdr;
    trkhdr  thdr;

    int n; // A generic loop counter

    char  in_filename[MAX_STRLEN];
    char out_basename[MAX_STRLEN];
    char out_filename[MAX_STRLEN];

    sprintf(  in_filename, "%s", argv[1] );
    sprintf( out_basename, "%s", argv[2] );

    // Open file for reading
    FILE *f_in = fopen( in_filename, "r" );
    fprintf(stderr, "Reading header chunk...\n");
    read_hdrchunk( f_in, &mhdr );
    fprintf(stderr, "  ntracks = %d\n  division = %d\n", mhdr.ntracks - 1, mhdr.division);
    int ntracks = mhdr.ntracks - 1;

    // Read the first (control) track
    read_trkchunk( f_in, &thdr );
    int control_length = thdr.track_length;
    char control_data[control_length];
    for (n = 0; n < control_length; n++)
        control_data[n] = fgetc( f_in );

    // Prepare the header track for writing
    mhdr.format = 1;      // The control track + the sound track will
    mhdr.ntracks = 2;     // be present in each file

    // Open the appropriate number of output files
    // and write the (adjusted) header and first (control) track
    fprintf( stderr, "Preparing track files...\n" );
    FILE *f[ntracks];
    int t; // A counter for "track number" (not including control track)
    for (t = 0; t < ntracks; t++) {
        sprintf(out_filename, "%s_%02d.mid", out_basename, t+1);
        f[t] = fopen(out_filename, "w");
        write_hdrchunk( f[t], &mhdr );
        // Copy the first (control) track wholesale
        write_trkchunk( f[t], &thdr );
        for (n = 0; n < control_length; n++)
            fputc( control_data[n], f[t] );
    }

    // Iterate through the tracks and copy them directly to
    // the corresponding output files
    for (t = 0; t < ntracks; t++) {
        fprintf( stderr, "Writing track %d to file...\n", t+1 );
        read_trkchunk( f_in, &thdr );
        write_trkchunk( f[t], &thdr );
        for (n = 0; n < thdr.track_length; n++)
            fputc( fgetc(f_in), f[t] );
        fclose(f[t]);
    }

    fclose(f_in);

    return 0;
}
