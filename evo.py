from typing import List
import subprocess
import random
import sys
import copy

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

#indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck]

# {num_cen: [numAp, numSta, duration, dataRate]}
scenario = {0: [2, 1, 10, 20], 1: [12, 16, 30, 20], 2: [12, 64, 60, 20]}

# [0 - 802.11ax, 1 - 802.11n]
technology = [0, 1]

# {technology: [2.4GHz, 5GHz, 6GHz]}
frequency = {0: [2, 5, 6], 1: [2, 5]}

# {frequency: [20MHz, 40MHz, 80MHz, 160 MHz]}
channelWidth = {2 : [20,40], 5 : [20, 40, 80, 160], 6 : [20, 40, 80, 160]}

# {technology: [800ns, 1600ns, 3200ns]}
guardInterval = {0: [800, 1600, 3200], 1: [0, 1]}

param = [technology, frequency, channelWidth, [0, 1], [0, 1], guardInterval, [0, 1], [0, 1]]

mcs 

cmd_str ="wifi-spatial-reuse-modified -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -guardInterval=%d -enableObssPd=%d -useExtendedBlockAck=%d"



def gen_indiv():
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck]
    global param

    #param[][random.randint(0, len(param[]) - 1)]
    tech = param[0][random.randint(0, len(param[0]) - 1)]
    freq = param[1][tech][random.randint(0, len (param[1][tech]) - 1)]


    indiv = [tech, freq, param[2][freq][random.randint(0, len(param[2][freq]) - 1)], param[3][random.randint(0, len(param[3]) - 1)], param[4][random.randint(0, len(param[4]) - 1)], param[5][tech][random.randint(0, len(param[5][tech]) - 1)], param[6][random.randint(0, len(param[6]) - 1)], param[7][random.randint(0, len(param[7]) - 1)], sys.maxsize]
    return indiv

def heuristic (energy, real_throughput, minimum_throughput):
    if (real_throughput < minimum_throughput):
        return sys.maxsize
    return (energy/real_throughput)

def run_indiv (indiv, scen):
    global minimum_throughput
    global cmd_str
    command = cmd_str % tuple(scen + indiv[0:-1])
    result = subprocess.run(['./waf','--run', command])
    result.stdout
    print (result)
    #TODO parse stats
    energy=10 #remove
    real_throughput=15 #remove
    return heuristic(energy, real_throughput, minimum_throughput)

def run_all (population, current_scen):

    for i in range (len(population)):
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

def mutate(original_indiv, mutation_prob):    #TODO test
    global param
    indiv = original_indiv
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck]
    for i in range(len(indiv) - 1):
        if random.random() < mutation_prob:

            #frequency and channel width
            if (i == 1) or (i == 2):
                temp=indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][indiv[i-1]][random.randint(0, len(param[i][indiv[i-1]]) - 1)]
                indiv[i] = temp

            #guardInterval
            elif i == 5:
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
    
    return indiv

def mutate_all(population, mutation_prob):
    offspring=[];
    for i in range(len(population)):
        offspring.append(mutate(population[i], mutation_prob))
    return offspring

def main ():
    global pop_size
    global mutation_prob
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput

    global scenario
    population = gen_population(pop_size, scenario[0])
    if verbose:
        print(population)
    #TODO add scenarios
    for run in range(number_generations):
        if verbose:
            print("run: %d" % run)
        offspring = mutate_all(population, mutation_prob)
        run_all (offspring, scenario[0])
        population = gen_new_population (population, offspring)
        if verbose:
            print(population)

if __name__ == "__main__":
    main()