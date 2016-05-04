#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <string.h>

// Meta-type events (following a 0xFF byte)
#define SEQUENCE_NUMBER                   0X00
#define TEXT_EVENT                        0X01
#define COPYRIGHT_NOTICE                  0X02
#define SEQUENCE_OR_TRACK_NAME            0X03
#define INSTRUMENT_NAME                   0X04
#define LYRIC_TEXT                        0X05
#define MARKER_TEXT                       0X06
#define CUE_POINT                         0X07
#define MIDI_CHANNEL_PREFIX_ASSIGNMENT    0X20
#define END_OF_TRACK                      0X2F
#define TEMPO_SETTING                     0X51
#define SMPTE_OFFSET                      0X54
#define TIME_SIGNATURE                    0X58
#define KEY_SIGNATURE                     0X59
#define SEQUENCER_SPECIFIC_EVENT          0X7F

// Midi-type events
#define NOTE_OFF                          0x80
#define NOTE_ON                           0x90
#define POLYPHONIC_KEY_PRESSURE           0xA0
#define CONTROL_CHANGE                    0xB0
#define PROGRAM_CHANGE                    0xC0
#define CHANNEL_PRESSURE                  0xD0
#define PITCH_BEND_CHANGE                 0xE0

// System common messages
#define SYSTEM_EXCLUSIVE                  0xF0
#define TIME_CODE_QTR_FRAME               0xF1
#define SONG_POSITION_PTR                 0xF2
#define SONG_SELECT                       0xF3
#define TUNE_REQUEST                      0xF6
#define END_EXCLUSIVE                     0xF7

// System real time messages
#define TIMING_CLOCK                      0xF8
#define START_SEQUENCE                    0xFA
#define CONTINUE_SEQUENCE                 0xFB
#define STOP_SEQUENCE                     0xFC
#define ACTIVE_SENSING                    0xFE
#define RESET                             0xFF


#define BITS_PER_BYTE                     8
#define MAX_BUFFER_SIZE                   1024


typedef enum { MIDI, META, SYSEX } event_type;

typedef struct _midihdr {
    unsigned int  format;        // 0 = single track; 1 = multi-track; 2 = multi-song
    unsigned int  ntracks;
    int           division;      // unit of time for delta timing
} midihdr;

typedef struct _trkhdr {
    unsigned int track_length;  // in bytes
} trkhdr;

typedef struct _event {
    unsigned int  v_time;       // the elapsed time (delta time) from the previous event to this event
    event_type    evt_type;
    unsigned char evt_id;
    // Midi event fields:
    unsigned char chan;
    unsigned char key;
    unsigned char velocity;
    unsigned char ctrl;
    unsigned char pgrm;
    unsigned int  pitchbend;
    // Meta event fields:
    unsigned int  data_size;
    // Meta event and Sysex event fields:
    char          data[MAX_BUFFER_SIZE];   // Should be enough...
} event;

// Functions for reading MIDI data
unsigned int read_hdrchunk( FILE*, midihdr* );
unsigned int read_trkchunk( FILE*, trkhdr*  );
unsigned int read_event( FILE*, event*  );

// Functions for writing MIDI data
unsigned int write_hdrchunk( FILE*, midihdr* );
unsigned int write_trkchunk( FILE*, trkhdr*  );
unsigned int write_event( FILE*, event*  );

// Functions for reading variable length integers
unsigned int read_varnum( FILE*, unsigned int* );
unsigned int write_varnum( FILE*, unsigned int );

// Functions for handling big endianness
unsigned int read_be_int( FILE*, unsigned int );
unsigned int write_be_int( FILE*, unsigned int, unsigned int );


#endif
