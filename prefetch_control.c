/*
 * Prefetch Control Tool - Based on code from FAST Lab @ ECE-UIUC
 * This program is used to enable or disable hardware prefetching on specific CPU cores
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>

#define PREFETCH_REG_ADDR   0x1A4
#define MAX_CORE_NUM        63  // Adjust this value based on your system

// Color definitions for output
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YEL     "\x1b[33m"
#define RESET   "\x1b[0m"

/* 
 * Read MSR register value from specified CPU core
 */
uint64_t read_MSR(int cpu) {
    int fd;
    uint64_t data;
    char msr_file_name[64];

    sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
    fd = open(msr_file_name, O_RDONLY);

    if (fd < 0) {
        if (errno == ENXIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d not found\n", cpu);
            exit(2);
        } else if (errno == EIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d doesn't support MSR\n", cpu);
            exit(3);
        } else {
            perror("Failed to open MSR for reading");
            exit(127);
        }
    }

    if (pread(fd, &data, sizeof data, PREFETCH_REG_ADDR) != sizeof data) {
        if (errno == EIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d cannot read MSR\n", cpu);
            exit(4);
        } else {
            perror("MSR read failed");
            exit(127);
        }
    }

    close(fd);
    return data;
}

/*
 * Write value to MSR register of specified CPU core
 */
void write_MSR(int cpu, uint64_t val) {
    int fd;
    char msr_file_name[64];

    sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
    fd = open(msr_file_name, O_WRONLY);

    if (fd < 0) {
        if (errno == ENXIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d not found\n", cpu);
            exit(2);
        } else if (errno == EIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d doesn't support MSR\n", cpu);
            exit(3);
        } else {
            perror("Failed to open MSR for writing");
            exit(127);
        }
    }

    if (pwrite(fd, &val, sizeof(val), PREFETCH_REG_ADDR) != sizeof(val)) {
        if (errno == EIO) {
            fprintf(stderr, RED "[ERROR]" RESET " CPU %d cannot set MSR\n", cpu);
            exit(4);
        } else {
            perror("MSR write failed");
            exit(127);
        }
    }

    close(fd);
}

/*
 * Disable prefetching on specified CPU core
 */
void disable_prefetch(int cpu) {
    uint64_t val;
    val = read_MSR(cpu);
    write_MSR(cpu, val | 0xF);
    val = read_MSR(cpu);
    printf(YEL "[INFO]" RESET " CPU %d prefetch disabled. Current 0x1A4 register value: %lx\n", cpu, val);
}

/*
 * Enable prefetching on specified CPU core
 */
void enable_prefetch(int cpu) {
    uint64_t val;
    val = read_MSR(cpu);
    write_MSR(cpu, val & 0xFFFFFFFFFFFFFFF0);
    val = read_MSR(cpu);
    printf(YEL "[INFO]" RESET " CPU %d prefetch enabled. Current 0x1A4 register value: %lx\n", cpu, val);
}

/*
 * Display help information
 */
void print_help() {
    printf("Prefetch Control Tool - Enable or disable hardware prefetching on CPU cores\n\n");
    printf("Usage:\n");
    printf("  -c, --core NUM     Specify CPU core number to operate on (0-%d)\n", MAX_CORE_NUM);
    printf("  -d, --disable      Disable prefetching on specified core(s)\n");
    printf("  -e, --enable       Enable prefetching on specified core(s)\n");
    printf("  -a, --all          Apply operation to all cores\n");
    printf("  -s, --status       Show current prefetch status of specified core(s)\n");
    printf("  -h, --help         Display this help information\n\n");
    printf("Examples:\n");
    printf("  Disable prefetch on core 0:   %s -c 0 -d\n", "prefetch_control");
    printf("  Enable prefetch on core 1:    %s -c 1 -e\n", "prefetch_control");
    printf("  Check status of core 2:       %s -c 2 -s\n", "prefetch_control");
    printf("  Disable prefetch on all cores: %s -a -d\n", "prefetch_control");
}

/*
 * Display prefetch status
 */
void show_prefetch_status(int cpu) {
    uint64_t val = read_MSR(cpu);
    printf("CPU %d prefetch status: ", cpu);
    if ((val & 0xF) == 0xF) {
        printf(RED "Disabled" RESET " (0x1A4 = %lx)\n", val);
    } else if ((val & 0xF) == 0x0) {
        printf(GREEN "Enabled" RESET " (0x1A4 = %lx)\n", val);
    } else {
        printf(YEL "Partially enabled" RESET " (0x1A4 = %lx)\n", val);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    int core_num = -1;
    int action = 0; // 0=no action, 1=enable, 2=disable, 3=status
    int all_cores = 0;
    
    struct option long_options[] = {
        {"core",    required_argument, 0, 'c'},
        {"disable", no_argument,       0, 'd'},
        {"enable",  no_argument,       0, 'e'},
        {"all",     no_argument,       0, 'a'},
        {"status",  no_argument,       0, 's'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    // Parse command line arguments
    while ((opt = getopt_long(argc, argv, "c:deasvh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                core_num = atoi(optarg);
                if (core_num < 0 || core_num > MAX_CORE_NUM) {
                    fprintf(stderr, RED "[ERROR]" RESET " Core number must be between 0 and %d\n", MAX_CORE_NUM);
                    return 1;
                }
                break;
            case 'd':
                action = 2; // Disable
                break;
            case 'e':
                action = 1; // Enable
                break;
            case 'a':
                all_cores = 1;
                break;
            case 's':
                action = 3; // Status
                break;
            case 'h':
                print_help();
                return 0;
            default:
                fprintf(stderr, "Use -h or --help to see usage information\n");
                return 1;
        }
    }

    // Validate parameters
    if (action == 0) {
        fprintf(stderr, RED "[ERROR]" RESET " Please specify an operation (-d, -e, or -s)\n");
        return 1;
    }

    if (!all_cores && core_num == -1) {
        fprintf(stderr, RED "[ERROR]" RESET " Please specify a CPU core number (-c) or use -a to operate on all cores\n");
        return 1;
    }

    // Execute operation
    if (all_cores) {
        // Operate on all cores
        for (int i = 0; i <= MAX_CORE_NUM; i++) {
            // Try to operate on each core, ignore non-existent ones
            char msr_file_name[64];
            sprintf(msr_file_name, "/dev/cpu/%d/msr", i);
            if (access(msr_file_name, F_OK) != -1) {
                switch (action) {
                    case 1:
                        enable_prefetch(i);
                        break;
                    case 2:
                        disable_prefetch(i);
                        break;
                    case 3:
                        show_prefetch_status(i);
                        break;
                }
            }
        }
    } else {
        // Operate on a single core
        switch (action) {
            case 1:
                enable_prefetch(core_num);
                break;
            case 2:
                disable_prefetch(core_num);
                break;
            case 3:
                show_prefetch_status(core_num);
                break;
        }
    }

    return 0;
} 