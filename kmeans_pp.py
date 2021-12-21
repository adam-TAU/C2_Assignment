import sys
from typing import List
import numpy as np
import mykmeanssp



def main(K: int, eps: int, maxiter: int, infile: str, outfile: str) -> None:
	# reading from the input file
    datapoints = parse_datapoints(infile)

    # initializing K centroids from the observations
    initialized_centroids = initialize_centroids(K, datapoints)

    # powering the kmeans.c centroid creation module
    centroids = mykmeanssp.kmeans.fit(K, eps, max_iter, infile, outfile, initialized_centroids)
	
    # output


def initialize_centroids(K: int, datapoints: List[List[float]]) -> List[List[float]]:
	np.random.seed(0)
	centroids = list()
	centroids[0] = data_point[0]

	for i in range(1, K):
		prev_observation = centroids[i-1]
		distance_list = [calc_distance(dp, prev_observation) for dp in datapoints]
		sum_of_distances = sum(distance_list)
		probability_list = [(dist/sum_of_distances, ind) for ind, dist in enumerate(distance_list)]
		centroids[i] = datapoints[np.random.choice(probability_list)]
	
	return centroids



def calc_distance(u1: List[float], u2: List[float]) -> float:
	assert(len(u1) == len(u2)
	return sum([(u1[ind] - u2[ind]) ** 2 for ind in range(len(u1))])



def parse_datapoints(infile: str) -> List[List[float]]:
    """
    Given an input filename, parses the datapoints contained in the file
    and returns a list of them.
    """
    datapoints = list()
    dim = -1

    try:
        with open(infile) as file:
            for line in file.readlines():
                numbers = line.strip().split(",")
                if dim == -1:
                    dim = len(numbers)
                else:
                    assert_valid_input(dim == len(numbers))

                try:
                    lst = [float(number) for number in numbers]
                    datapoints.append(lst)
                except ValueError:
                    assert_valid_input(False)

    except OSError:
        assert_generic(False)

    return datapoints


def assert_valid_input(cond: bool):
    """
    If the provided condition is not satisfied, exits with the message 'Invalid Input!'
    """
    if not cond:
        print("Invalid Input!")
        sys.exit(1)


def assert_generic(cond: bool):
    """
    If the provided condition is not satisfied, exits with the message 'An Error Has Occurred'
    """
    if not cond:
        print("An Error Has Occurred")
        sys.exit(1)



def check_positive_numstr(string: str):
    """
    Checks if the input string represents a valid positive integer,
    and returns both the integer and whether it is valid.
    The value of the integer is 0 if it could not be parsed correctly.
    """
    if not string.isdecimal():
        return 0, False
    x = int(string)
    return x, (x > 0)



# Only run the program if it's actually called from the command line.
# Argument validation also happens here.
if __name__ == "__main__":
    num_args = len(sys.argv)
    assert_valid_input(num_args in [3+1, 4+1])  # +1 because of script name

    # We want K to be both numeric and positive.
    K, valid = check_positive_numstr(sys.argv[1])
    assert_valid_input(valid)

    i = 2  # allows for code deduplication later
    maxiter = 300  # default value
    if num_args == 4+1:  # the max_iter argument is present
        i = 3
        maxiter, valid = check_positive_numstr(sys.argv[2])
        assert_valid_input(valid)

    eps = sys.argv[i]
    infile1 = sys.argv[i+1]
    infile2 = sys.argv[i+2]

    # The document specified that filenames must end with .txt, so we verify this here.
    assert_valid_input((infile1.endswith((".txt", ".csv")) and infile2.endswith((".txt", ".csv")))

    main(K, eps, maxiter, infile1, infile2)
