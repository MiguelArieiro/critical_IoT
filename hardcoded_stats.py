import numpy
__author__ = 'Miguel Arieiro'

verbose = True

#parameters
pop_size = 25
number_generations = 5
runs_per_scen = 5
elite_per = 0.3
random_per = 0.2
minimum_throughput = 0.0
mutation_prob = 0.1

best_per_gen = list()
avg_per_gen = list()
diversity = list()
avgBest = 0
stdBest = 0
avgGen = 0
stdGen = 0
avgDiversity = 0
stdDiversity = 0
maxFit = 0
minFit = 0
genMin = 0
genMax = 0

def hamming_distance(v1, v2):
    return sum([1 for i in range (len(v1)-1) if v1[i] != v2 [i]]) # heuristic = v[-1]

def reset_stats():
    global best_per_gen
    global avg_per_gen
    global diversity
    global avgBest
    global stdBest
    global avgGen
    global stdGen
    global maxFit
    global minFit
    global genMin
    global genMax
    global avgDiversity
    global stdDiversity

    best_per_gen = list()
    avg_per_gen = list()
    diversity = list()
    avgBest = 0
    stdBest = 0
    avgGen = 0
    stdGen = 0
    avgDiversity = 0
    stdDiversity = 0
    maxFit = 0
    minFit = 0
    genMin = 0
    genMax = 0

def update_stats(population, metric=hamming_distance):

    global best_per_gen
    global avg_per_gen
    global diversity

    population.sort(key=lambda x: x[-1])
    best_per_gen.append(population[0][-1])
    avg_per_gen.append(sum([f[-1] for f in population]) / len(population))
    
    div=0
    for i in range (len(population)):
        for j in range (i+1, len (population)):
            distance = metric(population[i], population[j])
            if distance !=0:
                div+=1
    
    diversity.append(div)

def calculate_stats():

    global best_per_gen
    global avg_per_gen
    global diversity
    global avgBest
    global stdBest
    global avgGen
    global stdGen
    global maxFit
    global minFit
    global genMin
    global genMax
    global avgDiversity
    global stdDiversity
    
    avgBest = numpy.mean(best_per_gen)
    stdBest = numpy.std(best_per_gen)
    avgGen = numpy.mean(avg_per_gen)
    stdGen = numpy.std(avg_per_gen)
    maxFit = max(best_per_gen)
    minFit = min(best_per_gen)
    genMax = best_per_gen.index(maxFit)
    genMin = best_per_gen.index(minFit)
    avgDiversity = numpy.mean(diversity)
    stdDiversity = numpy.std(diversity)

def stats_string():
    global best_per_gen
    global avg_per_gen
    global diversity
    global avgBest
    global stdBest
    global avgGen
    global stdGen
    global maxFit
    global minFit
    global genMin
    global genMax

    string = 'max: {} -> gen: {}/{}'.format(maxFit, genMax, number_generations)
    string += '\nmin: {} -> gen: {}/{}'.format(minFit, genMin, number_generations)
    string += '\navgBest: {} +- {}'.format(avgBest, stdBest)
    string += '\navgGen: {} +- {}'.format(avgGen, stdGen)
    string += '\nAvgDiversity: {} +- {}'.format(avgDiversity, stdDiversity)
    return string

def main():
    global pop_size
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput
    global runs_per_scen
    global mutation_prob
    metric = hamming_distance

    global mutation_prob
    population = [[1, 5, 40, 1, 0, 0, 0, 1, 13, 9.622488685058805e-06], [1, 5, 40, 1, 0, 0, 0, 0, 3, 9.705182160975903e-06], [1, 5, 40, 0, 0, 0, 0, 1, 15, 8.125000000000001e-06], [0, 5, 160, 0, 1, 1600, 1, 0, 9, 3.2750493889293676e-05], [1, 5, 80, 0, 0, 0, 1, 0, 27, 8.134318303249937e-06], [0, 6, 20, 1, 1, 1600, 0, 0, 8, 9.561248447102993e-06], [0, 6, 160, 0, 1, 3200, 1, 1, 4, 8.182019568026584e-06], [1, 5, 160, 1, 0, 0, 1, 0, 16, 9.80960251309359e-06], [1, 5, 160, 0, 0, 0, 1, 0, 3, 8.172349010679846e-06], [1, 5, 20, 1, 0, 1, 1, 1, 4, 9.662604050940494e-06], [0, 5, 160, 0, 0, 1600, 0, 1, 10, 3.2765509989484754e-05], [0, 5, 20, 0, 0, 1600, 0, 1, 2, 8.275839352516293e-06], [0, 6, 80, 0, 1, 3200, 1, 1, 2, 8.242474349387127e-06], [1, 5, 160, 0, 0, 0, 1, 1, 20, 8.08288863858781e-06], [0, 5, 40, 0, 1, 3200, 0, 0, 9, 3.275762993232027e-05], [1, 5, 80, 1, 1, 1, 0, 1, 1, 9.946257449801065e-06], [0, 5, 40, 1, 1, 3200, 1, 1, 9, 9.563930101848152e-06], [1, 5, 80, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 20, 0, 0, 0, 0, 1, 5, 8.261852291348817e-06], [0, 6, 40, 0, 1, 800, 0, 1, 0, 8.281781511980223e-06], [1, 5, 160, 0, 1, 0, 0, 1, 22, 8.118723653774234e-06], [1, 5, 20, 1, 1, 0, 1, 1, 3, 9.786781083935794e-06], [1, 5, 80, 1, 1, 1, 1, 0, 8, 1.0035463515813554e-05], [0, 5, 20, 1, 1, 1600, 0, 1, 8, 9.561193719455132e-06], [0, 5, 40, 1, 0, 1600, 1, 0, 8, 9.531914427849805e-06]]
    update_stats(population, metric)
    population = [[1, 5, 160, 0, 0, 1, 1, 1, 20, 8.08288863858781e-06], [1, 5, 160, 0, 0, 1, 1, 1, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 0, 1, 1, 27, 8.134318303249937e-06], [1, 5, 80, 0, 0, 0, 1, 1, 27, 8.134318303249937e-06], [1, 5, 160, 0, 0, 0, 0, 0, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 0, 3, 8.172349010679846e-06], [0, 5, 160, 0, 1, 1600, 0, 1, 4, 8.178112984597205e-06], [0, 5, 160, 0, 1, 1600, 0, 1, 4, 8.178112984597205e-06], [0, 6, 20, 0, 1, 3200, 1, 1, 4, 8.182019568026584e-06], [0, 6, 20, 0, 1, 3200, 1, 1, 4, 8.182019568026584e-06], [1, 5, 20, 0, 1, 0, 0, 1, 5, 8.199716417563515e-06], [1, 5, 20, 0, 1, 0, 0, 1, 5, 8.199716417563515e-06], [1, 5, 80, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 80, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 160, 0, 0, 0, 1, 0, 16, 8.261666117818398e-06], [0, 6, 20, 0, 0, 1600, 0, 1, 2, 8.275839352516293e-06], [0, 6, 40, 0, 1, 3200, 0, 1, 0, 8.343844152927746e-06], [1, 5, 160, 0, 1, 1, 0, 0, 1, 8.377251687022475e-06], [1, 5, 80, 0, 1, 1, 1, 0, 8, 8.446475984989747e-06], [0, 5, 160, 1, 0, 800, 0, 0, 8, 9.53125769607548e-06], [0, 5, 40, 1, 0, 1600, 0, 0, 8, 9.531914427849805e-06], [0, 5, 40, 1, 1, 1600, 0, 1, 8, 9.561193719455132e-06], [1, 5, 80, 1, 0, 0, 0, 1, 29, 9.617563196751367e-06], [0, 5, 20, 0, 1, 3200, 1, 1, 6, 1.0969332554003806e-05], [0, 5, 80, 0, 1, 1600, 1, 0, 8, 3.2750493889293676e-05]]
    update_stats(population, metric)
    population = [[0, 6, 20, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 6, 20, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 5, 40, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 5, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 5, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [1, 5, 80, 0, 0, 1, 1, 1, 27, 8.134318303249937e-06], [1, 5, 80, 0, 0, 1, 1, 1, 27, 8.134318303249937e-06], [1, 5, 160, 0, 0, 1, 1, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 1, 1, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [0, 5, 80, 0, 1, 1600, 0, 0, 4, 8.175981907970646e-06], [0, 5, 80, 0, 1, 1600, 0, 0, 4, 8.175981907970646e-06], [0, 6, 40, 0, 1, 3200, 1, 1, 4, 8.182019568026584e-06], [0, 5, 20, 0, 1, 3200, 0, 1, 4, 8.182019568026584e-06], [1, 5, 20, 0, 1, 0, 1, 1, 5, 8.199716417563515e-06], [0, 6, 20, 0, 1, 3200, 1, 1, 2, 8.242474349387127e-06], [0, 6, 40, 0, 1, 3200, 0, 0, 0, 8.242916849470063e-06], [1, 5, 20, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 160, 0, 0, 1, 1, 0, 16, 8.261666117818398e-06], [0, 5, 160, 0, 1, 1600, 1, 1, 0, 8.283389635191836e-06], [0, 5, 160, 0, 0, 1600, 0, 1, 4, 8.322033175557546e-06], [0, 6, 20, 1, 0, 3200, 1, 1, 0, 9.730904155470302e-06]]
    update_stats(population, metric)
    population = [[1, 5, 80, 0, 0, 1, 1, 1, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 1, 1, 1, 20, 8.08288863858781e-06], [0, 5, 80, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 5, 80, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 6, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 6, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [1, 5, 80, 0, 1, 1, 1, 1, 27, 8.129912714173555e-06], [1, 5, 80, 0, 1, 1, 1, 1, 27, 8.129912714173555e-06], [0, 5, 80, 0, 0, 1600, 0, 0, 4, 8.161118075869434e-06], [0, 5, 80, 0, 0, 1600, 0, 0, 4, 8.161118075869434e-06], [1, 5, 20, 0, 1, 1, 0, 0, 31, 8.165579588954547e-06], [1, 5, 20, 0, 1, 1, 0, 0, 31, 8.165579588954547e-06], [1, 5, 20, 0, 0, 1, 1, 1, 3, 8.172349010679846e-06], [1, 5, 20, 0, 0, 1, 1, 1, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 0, 0, 0, 3, 8.172349010679846e-06], [1, 5, 160, 0, 0, 1, 1, 0, 3, 8.172349010679846e-06], [1, 5, 80, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [0, 5, 80, 0, 1, 1600, 0, 1, 4, 8.178112984597205e-06], [0, 6, 40, 0, 1, 3200, 0, 1, 4, 8.182019568026584e-06], [0, 5, 20, 0, 1, 3200, 1, 1, 4, 8.182019568026584e-06], [0, 6, 80, 1, 1, 1600, 0, 1, 8, 9.561193719455132e-06], [0, 5, 20, 1, 0, 3200, 1, 1, 11, 9.562945004186666e-06], [0, 6, 80, 1, 0, 3200, 1, 1, 7, 9.718645162349567e-06], [0, 5, 160, 1, 0, 3200, 1, 1, 7, 9.718645162349567e-06], [1, 5, 160, 1, 0, 1, 1, 1, 31, 9.741466591507363e-06]]
    update_stats(population, metric)
    population = [[1, 5, 80, 0, 0, 1, 1, 0, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 1, 1, 0, 20, 8.08288863858781e-06], [0, 5, 80, 0, 0, 3200, 1, 1, 4, 8.114904984014062e-06], [0, 5, 80, 0, 0, 3200, 1, 1, 4, 8.114904984014062e-06], [0, 5, 20, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 5, 20, 0, 0, 3200, 0, 1, 2, 8.124561630181993e-06], [0, 5, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 6, 40, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 5, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 6, 40, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [1, 5, 80, 0, 1, 1, 0, 1, 27, 8.129912714173555e-06], [1, 5, 80, 0, 1, 1, 0, 1, 27, 8.129912714173555e-06], [1, 5, 80, 0, 0, 1, 1, 1, 27, 8.134318303249937e-06], [1, 5, 80, 0, 0, 1, 1, 1, 27, 8.134318303249937e-06], [0, 5, 160, 0, 0, 1600, 1, 0, 3, 8.163910675229511e-06], [1, 5, 20, 0, 1, 1, 0, 1, 31, 8.165579588954547e-06], [1, 5, 20, 0, 1, 0, 0, 0, 31, 8.165579588954547e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 3, 8.166658152958823e-06], [1, 5, 20, 0, 0, 1, 0, 1, 3, 8.172349010679846e-06], [1, 5, 20, 0, 0, 0, 1, 1, 3, 8.172349010679846e-06], [1, 5, 80, 1, 0, 0, 1, 0, 12, 9.6245683356775e-06], [1, 5, 40, 1, 0, 1, 1, 1, 17, 9.691062427827914e-06], [1, 5, 80, 1, 0, 1, 0, 1, 30, 9.742889510351734e-06], [0, 5, 20, 1, 1, 800, 1, 0, 3, 9.780323221488263e-06], [1, 5, 160, 1, 1, 1, 1, 0, 24, 9.921520552968154e-06]]
    update_stats(population, metric)
    population = [[0, 6, 40, 0, 0, 1600, 1, 1, 5, 8.040401088564592e-06], [0, 6, 40, 0, 0, 1600, 1, 1, 5, 8.040401088564592e-06], [1, 5, 80, 0, 0, 1, 0, 0, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 1, 0, 0, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 1, 0, 0, 20, 8.08288863858781e-06], [1, 5, 80, 0, 0, 1, 0, 0, 20, 8.08288863858781e-06], [0, 5, 80, 0, 0, 3200, 1, 0, 4, 8.115040736674079e-06], [0, 5, 80, 0, 0, 3200, 1, 0, 4, 8.115040736674079e-06], [0, 6, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 6, 40, 0, 0, 800, 0, 1, 5, 8.129224826617506e-06], [0, 6, 160, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 6, 40, 0, 0, 800, 0, 1, 5, 8.129224826617506e-06], [1, 5, 80, 0, 1, 1, 0, 0, 27, 8.129912714173555e-06], [1, 5, 80, 0, 1, 1, 0, 0, 27, 8.129912714173555e-06], [1, 5, 40, 0, 1, 1, 0, 1, 27, 8.129912714173555e-06], [1, 5, 80, 0, 1, 1, 1, 1, 27, 8.129912714173555e-06], [1, 5, 80, 0, 1, 1, 1, 1, 27, 8.129912714173555e-06], [1, 5, 20, 0, 1, 1, 1, 1, 31, 8.165579588954547e-06], [1, 5, 20, 0, 1, 0, 0, 1, 31, 8.165579588954547e-06], [0, 5, 20, 0, 1, 800, 1, 0, 3, 8.221385178442045e-06], [1, 5, 20, 0, 0, 1, 0, 1, 1, 8.318636068998644e-06], [1, 5, 20, 1, 0, 0, 1, 0, 21, 9.619916485609365e-06], [1, 5, 20, 1, 1, 0, 1, 0, 19, 9.707754360425343e-06], [1, 5, 160, 0, 0, 0, 0, 1, 30, 2.0533007174555866e-05], [0, 6, 160, 0, 0, 3200, 1, 1, 11, 3.277358419708577e-05]]
    update_stats(population, metric)

    calculate_stats()

    if verbose:
        print(stats_string())
    
    directory = "/mnt/d/Users/Miguel/Documents/Engenharia Informática/UC/Ano 5/IoT/cenario_IoT/experimentation/evo/"
    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, "mutate_one", mutation_prob)
    file_path = directory + filename
    with open(file_path, 'w+') as file:
        file.write(stats_string()+'\n')

    #next
    reset_stats()
    population = [[0, 6, 40, 0, 1, 800, 0, 0, 6, 1.0876418896434427e-05], [1, 5, 20, 0, 0, 1, 0, 0, 9, 8.384451478495261e-06], [1, 5, 40, 0, 0, 0, 1, 0, 5, 8.261852291348817e-06], [0, 6, 80, 1, 1, 3200, 0, 0, 8, 9.563930101848152e-06], [1, 5, 40, 1, 1, 1, 1, 1, 28, 9.693579899629493e-06], [0, 6, 40, 1, 1, 800, 1, 1, 1, 9.798273889986483e-06], [1, 5, 80, 0, 0, 0, 0, 1, 30, 2.0533007174555866e-05], [1, 5, 40, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 20, 1, 1, 0, 1, 0, 2, 9.862414693278897e-06], [1, 5, 80, 0, 1, 1, 0, 0, 18, 8.176625009243512e-06], [1, 5, 160, 0, 0, 0, 0, 1, 1, 8.318636068998644e-06], [0, 5, 20, 1, 1, 3200, 1, 1, 8, 9.563930101848152e-06], [1, 5, 160, 0, 1, 0, 1, 0, 26, 8.156754609614603e-06], [0, 6, 160, 1, 0, 800, 1, 0, 1, 9.688709138969916e-06], [1, 5, 80, 0, 1, 0, 0, 0, 21, 8.168370191414234e-06], [0, 6, 20, 1, 1, 3200, 0, 0, 4, 9.801776459449549e-06], [1, 5, 20, 1, 0, 0, 1, 0, 19, 9.636170597023911e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [0, 5, 80, 1, 0, 800, 0, 1, 4, 9.679679077072947e-06], [1, 5, 160, 0, 0, 0, 1, 0, 2, 8.301164273126102e-06], [1, 5, 40, 1, 0, 0, 0, 0, 0, 1.0096265932586483e-05], [0, 6, 80, 1, 1, 1600, 0, 0, 0, 9.866792905107732e-06], [0, 5, 20, 0, 1, 800, 0, 1, 4, 8.222976770543507e-06], [1, 5, 80, 0, 1, 0, 0, 0, 4, 8.330396185162847e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06]]
    update_stats(population, metric)
    population = [[1, 5, 80, 0, 0, 0, 0, 0, 13, 8.127897362529148e-06], [1, 5, 80, 0, 0, 0, 0, 0, 13, 8.127897362529148e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 80, 0, 1, 0, 0, 1, 21, 8.168370191414234e-06], [1, 5, 80, 0, 1, 0, 0, 1, 21, 8.168370191414234e-06], [1, 5, 80, 0, 1, 1, 0, 0, 18, 8.176625009243512e-06], [1, 5, 80, 0, 1, 1, 0, 0, 18, 8.176625009243512e-06], [1, 5, 40, 0, 1, 0, 0, 1, 6, 8.252486252486252e-06], [1, 5, 40, 0, 1, 0, 0, 1, 6, 8.252486252486252e-06], [1, 5, 40, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 40, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 0, 8.284259289375009e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 0, 8.284259289375009e-06], [1, 5, 20, 0, 1, 1, 0, 1, 9, 8.305252033353042e-06], [1, 5, 160, 0, 0, 1, 0, 1, 1, 8.318636068998644e-06], [1, 5, 160, 0, 1, 1, 1, 0, 2, 8.324713928957471e-06], [1, 5, 80, 0, 1, 1, 0, 0, 4, 8.330396185162847e-06], [0, 5, 20, 1, 0, 3200, 1, 1, 8, 9.533282619046316e-06], [0, 5, 20, 1, 1, 800, 0, 0, 11, 9.583030050951439e-06], [1, 5, 40, 1, 1, 0, 1, 1, 13, 9.693744082573075e-06], [0, 5, 40, 1, 1, 800, 1, 1, 6, 9.738949119705784e-06], [0, 5, 20, 1, 1, 800, 1, 1, 5, 9.812557806078053e-06], [0, 6, 80, 0, 0, 1600, 1, 0, 7, 2.1766077258703834e-05], [0, 6, 20, 0, 1, 3200, 1, 1, 9, 3.27574421417143e-05]]
    update_stats(population, metric)
    population = [[1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 40, 0, 0, 0, 0, 1, 6, 8.155475619504395e-06], [1, 5, 40, 0, 0, 0, 0, 1, 6, 8.155475619504395e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 18, 8.176625009243512e-06], [1, 5, 80, 0, 1, 0, 0, 0, 18, 8.176625009243512e-06], [1, 5, 80, 0, 1, 0, 1, 1, 3, 8.225022227161592e-06], [1, 5, 80, 0, 1, 0, 1, 1, 3, 8.225022227161592e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 2, 8.237586873304346e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 2, 8.237586873304346e-06], [1, 5, 40, 0, 1, 0, 0, 1, 6, 8.252486252486252e-06], [1, 5, 40, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 40, 0, 1, 1, 0, 0, 17, 8.252986824475212e-06], [1, 5, 80, 0, 0, 0, 0, 0, 18, 8.270999877592912e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 0, 8.284259289375009e-06], [1, 5, 20, 0, 1, 1, 0, 1, 9, 8.305252033353042e-06], [0, 5, 160, 0, 1, 800, 1, 0, 4, 8.32899511892042e-06], [1, 5, 40, 0, 1, 0, 1, 0, 1, 8.377251687022475e-06], [0, 6, 80, 1, 1, 3200, 1, 1, 1, 9.813980724922423e-06], [0, 5, 80, 1, 1, 800, 0, 0, 0, 9.861101229730247e-06], [1, 5, 80, 0, 0, 1, 0, 1, 23, 1.90689784409687e-05]]
    update_stats(population, metric)
    population = [[1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 40, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 40, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 40, 0, 0, 0, 0, 1, 6, 8.155475619504395e-06], [1, 5, 40, 0, 0, 0, 0, 1, 6, 8.155475619504395e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 18, 8.176625009243512e-06], [1, 5, 80, 0, 1, 0, 0, 0, 18, 8.176625009243512e-06], [1, 5, 80, 0, 1, 0, 0, 0, 19, 8.190451840833023e-06], [0, 6, 160, 0, 1, 3200, 0, 0, 2, 8.192597036968662e-06], [1, 5, 20, 0, 1, 0, 1, 1, 3, 8.225022227161592e-06], [0, 5, 80, 0, 1, 1600, 1, 1, 3, 8.22660212935952e-06], [0, 6, 80, 0, 1, 1600, 0, 0, 2, 8.237586873304346e-06], [1, 5, 160, 0, 0, 1, 1, 1, 10, 8.238127109085039e-06], [1, 5, 40, 0, 1, 1, 0, 1, 6, 8.252486252486252e-06], [0, 5, 40, 1, 1, 800, 0, 1, 9, 9.559880255906482e-06], [1, 5, 40, 1, 0, 1, 1, 0, 21, 9.619916485609365e-06], [0, 6, 20, 1, 1, 800, 0, 0, 5, 9.814637456696748e-06], [0, 6, 80, 0, 1, 800, 0, 0, 8, 3.2745799124144616e-05]]
    update_stats(population, metric)
    population = [[0, 5, 160, 0, 0, 3200, 0, 0, 2, 8.125582640318248e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 2, 8.125582640318248e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 40, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 40, 0, 0, 0, 0, 1, 17, 8.149510144042631e-06], [1, 5, 40, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 40, 0, 0, 0, 0, 1, 17, 8.149510144042631e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 13, 8.171630348929053e-06], [1, 5, 40, 0, 1, 1, 1, 1, 13, 8.171630348929053e-06], [1, 5, 80, 0, 1, 0, 0, 0, 18, 8.176625009243512e-06], [0, 5, 80, 0, 1, 1600, 1, 1, 4, 8.178112984597205e-06], [0, 5, 40, 0, 0, 800, 0, 1, 4, 8.211262800229948e-06], [1, 5, 20, 0, 1, 0, 1, 0, 3, 8.225022227161592e-06], [0, 6, 80, 1, 1, 1600, 1, 0, 8, 9.561248447102993e-06], [0, 5, 160, 1, 0, 1600, 0, 0, 1, 9.69188334254582e-06], [1, 5, 160, 1, 1, 1, 0, 1, 6, 9.70600307569381e-06], [1, 5, 80, 0, 0, 0, 1, 0, 23, 1.90689784409687e-05]]
    update_stats(population, metric)
    population = [[0, 5, 160, 0, 0, 800, 1, 0, 2, 8.117445143669145e-06], [0, 5, 160, 0, 0, 800, 1, 0, 2, 8.117445143669145e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 2, 8.125582640318248e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 2, 8.125582640318248e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 1, 1, 0, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 0, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 40, 0, 0, 0, 0, 1, 17, 8.149510144042631e-06], [1, 5, 20, 0, 0, 1, 0, 0, 17, 8.149510144042631e-06], [1, 5, 80, 0, 1, 0, 0, 1, 13, 8.171630348929053e-06], [1, 5, 20, 0, 0, 0, 1, 0, 3, 8.172349010679846e-06], [1, 5, 40, 0, 1, 0, 1, 1, 6, 8.252486252486252e-06], [1, 5, 160, 1, 0, 1, 1, 0, 18, 9.663041872123379e-06], [1, 5, 20, 1, 1, 1, 0, 1, 6, 9.70600307569381e-06], [1, 5, 80, 1, 1, 0, 1, 1, 1, 9.946257449801065e-06], [1, 5, 20, 0, 0, 1, 0, 1, 31, 2.0857138779607157e-05]]
    update_stats(population, metric)

    calculate_stats()

    if verbose:
        print(stats_string())
    
    directory = "/mnt/d/Users/Miguel/Documents/Engenharia Informática/UC/Ano 5/IoT/cenario_IoT/experimentation/evo/"
    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, "mutate_prob", mutation_prob)
    file_path = directory + filename
    with open(file_path, 'w+') as file:
        file.write(stats_string()+'\n')

    #next
    mutation_prob = 0.3

    reset_stats()
    population = [[1, 5, 80, 0, 1, 0, 0, 1, 15, 8.121450536055627e-06], [0, 6, 160, 1, 0, 1600, 0, 1, 7, 9.531914427849805e-06], [0, 5, 40, 1, 1, 1600, 1, 1, 0, 9.868325279247823e-06], [1, 5, 40, 0, 0, 1, 1, 1, 4, 8.2278279590022e-06], [0, 5, 80, 0, 1, 3200, 1, 0, 0, 8.242916849470063e-06], [1, 5, 40, 0, 1, 0, 1, 0, 25, 8.228669211775703e-06], [0, 5, 20, 0, 0, 3200, 0, 0, 9, 3.267444043863602e-05], [0, 5, 160, 1, 1, 800, 0, 0, 3, 9.780323221488263e-06], [1, 5, 160, 0, 1, 0, 1, 0, 27, 8.129912714173555e-06], [0, 6, 20, 0, 0, 1600, 1, 1, 4, 8.322033175557546e-06], [0, 6, 20, 1, 1, 1600, 0, 1, 2, 9.804950663025455e-06], [1, 5, 160, 0, 0, 0, 0, 1, 7, 8.145046610838994e-06], [0, 6, 20, 1, 1, 1600, 1, 1, 3, 9.786562173344352e-06], [0, 6, 80, 0, 1, 1600, 1, 1, 11, 3.2808322066997144e-05], [0, 6, 40, 1, 0, 800, 0, 0, 8, 9.53125769607548e-06], [1, 5, 160, 0, 0, 1, 0, 1, 4, 8.2278279590022e-06], [0, 5, 40, 0, 1, 3200, 0, 1, 4, 8.182019568026584e-06], [1, 5, 20, 1, 1, 0, 0, 1, 3, 9.786781083935794e-06], [0, 5, 20, 1, 1, 800, 1, 0, 4, 9.78393524624705e-06], [0, 6, 160, 1, 1, 3200, 1, 0, 7, 9.783552152712028e-06], [0, 5, 80, 1, 1, 1600, 0, 1, 11, 9.578542383826886e-06], [0, 6, 40, 0, 1, 1600, 0, 1, 8, 3.2750681679899645e-05], [1, 5, 20, 1, 0, 1, 0, 0, 3, 9.705182160975903e-06], [0, 5, 20, 1, 1, 3200, 1, 0, 6, 9.730904155470302e-06], [1, 5, 40, 0, 1, 0, 1, 1, 12, 8.18507090946265e-06]]
    update_stats(population, metric)
    population = [[0, 5, 20, 0, 0, 3200, 1, 1, 4, 8.114904984014062e-06], [0, 5, 20, 0, 0, 3200, 1, 1, 4, 8.114904984014062e-06], [1, 5, 20, 0, 1, 1, 1, 1, 15, 8.121450536055627e-06], [1, 5, 20, 0, 1, 1, 1, 1, 15, 8.121450536055627e-06], [1, 5, 40, 0, 1, 1, 0, 1, 27, 8.129912714173555e-06], [1, 5, 40, 0, 1, 1, 0, 1, 27, 8.129912714173555e-06], [1, 5, 40, 0, 0, 1, 0, 0, 27, 8.134318303249937e-06], [1, 5, 20, 0, 1, 1, 0, 1, 7, 8.142036109622456e-06], [1, 5, 20, 0, 1, 1, 0, 1, 7, 8.142036109622456e-06], [1, 5, 40, 0, 0, 0, 1, 1, 25, 8.172892308407523e-06], [1, 5, 40, 0, 0, 0, 1, 1, 25, 8.172892308407523e-06], [1, 5, 80, 0, 1, 1, 0, 1, 12, 8.18507090946265e-06], [1, 5, 80, 0, 1, 1, 0, 1, 12, 8.18507090946265e-06], [0, 6, 20, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [0, 6, 20, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [1, 5, 20, 0, 0, 0, 0, 1, 4, 8.2278279590022e-06], [0, 6, 20, 0, 1, 3200, 0, 1, 3, 8.231378778589079e-06], [0, 5, 80, 0, 0, 3200, 0, 1, 0, 8.251582796382713e-06], [1, 5, 20, 0, 0, 0, 0, 1, 18, 8.270999877592912e-06], [1, 5, 80, 0, 1, 1, 0, 0, 4, 8.330396185162847e-06], [0, 6, 40, 1, 0, 1600, 0, 0, 7, 9.531914427849805e-06], [0, 6, 20, 1, 1, 800, 0, 0, 8, 9.5597160729629e-06], [0, 6, 160, 1, 1, 800, 1, 1, 7, 9.559825528258621e-06], [0, 5, 160, 1, 0, 1600, 0, 0, 0, 9.72280446358696e-06], [1, 5, 40, 1, 1, 0, 1, 0, 2, 9.862414693278897e-06]]
    update_stats(population, metric)
    population = [[0, 5, 40, 0, 0, 1600, 0, 0, 2, 8.118599029359119e-06], [0, 5, 40, 0, 0, 1600, 0, 0, 2, 8.118599029359119e-06], [0, 5, 40, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [0, 5, 40, 0, 0, 800, 1, 1, 5, 8.129224826617506e-06], [1, 5, 20, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 20, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 0, 1, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 0, 1, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 1, 0, 1, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [1, 5, 20, 0, 0, 0, 0, 1, 3, 8.172349010679846e-06], [0, 5, 160, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [0, 5, 160, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [0, 6, 20, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [1, 5, 80, 0, 1, 1, 0, 1, 10, 8.201611187672843e-06], [1, 5, 40, 0, 0, 1, 0, 0, 18, 8.270999877592912e-06], [1, 5, 80, 0, 1, 1, 0, 1, 24, 8.363514482296312e-06], [0, 5, 40, 0, 1, 1600, 1, 1, 5, 9.039949640729594e-06], [0, 5, 20, 1, 0, 800, 0, 1, 8, 9.53125769607548e-06], [0, 6, 40, 1, 1, 3200, 1, 1, 10, 9.595343771720035e-06], [1, 5, 40, 1, 0, 0, 0, 1, 28, 9.619095570891459e-06], [0, 5, 80, 1, 0, 3200, 0, 1, 2, 9.702117412695719e-06], [0, 6, 20, 1, 1, 800, 0, 0, 2, 9.798711711169366e-06], [0, 5, 80, 0, 0, 800, 0, 1, 6, 1.080160616847525e-05]]
    update_stats(population, metric)
    population = [[1, 5, 160, 0, 0, 1, 0, 1, 29, 8.069229952890041e-06], [1, 5, 160, 0, 0, 1, 0, 1, 29, 8.069229952890041e-06], [0, 5, 20, 0, 0, 800, 0, 0, 2, 8.117445143669145e-06], [0, 5, 20, 0, 0, 800, 0, 0, 2, 8.117445143669145e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 5, 8.140763005780346e-06], [0, 5, 160, 0, 0, 3200, 0, 0, 5, 8.140763005780346e-06], [1, 5, 20, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 20, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 80, 0, 0, 0, 1, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 1, 0, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 0, 0, 1, 7, 8.145046610838994e-06], [1, 5, 80, 0, 0, 0, 1, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 1, 0, 0, 7, 8.145046610838994e-06], [1, 5, 20, 0, 0, 0, 0, 1, 7, 8.145046610838994e-06], [0, 5, 20, 0, 0, 3200, 1, 1, 5, 8.19362917378493e-06], [1, 5, 20, 0, 1, 0, 1, 1, 3, 8.225022227161592e-06], [0, 5, 40, 0, 1, 3200, 1, 0, 5, 8.312193567136335e-06], [0, 5, 40, 0, 1, 800, 0, 1, 1, 8.338464000602501e-06], [0, 5, 80, 0, 0, 800, 0, 0, 5, 8.913491821599107e-06], [0, 6, 160, 0, 1, 1600, 0, 1, 5, 9.039949640729594e-06], [0, 5, 20, 1, 0, 800, 1, 1, 8, 9.53125769607548e-06], [0, 5, 40, 1, 0, 3200, 1, 0, 9, 9.533282619046316e-06], [0, 6, 80, 1, 0, 3200, 0, 0, 8, 9.533282619046316e-06], [0, 5, 20, 1, 0, 800, 0, 1, 11, 9.562452455355921e-06], [0, 6, 160, 1, 0, 3200, 0, 0, 1, 9.699381030302699e-06]]
    update_stats(population, metric)
    population = [[0, 5, 160, 0, 0, 1600, 0, 1, 5, 8.040401088564592e-06], [0, 5, 20, 0, 0, 800, 0, 0, 2, 8.117445143669145e-06], [0, 5, 20, 0, 0, 800, 0, 0, 2, 8.117445143669145e-06], [1, 5, 40, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 40, 0, 1, 1, 1, 0, 7, 8.142036109622456e-06], [1, 5, 160, 0, 1, 0, 0, 0, 7, 8.142036109622456e-06], [1, 5, 80, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 80, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 1, 1, 0, 0, 31, 8.165579588954547e-06], [1, 5, 20, 0, 1, 1, 1, 1, 5, 8.199716417563515e-06], [1, 5, 20, 0, 1, 1, 1, 1, 5, 8.199716417563515e-06], [0, 6, 20, 0, 1, 3200, 1, 1, 2, 8.242474349387127e-06], [0, 6, 20, 0, 1, 3200, 1, 1, 2, 8.242474349387127e-06], [0, 5, 20, 0, 0, 1600, 0, 1, 2, 8.275839352516293e-06], [0, 5, 20, 0, 0, 1600, 0, 1, 2, 8.275839352516293e-06], [0, 6, 160, 0, 1, 3200, 0, 0, 5, 8.312193567136335e-06], [0, 6, 160, 0, 1, 3200, 0, 0, 5, 8.312193567136335e-06], [1, 5, 20, 0, 1, 1, 0, 1, 0, 8.682335145089233e-06], [0, 5, 160, 0, 1, 1600, 0, 1, 5, 9.039949640729594e-06], [0, 5, 80, 1, 0, 800, 0, 0, 9, 9.53125769607548e-06], [0, 5, 20, 1, 0, 3200, 1, 1, 8, 9.533282619046316e-06], [0, 5, 20, 1, 0, 800, 1, 1, 11, 9.562452455355921e-06], [0, 5, 160, 1, 1, 3200, 0, 0, 9, 9.563820646552432e-06], [0, 6, 80, 1, 0, 1600, 1, 1, 4, 9.681704000043782e-06], [1, 5, 20, 1, 1, 1, 1, 1, 11, 9.728715049555884e-06]]
    update_stats(population, metric)
    population = [[0, 5, 80, 0, 0, 1600, 0, 0, 2, 8.118599029359119e-06], [0, 5, 80, 0, 0, 1600, 0, 0, 2, 8.118599029359119e-06], [0, 6, 20, 0, 0, 3200, 1, 0, 2, 8.125582640318248e-06], [0, 6, 20, 0, 0, 3200, 1, 0, 2, 8.125582640318248e-06], [0, 6, 40, 0, 0, 3200, 0, 0, 5, 8.140763005780346e-06], [0, 6, 40, 0, 0, 3200, 0, 0, 5, 8.140763005780346e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 80, 0, 1, 1, 1, 1, 7, 8.142036109622456e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 160, 0, 0, 1, 1, 1, 7, 8.145046610838994e-06], [1, 5, 20, 0, 1, 0, 0, 1, 18, 8.176625009243512e-06], [1, 5, 20, 0, 1, 0, 0, 1, 18, 8.176625009243512e-06], [1, 5, 160, 0, 1, 0, 1, 0, 5, 8.199716417563515e-06], [1, 5, 160, 0, 1, 0, 1, 0, 5, 8.199716417563515e-06], [1, 5, 160, 0, 1, 0, 1, 1, 30, 8.215708461135633e-06], [0, 6, 20, 0, 0, 800, 1, 1, 2, 8.218946994584036e-06], [1, 5, 40, 0, 0, 1, 1, 1, 2, 8.301164273126102e-06], [0, 5, 80, 0, 1, 800, 0, 0, 4, 8.32899511892042e-06], [1, 5, 80, 0, 1, 0, 1, 0, 0, 8.682335145089233e-06], [0, 6, 160, 0, 0, 1600, 0, 0, 5, 9.188322389933816e-06], [0, 6, 40, 1, 0, 3200, 1, 0, 8, 9.533282619046316e-06], [0, 5, 20, 1, 0, 3200, 1, 1, 11, 9.562945004186666e-06], [1, 5, 160, 1, 0, 1, 0, 0, 25, 9.678639251763599e-06], [0, 6, 160, 1, 1, 800, 1, 0, 1, 9.797671885860018e-06], [1, 5, 20, 1, 1, 1, 1, 0, 1, 9.946257449801065e-06]]
    update_stats(population, metric)

    calculate_stats()

    if verbose:
        print(stats_string())
    
    directory = "/mnt/d/Users/Miguel/Documents/Engenharia Informática/UC/Ano 5/IoT/cenario_IoT/experimentation/evo/"
    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, "mutate_prob", mutation_prob)
    file_path = directory + filename
    with open(file_path, 'w+') as file:
        file.write(stats_string()+'\n')

if __name__ == "__main__":
    main()
