#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

enum commands {Add, Remove, List, Read};

char equipment_sensors[4][4][3];
int equipment_sensors_count[4];
int total_sensors = 0;

int is_valid_id(char* id) {
    const char available_ids[4][3] = {"01", "02", "03", "04"};
    for (int i=0; i<4; i++) {
        if (strcmp(id, available_ids[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_sensor_added(int equipment_id, char* sensor_id) {
    for (int i=0; i<equipment_sensors_count[equipment_id]; i++) {
        if (strcmp(equipment_sensors[equipment_id][i], sensor_id) == 0) {
            return 1;
        }
    }
    return 0;
}

void add_sensor(int equipment_id, char* sensor_id) {
    int num_of_sensors = equipment_sensors_count[equipment_id];
    strcpy(equipment_sensors[equipment_id][num_of_sensors], sensor_id);
    equipment_sensors_count[equipment_id]++;
    total_sensors++;
}

void remove_sensor(int equipment_id, char* sensor_id) {
    int num_of_sensors = equipment_sensors_count[equipment_id];
    for (int i=0; i<num_of_sensors; i++) {
        if (strcmp(equipment_sensors[equipment_id][i], sensor_id) == 0) {
            for (int j=i; j<num_of_sensors-1; j++) {
                strcpy(equipment_sensors[equipment_id][j], equipment_sensors[equipment_id][j+1]);
            }
            equipment_sensors_count[equipment_id]--;
            total_sensors--;
            return;
        }
    }
}

void join_str(char arr[][3], int arr_size, char* buf) {
    for (int i=0; i<arr_size; i++) {
        if (i == 0) {
            strcpy(buf, arr[i]);
        } else {
            strcat(buf, arr[i]);
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

    char sensors_id[4][3];
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

                if (!is_valid_id(ptr)) {
                    sprintf(response, "invalid sensor\n");
                    return 0;
                }

                strcpy(sensors_id[count_sensors], ptr);
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

    switch (command) {
        case Add:
            if (total_sensors + count_sensors > 15) {
                sprintf(response, "limit exceeded\n");
                return 0;
            }

            for (int i=0; i<count_sensors; i++) {
                if (is_sensor_added(equipment_id-1, sensors_id[i])) {
                    sprintf(response, "sensor %s already exists in 0%d\n", sensors_id[i], equipment_id);
                    return 0;
                }
            }

            for (int i=0; i<count_sensors; i++) {
                add_sensor(equipment_id-1, sensors_id[i]);
            }

            join_str(sensors_id, count_sensors, aux);
            
            sprintf(response, "sensor %s added\n", aux);

            break;
        case Remove:
            for (int i=0; i<count_sensors; i++) {
                if (!is_sensor_added(equipment_id-1, sensors_id[i])) {
                    sprintf(response, "sensor %s does not exists in 0%d\n", sensors_id[i], equipment_id);
                    return 0;
                }
            }

            for (int i=0; i<count_sensors; i++) {
                remove_sensor(equipment_id-1, sensors_id[i]);
            }

            join_str(sensors_id, count_sensors, aux);
            
            sprintf(response, "sensor %s removed\n", aux);

            break;
        case List:
            if (equipment_sensors_count[equipment_id-1] == 0) {
                sprintf(response, "none\n");
            } else {
                join_str(equipment_sensors[equipment_id-1], equipment_sensors_count[equipment_id-1], aux);
                sprintf(response, "%s\n", aux);
            }

            break;
        case Read:
            for (int i=0; i<count_sensors; i++) {
                if (!is_sensor_added(equipment_id-1, sensors_id[i])) {
                    if (strcmp(aux, "") == 0) {
                        strcpy(aux, sensors_id[i]);
                    } else {
                        strcat(aux, " ");
                        strcat(aux, sensors_id[i]);
                    }
                }
            }

            if (strcmp(aux, "") != 0) {
                sprintf(response, "sensor(s) %s not installed\n", aux);
                return 0;
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

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char recv_buf[BUFSZ];
    char send_buf[BUFSZ];

    while (1) {
        memset(recv_buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, recv_buf, BUFSZ - 1, 0);

        if (count == 0) {
            printf("[log] connection closed from %s\n", caddrstr);
            break;
        }

        if (strcmp(recv_buf, "kill\n") == 0) {
            printf("[log] server killed\n");
            exit(EXIT_SUCCESS);
        }

        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, strtok(recv_buf, "\n"));

        if (process_command(recv_buf, send_buf) == -1) {
            printf("[log] connection from %s closed\n", caddrstr);
            break;
        }

        count = send(cdata->csock, strtok(send_buf, "\0"), strlen(send_buf), 0);
        if (count != strlen(send_buf)) {
            logexit("send");
        }
    }

    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
