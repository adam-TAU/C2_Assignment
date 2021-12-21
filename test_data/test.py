import pandas as pd




df1 = pd.read_csv("test_data/input_1_db_1.txt", header=None)
df2 = pd.read_csv("test_data/input_1_db_1.txt", header=None)


result = pd.concat([df1, df2], axis=1, join="inner").values.tolist()
print(result)

print("\n\n\n" + str(type(result[0][0])))
