#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// Traffic directions
typedef enum {NORTH, SOUTH, EAST, WEST} Direction;

// Light states
typedef enum {RED, GREEN, YELLOW} LightState;

// Traffic light structure
typedef struct {
    LightState state;
    int duration;
    Direction direction;
} TrafficLight;

// Vehicle sensor structure
typedef struct {
    bool vehicleDetected;
    Direction direction;
    time_t detectionTime;
} VehicleSensor;

// Traffic system configuration
typedef struct {
    TrafficLight lights[4];  // NORTH, SOUTH, EAST, WEST
    VehicleSensor sensors[4];
    int greenDuration;
    int yellowDuration;
    int minGreenTime;
    int maxGreenTime;
    bool emergencyMode;
} TrafficSystem;

// Initialize traffic system
void initializeSystem(TrafficSystem *system) {
    // Initialize lights
    for (int i = 0; i < 4; i++) {
        system->lights[i].state = RED;
        system->lights[i].duration = 0;
        system->lights[i].direction = i;
    }
    
    // Initialize sensors
    for (int i = 0; i < 4; i++) {
        system->sensors[i].vehicleDetected = false;
        system->sensors[i].direction = i;
        system->sensors[i].detectionTime = 0;
    }
    
    // Default timing configuration
    system->greenDuration = 30;
    system->yellowDuration = 5;
    system->minGreenTime = 10;
    system->maxGreenTime = 60;
    system->emergencyMode = false;
}

// Update sensor data
void updateSensors(TrafficSystem *system, Direction dir, bool detected) {
    system->sensors[dir].vehicleDetected = detected;
    if (detected) {
        system->sensors[dir].detectionTime = time(NULL);
    }
}

// Calculate traffic density (simple implementation)
int calculateDensity(TrafficSystem *system, Direction primary, Direction secondary) {
    int density = 0;
    if (system->sensors[primary].vehicleDetected) density += 3;
    if (system->sensors[secondary].vehicleDetected) density += 3;
    return density;
}

// Adjust timing based on traffic density
void adjustTiming(TrafficSystem *system) {
    int nsDensity = calculateDensity(system, NORTH, SOUTH);
    int ewDensity = calculateDensity(system, EAST, WEST);
    
    // Dynamic timing adjustment
    int baseTime = system->greenDuration;
    
    if (nsDensity > ewDensity + 4) {
        system->lights[NORTH].duration = baseTime + 15;
        system->lights[SOUTH].duration = baseTime + 15;
        system->lights[EAST].duration = baseTime - 10;
        system->lights[WEST].duration = baseTime - 10;
    } else if (ewDensity > nsDensity + 4) {
        system->lights[EAST].duration = baseTime + 15;
        system->lights[WEST].duration = baseTime + 15;
        system->lights[NORTH].duration = baseTime - 10;
        system->lights[SOUTH].duration = baseTime - 10;
    } else {
        for (int i = 0; i < 4; i++) {
            system->lights[i].duration = baseTime;
        }
    }
    
    // Ensure within min/max limits
    for (int i = 0; i < 4; i++) {
        if (system->lights[i].duration < system->minGreenTime)
            system->lights[i].duration = system->minGreenTime;
        if (system->lights[i].duration > system->maxGreenTime)
            system->lights[i].duration = system->maxGreenTime;
    }
}

// Control light transitions
void controlLights(TrafficSystem *system) {
    static time_t lastChange = 0;
    time_t currentTime = time(NULL);
    int elapsed = currentTime - lastChange;
    
    // Find active green light
    int activeGreen = -1;
    for (int i = 0; i < 4; i++) {
        if (system->lights[i].state == GREEN) {
            activeGreen = i;
            break;
        }
    }
    
    // Emergency mode override
    if (system->emergencyMode) {
        for (int i = 0; i < 4; i++) {
            system->lights[i].state = RED;
        }
        system->lights[NORTH].state = GREEN;
        system->lights[SOUTH].state = GREEN;
        printf("EMERGENCY MODE: All directions RED except North/South GREEN\n");
        return;
    }
    
    // Check if need to change lights
    if (activeGreen == -1 || elapsed >= system->lights[activeGreen].duration) {
        // Switch to yellow before changing
        if (activeGreen != -1 && system->lights[activeGreen].state == GREEN) {
            system->lights[activeGreen].state = YELLOW;
            system->lights[activeGreen == NORTH ? SOUTH : activeGreen].state = YELLOW;
            lastChange = currentTime;
            return;
        }
        
        // Switch to next green light
        if (activeGreen == NORTH || activeGreen == SOUTH) {
            // Switch to EAST-WEST
            system->lights[EAST].state = GREEN;
            system->lights[WEST].state = GREEN;
            system->lights[NORTH].state = RED;
            system->lights[SOUTH].state = RED;
        } else {
            // Switch to NORTH-SOUTH
            system->lights[NORTH].state = GREEN;
            system->lights[SOUTH].state = GREEN;
            system->lights[EAST].state = RED;
            system->lights[WEST].state = RED;
        }
        
        lastChange = currentTime;
        adjustTiming(system);
    }
}

// Display current traffic light status
void displayStatus(TrafficSystem *system) {
    printf("\n--- Traffic Light Status ---\n");
    printf("North: %s\tSouth: %s\n", 
           system->lights[NORTH].state == GREEN ? "GREEN" : 
           system->lights[NORTH].state == YELLOW ? "YELLOW" : "RED",
           system->lights[SOUTH].state == GREEN ? "GREEN" : 
           system->lights[SOUTH].state == YELLOW ? "YELLOW" : "RED");
    
    printf("East:  %s\tWest:  %s\n", 
           system->lights[EAST].state == GREEN ? "GREEN" : 
           system->lights[EAST].state == YELLOW ? "YELLOW" : "RED",
           system->lights[WEST].state == GREEN ? "GREEN" : 
           system->lights[WEST].state == YELLOW ? "YELLOW" : "RED");
    
    printf("Timing: N/S:%ds, E/W:%ds\n", 
           system->lights[NORTH].duration,
           system->lights[EAST].duration);
    printf("Emergency Mode: %s\n", system->emergencyMode ? "ON" : "OFF");
    printf("---------------------------\n");
}

// Sensor monitoring thread
void* sensorThread(void* arg) {
    TrafficSystem *system = (TrafficSystem*)arg;
    while (1) {
        // Simulate random vehicle detection
        for (int i = 0; i < 4; i++) {
            bool detected = rand() % 5 == 0;  // 20% probability
            updateSensors(system, i, detected);
        }
        sleep(1);
    }
    return NULL;
}

// Main control loop
int main() {
    TrafficSystem system;
    initializeSystem(&system);
    
    // Create sensor thread
    pthread_t sensor_tid;
    pthread_create(&sensor_tid, NULL, sensorThread, &system);
    
    printf("Smart Traffic Lighting System\n");
    printf("Initializing...\n");
    
    // Main control loop
    while (1) {
        controlLights(&system);
        displayStatus(&system);
        
        // Simulate emergency mode activation
        if (rand() % 20 == 0) {  // 5% probability
            system.emergencyMode = true;
            printf("\n!!! EMERGENCY VEHICLE DETECTED !!!\n");
            sleep(5);  // Emergency mode duration
            system.emergencyMode = false;
        }
        
        sleep(1);  // Control cycle
    }
    
    return 0;
}