import time
from datetime import datetime

path_to_file = "./test.txt"

while True:
    with open(path_to_file, "a") as f:
        f.write("The current timestamp is: " + str(datetime.now()) + "\n")
        f.close()
    time.sleep(10)
