#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <open62541.h>

#define NUM_LIGHTS 6
#define HISTORY_LENGTH 600
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 600
#define LIGHT_SIZE 50
#define LIGHT_SPACING 10
#define LIGHT_OFFSET 70

typedef struct {
    bool red;
    bool yellow;
    bool green;
    const char *red_node;
    const char *yellow_node;
    const char *green_node;
    const UA_NodeId *red_id;
    const UA_NodeId *yellow_id;
    const UA_NodeId *green_id;
} TrafficLight;

typedef struct {
    int state_history[HISTORY_LENGTH];
    int history_index;
} TrafficLightHistory;

TrafficLight lights[NUM_LIGHTS];
TrafficLightHistory histories[NUM_LIGHTS];

SDL_Color getColor(const char *state) {
    if (strcmp(state, "red") == 0) return (SDL_Color){255, 0, 0, 255};
    if (strcmp(state, "yellow") == 0) return (SDL_Color){255, 255, 0, 255};
    if (strcmp(state, "green") == 0) return (SDL_Color){0, 255, 0, 255};
    if (strcmp(state, "red-yellow") == 0) return (SDL_Color){255, 165, 0, 255};
    return (SDL_Color){128, 128, 128, 255};
}

void drawCircle(SDL_Renderer *renderer, int x, int y, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

void drawTrafficLight(SDL_Renderer *renderer, int index, TrafficLight *light) {
    int x = index * (LIGHT_SIZE + LIGHT_SPACING) + LIGHT_SPACING;
    int y = LIGHT_OFFSET;

    // Red
    drawCircle(renderer, x, y, LIGHT_SIZE / 2, getColor(light->red ? "red" : "off"));
    // Yellow
    drawCircle(renderer, x, y + LIGHT_SIZE, LIGHT_SIZE / 2, getColor(light->yellow ? "yellow" : "off"));
    // Green
    drawCircle(renderer, x, y + LIGHT_SIZE * 2, LIGHT_SIZE / 2, getColor(light->green ? "green" : "off"));
}

void drawHistory(SDL_Renderer *renderer, int index, TrafficLightHistory *history) {
    int x_offset = index * (WINDOW_WIDTH / NUM_LIGHTS);
    int bar_width = WINDOW_WIDTH / NUM_LIGHTS / HISTORY_LENGTH;

    for (int i = 0; i < HISTORY_LENGTH; i++) {
        int x = x_offset + i * bar_width;
        int y = WINDOW_HEIGHT - 50;
        int state = history->state_history[(history->history_index + i) % HISTORY_LENGTH];
        SDL_Color color = getColor(state == 1 ? "red" : state == 2 ? "yellow" : state == 3 ? "green" : "off");
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &(SDL_Rect){x, y, bar_width, 30});
    }
}

void updateState(UA_Client *client, TrafficLight *light, TrafficLightHistory *history) {
    UA_Variant value;
    UA_Client_readValueAttribute(client, *light->red_id, &value);
    light->red = *(UA_Boolean *)value.data;

    UA_Client_readValueAttribute(client, *light->yellow_id, &value);
    light->yellow = *(UA_Boolean *)value.data;

    UA_Client_readValueAttribute(client, *light->green_id, &value);
    light->green = *(UA_Boolean *)value.data;

    int state = light->red ? 1 : light->yellow ? 2 : light->green ? 3 : 0;
    history->state_history[history->history_index] = state;
    history->history_index = (history->history_index + 1) % HISTORY_LENGTH;
}

int main(int argc, char **argv) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://192.168.1.101:4840");
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    // Initialize lights and histories
    for (int i = 0; i < NUM_LIGHTS; i++) {
        lights[i].red_node = "red_node_id_here";
        lights[i].yellow_node = "yellow_node_id_here";
        lights[i].green_node = "green_node_id_here";
        lights[i].red = false;
        lights[i].yellow = false;
        lights[i].green = false;
        histories[i].history_index = 0;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Traffic Light HMI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < NUM_LIGHTS; i++) {
            updateState(client, &lights[i], &histories[i]);
            drawTrafficLight(renderer, i, &lights[i]);
            drawHistory(renderer, i, &histories[i]);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    UA_Client_delete(client);

    return EXIT_SUCCESS;
}