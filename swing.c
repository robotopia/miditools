#include <stdio.h>
#include "common.h"

int main (int argc, char *argv[]) {
    midihdr mhdr;
    trkhdr  thdr;
    event   evt;

    unsigned int tick_number;
    //unsigned int beat_number;
    unsigned int frac_beat;
    int is_swinging = 0; // true iff previous event has been altered

    fprintf(stderr, "Reading header chunk...\n");
    read_hdrchunk( stdin, &mhdr );
    write_hdrchunk( stdout, &mhdr );
    fprintf(stderr, "  ntracks = %d\n  division = %d\n", mhdr.ntracks, mhdr.division);

    int swing_diff = mhdr.division / 6; // == 2/3 - 1/2
    int eighth     = mhdr.division / 2;

    int track_bytes_read;
    int track_number;
    // Iterate through the tracks
    for (track_number = 0; track_number < mhdr.ntracks; track_number++) {
        fprintf(stderr, "Reading track #%d...\n", track_number+1);
        read_trkchunk( stdin, &thdr );
        write_trkchunk( stdout, &thdr );
        tick_number = 0;
        track_bytes_read = 0;
        // Iterate through the events
        while (track_bytes_read < thdr.track_length) {
            track_bytes_read += read_event( stdin, &evt );
            // Swing eights and write out swung events
            tick_number += evt.v_time;
            if (evt.v_time > 0) {
                //beat_number = tick_number / mhdr.division;
                frac_beat   = tick_number % mhdr.division;
                if (is_swinging) {
                    evt.v_time -= swing_diff;
                    is_swinging = 0;
                }
                if (frac_beat == eighth) {
                    evt.v_time += swing_diff;
                    is_swinging = 1;
                }
            }
            write_event( stdout, &evt );
        }
    }

    return 0;
}
