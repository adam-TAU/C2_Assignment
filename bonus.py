from matplotlib import pyplot as plt
from matplotlib.patches import Circle, FancyArrowPatch
from sklearn.datasets import load_iris
from sklearn.cluster import KMeans
from typing import List
import numpy as np


def run() -> None:
	"""
	The main function of the file. This function gets powered
	when the file is ran as whole. No Arguments Accepted, and nothing is returned.
	At the end of the running of the function, a file named elbow.png will be saved
	within the directory that the file was placed at, showing the optimal K for the 
	K-means clusterization.
	"""
	# loading the needed data
	k_vals = np.arange(2,11)
	iris_data = load_iris().data
	prev: list
	circle: plt.Circle
	
	# iterating over the different K values - the number of clusters/centroids
	for ind, k in enumerate(k_vals):
		curr = [k, KMeans(n_clusters = k,  init = 'k-means++', random_state=0).fit(iris_data).inertia_ / len(iris_data)]
		if ind != 0:
			plt.plot([prev[0], curr[0]], [prev[1], curr[1]], color="blue")
		prev = curr
		
		# annotate the circle for K=2 (since its the most fitted one by how the question was worded)
		if k == 2:
			circle = Circle(tuple(curr), 0.3, facecolor="#ffffff", linewidth=1, alpha=1, linestyle = '-', edgecolor='black')
			arrow = FancyArrowPatch((curr[0] + 0.35, curr[1] + 0.35), (curr[0] + 2.5, curr[1] + 2.5), mutation_scale=30, arrowstyle="<|-", linewidth=2, linestyle="--")
			ax = plt.gca()
			ax.add_patch(circle)
			ax.add_patch(arrow)
			plt.text(curr[0] + 2.75, curr[1] + 2.75, "Elbow Point", fontsize=15)
			
			
	
	# pyplot manipulations to find and annotate the elbow
	plt.title("Elbow Method for Selection fo optimal \"K\" clusters")
	plt.ylabel("Average Dispresion")
	plt.xlabel("------> K")
	plt.locator_params(axis='x', nbins=10)
	plt.savefig("elbow.png")
	
	
	
if __name__ == "__main__":
	run()
