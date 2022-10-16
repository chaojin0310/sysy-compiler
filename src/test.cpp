#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>
#include <string>
using namespace std;

/* Parameters */
const char params_file[] = "./params.txt";
int obj_size = 1 << 22;  // object size, default 4M
int num_replica = 3;     // replica number, default 3
int num_nodes = 100;     // number of nodes, default 100
std::vector<float> node_weights;    // weight array of all nodes

/* Command line processing */
extern int optind, opterr, optopt;
extern char *optarg;
int getopt(int argc, char *const argv[], const char *optstring);
int getopt_long(int argc, char *const argv[], const char *optstring, 
                const struct option *longopts, int *longindex);

void process_line(std::string &line, const std::string &comment_str="#") {
    std::cout << line << endl;
}

int main(int argc, char *argv[]) {
    std::ifstream fin(params_file);
    std::string tmp;
    while (std::getline(fin, tmp)) {
        process_line(tmp);
    }
    return 0;
}