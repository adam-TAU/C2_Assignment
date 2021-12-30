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

static void assert_other(bool condition);
static void initialize_sets(int*);
static void assign_to_closest(dpoint_t dpoint);
static double sqdist(dpoint_t p1, dpoint_t p2);
static void add_to_set(set_t *set, dpoint_t dpoint);
static int update_centroid(set_t *set);
static void init_datapoint(dpoint_t *dpoint);
static void free_datapoint(dpoint_t);
static void free_program(void);
static void converge(int max_iter);

/* function declarations for supporting the C API */
static PyObject* fit_capi(PyObject*, PyObject*);
static double** fit_c(double eps, int max_iter, double** datapoints_arg, int* initial_centroids_indices); /* will most likely have to change that to an extern function */
static PyObject* arrayToList_D(double* array, int length);
static double* listToArray_D(PyObject *list, int length);
static int* listToArray_I(PyObject *list, int length);


/*****************************************************************************/


static void assert_other(bool condition) {
    if(!condition) {
        puts("An Error Has Occurred");
        free_program();
        exit(1);
    }
}

/*****************************************************************************/

int dim;
int num_data;
dpoint_t *datapoints = NULL;

int K;
set_t *sets = NULL;

double epsilon;

/*****************************************************************************/


/* the fit function. uses its given initialized centroids, datapoints and other cruical data
 * and performs the kmeans algorithm until convergence
 */
static double** fit_c(double epsilon, int max_iter, double** datapoints_arg, int* initial_centroids_indices) {
	#undef EPSILON
	#define EPSILON epsilon

	int i;
	datapoints = calloc(num_data, sizeof(*datapoints));
	assert_other(NULL != datapoints);
	
	for(i = 0; i < num_data; i++) {
		datapoints[i].data = datapoints_arg[i];
	}

	
	/* contaminated */
	printf("checkpoint 1\n");
	initialize_sets(initial_centroids_indices);
	printf("checkpoint 1\n");
	converge(max_iter);
	/* contaminated-end */
	
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
		
		/* contaminated */
		printf("checkpoint 1\n");
        /* Copy initial current_centroid from i-th datapoint */
        for(j = 0; j < dim; j++) {
            sets[i].current_centroid.data[j] = datapoints[indices[i]].data[j];
        }
        printf("checkpoint 2\n");
        /* contaminated-end */
    }
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




/************************* configuring the C API ****************************************************/


/******************************************************************************/

static PyObject* fit_capi(PyObject *self, PyObject *args) {
	int max_iter;
	double** centroids_c;
	PyObject *centroids_py;
	PyObject *result;
	PyObject *initial_centroids_py;
	PyObject *datapoints_py;

	if(!PyArg_ParseTuple(args, "iiidiO!O!", &K, &dim, &num_data, &epsilon, &max_iter, &PyList_Type, &datapoints_py, &PyList_Type, &initial_centroids_py)) {
		return NULL;
	}
	
	
	/* parsing the given lists as arrays */
	int i;
	double** datapoints_arg;
	int* initial_centroids_indices;

	datapoints_arg = (double**)calloc(num_data, sizeof(double*));
	for (i = 0; i < num_data; i++) {
		datapoints_arg[i] = listToArray_D(PyList_GetItem(datapoints_py, i), dim);
	}
	
	initial_centroids_indices = listToArray_I(initial_centroids_py, K);

	/* building the returned centroids' list */
	/* segmentation fault occurs in the following line (lol) */
	centroids_c = fit_c(epsilon, max_iter, datapoints_arg, initial_centroids_indices);
	centroids_py = PyList_New(num_data);

	for(i = 0; i < num_data; i++) {
		PyList_Append(centroids_py, arrayToList_D(centroids_c[i], dim));
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



/**************************************************************************/


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





