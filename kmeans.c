#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define EPSILON 0.001
#define bool int
#define true 1
#define false 0

typedef struct {
    double *data;
} dpoint_t;

typedef struct {
    dpoint_t current_centroid;
    dpoint_t sum;
    int count;
} set_t;

static void assert_input(bool condition);
static void assert_other(bool condition);
static void collect_data(const char *filename);
static void initialize_sets(int*);
static void get_num_and_dim(FILE *file);
static void parse_datapoint(FILE *file, dpoint_t *dpoint);
static void assign_to_closest(dpoint_t dpoint);
static double sqdist(dpoint_t p1, dpoint_t p2);
static void add_to_set(set_t *set, dpoint_t dpoint);
static int update_centroid(set_t *set);
static void write_output(char *filename);
static void parse_args(int argc, char **argv, char **infile, char **outfile, int *max_iter);
static void init_datapoint(dpoint_t *dpoint);
static void free_datapoint(dpoint_t);
static void free_program(void);
static void converge(int max_iter);

/* function declarations for supporting the C API */
static PyObject* fit_capi(PyObject*, PyObject*);
static double** fit_c(int K_arg, int dim_arg, int num_data_arg, double eps, int max_iter, double** datapoints_arg, int* initial_centroids_indices); /* will most likely have to change that to an extern function */
static PyObject* arrayToList_D(double* array, int length);
static double* listToArray_D(PyObject *list, int length);
static int* listToArray_I(PyObject *list, int length);


/************************* configuring the C API ****************************************************/


static PyObject* fit_capi(PyObject *self, PyObject *args) {
	int K_arg, dim_arg, num_data_arg, max_iter;
	double epsilon;
	double** centroids_c;
	PyObject *centroids_py;
	PyObject *result;
	PyObject *initial_centroids_py;
	PyObject *datapoints_py;

	if(!PyArg_ParseTuple(args, "iiidiO!O!", &K_arg, &dim_arg, &num_data_arg, &epsilon, &max_iter, &PyList_Type, &datapoints_py, &PyList_Type, &initial_centroids_py)) {
		return NULL;
	}
	
	/* parsing the given lists as arrays */
	int i;
	double** datapoints_arg;
	int* initial_centroids_indices;

	datapoints_arg = (double**)calloc(num_data_arg, sizeof(double*));
	for (i = 0; i < num_data_arg; i++) {
		datapoints_arg[i] = listToArray_D(PyList_GetItem(datapoints_py, i), dim_arg);
	}
	
	initial_centroids_indices = listToArray_I(initial_centroids_py, K_arg);

	/* building the returned centroids' list */
	centroids_c = fit_c(K_arg, dim_arg, num_data_arg, epsilon, max_iter, datapoints_arg, initial_centroids_indices);
	centroids_py = PyList_New(num_data_arg);

	for(i = 0; i < num_data_arg; i++) {
		PyList_Append(centroids_py, arrayToList_D(centroids_c[i], dim_arg));
	}

	result = Py_BuildValue("O", centroids_py);

	/* free-ing the program and returning the centroids */
	free(centroids_c);
	free_program();
	return result;
}



static PyObject* arrayToList_D(double* array, int length) {
	PyObject *list;
	int i;

	list = PyList_New(length);
	for (i = 0; i < length; i++) {
		if(PyList_Append(list, PyFloat_FromDouble(array[i]))) {
			assert_other(false);
		}
	}

	return list;
}





static double* listToArray_D(PyObject *list, int length) {
	int i;
	double* result;
	PyObject *pypoint;

	/* first check if the given PyObject is indeed a list */
	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "the passed argument isn't a list");
		return NULL;
	}

	/* casting the list of float to an array of doubles */
	result = (double*)calloc(length, sizeof(double));

	for(i = 0; i < length; ++i) {
		pypoint = PyList_GetItem(list, (Py_ssize_t)i);
		if (!PyFloat_Check(pypoint)) {
			PyErr_SetString(PyExc_TypeError, "must pass an list of floats");
			return NULL;
		}
		result[i] = (double) PyFloat_AsDouble(pypoint);
	}

	return result;
}





static int* listToArray_I(PyObject *list, int length) {
	int i;
	int* result;
	PyObject *pypoint;
	
	/* first check if the given PyObject is indeed a list */
	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "the passed argument isn't a list");
		return NULL;
	}


	/* casting the list of integers to an array of integers */
	result = (int*)calloc(length, sizeof(int));

	for(i = 0; i < length; ++i) {
		pypoint = PyList_GetItem(list, (Py_ssize_t)i);
		if (!PyLong_Check(pypoint)) {
			PyErr_SetString(PyExc_TypeError, "must pass an list of floats");
			return NULL;
		}
		result[i] = (int) PyLong_AsLong(pypoint);
	}

	return result;
}






static PyMethodDef capiMethods[] = {
	{"fit",
		(PyCFunction) fit_capi,
		METH_VARARGS,
		PyDoc_STR("A fit method for the kmeans algorithm")
	},
	{NULL, NULL, 0, NULL}
};




static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"mykmeanssp",
	NULL,
	-1,
	capiMethods
};



PyMODINIT_FUNC
PyInit_mykmeanssp(void) {
	PyObject *m;
	m = PyModule_Create(&moduledef);
	if (!m) {
		return NULL;
	}
	return m;
}


/*****************************************************************************/

static void assert_input(bool condition) {
    if(!condition) {
        puts("Invalid Input!");
        free_program();
        exit(1);
    }
}

static void assert_other(bool condition) {
    if(!condition) {
        puts("An Error Has Occurred");
        free_program();
        exit(1);
    }
}

/*****************************************************************************/

int dim = 0;
int num_data = 0;
dpoint_t *datapoints = NULL;

int K = 0;
set_t *sets = NULL;

/*****************************************************************************/

int main(int argc, char **argv) {
    char *infile, *outfile;
    int i, max_iter;

    parse_args(argc, argv, &infile, &outfile, &max_iter);
    printf("K = %i, max_iter = %i, infile = '%s', outfile = '%s'\n", K,
           max_iter, infile, outfile);

    collect_data(infile);
    printf("dim = %i, N = %i\n", dim, num_data);

    int* range_0_to_K_minus_1;
    range_0_to_K_minus_1 = (int*)calloc(K, sizeof(int));

    for (i = 0; i < K; i++) {
	    range_0_to_K_minus_1[i] = i;
    }

    initialize_sets(range_0_to_K_minus_1);
    free(range_0_to_K_minus_1);
    converge(max_iter);
    write_output(outfile);

    free_program();
    return 0;
}


/* the fit function. uses its given initialized centroids, datapoints and other cruical data
 * and performs the kmeans algorithm until convergence
 */
static double** fit_c(int K_arg, int dim_arg, int num_data_arg, double epsilon, int max_iter, double** datapoints_arg, int* initial_centroids_indices) {
	#undef EPSILON
	#define EPSILON epsilon /* the stem of the issue */

	int i;
	dim = dim_arg;
	num_data = num_data_arg;
	datapoints = calloc(num_data, sizeof(*datapoints));
	assert_other(NULL != datapoints);
	
	for(i = 0; i < num_data; i++) {
		datapoints[i].data = datapoints_arg[i];
	}
	
	initialize_sets(initial_centroids_indices);
	converge(max_iter);
	
	double** centroids_out;
	centroids_out = (double**)calloc(K, sizeof(double*));

	for (i = 0; i < K; i++) {
		centroids_out[i] = sets[i].current_centroid.data;
	}
	
	return centroids_out;
}




/* This function was created to reduce code duplication. It is the engine of the algorithm,
 * providing the structurization of the clusters until convergence 
 */
static void converge(int max_iter) {
	int iter, i, updated_centroids;
    for(iter = 0; iter < max_iter; iter++) {
        for(i = 0; i < num_data; i++) {
            assign_to_closest(datapoints[i]);
        }

        updated_centroids = 0;
        for(i = 0; i < K; i++) {
            updated_centroids += update_centroid(&sets[i]);
        }

        if(updated_centroids == 0) { /* Convergence */
            break;
        }
    }
}




/* Assigns the given datapoint to the closest set that it can find, using the
 * sqdist function. */
static void assign_to_closest(dpoint_t dpoint) {
    int i, min_idx = 0;
    double min_dist = -1.0;

    for(i = 0; i < K; i++) {
        double dist = sqdist(sets[i].current_centroid, dpoint);

        if((min_dist < 0.0) || (dist < min_dist)) {
            min_idx = i;
            min_dist = dist;
        }
    }

    add_to_set(&sets[min_idx], dpoint);
}

/* Updates the centroid of the given set using its stored `sum` and `count`
 * properties, while also resetting them to 0 for the next iteration. */
static int update_centroid(set_t *set) {
    double dist;
    int i;

    for(i = 0; i < dim; i++) {
        set->sum.data[i] /= (double)set->count;
    }

    dist = sqrt(sqdist(set->sum, set->current_centroid));

    for(i = 0; i < dim; i++) {
        set->current_centroid.data[i] = set->sum.data[i];
        set->sum.data[i] = 0.0;
    }

    set->count = 0;

    return (dist >= EPSILON) ? 1 /* If this set's centroid changed, return 1 */
                             : 0;
}

/* Calculates the squared distance between two given datapoints. */
static double sqdist(dpoint_t p1, dpoint_t p2) {
    double dot = 0;
    int i;

    for(i = 0; i < dim; i++) {
        double temp = p1.data[i] - p2.data[i];
        dot += temp * temp;
    }

    return dot;
}

/* Adds the given datapoint to the provided set, taking into account both the
 * `sum` and `count` properties. */
static void add_to_set(set_t *set, dpoint_t dpoint) {
    int i;

    set->count += 1;
    for(i = 0; i < dim; i++) {
        set->sum.data[i] += dpoint.data[i];
    }
}



/* Initializes all of the sets, both allocating memory for the `sum` and
 * `current_centroid` properties and copying the data from a given index list
 *  that indicates what datapoint we should initialize the current centroid with.
 *  In the case of a regular kmeans.c run in HW1, we would power this with the plain [0,1,...,K-1] index list.
 *  Though, with the kmeans++, we will power it with the observation indices list */
static void initialize_sets(int* indices) {
    int i, j;

    sets = calloc(K, sizeof(*sets));
    assert_other(NULL != sets);

    for(i = 0; i < K; i++) {
        /* count is already zero. We just need to allocate the centroid and sum
           datapoints. */
        init_datapoint(&sets[i].sum);
        init_datapoint(&sets[i].current_centroid);

        /* Copy initial current_centroid from i-th datapoint */
        for(j = 0; j < dim; j++) {
            sets[i].current_centroid.data[j] = datapoints[indices[i]].data[j];
        }
    }
}



/* Given an input filename, gathers all of the datapoints stored in that file,
 * while also figuring out what `dim` and `num_data` are supposed to be. */
static void collect_data(const char *filename) {
    FILE *input;
    int i;

    input = fopen(filename, "r");
    assert_input(NULL != input);
    get_num_and_dim(input);

    datapoints = calloc(num_data, sizeof(*datapoints));
    assert_other(NULL != datapoints);

    for(i = 0; i < num_data; i++) {
        parse_datapoint(input, &datapoints[i]);
    }

    fclose(input);
}

/* Parses a single datapoint from the given file, assuming that `dim` has
 * already been figured out. */
static void parse_datapoint(FILE *file, dpoint_t *dpoint) {
    int i;

    init_datapoint(dpoint);

    for(i = 0; i < dim; i++) {
        /* The following ',' is okay, because even if it isn't found parsing
           will be successful. */
        fscanf(file, "%lf,", &dpoint->data[i]);
    }

    /* Get rid of extra whitespace. */
    fscanf(file, "\n");
}

/* Determines `num_data` and `dim` from the current file by inspecting line
 * structure and amount. */
static void get_num_and_dim(FILE *file) {
    int c;

    dim = 1; /* Starting with 1 because the amount of numbers is always 1 more
                than the amount of commas. */
    num_data = 0;

    rewind(file);
    while(EOF != (c = fgetc(file))) {
        if(c == '\n') {
            num_data++;
        } else if(c == ',' && num_data == 0) {
            dim++;
        }
    }
    rewind(file);
}

/* Parses the arguments given to the program into K, max_iter, input_file and
 * output_file. */
static void parse_args(int argc, char **argv, char **infile, char **outfile,
                       int *max_iter) {
    int offset;

    /* +1s because the executable filename is an argument too */
    assert_input(argc == 3 + 1 || argc == 4 + 1);

    K = atoi(argv[1]);
    assert_input(K > 0);

    if(argc == 3 + 1) {
        *max_iter = 200;

        offset = 0;
    } else /* (argc == 4 + 1) */ {
        *max_iter = atoi(argv[2]);
        assert_input(*max_iter > 0);

        offset = 1;
    }

    *infile = argv[2 + offset];
    *outfile = argv[3 + offset];
}

/* Initializes a single datapoint - allocates enough space for it and sets all
 * the values to zero. */
static void init_datapoint(dpoint_t *dpoint) {
    assert_other(dim > 0);

    dpoint->data = calloc(dim, sizeof(*dpoint->data));
    assert_other(NULL != dpoint->data);
}

/* Frees the given datapoint. If it's already been freed or not yet allocated,
 * this function safely does nothing. */
static void free_datapoint(dpoint_t dpoint) {
    if(NULL != dpoint.data) {
        free(dpoint.data);
    }
}

/* Frees all of the memory allocated by the program. If a certain variable
 * hasn't been allocated yet, this function does not attempt to free it. */
static void free_program() {
    int i;

    if(NULL != datapoints) {
        for(i = 0; i < num_data; i++) {
            free_datapoint(datapoints[i]);
        }
        free(datapoints);
    }

    if(NULL != sets) {
        for(i = 0; i < K; i++) {
            free_datapoint(sets[i].current_centroid);
            free_datapoint(sets[i].sum);
        }
        free(sets);
    }
}

/* Writes the centroid data into the file with the given filename. */
static void write_output(char *filename) {
    int i, j;
    FILE *file = fopen(filename, "w");

    for(i = 0; i < K; i++) {
        for(j = 0; j < dim; j++) {
            fprintf(file, "%.4f", sets[i].current_centroid.data[j]);
            if(j < dim - 1) {
                fputc(',', file);
            }
        }
        fputc('\n', file);
    }

    fclose(file);
}

