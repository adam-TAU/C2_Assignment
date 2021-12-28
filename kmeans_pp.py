import sys
from typing import List
import numpy as np
import pandas as pd
import mykmeanssp




def main(K: int, eps: int, maxiter: int, infile1: str, infile2: str) -> None:
	# reading from the input files
	df1 = pd.read_csv(infile1, header=None)
	df2 = pd.read_csv(infile2, header=None)
	joined_df = pd.concat([df1, df2], axis=1, join="inner")
	datapoints = joined_df.values.tolist()
   	
	# initializing K centroids from the observations
	observation_centroids_indices = initialize_centroids(K, datapoints)

	# powering the kmeans.c centroid creation module
	centroids = mykmeanssp.kmeans.fit(K, len(datapoints[0]), len(datapoints), eps, max_iter, datapoints, observation_centroids_indices)

	# output
	output_string = str()
	output_string += ",".join(observation_centroids_indices) + "\n"
	output_string += [",".join(["{:.4f}".format(num) for num in centroid]) + "\n" for centroid in centroids]

	# return
	return output_string



def initialize_centroids(K: int, datapoints: List[List[float]]) -> List[List[float]]:
	"""
	Given a list of datapoints and an amount of centroids we wish to output 'K',
	weightedly pick the initial centroids for the kmeans process we'll perform later
	"""
	np.random.seed(0)
	ind_range = list(range(len(datapoints)))
	centroids = list()
	centroids[0] = np.random.randint(0, len(datapoints))

	for i in range(1, K):
		prev_observation = datapoints[centroids[i-1]]
		distance_list = [calc_distance(dp, prev_observation) for dp in datapoints]
		sum_of_distances = sum(distance_list)
		probability_list = [dist/sum_of_distances for dist in distance_list]
		centroids[i] = np.random.choice(ind_range, p=probability_list)

	return centroids


def calc_distance(u1: List[float], u2: List[float]) -> float:
	"""
	Given two same length vectors, calculate the euclidean norm of their distance,
	and return it as the value
	"""
	assert(len(u1) == len(u2))
	return sum([(u1[ind] - u2[ind]) ** 2 for ind in range(len(u1))])


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
	x=int(string)
	return x, (x > 0)



# Only run the program if it's actually called from the command line.
# Argument validation also happens here.
if __name__ == "__main__":
	num_args=len(sys.argv)
	assert_valid_input(num_args in [3+1, 4+1])  # +1 because of script name

	# We want K to be both numeric and positive.
	K, valid=check_positive_numstr(sys.argv[1])
	assert_valid_input(valid)

	i=2  # allows for code deduplication later
	maxiter=300  # default value
	if num_args == 4+1:  # the max_iter argument is present
		i=3
		maxiter, valid=check_positive_numstr(sys.argv[2])
		assert_valid_input(valid)

	eps=sys.argv[i]
	infile1=sys.argv[i+1]
	infile2=sys.argv[i+2]

	# The document specified that filenames must end with .txt, so we verify this here.
	assert_valid_input((infile1.endswith((".txt", ".csv"))) and (infile2.endswith((".txt", ".csv"))))

	main(K, eps, maxiter, infile1, infile2)
