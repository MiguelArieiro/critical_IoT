from typing import List
import subprocess
import random
import sys
import copy
import numpy
import concurrent.futures
import time
from math import sqrt

__author__ = 'Miguel Arieiro'

verbose = True

#parameters
pop_size = 20
number_generations = 2
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

# {num_cen: [numAp, numSta, duration, dataRate]}
scenario = {0: [2, 4, 10, 100000], 1: [12, 16, 30, 100000], 2: [12, 64, 60, 100000]}

# [0 - 802.11ax, 1 - 802.11n]
technology = [0, 1]

# {technology: [2.4GHz, 5GHz, 6GHz]}
frequency = {0: [5, 6], 1: [5]}

# {frequency: [20MHz, 40MHz, 80MHz, 160 MHz]}
channelWidth = {2 : [20,40], 5 : [20, 40, 80, 160], 6 : [20, 40, 80, 160]}

# {technology: [800ns, 1600ns, 3200ns]}
guardInterval = {0: [800, 1600, 3200], 1: [0, 1]}

# {technology: [mcs#]}
mcs = {0: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], 1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]}

#indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]
param = [technology, frequency, channelWidth, [0, 1], [0, 1], guardInterval, [0, 1], [0, 1], mcs]

cmd_str ="wifi-spatial-reuse-modified -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -guardInterval=%d -enableObssPd=%d -useExtendedBlockAck=%d -mcs=%d"

def gen_indiv():
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]
    global param

    #param[][random.randint(0, len(param[]) - 1)]
    tech = param[0][random.randint(0, len(param[0]) - 1)]
    freq = param[1][tech][random.randint(0, len (param[1][tech]) - 1)]

    indiv = [tech, freq, param[2][freq][random.randint(0, len(param[2][freq]) - 1)], param[3][random.randint(0, len(param[3]) - 1)], param[4][random.randint(0, len(param[4]) - 1)], param[5][tech][random.randint(0, len(param[5][tech]) - 1)], param[6][random.randint(0, len(param[6]) - 1)], param[7][random.randint(0, len(param[7]) - 1)], param[8][tech][random.randint(0, len(param[8][tech]) - 1)], -1]
    return indiv

def heuristic (energy, throughput, minimum_throughput=0.0):
    if (throughput < minimum_throughput):
        return sys.maxsize
    return (energy/throughput)

def run_indiv (indiv, scen):
    global minimum_throughput
    global cmd_str
    command = cmd_str % tuple(scen + indiv[0:-1])
    result = subprocess.run(['./waf','--run-no-build', command], capture_output=True, text=True)

    try:
        res = result.stdout.splitlines()[1].split()
        energy = float(res[0])
        throughput = float(res[1])
    except:
        print ("*********************************Errror*********************************:\n"+command)
        print (result)
        energy=1.0
        throughput=1.0

    if verbose:
        print ("energy: " + str(energy) + "\tthroughput: " + str(throughput))
    
    return heuristic(energy, throughput, minimum_throughput)

def run_all (population, current_scen):
    
    res=list()
    with concurrent.futures.ThreadPoolExecutor() as executor:
        
        for i in range (len(population)):
            if (population[i][-1]==(-1)):
                f = executor.submit(run_indiv, population[i], current_scen)
                res.append(f)
                #  = run_indiv (population[i], current_scen)
    count=0
    for i in range (len(population)):
        if (population[i][-1]==(-1)):
            population[i][-1] = res[count].result()
            count+=1
    
    return population

def gen_population(pop_size, current_scen):
    population=[]
    for i in range (pop_size):
        indiv = gen_indiv()
        population.append(indiv)
    
    population = run_all(population, current_scen)
    return population

def gen_new_population(parents, offspring, current_scen):
    #TODO test
    global elite_per
    global random_per
    size = len(parents)
    comp_elite = int(size * elite_per)
    comp_random = int(size * random_per)
    offspring.sort(key=lambda x: x[-1])
    parents.sort(key=lambda x: x[-1])
    new_population = copy.deepcopy(parents[:comp_elite]) + offspring[:size - comp_elite - comp_random] + gen_population (comp_random, current_scen)
    return new_population

def mutate_prob(original_indiv):    #TODO test
    global param
    global mutation_prob
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck]
    indiv = original_indiv
    indiv [-1] = -1
    
    for i in range(len(indiv) - 1):
        if random.random() < mutation_prob:
            if ((i==1) and (len(param[1][indiv[0]])==1)): #case tech=1 == 802.11n, and there's only 5GHz
                continue

            #frequency and channel width
            if (i == 1) or (i == 2):
                temp=indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][indiv[i-1]][random.randint(0, len(param[i][indiv[i-1]]) - 1)]
                indiv[i] = temp

            #guardInterval and mcs
            elif (i == 5) or (i == 8):
                temp=indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][indiv[0]][random.randint(0, len(param[i][indiv[0]]) - 1)]
                indiv[i] = temp

            #other
            else:
                temp=indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][random.randint(0, len(param[i]) - 1)]
                indiv[i] = temp

            #dependent parameters
            if indiv[1] not in param[1][indiv[0]]:
                indiv[1] = param[1][indiv[0]][random.randint(0, len(param[1][indiv[0]]) - 1)]
            if indiv[2] not in param[2][indiv[1]]:
                indiv[2] = param[2][indiv[1]][random.randint(0, len(param[2][indiv[1]]) - 1)]
            if indiv[5] not in param[5][indiv[0]]:
                indiv[5] = param[5][indiv[0]][random.randint(0, len(param[5][indiv[0]]) - 1)]
            if indiv[8] not in param[8][indiv[0]]:
                indiv[8] = param[8][indiv[0]][random.randint(0, len(param[8][indiv[0]]) - 1)]
    
    return indiv

def mutate_one(original_indiv):    #TODO test
    global param
    # indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]
    indiv = original_indiv
    indiv [-1] = -1

    i=random.randint (0,len(original_indiv) - 2)

    while ((i==1) and (len(param[1][indiv[0]])==1)): #case tech=1 == 802.11n, and there's only 5GHz
        i=random.randint (0,len(original_indiv) - 2)
    
    # frequency and channel width
    if (i == 1) or (i == 2):
        temp=indiv[i]
        while (temp == indiv[i]):
            temp = param[i][indiv[i-1]][random.randint(0, len(param[i][indiv[i-1]]) - 1)]
        indiv[i] = temp

    #guardInterval and mcs
    elif (i == 5) or (i == 8):
        temp=indiv[i]
        while (temp == indiv[i]):
            temp = param[i][indiv[0]][random.randint(0, len(param[i][indiv[0]]) - 1)]
        indiv[i] = temp

    #other
    else:
        temp=indiv[i]
        while (temp == indiv[i]):
            temp = param[i][random.randint(0, len(param[i]) - 1)]
        indiv[i] = temp

    #dependent parameters
    if indiv[1] not in param[1][indiv[0]]:
        indiv[1] = param[1][indiv[0]][random.randint(0, len(param[1][indiv[0]]) - 1)]
    if indiv[2] not in param[2][indiv[1]]:
        indiv[2] = param[2][indiv[1]][random.randint(0, len(param[2][indiv[1]]) - 1)]
    if indiv[5] not in param[5][indiv[0]]:
        indiv[5] = param[5][indiv[0]][random.randint(0, len(param[5][indiv[0]]) - 1)]
    if indiv[8] not in param[8][indiv[0]]:
        indiv[8] = param[8][indiv[0]][random.randint(0, len(param[8][indiv[0]]) - 1)]
    
    return indiv

def mutate_all(population, mutation_op=mutate_one):
    offspring=[]
    for i in range(len(population)):
        offspring.append(mutation_op(population[i]))
    
    return offspring

def hamming_distance(v1, v2):
    return sum([l for i in range (len(vec_1)) if vec1])

def hamming_distance(v1, v2):
    pairs = list(zip(v1, v2))
    return sqrt(sum([(pair[0] - pair[1])**2 for pair in pairs]))

def update_stats(population, metric=hamming_distance):

    global best_per_gen
    global avg_per_gen
    global diversity
    global pop_size

    population.sort(key=lambda x: x[-1])
    best_per_gen.append(population[0][-1])
    avg_per_gen.append(sum([f[-1] for f in population]) / len(population))
    
    div=0
    for i in range (len(population)):
        for j in range (i+1, pop_size):
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

def print_stats():
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

    string = 'max: %f -> gen: %d/%d' % (maxFit, genMax, number_generations)
    string += '\nmin: %f -> gen: %d/%d' % (minFit, genMin, number_generations)
    string += '\navgBest: %f +- %f' % (avgBest, stdBest)
    string += '\navgGen: %f +- %f' % (avgGen, stdGen)
    string += '\nAvgDiversity: %f +- %f' % (avgDiversity, stdDiversity)
    print (string)
    return

def main ():
    global pop_size
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput
    global scenario

    current_scen = scenario[0]

    # gen original population
    population = gen_population(pop_size, current_scen) 

    #update_stats(population)

    if verbose:
        print(population)
    #TODO add scenarios
    
    for run in range(number_generations):
        if verbose:
            print("run: %d" % run)

        offspring = mutate_all(population, mutate_one)
        run_all (offspring, current_scen)
        population = gen_new_population (population, offspring, current_scen)
        run_all (population, current_scen)
        update_stats(population, hamming_distance)
        if verbose:
            print(population)
    
    calculate_stats()
    print_stats()

if __name__ == "__main__":
    main()


# pop -> offspring
# pop = top(pop+offspring)

# top(pop) -> offsprig
# pop = top(pop) + offspring

# top(pop) -> offsprig
# pop = top(pop) + offspring + estrangeiros
