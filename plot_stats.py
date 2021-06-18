import matplotlib.pyplot as plt
from matplotlib.collections import EventCollection
import numpy as np

runs=5

diversity=[[300, 293, 297, 287, 293, 293, 291, 289, 293, 291, 293],
            [300, 294, 293, 295, 293, 294, 296, 295, 297, 295, 291]]

avg = [[1.590831301750845e-05, 1.3676922556859658e-05, 1.0439763467715613e-05, 8.219747617495894e-06, 7.567752596851802e-06, 7.2446749599144735e-06, 9.921080921722585e-06, 7.328130815881268e-06, 3.0295768040222177e-05, 1.1977936668750194e-05, 1.0883744874552183e-05],
        [6.130100572638133e-05, 1.0206937362419025e-05, 8.573787184543817e-06, 1.2816521857800207e-05, 1.4020428446953784e-05, 1.0882191646887438e-05, 1.3286548658444479e-05, 1.1594046095121384e-05, 1.1951355207583725e-05, 1.1178575612215689e-05, 9.893303726052997e-06]]

# plot the data
fig_div = plt.figure()

for i in range(len(diversity)):
    plt.plot(range(len(diversity[i])), diversity[i])

plt.title('Diversity')
plt.legend(["Single Gene M. O.", "Multi-gene M. O."], loc ="lower right")

plt.ylabel("Diversity")
plt.xlabel("Generation")

# display the plot
plt.savefig("Diversity.jpg")


# plot the data
fig_avg = plt.figure()

for i in range(len(avg)):
    plt.plot(range(len(avg[i])), avg[i])

plt.title('Average fitness')
plt.legend(["Single Gene M. O.", "Multi-gene M. O."], loc ="upper right")

plt.ylabel("Avg. Fitness")
plt.xlabel("Generation")

# save plot
plt.savefig("Average.jpg")