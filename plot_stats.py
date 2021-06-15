import matplotlib.pyplot as plt
from matplotlib.collections import EventCollection
import numpy as np

runs=5

diversity=[[300, 293, 293, 293, 293, 292],
            [300, 293, 293, 293, 293, 292]]

# plot the data
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
for i in range(len(diversity)):
    ax.plot(range(len(diversity[i])), diversity[i])

    # create the events marking the x data points
    ax.add_collection(EventCollection(range(len(diversity[i])), linelength=0.05))

    # create the events marking the y data points
    ax.add_collection(EventCollection(diversity[i], linelength=0.05, orientation='vertical'))


# set the limits
# ax.set_xlim([0, 1])
# ax.set_ylim([0, 1])

ax.set_title('Diversity')

# display the plot
plt.savefig("output.jpg")