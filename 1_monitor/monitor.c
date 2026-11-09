/*
 * Smart Building Environmental Monitoring System
 *
 * Simulates a network of environmental sensors (temperature, humidity,
 * pressure, CO2, and light) spread across a building, generates
 * simulated readings over several rounds, computes statistics for each
 * sensor, flags out-of-threshold readings as alerts, ranks sensors by
 * how many alerts they produced, and prints a full building-wide
 * dashboard report.
 *
 * TABLE OF CONTENTS (all in this one file -- for now):
 *   1. Constants and magic numbers
 *   2. Enums: SensorType, SensorStatus
 *   3. Structs: Reading, Sensor
 *   4. Sensor lifecycle / data generation
 *      initSensor, simulateReadingValue, generateReadingsForSensor,
 *      resetSensorHistory
 *   5. Statistics
 *      computeMin, computeMax, computeAverage, computeStdDev,
 *      computeSensorStats
 *   6. Alerting / status classification
 *      isAlertReading, countAlerts, classifySensorStatus,
 *      statusToString, sensorTypeToString
 *   7. Reporting / logging
 *      printSeparator, logEvent, printSensorHeader, printSensorHistory,
 *      printSensorReport
 *   8. Network-wide aggregation and comparison
 *      computeNetworkAverageByType, findWorstSensor,
 *      compareSensorsByAlertCount, swapSensors, sortSensorsByAlertCount,
 *      printNetworkSummary, computeBuildingWideAlertRate,
 *      printFinalDashboard
 *   9. Simulation driver
 *      setupBuilding, runSimulationRounds
 *  10. Cross-sensor correlation
 *      computeCorrelation, printTemperatureHumidityCorrelation
 *  11. Lookup utilities
 *      findSensorByLabel, printSensorByLabel
 *  12. Capacity reporting
 *      computeCapacityPercentage, printCapacityReport
 *  13. Maintenance recommendations
 *      recommendMaintenance, printMaintenanceRecommendations
 *  14. Side-by-side comparison
 *      printComparisonReport
 *  15. CSV-style export
 *      exportSummaryLine, exportAllSensorsCSV
 *  16. Trend direction, floor grouping, threshold validation, alert log
 *      computeTrendDirection, printTrendReport, extractFloorNumber,
 *      printFloorBreakdown, validateThresholds,
 *      printThresholdValidationReport, printAlertLog,
 *      printBuildingAlertLog
 *  17. main
 *
 * NOTE: this file is intentionally a single unstructured
 * file with no header separation, no Makefile, and no
 * compile-time configuration -- that's exactly what this assignment asks you to fix.
 * As you skim this file, use the table of contents above to sketch which 
 * functions belong together as a module
 * -- does it match the section breaks here, or would you group things differently?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---------------------------------------------------------------------
 * Constants (all magic numbers -- one of the things we'll fix later)
 * ------------------------------------------------------------------- */
#define MAX_HISTORY 60
#define MAX_SENSORS 15
#define SIM_ROUNDS 3
#define READINGS_PER_ROUND 12
#define RANDOM_SEED 42

/* ---------------------------------------------------------------------
 * Enums
 * ------------------------------------------------------------------- */
typedef enum {
    SENSOR_TEMPERATURE,
    SENSOR_HUMIDITY,
    SENSOR_PRESSURE,
    SENSOR_CO2,
    SENSOR_LIGHT
} SensorType;

typedef enum {
    STATUS_OK,
    STATUS_WARNING,
    STATUS_CRITICAL
} SensorStatus;

/* ---------------------------------------------------------------------
 * Structs
 * ------------------------------------------------------------------- */
typedef struct {
    double value;
    int timestampSec;
} Reading;

typedef struct {
    char label[24];
    SensorType type;
    double minThreshold;
    double maxThreshold;
    Reading history[MAX_HISTORY];
    int historyCount;
} Sensor;

/* ---------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------- */
void initSensor(Sensor *s, const char *label, SensorType type,
                 double minThreshold, double maxThreshold);
double simulateReadingValue(SensorType type, int step);
void generateReadingsForSensor(Sensor *s, int count, int startTimestamp);
void resetSensorHistory(Sensor *s);

double computeMin(const Sensor *s);
double computeMax(const Sensor *s);
double computeAverage(const Sensor *s);
double computeStdDev(const Sensor *s, double average);
void computeSensorStats(const Sensor *s, double *minOut, double *maxOut,
                         double *avgOut, double *stdDevOut);

int isAlertReading(const Sensor *s, double value);
int countAlerts(const Sensor *s);
SensorStatus classifySensorStatus(const Sensor *s);
const char *statusToString(SensorStatus status);
const char *sensorTypeToString(SensorType type);

void printSeparator(void);
void logEvent(const char *message);
void printSensorHeader(const Sensor *s);
void printSensorHistory(const Sensor *s);
void printSensorReport(const Sensor *s);

double computeNetworkAverageByType(const Sensor sensors[], int n, SensorType type);
int findWorstSensor(const Sensor sensors[], int n);
int compareSensorsByAlertCount(const Sensor *a, const Sensor *b);
void swapSensors(Sensor *a, Sensor *b);
void sortSensorsByAlertCount(Sensor sensors[], int n);
void printNetworkSummary(const Sensor sensors[], int n);
double computeBuildingWideAlertRate(const Sensor sensors[], int n);
void printFinalDashboard(const Sensor sensors[], int n);

void setupBuilding(Sensor sensors[], int *countOut);
void runSimulationRounds(Sensor sensors[], int n, int rounds, int readingsPerRound);

double computeCorrelation(const Sensor *a, const Sensor *b);
void printTemperatureHumidityCorrelation(const Sensor sensors[], int n);
int findSensorByLabel(const Sensor sensors[], int n, const char *label);
void printSensorByLabel(const Sensor sensors[], int n, const char *label);
double computeCapacityPercentage(const Sensor *s);
void printCapacityReport(const Sensor sensors[], int n);
const char *recommendMaintenance(const Sensor *s);
void printMaintenanceRecommendations(const Sensor sensors[], int n);
void printComparisonReport(const Sensor *a, const Sensor *b);
void exportSummaryLine(const Sensor *s);
void exportAllSensorsCSV(const Sensor sensors[], int n);

const char *computeTrendDirection(const Sensor *s);
void printTrendReport(const Sensor sensors[], int n);
int extractFloorNumber(const Sensor *s);
void printFloorBreakdown(const Sensor sensors[], int n);
int validateThresholds(const Sensor *s);
void printThresholdValidationReport(const Sensor sensors[], int n);
void printAlertLog(const Sensor *s);
void printBuildingAlertLog(const Sensor sensors[], int n);

/* ---------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------- */

int main(void) {
    Sensor building[MAX_SENSORS];
    int sensorCount;
    int i;

    srand(RANDOM_SEED);

    setupBuilding(building, &sensorCount);
    runSimulationRounds(building, sensorCount, SIM_ROUNDS, READINGS_PER_ROUND);

    printf("\n=== PER-SENSOR REPORTS ===\n");
    printSeparator();
    for (i = 0; i < sensorCount; i++) {
        printSensorReport(&building[i]);
        printSeparator();
    }

    printf("\n=== NETWORK SUMMARY ===\n");
    printNetworkSummary(building, sensorCount);

    printf("\n=== CROSS-SENSOR CORRELATION ===\n");
    printTemperatureHumidityCorrelation(building, sensorCount);
    printSeparator();

    printf("\n=== TREND REPORT ===\n");
    printTrendReport(building, sensorCount);

    printf("\n=== FLOOR BREAKDOWN ===\n");
    printFloorBreakdown(building, sensorCount);

    printf("\n=== THRESHOLD VALIDATION ===\n");
    printThresholdValidationReport(building, sensorCount);

    printf("\n=== ALERT LOG ===\n");
    printBuildingAlertLog(building, sensorCount);

    printf("\n=== LOOKUP DEMO ===\n");
    printSensorByLabel(building, sensorCount, "CO2-F1-101");

    printf("\n=== CAPACITY REPORT ===\n");
    printCapacityReport(building, sensorCount);

    printf("\n=== MAINTENANCE RECOMMENDATIONS ===\n");
    printMaintenanceRecommendations(building, sensorCount);

    printf("\n=== SIDE-BY-SIDE COMPARISON ===\n");
    printComparisonReport(&building[0], &building[2]);

    sortSensorsByAlertCount(building, sensorCount);
    printf("\n=== SENSORS RANKED BY ALERT COUNT (WORST FIRST) ===\n");
    for (i = 0; i < sensorCount; i++) {
        printf("  %2d. %-20s %d alerts (%s)\n", i + 1, building[i].label,
               countAlerts(&building[i]), statusToString(classifySensorStatus(&building[i])));
    }
    printSeparator();

    printf("\n=== CSV EXPORT ===\n");
    exportAllSensorsCSV(building, sensorCount);

    printf("\n");
    printFinalDashboard(building, sensorCount);

    return 0;
}

/* ---------------------------------------------------------------------
 * Sensor lifecycle / data generation
 * ------------------------------------------------------------------- */

void initSensor(Sensor *s, const char *label, SensorType type,
                 double minThreshold, double maxThreshold) {
    strncpy(s->label, label, sizeof(s->label) - 1);
    s->label[sizeof(s->label) - 1] = '\0';
    s->type = type;
    s->minThreshold = minThreshold;
    s->maxThreshold = maxThreshold;
    s->historyCount = 0;
}

/*
 * Produces a plausible simulated raw reading for a given sensor type at
 * a given simulation step. Each sensor type follows a different pattern
 * (a slow oscillation plus a bit of pseudo-random noise) so the five
 * sensor types behave visibly differently in the generated data.
 */
double simulateReadingValue(SensorType type, int step) {
    double noise = (rand() % 100) / 100.0;   /* 0.00 - 0.99 */
    double wave;

    switch (type) {
        case SENSOR_TEMPERATURE:
            wave = sin(step * 0.3) * 4.0;
            return 21.0 + wave + noise;
        case SENSOR_HUMIDITY:
            wave = cos(step * 0.2) * 8.0;
            return 45.0 + wave + noise * 3.0;
        case SENSOR_PRESSURE:
            wave = sin(step * 0.1) * 3.0;
            return 1012.0 + wave + noise;
        case SENSOR_CO2:
            wave = (step % 10) * 15.0;
            return 400.0 + wave + noise * 20.0;
        case SENSOR_LIGHT:
            wave = fabs(sin(step * 0.25)) * 300.0;
            return 50.0 + wave + noise * 10.0;
        default:
            return 0.0;
    }
}

void generateReadingsForSensor(Sensor *s, int count, int startTimestamp) {
    int i;
    for (i = 0; i < count; i++) {
        if (s->historyCount >= MAX_HISTORY) {
            break;   /* history buffer is full, stop generating */
        }
        s->history[s->historyCount].value = simulateReadingValue(s->type, s->historyCount);
        s->history[s->historyCount].timestampSec = startTimestamp + i * 5;
        s->historyCount++;
    }
}

void resetSensorHistory(Sensor *s) {
    s->historyCount = 0;
}

/* ---------------------------------------------------------------------
 * Statistics
 * ------------------------------------------------------------------- */

double computeMin(const Sensor *s) {
    double minVal;
    int i;
    if (s->historyCount == 0) {
        return 0.0;
    }
    minVal = s->history[0].value;
    for (i = 1; i < s->historyCount; i++) {
        if (s->history[i].value < minVal) {
            minVal = s->history[i].value;
        }
    }
    return minVal;
}

double computeMax(const Sensor *s) {
    double maxVal;
    int i;
    if (s->historyCount == 0) {
        return 0.0;
    }
    maxVal = s->history[0].value;
    for (i = 1; i < s->historyCount; i++) {
        if (s->history[i].value > maxVal) {
            maxVal = s->history[i].value;
        }
    }
    return maxVal;
}

double computeAverage(const Sensor *s) {
    double sum = 0.0;
    int i;
    if (s->historyCount == 0) {
        return 0.0;
    }
    for (i = 0; i < s->historyCount; i++) {
        sum += s->history[i].value;
    }
    return sum / s->historyCount;
}

double computeStdDev(const Sensor *s, double average) {
    double sumSquaredDiff = 0.0;
    int i;
    if (s->historyCount == 0) {
        return 0.0;
    }
    for (i = 0; i < s->historyCount; i++) {
        double diff = s->history[i].value - average;
        sumSquaredDiff += diff * diff;
    }
    return sqrt(sumSquaredDiff / s->historyCount);
}

void computeSensorStats(const Sensor *s, double *minOut, double *maxOut,
                         double *avgOut, double *stdDevOut) {
    *minOut = computeMin(s);
    *maxOut = computeMax(s);
    *avgOut = computeAverage(s);
    *stdDevOut = computeStdDev(s, *avgOut);
}

/* ---------------------------------------------------------------------
 * Alerting / status classification
 * ------------------------------------------------------------------- */

int isAlertReading(const Sensor *s, double value) {
    if (value < s->minThreshold || value > s->maxThreshold) {
        return 1;
    }
    return 0;
}

int countAlerts(const Sensor *s) {
    int i;
    int count = 0;
    for (i = 0; i < s->historyCount; i++) {
        if (isAlertReading(s, s->history[i].value)) {
            count++;
        }
    }
    return count;
}

/*
 * A sensor is CRITICAL if more than a third of its readings are alerts,
 * WARNING if it has any alerts at all, and OK otherwise.
 */
SensorStatus classifySensorStatus(const Sensor *s) {
    int alerts = countAlerts(s);
    if (s->historyCount == 0) {
        return STATUS_OK;
    }
    if (alerts > s->historyCount / 3) {
        return STATUS_CRITICAL;
    }
    if (alerts > 0) {
        return STATUS_WARNING;
    }
    return STATUS_OK;
}

const char *statusToString(SensorStatus status) {
    switch (status) {
        case STATUS_OK:       return "OK";
        case STATUS_WARNING:  return "WARNING";
        case STATUS_CRITICAL: return "CRITICAL";
        default:               return "UNKNOWN";
    }
}

const char *sensorTypeToString(SensorType type) {
    switch (type) {
        case SENSOR_TEMPERATURE: return "Temperature";
        case SENSOR_HUMIDITY:    return "Humidity";
        case SENSOR_PRESSURE:    return "Pressure";
        case SENSOR_CO2:         return "CO2";
        case SENSOR_LIGHT:       return "Light";
        default:                  return "Unknown";
    }
}

/* ---------------------------------------------------------------------
 * Reporting / logging
 * ------------------------------------------------------------------- */

void printSeparator(void) {
    int i;
    for (i = 0; i < 60; i++) {
        printf("-");
    }
    printf("\n");
}

/*
 * A simple event logger. Right now this always prints -- later in the
 * activity we'll gate this behind a DEBUG macro so it can be silenced
 * entirely at compile time.
 */
void logEvent(const char *message) {
    printf("[LOG] %s\n", message);
}

void printSensorHeader(const Sensor *s) {
    printf("Sensor: %-20s Type: %-12s Range: [%.1f, %.1f]\n",
           s->label, sensorTypeToString(s->type), s->minThreshold, s->maxThreshold);
}

void printSensorHistory(const Sensor *s) {
    int i;
    printf("  History (%d readings): ", s->historyCount);
    for (i = 0; i < s->historyCount; i++) {
        printf("%.1f ", s->history[i].value);
    }
    printf("\n");
}

void printSensorReport(const Sensor *s) {
    double minVal, maxVal, avgVal, stdDev;
    int alerts;
    SensorStatus status;

    computeSensorStats(s, &minVal, &maxVal, &avgVal, &stdDev);
    alerts = countAlerts(s);
    status = classifySensorStatus(s);

    printSensorHeader(s);
    printSensorHistory(s);
    printf("  Min: %.2f  Max: %.2f  Avg: %.2f  StdDev: %.2f\n",
           minVal, maxVal, avgVal, stdDev);
    printf("  Alerts: %d/%d  Status: %s\n", alerts, s->historyCount, statusToString(status));
}

/* ---------------------------------------------------------------------
 * Network-wide aggregation and comparison
 * ------------------------------------------------------------------- */

double computeNetworkAverageByType(const Sensor sensors[], int n, SensorType type) {
    double sum = 0.0;
    int matchCount = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (sensors[i].type == type) {
            sum += computeAverage(&sensors[i]);
            matchCount++;
        }
    }
    if (matchCount == 0) {
        return 0.0;
    }
    return sum / matchCount;
}

int findWorstSensor(const Sensor sensors[], int n) {
    int worstIndex = 0;
    int worstAlerts = countAlerts(&sensors[0]);
    int i;
    for (i = 1; i < n; i++) {
        int alerts = countAlerts(&sensors[i]);
        if (alerts > worstAlerts) {
            worstAlerts = alerts;
            worstIndex = i;
        }
    }
    return worstIndex;
}

/*
 * Comparator in the spirit of Java's Comparable.compareTo(): negative
 * if a should sort before b, positive if a should sort after b, zero
 * if they're equivalent. Here we rank by alert count, descending
 * (worst offenders first).
 */
int compareSensorsByAlertCount(const Sensor *a, const Sensor *b) {
    int alertsA = countAlerts(a);
    int alertsB = countAlerts(b);
    return alertsB - alertsA;
}

void swapSensors(Sensor *a, Sensor *b) {
    Sensor temp = *a;
    *a = *b;
    *b = temp;
}

/* A simple selection sort -- fine for MAX_SENSORS-sized arrays. */
void sortSensorsByAlertCount(Sensor sensors[], int n) {
    int i, j;
    int bestIndex;
    for (i = 0; i < n - 1; i++) {
        bestIndex = i;
        for (j = i + 1; j < n; j++) {
            if (compareSensorsByAlertCount(&sensors[j], &sensors[bestIndex]) < 0) {
                bestIndex = j;
            }
        }
        if (bestIndex != i) {
            swapSensors(&sensors[i], &sensors[bestIndex]);
        }
    }
}

void printNetworkSummary(const Sensor sensors[], int n) {
    SensorType types[5] = {SENSOR_TEMPERATURE, SENSOR_HUMIDITY, SENSOR_PRESSURE,
                            SENSOR_CO2, SENSOR_LIGHT};
    int i;
    int worstIndex;

    printf("Network Averages By Type:\n");
    for (i = 0; i < 5; i++) {
        double avg = computeNetworkAverageByType(sensors, n, types[i]);
        printf("  %-12s average: %.2f\n", sensorTypeToString(types[i]), avg);
    }

    worstIndex = findWorstSensor(sensors, n);
    printf("\nWorst-performing sensor: %s (%d alerts)\n",
           sensors[worstIndex].label, countAlerts(&sensors[worstIndex]));
    printSeparator();
}

double computeBuildingWideAlertRate(const Sensor sensors[], int n) {
    int totalReadings = 0;
    int totalAlerts = 0;
    int i;
    for (i = 0; i < n; i++) {
        totalReadings += sensors[i].historyCount;
        totalAlerts += countAlerts(&sensors[i]);
    }
    if (totalReadings == 0) {
        return 0.0;
    }
    return (double) totalAlerts / totalReadings * 100.0;
}

void printFinalDashboard(const Sensor sensors[], int n) {
    int i;
    int okCount = 0, warningCount = 0, criticalCount = 0;

    for (i = 0; i < n; i++) {
        SensorStatus status = classifySensorStatus(&sensors[i]);
        if (status == STATUS_OK) {
            okCount++;
        } else if (status == STATUS_WARNING) {
            warningCount++;
        } else {
            criticalCount++;
        }
    }

    printf("=== FINAL BUILDING DASHBOARD ===\n");
    printf("Total sensors: %d\n", n);
    printf("  OK:       %d\n", okCount);
    printf("  WARNING:  %d\n", warningCount);
    printf("  CRITICAL: %d\n", criticalCount);
    printf("Building-wide alert rate: %.2f%%\n", computeBuildingWideAlertRate(sensors, n));
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Simulation driver
 * ------------------------------------------------------------------- */

void setupBuilding(Sensor sensors[], int *countOut) {
    int i = 0;

    initSensor(&sensors[i++], "TEMP-F1-101", SENSOR_TEMPERATURE, 18.0, 26.0);
    initSensor(&sensors[i++], "TEMP-F1-102", SENSOR_TEMPERATURE, 18.0, 26.0);
    initSensor(&sensors[i++], "TEMP-F2-201", SENSOR_TEMPERATURE, 19.0, 25.0);
    initSensor(&sensors[i++], "HUM-F1-101",  SENSOR_HUMIDITY,    30.0, 55.0);
    initSensor(&sensors[i++], "HUM-F1-102",  SENSOR_HUMIDITY,    30.0, 55.0);
    initSensor(&sensors[i++], "HUM-F2-201",  SENSOR_HUMIDITY,    35.0, 60.0);
    initSensor(&sensors[i++], "PRES-F1-LOBBY", SENSOR_PRESSURE,  1005.0, 1020.0);
    initSensor(&sensors[i++], "PRES-F2-LOBBY", SENSOR_PRESSURE,  1005.0, 1020.0);
    initSensor(&sensors[i++], "CO2-F1-101",  SENSOR_CO2,         400.0, 800.0);
    initSensor(&sensors[i++], "CO2-F1-102",  SENSOR_CO2,         400.0, 800.0);
    initSensor(&sensors[i++], "CO2-F2-201",  SENSOR_CO2,         400.0, 800.0);
    initSensor(&sensors[i++], "LIGHT-F1-101", SENSOR_LIGHT,      100.0, 400.0);
    initSensor(&sensors[i++], "LIGHT-F1-102", SENSOR_LIGHT,      100.0, 400.0);
    initSensor(&sensors[i++], "LIGHT-F2-201", SENSOR_LIGHT,      100.0, 400.0);
    initSensor(&sensors[i++], "LIGHT-F2-202", SENSOR_LIGHT,      100.0, 400.0);

    *countOut = i;
}

/*
 * Runs the simulation for `rounds` rounds, generating `readingsPerRound`
 * new readings for every sensor each round, and logging progress as it
 * goes.
 */
void runSimulationRounds(Sensor sensors[], int n, int rounds, int readingsPerRound) {
    int round;
    int i;
    char msg[64];

    for (round = 0; round < rounds; round++) {
        snprintf(msg, sizeof(msg), "Starting simulation round %d of %d", round + 1, rounds);
        logEvent(msg);
        for (i = 0; i < n; i++) {
            generateReadingsForSensor(&sensors[i], readingsPerRound, round * 60);
        }
    }
    logEvent("Simulation complete.");
}

/* ---------------------------------------------------------------------
 * Cross-sensor correlation
 * ------------------------------------------------------------------- */

/*
 * Pearson correlation coefficient between two sensors' reading
 * histories, compared index-for-index (both sensors must have at least
 * as many readings as the shorter of the two; only the overlapping
 * portion is used). Returns a value from -1.0 (perfectly inverse) to
 * 1.0 (perfectly correlated), or 0.0 if there isn't enough data.
 */
double computeCorrelation(const Sensor *a, const Sensor *b) {
    int n = a->historyCount < b->historyCount ? a->historyCount : b->historyCount;
    double avgA = computeAverage(a);
    double avgB = computeAverage(b);
    double numerator = 0.0;
    double sumSqA = 0.0;
    double sumSqB = 0.0;
    int i;

    if (n < 2) {
        return 0.0;
    }

    for (i = 0; i < n; i++) {
        double diffA = a->history[i].value - avgA;
        double diffB = b->history[i].value - avgB;
        numerator += diffA * diffB;
        sumSqA += diffA * diffA;
        sumSqB += diffB * diffB;
    }

    if (sumSqA <= 0.0 || sumSqB <= 0.0) {
        return 0.0;
    }

    return numerator / sqrt(sumSqA * sumSqB);
}

/*
 * A common real-world question for building monitoring: does humidity
 * track with temperature? This finds the first temperature sensor and
 * first humidity sensor in the array and reports their correlation.
 */
void printTemperatureHumidityCorrelation(const Sensor sensors[], int n) {
    int tempIndex = -1;
    int humIndex = -1;
    int i;

    for (i = 0; i < n; i++) {
        if (tempIndex == -1 && sensors[i].type == SENSOR_TEMPERATURE) {
            tempIndex = i;
        }
        if (humIndex == -1 && sensors[i].type == SENSOR_HUMIDITY) {
            humIndex = i;
        }
    }

    if (tempIndex == -1 || humIndex == -1) {
        printf("Cannot compute temperature/humidity correlation: missing a sensor of one type.\n");
        return;
    }

    printf("Correlation between %s and %s: %.3f\n",
           sensors[tempIndex].label, sensors[humIndex].label,
           computeCorrelation(&sensors[tempIndex], &sensors[humIndex]));
}

/* ---------------------------------------------------------------------
 * Lookup utilities
 * ------------------------------------------------------------------- */

int findSensorByLabel(const Sensor sensors[], int n, const char *label) {
    int i;
    for (i = 0; i < n; i++) {
        if (strcmp(sensors[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}

void printSensorByLabel(const Sensor sensors[], int n, const char *label) {
    int index = findSensorByLabel(sensors, n, label);
    if (index == -1) {
        printf("No sensor found with label '%s'.\n", label);
        return;
    }
    printSensorReport(&sensors[index]);
}

/* ---------------------------------------------------------------------
 * Capacity reporting
 * ------------------------------------------------------------------- */

double computeCapacityPercentage(const Sensor *s) {
    return (double) s->historyCount / MAX_HISTORY * 100.0;
}

void printCapacityReport(const Sensor sensors[], int n) {
    int i;
    printf("Sensor History Buffer Usage:\n");
    for (i = 0; i < n; i++) {
        printf("  %-20s %d/%d (%.1f%%)\n", sensors[i].label, sensors[i].historyCount,
               MAX_HISTORY, computeCapacityPercentage(&sensors[i]));
    }
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Maintenance recommendations
 * ------------------------------------------------------------------- */

const char *recommendMaintenance(const Sensor *s) {
    SensorStatus status = classifySensorStatus(s);
    double avg = computeAverage(s);
    double stdDev = computeStdDev(s, avg);

    if (status == STATUS_CRITICAL) {
        return "Immediate inspection recommended";
    }
    if (status == STATUS_WARNING && stdDev > 5.0) {
        return "Monitor closely; readings are volatile";
    }
    if (status == STATUS_WARNING) {
        return "Schedule routine check";
    }
    return "No action needed";
}

void printMaintenanceRecommendations(const Sensor sensors[], int n) {
    int i;
    printf("Maintenance Recommendations:\n");
    for (i = 0; i < n; i++) {
        printf("  %-20s %s\n", sensors[i].label, recommendMaintenance(&sensors[i]));
    }
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Side-by-side comparison
 * ------------------------------------------------------------------- */

void printComparisonReport(const Sensor *a, const Sensor *b) {
    double minA, maxA, avgA, stdDevA;
    double minB, maxB, avgB, stdDevB;

    computeSensorStats(a, &minA, &maxA, &avgA, &stdDevA);
    computeSensorStats(b, &minB, &maxB, &avgB, &stdDevB);

    printf("Comparing %s vs %s:\n", a->label, b->label);
    printf("  %-10s %10s %10s\n", "Metric", a->label, b->label);
    printf("  %-10s %10.2f %10.2f\n", "Min", minA, minB);
    printf("  %-10s %10.2f %10.2f\n", "Max", maxA, maxB);
    printf("  %-10s %10.2f %10.2f\n", "Avg", avgA, avgB);
    printf("  %-10s %10.2f %10.2f\n", "StdDev", stdDevA, stdDevB);
    printf("  %-10s %10d %10d\n", "Alerts", countAlerts(a), countAlerts(b));
    printSeparator();
}

/* ---------------------------------------------------------------------
 * CSV-style export (printed to stdout -- no file I/O in this program)
 * ------------------------------------------------------------------- */

void exportSummaryLine(const Sensor *s) {
    double minVal, maxVal, avgVal, stdDev;
    computeSensorStats(s, &minVal, &maxVal, &avgVal, &stdDev);
    printf("%s,%s,%.2f,%.2f,%.2f,%.2f,%d,%s\n",
           s->label, sensorTypeToString(s->type), minVal, maxVal, avgVal, stdDev,
           countAlerts(s), statusToString(classifySensorStatus(s)));
}

void exportAllSensorsCSV(const Sensor sensors[], int n) {
    int i;
    printf("label,type,min,max,avg,stddev,alerts,status\n");
    for (i = 0; i < n; i++) {
        exportSummaryLine(&sensors[i]);
    }
}

/* ---------------------------------------------------------------------
 * Trend direction (comparing the first half of history to the second)
 * ------------------------------------------------------------------- */

const char *computeTrendDirection(const Sensor *s) {
    int half;
    double firstHalfSum = 0.0;
    double secondHalfSum = 0.0;
    double firstAvg, secondAvg;
    double diff;
    int i;

    if (s->historyCount < 4) {
        return "INSUFFICIENT DATA";
    }

    half = s->historyCount / 2;
    for (i = 0; i < half; i++) {
        firstHalfSum += s->history[i].value;
    }
    for (i = half; i < s->historyCount; i++) {
        secondHalfSum += s->history[i].value;
    }

    firstAvg = firstHalfSum / half;
    secondAvg = secondHalfSum / (s->historyCount - half);
    diff = secondAvg - firstAvg;

    if (diff > 1.0) {
        return "RISING";
    }
    if (diff < -1.0) {
        return "FALLING";
    }
    return "STABLE";
}

void printTrendReport(const Sensor sensors[], int n) {
    int i;
    printf("Trend Report (first half vs. second half of history):\n");
    for (i = 0; i < n; i++) {
        printf("  %-20s %s\n", sensors[i].label, computeTrendDirection(&sensors[i]));
    }
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Floor grouping (parses a floor number out of labels like "TEMP-F2-201")
 * ------------------------------------------------------------------- */

int extractFloorNumber(const Sensor *s) {
    int i;
    int len = (int) strlen(s->label);
    for (i = 0; i < len - 1; i++) {
        if (s->label[i] == 'F' && s->label[i + 1] >= '0' && s->label[i + 1] <= '9') {
            return s->label[i + 1] - '0';
        }
    }
    return -1;   /* no floor marker found in the label */
}

void printFloorBreakdown(const Sensor sensors[], int n) {
    int floor;
    int i;
    printf("Sensors By Floor:\n");
    for (floor = 1; floor <= 9; floor++) {
        int found = 0;
        for (i = 0; i < n; i++) {
            if (extractFloorNumber(&sensors[i]) == floor) {
                if (!found) {
                    printf("  Floor %d: ", floor);
                    found = 1;
                } else {
                    printf(", ");
                }
                printf("%s", sensors[i].label);
            }
        }
        if (found) {
            printf("\n");
        }
    }
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Threshold validation (a basic sanity check on sensor configuration)
 * ------------------------------------------------------------------- */

int validateThresholds(const Sensor *s) {
    if (s->minThreshold >= s->maxThreshold) {
        return 0;
    }
    return 1;
}

void printThresholdValidationReport(const Sensor sensors[], int n) {
    int i;
    int allValid = 1;
    printf("Threshold Validation:\n");
    for (i = 0; i < n; i++) {
        int valid = validateThresholds(&sensors[i]);
        if (!valid) {
            allValid = 0;
            printf("  %-20s INVALID (min %.1f >= max %.1f)\n", sensors[i].label,
                   sensors[i].minThreshold, sensors[i].maxThreshold);
        }
    }
    if (allValid) {
        printf("  All %d sensors have valid threshold configurations.\n", n);
    }
    printSeparator();
}

/* ---------------------------------------------------------------------
 * Alert log (lists every individual alert reading with its timestamp)
 * ------------------------------------------------------------------- */

void printAlertLog(const Sensor *s) {
    int i;
    int printedHeader = 0;
    for (i = 0; i < s->historyCount; i++) {
        if (isAlertReading(s, s->history[i].value)) {
            if (!printedHeader) {
                printf("  %s:\n", s->label);
                printedHeader = 1;
            }
            printf("    t=%4ds  value=%.2f  (range [%.1f, %.1f])\n",
                   s->history[i].timestampSec, s->history[i].value,
                   s->minThreshold, s->maxThreshold);
        }
    }
}

void printBuildingAlertLog(const Sensor sensors[], int n) {
    int i;
    printf("Building-Wide Alert Log:\n");
    for (i = 0; i < n; i++) {
        printAlertLog(&sensors[i]);
    }
    printSeparator();
}
