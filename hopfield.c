/**
 * Basic hopfield network for pattern recognition in bit patterns.
 * Asynchronous updating of neurons, training with Hebbian rule.
 * Compile: gcc -std=c99 -o hopfield hopfield.c
 * 
 * (c) L. Diener 2014
 */

// Set to float or double. Note that this impacts how things
// are saved. I am a very lazy person.
#define scalar double

#include <stdio.h>
#include <stdlib.h>

#include "bmp_handler.h"

// Holds data for a certain hopfield network
typedef struct hopfield_net {
	int num_neurons;
	scalar* weights;
} hopfield_net;

///////// Data Managment /////////

// Allocate space for a network
hopfield_net alloc_network(int num_neurons) {
	hopfield_net net;
	net.num_neurons = num_neurons;
	net.weights = malloc(sizeof(scalar) * num_neurons * num_neurons);
	for(int i = 0; i < num_neurons; i++) {
		for(int j = 0; j < num_neurons; j++) {
			if(i == j) {
				net.weights[i * num_neurons + j] = ((scalar)0.0);
			}
			else {
				net.weights[i * num_neurons + j] = ((scalar)1.0) / (scalar)(num_neurons - 1);
			}
		}
	}
	return net;
}

// Deallocate space allocated for a network
void free_network(hopfield_net net) {
	free(net.weights);
	net.weights = 0;
}

///////// Debugging /////////

// Print network data (for debugging really small networks)
void show_network(hopfield_net net) {
	printf("Number of neurons: %d\n", net.num_neurons);
	printf("Weights:\n");
	for(int i = 0; i < net.num_neurons; i++) {
		for(int j = 0; j < net.num_neurons; j++) {
			printf("%f\t", net.weights[i * net.num_neurons + j]);
		}
		printf("\n");
	}
}

///////// Neural Net I/O /////////

// Store a network to a file
void save_network(hopfield_net net, char* file_name) {
	FILE* file = fopen(file_name, "w");
	fwrite(&net.num_neurons, sizeof(int), 1, file);
	fwrite(net.weights, sizeof(scalar), net.num_neurons * net.num_neurons, file);
	fclose(file);
}

// Read a network from a file
hopfield_net read_network(char* file_name) {
	FILE* file = fopen(file_name, "r");
	
	int num_neurons;
	fread(&num_neurons, sizeof(int), 1, file);
	
	hopfield_net net = alloc_network(num_neurons);
	fread(net.weights, sizeof(scalar), net.num_neurons * net.num_neurons, file);
	
	fclose(file);
	return net;
}

///////// Training /////////

// Set up Hebbian rule weights
void train_network(hopfield_net net, int num_patterns, scalar** patterns) {
	for(int i = 0; i < net.num_neurons; i++) {
		for(int j = 0; j < net.num_neurons; j++) {
			scalar weight_ij = ((scalar)0.0);
			for(int p = 0; p < num_patterns; p++) {
				weight_ij += patterns[p][i] * patterns[p][j];
			}
			net.weights[i * net.num_neurons + j] = weight_ij / ((scalar)num_patterns);
		}
	}
}

///////// Pattern I/O Helpers /////////

// Helper: Read a bit pattern from a BMP image.
// Returns -1 / 1 bit pattern as linear 1D scalar array by checking luma > 0.5
scalar* read_pattern(char* file_name, int* x_size, int* y_size) {
	bmp_read(file_name, x_size, y_size);
	
	int num_entries = *x_size * *y_size;
	scalar* pattern = (scalar*)malloc(sizeof(scalar) * num_entries);
	for(int i = 0; i < num_entries; i++) {
		int r, g, b;
		bmp_read_pixel(&r, &g, &b);
		scalar luma = ((scalar)r) * ((scalar)0.3) + ((scalar)g) * ((scalar)0.6) + ((scalar)b) * ((scalar)0.1);
		pattern[i] = (luma < ((scalar)128.0) ? ((scalar)1.0) : ((scalar)-1.0));
	}
	return pattern;
}

// Print our pattern as ' ' / '#'
void show_pattern(scalar* pattern, int x_size, int y_size) {
	for(int x = 0; x < x_size + 2; x++) {
		printf("-");
	}
	printf("\n");
	
	for(int y = 0; y < y_size; y++) {
		printf("|");
		for(int x = 0; x < x_size; x++) {
			printf("%c", pattern[x + y * x_size] > ((scalar)0.0) ? '#' : ' ');
		}
		printf("|\n");
	}
	
	for(int x = 0; x < x_size + 2; x++) {
		printf("-");
	}
	printf("\n");
}

///////// Evaluation /////////

// Update a single neuron
void update_neuron(hopfield_net net, scalar* net_state, int neuron) {
	scalar neuron_update = ((scalar)0.0);
	for(int i = 0; i < net.num_neurons; i++) {
		neuron_update += net_state[i] * net.weights[neuron + i * net.num_neurons];
	}
	net_state[neuron] = neuron_update < ((scalar)0.0) ? ((scalar)-1.0) : ((scalar)1.0);
}

// Update as many times as there are neurons, times iterations, with
// neurons picked uniformly at random.
void run_network_iterations(hopfield_net net, scalar* net_state, int iterations) {
	for(int i = 0; i < iterations; i++) {
		for(int j = 0; j < net.num_neurons; j++) {
			int neuron = (int)(rand() / (((scalar)RAND_MAX + 1)/ ((scalar)net.num_neurons)));
			update_neuron(net, net_state, neuron);
		}
	}
}

///////// Main function /////////

int main(int argc, char** argv) {
	// Make randomness less random
	srand(1337);
	
	// Check arguments
	if(argc == 1 || (argv[1][0] != 't' && argv[1][0] != 'e' && argv[1][0] != 'd')) {
		printf("Usage:\n");
		printf("\t%s train <network output file> <pattern 1> <pattern 2> [...]\n", argv[0]);
		printf("\t%s evaluate <network input file> <input patern> <iterations>\n", argv[0]);
		printf("\t%s detail <network input file> <input patern> <iterations>\n\n", argv[0]);
		printf("Pattern files should be 24 bit BMP files, will be turned into bit patterns by luma.\n");
		printf("Make sure all pattern files are the same size, or bad things will result.\n");
		exit(0);
	}
	
	char* action = argv[1];
	if(action[0] == 't') {
		printf("Training mode\n");
		
		// Read patterns
		int num_patterns = argc - 3;
		int x_size = -1;
		int y_size = -1;
		
		scalar* patterns[num_patterns];
		for(int i = 0; i < num_patterns; i++) {
			printf("\nLoading pattern %d from %s:\n", i, argv[i + 3]);
			patterns[i] = read_pattern(argv[i + 3], &x_size, &y_size);
			show_pattern(patterns[i], x_size, y_size);
		}
		
		// Train
		printf("\nTraining with %d patterns and %d neurons...\n", num_patterns, x_size * y_size);
		hopfield_net net = alloc_network(x_size * y_size);
		train_network(net, num_patterns, patterns);
		
		// Store
		printf("Writing net to %s\n", argv[2]);
		save_network(net, argv[2]);
		
		// Free and quit
		for(int i = 0; i < num_patterns; i++) {
			free(patterns[i]);
			patterns[i] = 0;
		}
		free_network(net);
		printf("Done.\n");
		exit(0);
	}
	
	if(action[0] == 'e' || action[0] == 'd') {
		printf("Evaluation mode\n");
		
		// Read network
		printf("Loading network %s...\n", argv[2]);
		hopfield_net net = read_network(argv[2]);
		
		// Read pattern
		printf("Loading input pattern %s:\n", argv[3]);
		int x_size = -1;
		int y_size = -1;
		scalar* net_state = read_pattern(argv[3], &x_size, &y_size);
		show_pattern(net_state, x_size, y_size);
		
		// Run network for a few iterations
		int iterations = atoi(argv[4]);
		printf("\nRunning network for %d iterations...\n", iterations);
		if(action[0] == 'e') {
			run_network_iterations(net, net_state, iterations);
		}
		else {
			for(int iteration = 0; iteration < iterations; iteration++) {
				run_network_iterations(net, net_state, 1);
				if(iteration != iterations - 1) {
					printf("After iteration %d:\n", iteration);
					show_pattern(net_state, x_size, y_size);
					printf("\n");
				}
			}
		}
		
		// Print output
		printf("Done, output pattern:\n");
		show_pattern(net_state, x_size, y_size);
		printf("\n");
		
		// Free and quit
		free(net_state);
		free_network(net);
		exit(0);
	}
}