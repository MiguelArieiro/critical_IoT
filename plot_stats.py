import matplotlib.pyplot as plt
from matplotlib.collections import EventCollection
import numpy as np

avg = [[2.010624535379962e-05, 1.0656701879166711e-05, 9.854565610400319e-06, 1.0468966534229665e-05, 7.765673814168674e-06, 9.785124853607777e-06, 1.0529632273691264e-05, 1.4285852951963218e-05, 1.1333283229075504e-05, 1.0821549077628036e-05,
        9.879127583759494e-06, 1.0106500179161599e-05, 1.3136192791783246e-05, 1.0725720446604208e-05, 9.639057056735098e-06, 1.206964434011144e-05, 8.786681691765658e-06, 8.042114979133494e-06, 1.1546507581030505e-05, 1.2382607992148144e-05, 0.004601154474516015]]

diversity = [[1.0, 0.9866666666666667, 0.98, 0.98, 0.97, 0.9633333333333334, 0.9733333333333334, 0.9666666666666667, 0.9633333333333334, 0.9533333333333334, 0.9833333333333333,
              0.98, 0.9566666666666667, 0.9733333333333334, 0.9666666666666667, 0.9766666666666667, 0.9666666666666667, 0.9566666666666667, 0.9633333333333334, 0.9666666666666667, 0.98]]

# plot the data
fig_div = plt.figure()

for i in range(len(diversity)):
    plt.plot(range(len(diversity[i])), diversity[i])

plt.title('Diversity')
plt.legend(["SGMO"], loc="lower right")

plt.ylabel("Diversity")
plt.xlabel("Generation")
# display the plot
plt.savefig("Diversity.jpg")


# plot the data
fig_avg = plt.figure()

for i in range(len(avg)):
    plt.plot(range(len(avg[i])), avg[i])

plt.title('Average fitness')
plt.legend(["SGMO"], loc="upper right")
plt.ylabel("Avg. Fitness")
plt.xlabel("Generation")

# save plot
plt.savefig("Average.jpg")
