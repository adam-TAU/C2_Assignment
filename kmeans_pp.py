import sys
from typing import List
import numpy as np
import pandas as pd
import mykmeanssp




def main(K: int, eps: float, maxiter: int, infile1: str, infile2: str) -> None:
	# reading from the input files
	try:
		df1 = pd.read_csv(infile1, header=None)
		df2 = pd.read_csv(infile2, header=None)
	except FileNotFoundError:
		assert_valid_input(False)
	joined = df1.set_index(df1.columns[0]).join(df2.set_index(df2.columns[0]), how='inner', lsuffix='_first', rsuffix='_second')
	datapoints = joined.sort_index().values.tolist()
   	
	# initializing K centroids from the observations
	observation_centroids_indices = initialize_centroids(K, datapoints)

	# We want K to be less or equal to N
	assert_valid_input(K<=len(datapoints))

	# powering the kmeans.c centroid creation module
	centroids = mykmeanssp.fit(K, len(datapoints[0]), len(datapoints), eps, maxiter, datapoints, observation_centroids_indices)
	
	# output
	output_string = str()
	output_string += ",".join([str(ele) for ele in observation_centroids_indices]) + "\n"
	output_string += "\n".join([",".join(["{:.4f}".format(num) for num in centroid]) for centroid in centroids])

	# print + newline
	print(output_string + "\n")


def initialize_centroids(K: int, datapoints: List[List[float]]) -> List[List[float]]:
	"""
	Given a list of datapoints and an amount of centroids we wish to output 'K',
	weightedly pick the initial centroids for the kmeans process we'll perform later
	"""
	np.random.seed(0)
	ind_range = list(range(len(datapoints)))
	centroids = list()
	centroids.append(int(np.random.choice(ind_range)))

	for i in range(1, K):
		distance_list = [min([calc_distance(dp, datapoints[prev_obs]) for prev_obs in centroids]) for dp in datapoints]
		sum_dist = sum(distance_list)
		probability_list = [dist/sum_dist for dist in distance_list]
		centroids.append(int(np.random.choice(ind_range, p=probability_list)))

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
	assert_valid_input(num_args in [4+1, 5+1])  # +1 because of script name

	# We want K to be both numeric and positive.
	K, valid=check_positive_numstr(sys.argv[1])
	assert_valid_input(valid)

	i=2  # allows for code deduplication later
	maxiter=300  # default value
	if num_args == 5+1:  # the max_iter argument is present
		i=3
		maxiter, valid=check_positive_numstr(sys.argv[2])
		assert_valid_input(valid)

	try:
		eps=float(sys.argv[i])
	except ValueError:
		assert_valid_input(False)
	assert_valid_input(eps >= 0)
	infile1=sys.argv[i+1]
	infile2=sys.argv[i+2]

	# The document specified that filenames must end with .txt or .csv, so we verify this here.
	assert_valid_input((infile1.endswith((".txt", ".csv"))) and (infile2.endswith((".txt", ".csv"))))

	main(K, eps, maxiter, infile1, infile2)
