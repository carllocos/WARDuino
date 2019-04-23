/*
 __      __                  .___    .__
/  \    /  \_____ _______  __| _/_ __|__| ____   ____
\   \/\/   /\__  \\_  __ \/ __ |  |  \  |/    \ /  _ \
 \        /  / __ \|  | \/ /_/ |  |  /  |   |  (  <_> )
  \__/\  /  (____  /__|  \____ |____/|__|___|  /\____/
       \/        \/           \/             \/

WARDuino (c) by Christophe Scholliers & Robbert Gurdeep Singh

WARDuino is licensed under a
Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.

You should have received a copy of the license along with this
work. If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
*/

#include <string.h>
#include <iostream>
#include "../WARDuino.cpp.h"

#define MAIN "_main"
#define MAX_PATH 100
#define MAX_BYTE_CODE_SIZE 10000
#define BENCHMARK_PATH "./"

typedef const char *string;

void set_path(char *path, string name) {
    strncpy(path, BENCHMARK_PATH, strlen(BENCHMARK_PATH)+1);
    strncat(path, name, strlen(name)+1);
    strncat(path, "/wast/", 7+1);
    strncat(path, name, strlen(name)+1);
    strncat(path, ".wasm", 5+1);
}

unsigned int read_file_to_buf(unsigned char *bytes, string path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file: %s", path);
        exit(1);
    }
    fseek(file, 0L, SEEK_END);
    long num_bytes = ftell(file);
    if (num_bytes < MAX_BYTE_CODE_SIZE) {
        fseek(file, 0L, SEEK_SET);
        long result = fread(bytes, sizeof(char), num_bytes, file);
        if (result != num_bytes) {
            fprintf(stderr, "reading error while loading file %s", path);
            exit(1);
        }
        fclose(file);
        return (unsigned int)num_bytes;
    } else {
        fprintf(stderr, "File  < %s  > is too big for buffer", path);
        exit(1);
    }
}

void run_benchmarks(size_t num_benchmarks, string benchmarks[]) {
    char path[MAX_PATH];
    unsigned char bytes[MAX_BYTE_CODE_SIZE];
    unsigned int bytes_length;
    WARDuino *w = new WARDuino();

    for (size_t i = 0; i < num_benchmarks; i++) {
        string name = benchmarks[i];
        set_path(path, name);
        printf("[%lu/%lu]Starting test %s \n", i, num_benchmarks,path);
        bytes_length = read_file_to_buf(bytes, path);
        Options opt;
        Module *m = w->load_module(bytes, bytes_length, opt);
        int fidx = w->get_export_fidx(m, MAIN);
        bool succeed = w->invoke(m, fidx);
        if(!succeed){
            printf("test %s could not be interpreted\n", path);
            exit(1);
        } else {
          printf("      ok test %s \n", path);
        }
    }
}

int main(int argc, const char *argv[]) {
    string benchmarks[] = {"tak"};
    run_benchmarks((size_t)(sizeof(benchmarks) / sizeof(string *)), benchmarks);
    return 0;
}