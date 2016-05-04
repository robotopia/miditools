#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

const unsigned char HDR_LABEL_SIZE = 4;
const unsigned int  HDR_SIZE = 6;

/* Functions for reading MIDI data */

unsigned int read_hdrchunk( FILE *in, midihdr *mhdr ) {
    char hdr_label[HDR_LABEL_SIZE+1];
    char *retval;

    // Read in header label
    retval = fgets(hdr_label, HDR_LABEL_SIZE+1, in);
    if (!retval) {
        fprintf(stderr, "Error reading header\n");
        exit(1);
    }

    if (strcmp(hdr_label, "MThd") != 0) {
        fprintf(stderr, "Header chunk does not start with \"MThd\"!\n");
        exit(1);
    }

    // Read in header length
    unsigned int hdr_size = read_be_int( in, 4 );
    if (hdr_size != HDR_SIZE) {
        fprintf(stderr, "Header chunk (%d) is not the right size (%d)\n", hdr_size, HDR_SIZE);
        exit(1);
    }

    // Read in format
    mhdr->format = read_be_int( in, 2 );
    if (mhdr->format > 2) {
        fprintf(stderr, "Invalid MIDI format (%d)\n", mhdr->format);
        exit(1);
    }

    // Read in number of tracks and delta timing division
    mhdr->ntracks = read_be_int( in, 2 );
    mhdr->division = read_be_int( in, 2 );

    return 0;
}


unsigned int read_trkchunk( FILE *in, trkhdr *thdr ) {
    char trk_label[HDR_LABEL_SIZE+1];
    char *retval;

    // Read in track label
    retval = fgets(trk_label, HDR_LABEL_SIZE+1, in);
    if (!retval) {
        fprintf(stderr, "Error reading track header\n");
        exit(1);
    }

    if (strcmp(trk_label, "MTrk") != 0) {
        fprintf(stderr, "Track chunk does not start with \"MTrk\"!\n");
        exit(1);
    }

    // Read in track length
    thdr->track_length = read_be_int( in, 4 );

    return 0;
}


unsigned int read_event( FILE *in, event *evt ) {

    // Keep track of how much data is read in
    int data_read = 0;

    // Read in time since last event
    unsigned int varnum_bytes = read_varnum( in, &(evt->v_time) );
    if (varnum_bytes == 0) {
        fprintf(stderr, "Zero bytes read from variable number\n");
        exit(1);
    }
    data_read += varnum_bytes;
#ifdef DEBUG        
fprintf(stderr, "EVENT: \"Time\" since last event = %d (Have read %d event byte(s))\n", evt->v_time, data_read);
#endif

    // Read the "event" byte and interpret it
    unsigned char evt_firstbyte = fgetc( in );
    data_read++;

    if (evt_firstbyte == 0xFF)
        evt->evt_type = META;
    else if ((evt_firstbyte == 0xF0) || (evt_firstbyte == 0xF7))
        evt->evt_type = SYSEX;
    else
        evt->evt_type = MIDI;

    // Read in the appropriate number of bytes
    if (evt->evt_type == MIDI) {
        unsigned char pitch_lo, pitch_hi; // Just in case they're used later
        evt->evt_id = (evt_firstbyte & 0xF0); // Mask out the last 4 bits
        evt->chan   = (evt_firstbyte & 0x0F); // Mask out the first 4 bits
#ifdef DEBUG        
fprintf(stderr, " MIDI: evt_id = 0x%02X,  chan = 0x%02X, (have read %d event bytes)\n", evt->evt_id, evt->chan, data_read);
#endif
        switch (evt->evt_id) {
            case NOTE_OFF:
            case NOTE_ON:
            case POLYPHONIC_KEY_PRESSURE:
                evt->key      = fgetc( in );
                evt->velocity = fgetc( in );
                data_read += 2;
                break;
            case CONTROL_CHANGE:
                evt->ctrl     = fgetc( in );
                evt->velocity = fgetc( in );
                data_read += 2;
                break;
            case PROGRAM_CHANGE:
                evt->pgrm     = fgetc( in );
                data_read++;
                break;
            case CHANNEL_PRESSURE:
                evt->velocity = fgetc( in );
                data_read++;
                break;
            case PITCH_BEND_CHANGE:
                pitch_lo       = fgetc( in );
                pitch_hi       = fgetc( in );
                data_read += 2;
                evt->pitchbend = ((int)pitch_hi << 7) | (int)pitch_lo;
                break;
            default:
                fprintf(stderr, "Unrecognised MIDI event type: 0x%02X\n", evt->evt_id);
                exit(1);
        }
    }
    else if (evt->evt_type == META) {
        // Read the "event" byte
        evt->evt_id = fgetc( in );
        data_read++;
#ifdef DEBUG        
fprintf(stderr, " META: evt_id = 0x%02X  (have read %d event bytes)\n", evt->evt_id, data_read);
#endif

        // Read the varnum telling how much data follows
        varnum_bytes = read_varnum( in, &(evt->data_size) );
        if (varnum_bytes == 0) {
            fprintf(stderr, "Zero bytes read from variable number\n");
            exit(1);
        }
        data_read += varnum_bytes;
#ifdef DEBUG        
fprintf(stderr, " META: About to read %d bytes into evt->data:  (have read %d event bytes)\n", evt->data_size, data_read);
#endif

        // Read in the meta data
//fprintf(stderr, "data_size+1 = %d, evt->data = %p\n", evt->data_size+1, evt->data);
        int i;
        for (i = 0; i < evt->data_size; i++)
            evt->data[i] = fgetc( in );
        data_read += evt->data_size;

#ifdef DEBUG        
if ((evt->evt_id == SEQUENCE_OR_TRACK_NAME) |
    (evt->evt_id == TEXT_EVENT) |
    (evt->evt_id == INSTRUMENT_NAME))
 fprintf(stderr, " META: Read \"%s\"  (have read %d event bytes)\n", evt->data, data_read);
else {
 fprintf(stderr, " META: Read");
 int i;
 for (i=0; i<evt->data_size; i++)
  fprintf(stderr, " %02X", evt->data[i]);
 fprintf(stderr, "  (have read %d event bytes)\n", data_read);
}
#endif

    }
    else if (evt->evt_type == SYSEX) {
        // Read the "event" byte and save it
        evt->evt_id = fgetc( in );
        data_read++;
fprintf(stderr, "SYSEX: evt_id = 0x%02X\n", evt->evt_id);

        // Read in the data
        int data_byte;
        for (data_byte = 0; data_byte < MAX_BUFFER_SIZE; data_byte++) {
            evt->data[data_byte] = fgetc( in );
            data_read++;
            if (evt->data[data_byte] == 0xF7)
                break;
        }
    }

    return data_read;
}

/* Functions for writing MIDI data */

unsigned int write_hdrchunk( FILE *out, midihdr *mhdr ) {

    unsigned int nbytes_written = 0;

    fprintf(out, "MThd");
    nbytes_written += 4;

    nbytes_written += write_be_int( out, 6, 4 );
    nbytes_written += write_be_int( out, mhdr->format, 2 );
    nbytes_written += write_be_int( out, mhdr->ntracks, 2 );
    nbytes_written += write_be_int( out, mhdr->division, 2 );

    return nbytes_written;
}

unsigned int write_trkchunk( FILE* out, trkhdr* thdr) {

    unsigned int nbytes_written = 0;

    fprintf(out, "MTrk");
    nbytes_written += 4;

    nbytes_written += write_be_int( out, thdr->track_length, 4 );

    return nbytes_written;
}

unsigned int write_event( FILE* out, event* evt ) {

    unsigned int nbytes_written = 0;
    unsigned char byte;

    nbytes_written += write_varnum( out, evt->v_time );

    switch (evt->evt_type) {
        case MIDI:
            byte = fputc( evt->evt_id | evt->chan, out );
            switch (evt->evt_id) {
                case NOTE_OFF:
                case NOTE_ON:
                case POLYPHONIC_KEY_PRESSURE:
                    byte = fputc( evt->key, out );
                    byte = fputc( evt->velocity, out );
                    nbytes_written += 2;
                    break;
                case CONTROL_CHANGE:
                    byte = fputc( evt->ctrl, out );
                    byte = fputc( evt->velocity, out );
                    nbytes_written += 2;
                    break;
                case PROGRAM_CHANGE:
                    byte = fputc( evt->pgrm, out );
                    nbytes_written++;
                    break;
                case CHANNEL_PRESSURE:
                    byte = fputc( evt->velocity, out );
                    nbytes_written++;
                    break;
                case PITCH_BEND_CHANGE:
                    byte = fputc( evt->pitchbend & 0x7F, out );
                    byte = fputc( (evt->pitchbend >> 7) & 0x7F, out );
                    nbytes_written += 2;
                    break;
                default:
                    fprintf(stderr, "Unrecognised MIDI event type: 0x%02X\n", evt->evt_id);
                    exit(1);
            }
            break;
        case META:
            byte = fputc( 0xFF, out );
            byte = fputc( evt->evt_id, out );
            nbytes_written += 2;

            nbytes_written += write_varnum( out, evt->data_size );
            nbytes_written += fwrite( evt->data, sizeof(char), evt->data_size, out );
            break;
        case SYSEX:
            byte = fputc( evt->evt_id, out );
            nbytes_written++;
            int i;
            for (i = 0; i < MAX_BUFFER_SIZE; i++) {
                byte = fputc( evt->data[i], out );
                nbytes_written++;
                if (byte == 0xF7)
                    break;
            }
            break;
    }

    return nbytes_written;
}

/* Functions for reading variable length integers */

unsigned int read_varnum( FILE *in, unsigned int *val ) {

    unsigned int nbytes_read = 0;
    unsigned int retval = 0;
    unsigned char byte;

    int read_next_byte = 1;
    while (read_next_byte) {
        byte = fgetc( in );
        nbytes_read++;
        if (!(byte & 0x80)) { // Check most significant bit
            read_next_byte = 0;
        }
        retval <<= 7; // Shift everything to the left 7 bits
        retval |= (byte & 0x7F); // Add on this "byte"'s value
    }

    *val = retval;
    return nbytes_read;

}

unsigned int write_varnum( FILE *out, unsigned int val ) {

    int val_tmp;

    // Work out how many bytes we'll need
    int nbytes = 1;
    if (val != 0) {
        nbytes = 0;
        val_tmp = val;
        for (val_tmp = val; val_tmp > 0; val_tmp >>= 7)
            nbytes++;
    }

    // Populate bytes from least significant (last in array) to most significant (first in array)
    unsigned char bytes[nbytes];
    int hi_bit = 0;
    int byte;
    for (byte = nbytes - 1; byte >= 0; byte--) {
        if (hi_bit)
            bytes[byte] = val | 0x80;
        else
            bytes[byte] = val & 0x7F;
        val >>= 7;
        hi_bit = 1;
    }

    return fwrite( bytes, sizeof(char), nbytes, out );
}

/* Functions for handling big endianness */

unsigned int read_be_int( FILE *in, unsigned int nbytes ) {
    int nread;
    unsigned int val = 0;
    unsigned char bytes[nbytes];

    nread = fread(bytes, sizeof(unsigned char), nbytes, in);
    if (nread != nbytes) {
        fprintf(stderr, "Error reading uint32 value\n");
        exit(1);
    }

    int byte;
    for (byte = 0; byte < nbytes; byte++)
        val |= (bytes[byte] << ((nbytes-byte-1)*BITS_PER_BYTE));

    return val;
}

unsigned int write_be_int( FILE *out, unsigned int val, unsigned int nbytes ) {

    unsigned char bytes[nbytes];

    int byte;
    for (byte = 0; byte < nbytes; byte++) {
        bytes[nbytes-byte-1] = (val & 0xFF);
        val >>= 8;
    }
    fwrite( bytes, sizeof(char), nbytes, out );

    return nbytes;
}
