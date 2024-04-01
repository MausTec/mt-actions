#ifndef __mt_actions_h
#define __mt_actions_h

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

struct mta_functions;
struct mta_actions;

typedef enum mta_condition_type {
    MTA_CONDITION_IF,
    MTA_CONDITION_UNLESS,
    MTA_CONDITION_WHILE,
    MTA_CONDITION_UNTIL,
} mta_condition_type_t;

/**
 * @brief Conditional action collection.
 *
 * Actions may have conditions. This is handled by an internal function "if", which is called with
 * the arguments "all", "any", and "none". Those are action lists which use the return value of the
 * function to determine success. 0 values are success.
 *
 * Internal functions are declared for "eq", "neq", "gt", "gte", "lt", and "lte".
 *
 * In response, the arguments "then" and "else" are available which are action lists for the
 * corresponding logical flow. In the future, the "if" action keyword will be expanded to include a
 * "while" action, which can be used in the same way to create while/then loops.
 *
 * "return", "continue", and "break" are reserved function names as well. There will be static
 * action pointers for these to control flow in the C API.
 *
 */
typedef struct mta_condition {
    enum mta_condition_type type;
    struct mta_actions* cond_all;
    struct mta_actions* cond_any;
    struct mta_actions* cond_none;
    struct mta_actions* cond_then;
    struct mta_actions* cond_else;
} mta_condition_t;

/**
 * @brief Linked list of driver action invocations, which reference a functinon and pass arguments.
 *
 */
typedef struct mta_actions {
    cJSON* args;
    struct mta_actions* next;
    struct mta_functions* fn;
    struct mta_condition* condition;
    char function[];
} mta_actions_t;

/**
 * @brief Linked list of driver event registrations, which reference an action list.
 *
 */
typedef struct mta_events {
    struct mta_actions* actions;
    struct mta_events* next;
    char event_name[];
} mta_events_t;

/**
 * @brief Linked list of function definitions loaded into a particular driver.
 *
 */
typedef struct mta_functions {
    struct mta_actions* actions;
    struct mta_functions* next;
    char function_name[];
} mta_functions_t;

/**
 * @brief Linked list of variables loaded into a particular driver.
 *
 */
typedef struct mta_variables {
    int value;
    struct mta_variables* next;
    char variable_name[];
} mta_variables_t;

/**
 * @brief Match struct containing parameters to automatically load a driver given an accessory
 * descriptor.
 *
 */
typedef struct mta_match {
    int vid;
    int pid;
    char serial[];
} mta_match_t;

/**
 * @brief Base driver installation, which contains all the relevant function definitions, actions,
 * and config.
 *
 */
typedef struct mta_plugin {
    mta_events_t* events;
    mta_functions_t* functions;
    mta_match_t* match;
    mta_variables_t* variables;
    cJSON* config;
    char display_name[];
} mta_plugin_t;

typedef void (*mta_system_function_t)(mta_plugin_t* driver, cJSON* args);

// Data Definitions
void mta_load(mta_plugin_t** driver, cJSON* root);
void mta_unload(mta_plugin_t* driver);
void mta_register_system_function(const char* fn_name, mta_system_function_t fn);

mta_functions_t* mta_define_function(mta_plugin_t* driver, const char* fn_name);

mta_actions_t*
mta_define_function_action(mta_functions_t* function, const char* callee_name, cJSON* args);

mta_events_t* mta_define_event(mta_plugin_t* driver, const char* evt_name);

mta_actions_t* mta_define_event_action(mta_events_t* events, const char* callee_name, cJSON* args);

// Runtime Invocations
void mta_function_call(mta_plugin_t* driver, const char* fn_name, cJSON* args);
void mta_get_variable(mta_plugin_t* driver, const char* var_name, int* value);
void mta_set_variable(mta_plugin_t* driver, const char* var_name, int value);
void mta_action_invoke(mta_plugin_t* driver, mta_actions_t* action);
void mta_event_invoke(mta_plugin_t* driver, const char* event, int arg);

/**
 * @brief This invokes any matching event across all drivers registered.
 *
 * @param event
 * @param arg
 */
void maus_bus_invoke_all_events(const char* event, int arg);

// Driver Search and Invocations
mta_plugin_t* maus_bus_find_driver(mta_match_t* match);

typedef void (*mta_enumeration_cb_t)(mta_plugin_t* driver);
size_t maus_bus_enumerate_drivers(mta_enumeration_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif
