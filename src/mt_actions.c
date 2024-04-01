#include "mt_actions.h"
#include <string.h>

#include "esp_log.h"
static const char* TAG = "MB_DRIVER_DEBUG_REMOVE_ME";
static const char* DEFAULT_DISPLAY_NAME = "<untitled>";

void mta_load_match(mta_plugin_t* driver, cJSON* match_json) {
    const char* serial_str = NULL;

    cJSON* serial = cJSON_GetObjectItem(match_json, "serial");
    if (serial != NULL) serial_str = serial->valuestring;

    mta_match_t* match = (mta_match_t*)malloc(
        sizeof(mta_match_t) + (serial_str != NULL ? strlen(serial_str) : 0) + 1
    );

    if (match == NULL) return;

    if (serial_str != NULL)
        strcpy(match->serial, serial_str);
    else
        match->serial[0] = '\0';

    cJSON* pid = cJSON_GetObjectItem(match_json, "pid");
    if (pid != NULL) match->pid = pid->valueint;

    cJSON* vid = cJSON_GetObjectItem(match_json, "vid");
    if (vid != NULL) match->vid = vid->valueint;

    driver->match = match;
}

void mta_load_actions(mta_actions_t** first, cJSON* actions) {
    mta_actions_t* action = NULL;
    mta_actions_t* head = NULL;
    mta_actions_t* ptr = NULL;

    if (actions == NULL || first == NULL) return;

    if (cJSON_IsArray(actions)) {
        cJSON* action_json = NULL;
        cJSON_ArrayForEach(action_json, actions) {
            mta_load_actions(first, action_json);
        }
        return;
    }

    if (cJSON_IsObject(actions)) {
        cJSON* action_json = NULL;
        cJSON_ArrayForEach(action_json, actions) {
            if (action_json->string == NULL) break;

            action = malloc(sizeof(mta_actions_t) + strlen(action_json->string) + 1);
            if (action == NULL) break;

            strcpy(action->function, action_json->string);
            action->args = cJSON_DetachItemFromObject(actions, action_json->string);
            action->next = NULL;

            // Since Object types contain multiple calls, build a chain
            if (head == NULL)
                head = action;
            else {
                for (ptr = head; ptr->next != NULL; ptr = ptr->next)
                    continue;

                ptr->next = action;
            }
        }
    }

    else if (cJSON_IsString(actions)) {
        action = malloc(sizeof(mta_actions_t) + strlen(actions->valuestring) + 1);
        if (action == NULL) return;

        strcpy(action->function, actions->valuestring);
        action->args = NULL;
        action->next = NULL;

        // Since string types are single calls, just assign head.
        head = action;
    }

    // Finally, attach the action.
    if (head != NULL) {
        if (*first == NULL) {
            *first = head;
        } else {
            for (ptr = *first; ptr->next != NULL; ptr = ptr->next)
                ;
            ptr->next = head;
        }
    }
}

void mta_load_functions(mta_plugin_t* driver, cJSON* functions_json) {
    cJSON* fn_json = NULL;
    mta_functions_t* ptr = NULL;
    mta_functions_t* function = NULL;

    if (driver == NULL || functions_json == NULL) return;
    if (!cJSON_IsObject(functions_json)) return;

    ptr = driver->functions;

    cJSON_ArrayForEach(fn_json, functions_json) {
        if (fn_json->string == NULL) continue;

        function = (mta_functions_t*)malloc(sizeof(mta_functions_t) + strlen(fn_json->string) + 1);

        if (function == NULL) break;
        strcpy(function->function_name, fn_json->string);
        function->next = NULL;
        function->actions = NULL;

        mta_load_actions(&function->actions, fn_json);

        // Register:
        if (ptr == NULL) {
            driver->functions = function;
        } else {
            ptr->next = function;
        }

        ptr = function;
    }
}

void mta_load_variables(mta_plugin_t* driver, cJSON* variables_json) {
    cJSON* var_json = NULL;
    mta_variables_t* ptr = NULL;
    mta_variables_t* variable = NULL;

    if (driver == NULL || variables_json == NULL) return;
    if (!cJSON_IsObject(variables_json)) return;

    ptr = driver->variables;

    cJSON_ArrayForEach(var_json, variables_json) {
        if (var_json->string == NULL) continue;

        variable = (mta_variables_t*)malloc(sizeof(mta_variables_t) + strlen(var_json->string) + 1);

        if (variable == NULL) break;
        strcpy(variable->variable_name, var_json->string);
        variable->next = NULL;
        variable->value = var_json->valueint;

        // Register:
        if (ptr == NULL)
            driver->variables = variable;
        else
            ptr->next = variable;

        ptr = variable;
    }
}

void mta_load_events(mta_plugin_t* driver, cJSON* events_json) {
    cJSON* evt_json = NULL;
    mta_events_t* ptr = NULL;
    mta_events_t* event = NULL;

    if (driver == NULL || events_json == NULL) return;
    if (!cJSON_IsObject(events_json)) return;

    ptr = driver->events;

    cJSON_ArrayForEach(evt_json, events_json) {
        if (evt_json->string == NULL) continue;

        event = (mta_events_t*)malloc(sizeof(mta_events_t) + strlen(evt_json->string) + 1);

        if (event == NULL) break;
        strcpy(event->event_name, evt_json->string);
        event->next = NULL;
        event->actions = NULL;

        mta_load_actions(&event->actions, evt_json);

        // Register:
        if (ptr == NULL)
            driver->events = event;
        else
            ptr->next = event;

        ptr = event;
    }
}

void mta_load(mta_plugin_t** driver, cJSON* root) {
    const char* display_name_str;
    cJSON* display_name = cJSON_GetObjectItem(root, "displayName");

    if (display_name == NULL) {
        display_name_str = DEFAULT_DISPLAY_NAME;
    } else {
        display_name_str = display_name->valuestring;
    }

    *driver = (mta_plugin_t*)malloc(sizeof(mta_plugin_t) + strlen(display_name_str) + 1);

    if (driver == NULL) {
        return;
    }

    // Initialize Default Values
    (*driver)->functions = NULL;
    (*driver)->events = NULL;
    (*driver)->match = NULL;
    (*driver)->variables = NULL;
    (*driver)->config = NULL;
    strcpy((*driver)->display_name, display_name_str);

    // Populate Data
    cJSON* match = cJSON_GetObjectItem(root, "match");
    if (match != NULL) mta_load_match(*driver, match);

    cJSON* functions = cJSON_GetObjectItem(root, "functions");
    if (functions != NULL) mta_load_functions(*driver, functions);

    cJSON* variables = cJSON_GetObjectItem(root, "variables");
    if (variables != NULL) mta_load_variables(*driver, variables);

    cJSON* config = cJSON_DetachItemFromObject(root, "config");
    if (config != NULL) (*driver)->config = config;

    cJSON* events = cJSON_GetObjectItem(root, "events");
    if (events != NULL) mta_load_events(*driver, events);
}

mta_functions_t* mta_find_internal_function(mta_plugin_t* driver, const char* fn_name) {
    mta_functions_t* fn;
    for (fn = driver->functions; fn != NULL; fn = fn->next) {
        if (!strcmp(fn_name, fn->function_name)) {
            return fn;
        }
    }
    return NULL;
}

mta_functions_t* mta_find_system_function(mta_plugin_t* driver, const char* fn_name) {
    return NULL;
}

mta_functions_t* mta_find_driver_function(mta_plugin_t* driver, const char* fn_name) {
    return NULL;
}

void mta_function_call(mta_plugin_t* driver, const char* fn_name, cJSON* fn_args) {
    static int call_depth = 0;
    mta_functions_t* fn;

    // Todo: bail out if call_depth is too high

    if (fn_name[0] == '@') {
        fn = mta_find_internal_function(driver, fn_name);

        if (fn != NULL) {
            call_depth++;

            ESP_LOGI(TAG, "driver[%s]::%s()", driver->display_name, fn->function_name);

            mta_action_invoke(driver, fn->actions);

            call_depth--;
        }
    } else {
        // System function call:
        ESP_LOGI(TAG, "system::%s", fn_name);
    }
}

void mta_action_invoke(mta_plugin_t* driver, mta_actions_t* actions) {
    mta_actions_t* action;

    for (action = actions; action != NULL; action = action->next) {
        mta_function_call(driver, action->function, action->args);
    }
}

void mta_event_invoke(mta_plugin_t* driver, const char* event, int arg) {
    mta_events_t* evt;

    for (evt = driver->events; evt != NULL; evt = evt->next) {
        if (!strcmp(event, evt->event_name)) {
            ESP_LOGI(TAG, "MB Event Invocation: driver=%s, event=%s", driver->display_name, event);
            mta_action_invoke(driver, evt->actions);
        }
    }
}