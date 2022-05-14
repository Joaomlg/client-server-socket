#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum commands {Add, Remove, List, Read};

int equipment_sensors[4][4];
int total_sensors = 0;

void join_ids(int ids[], int arr_size, char* buf) {
    char aux[4][3] = {"01", "02", "03", "04"};

    for (int i=0; i<arr_size; i++) {
        if (i == 0) {
            strcpy(buf, aux[ids[i] - 1]);
        } else {
            strcat(buf, aux[ids[i] - 1]);
        }
        if (i != arr_size - 1) {
            strcat(buf, " ");
        }
    }
}

int process_command(char* data, char* response) {
    int state = 0;
    int next_state = 0;
    const int final_state = -1;

    enum commands command;

    int sensors_id[4];
    int count_sensors = 0;

    int equipment_id;

    const char delim[] = " ";
    char *ptr = strtok(data, delim);

    while (state != final_state) {
        if (ptr == NULL) {
            return -1;
        }

        switch (state) {
            case 0:
                if (strcmp(ptr, "add") == 0) {
                    command = Add;
                    next_state = 1;
                } else if (strcmp(ptr, "remove") == 0) {
                    command = Remove;
                    next_state = 1;
                } else if (strcmp(ptr, "list") == 0) {
                    command = List;
                    next_state = 2;
                } else if (strcmp(ptr, "read") == 0) {
                    command = Read;
                    next_state = 3;
                } else {
                    return -1;
                }
                
                break;
            case 1:
                if (strcmp(ptr, "sensor") == 0) {
                    next_state = 3;
                } else {
                    return -1;
                }

                break;
            case 2:
                if (strcmp(ptr, "sensors") == 0) {
                    next_state = 4;
                } else {
                    return -1;
                }

                break;
            case 3:
                if (strcmp(ptr, "in") == 0) {
                    if (count_sensors == 0) {
                        return -1;
                    }

                    next_state = 5;

                    break;
                }

                if (count_sensors == 4) {
                    return -1;
                }

                int sensor_id = atoi(ptr);

                if (sensor_id < 1 || sensor_id > 4) {
                    sprintf(response, "invalid sensor\n");
                    return 0;
                }

                sensors_id[count_sensors] = sensor_id;
                count_sensors += 1;

                break;
            case 4:
                if (strcmp(ptr, "in") == 0) {
                    next_state = 5;
                } else {
                    return -1;
                }

                break;
            case 5:
                equipment_id = atoi(ptr);

                if (equipment_id < 1 || equipment_id > 4) {
                    sprintf(response, "invalid equipment\n");
                    return 0;
                }

                next_state = final_state;

                break;
            default:
                return -1;
        }

        ptr = strtok(NULL, delim);
        state = next_state;

        if (state == final_state && ptr != NULL) {
            return -1;
        }
    }

    char aux[24] = "";
    int i_aux[4];
    int count_aux = 0;

    switch (command) {
        case Add:
            if (total_sensors + count_sensors > 15) {
                sprintf(response, "limit exceeded\n");
                return 0;
            }

            for (int i=0; i<count_sensors; i++) {
                if (equipment_sensors[equipment_id-1][sensors_id[i]-1] == 1) {
                    sprintf(response, "sensor 0%d already exists in 0%d\n", sensors_id[i], equipment_id);
                    return 0;
                }
            }

            for (int i=0; i<count_sensors; i++) {
                equipment_sensors[equipment_id-1][sensors_id[i]-1] = 1;
                total_sensors++;
            }

            join_ids(sensors_id, count_sensors, aux);
            
            sprintf(response, "sensor %s added\n", aux);

            break;
        case Remove:
            for (int i=0; i<count_sensors; i++) {
                if (equipment_sensors[equipment_id-1][sensors_id[i]-1] == 0) {
                    sprintf(response, "sensor 0%d does not exists in 0%d\n", sensors_id[i], equipment_id);
                    return 0;
                }
            }

            for (int i=0; i<count_sensors; i++) {
                equipment_sensors[equipment_id-1][sensors_id[i]-1] = 0;
                total_sensors--;
            }

            join_ids(sensors_id, count_sensors, aux);
            
            sprintf(response, "sensor %s removed\n", aux);

            break;
        case List:  
            count_sensors = 0;

            for (int i=0; i<4; i++) {
                if (equipment_sensors[equipment_id-1][i]) {
                    sensors_id[count_sensors] = i + 1;
                    count_sensors++;
                }
            }

            if (count_sensors == 0) {
                sprintf(response, "none\n");
            } else {
                join_ids(sensors_id, count_sensors, aux);
                sprintf(response, "%s\n", aux);
            }

            break;
        case Read:
            for (int i=0; i<count_sensors; i++) {
                if (equipment_sensors[equipment_id-1][sensors_id[i]-1] == 0) {
                    i_aux[count_aux] = sensors_id[i];
                    count_aux++;
                }
            }

            if (count_aux > 0) {
                join_ids(i_aux, count_aux, aux);
                sprintf(response, "sensor(s) %s not installed\n", aux);
                break;
            }

            for (int i=0; i<count_sensors; i++) {
                char rand_str[6];
                sprintf(rand_str, "%.2f", (float) (rand() % 999) / 100);

                if (strcmp(aux, "") == 0) {
                    strcpy(aux, rand_str);
                } else {
                    strcat(aux, " ");
                    strcat(aux, rand_str);
                }
            }

            sprintf(response, "%s\n", aux);

            break;
        default:
            return -1;
    }

    return 0;
}