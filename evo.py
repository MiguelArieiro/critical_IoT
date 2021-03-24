from typing import List
import subprocess
import random
import sys
import copy
import numpy

#from operator import itemgetter
__author__ = 'Miguel Arieiro'

verbose = True

#parameters
pop_size = 10
mutation_prob = 0.1
number_generations = 1
elite_per = 0.3
random_per = 0.2
minimum_throughput = 0.5

mutation_operator=0


best_per_gen = list()
avg_per_gen = list()
diversity = list()
nGen = 0
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
scenario = {0: [2, 4, 10, 1000000], 1: [12, 16, 30, 1000000], 2: [12, 64, 60, 1000000]}

# [0 - 802.11ax, 1 - 802.11n]
technology = [0, 1]

# {technology: [2.4GHz, 5GHz, 6GHz]}
frequency = {0: [5, 6], 1: [5]}

# {frequency: [20MHz, 40MHz, 80MHz, 160 MHz]}
channelWidth = {2 : [20,40], 5 : [20, 40, 80, 160], 6 : [20, 40, 80, 160]}

# {technology: [800ns, 1600ns, 3200ns]}
guardInterval = {0: [800, 1600, 3200], 1: [0, 1]}

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


    indiv = [tech, freq, param[2][freq][random.randint(0, len(param[2][freq]) - 1)], param[3][random.randint(0, len(param[3]) - 1)], param[4][random.randint(0, len(param[4]) - 1)], param[5][tech][random.randint(0, len(param[5][tech]) - 1)], param[6][random.randint(0, len(param[6]) - 1)], param[7][random.randint(0, len(param[7]) - 1)], param[8][tech][random.randint(0, len(param[8][tech]) - 1)], sys.maxsize]
    return indiv

def heuristic (energy, throughput, minimum_throughput=0):
    if (throughput < minimum_throughput):
        return sys.maxsize
    return (energy/throughput)

def run_indiv (indiv, scen):
    global minimum_throughput
    global cmd_str
    command = cmd_str % tuple(scen + indiv[0:-1])
    result = subprocess.run(['./waf','--run-no-build', command], capture_output=True, text=True)

    print (result)
    res = result.stdout.splitlines()[1].split()
    energy = float(res[0])
    throughput = float(res[1])

    if verbose:
        print ("energy: " + str(energy) + "\tthroughput: " + str(throughput))
    #TODO parse stats
    return heuristic(energy, throughput, minimum_throughput)

def run_all (population, current_scen):

    for i in range (len(population)):
        if (population[i][-1]==(-1)):
            population[i][-1] = run_indiv (population[i], current_scen)

def gen_population(pop_size, current_scen = scenario[0]):
    population=[]
    for i in range (pop_size):
        indiv = gen_indiv()
        indiv [-1] = run_indiv(indiv, current_scen)
        population.append(indiv)
    return population

def gen_new_population(parents, offspring, current_scen = scenario[0]):
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

    indiv = original_indiv
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck]
    
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

    indiv = original_indiv
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]

    
    i=random.randint (0,len(original_indiv) - 2)

    while ((i==1) and (len(param[1][indiv[0]])==1)): #case tech=1 == 802.11n, and there's only 5GHz
        i=random.randint (0,len(original_indiv) - 2)
    
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

def mutate_all(population):
    offspring=[];
    for i in range(len(population)):
        offspring.append(mutate_one(population[i]))
    return offspring

def update_stats(population):

    global best_per_gen
    global avg_per_gen
    global diversity

    population.sort(key=lambda x: x[-1])
    best_per_gen.append(population[0][-1])
    avg_per_gen.append(sum([f[-1] for f in population]) / len(population))
    #diversity.append(len(set(tuple(i.permutation) for i in population)) / len(population))

def calculate_stats():

    global best_per_gen
    global avg_per_gen
    global diversity
    global nGen
    global avgBest
    global stdBest
    global avgGen
    global stdGen
    global avgDiversity
    global stdDiversity
    global maxFit
    global minFit
    global genMin
    global genMax

    nGen = len(best_per_gen)
    avgBest = numpy.mean(best_per_gen)
    stdBest = numpy.std(best_per_gen)
    avgGen = numpy.mean(avg_per_gen)
    stdGen = numpy.std(avg_per_gen)
    # avgDiversity = numpy.mean(diversity)
    # stdDiversity = numpy.std(diversity)
    maxFit = max(best_per_gen)
    minFit = min(best_per_gen)
    genMax = best_per_gen.index(maxFit)
    genMin = best_per_gen.index(minFit)

def main ():
    global pop_size
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput

    global scenario
    population = gen_population(pop_size, scenario[0])

    update_stats(population) #original gen
    if verbose:
        print(population)
    #TODO add scenarios
    for run in range(number_generations):
        if verbose:
            print("run: %d" % run)

        offspring = mutate_all(population)
        run_all (offspring, scenario[0])
        population = gen_new_population (population, offspring)

        update_stats(population)
        if verbose:
            print(population)
    
    calculate_stats()
# pop -> offspring
# pop = top(pop+offspring)

# top(pop) -> offsprig
# pop = top(pop) + offspring

# top(pop) -> offsprig
# pop = top(pop) + offspring + estrangeiros

if __name__ == "__main__":
    main()