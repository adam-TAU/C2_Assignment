from setuptools import setup, find_packages, Extension


setup(
	name='mykmeanssp',
	version='0.0.1',
	description='a kmeans++ algorithm',
	packages = find_packages(),
    license = 'GPL-2',
    ext_modules=[
        Extension(
            'mykmeanssp',
            ['kmeans.c'],
        )
    ]
)
