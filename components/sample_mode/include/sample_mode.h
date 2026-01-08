#ifndef SAMPLE_MODE_H_
#define SAMPLE_MODE_H_
// pad mode section
// action function type
typedef void (*event_handler)(int pad_id);

typedef struct {
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;
} sample_mode_t;

// this is to pick the mode

#define HOLD 0
#define ONESHOT 1
#define LOOP 2
#define ONESHOT_LOOP 3
extern const sample_mode_t* SAMPLE_MODES[];

// exposed function to select the pad mode
void set_sample_mode(int, const sample_mode_t*);
void sample_mode_init();
#endif