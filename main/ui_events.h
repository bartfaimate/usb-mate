#pragma once


typedef enum {

    /*--------- System events -------------*/
    UI_SYSTEM_STARTING,
    UI_SYSTEM_STARTED,

} ui_event_type_t;




/**
 * ui event type definition. it hold event type and data
 */
typedef struct {
    ui_event_type_t type;
   
} ui_event_t;
