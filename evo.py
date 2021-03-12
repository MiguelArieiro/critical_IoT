from typing import List
import subprocess
import random
import sys

#from operator import itemgetter
__author__ = 'Miguel Arieiro'

verbose = True

#parameters
pop_size = 10
mutation_prob = 0.1
number_generations = 5
elite_per = 0.3
random_per = 0.2
minimum_throughput = 0.5

#indiv = [technology, frequency, channelWidth, useUDP, useRts, useExtendedBlockAck, guardInterval]

# {num_cen: [numAp, numSta, duration, dataRate]}
scenario = {0: [2, 4, 10, 20], 1: [12, 16, 30, 20], 2: [12, 64, 60, 20]}

# [0 - 802.11ax, 1 - 802.11n]
technology = [0, 1]

# {technology: [2.4GHz, 5GHz, 6GHz]}
frequency = {0: [2, 5, 6], 1: [2, 5]}

# {frequency: [20MHz, 40MHz, 80MHz, 160 MHz]}
channelWidth = {2 : [20,40], 5 : [20, 40, 80, 160], 6 : [20, 40, 80, 160]}

# [800ns, 1600ns, 3200ns]
guardInterval = [800, 1600, 3200]

cmd_str ="wifi-spatial-reuse-modified -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -useExtendedBlockAck=%d -guardInterval=%s"

def evo():
    global pop_size
    global mutation_prob
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput

    global scenario
    population = gen_population(pop_size, scenario[0])
    print(population)
    #TODO add scenarios
    for run in range(number_generations):
        if verbose:
            print("run: %d" % run)
        offspring = mutate_all(population, mutation_prob)
        run_all (offsprint, scenario[0])
        population = gen_new_population (population, offspring)
    return

def gen_indiv():
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, useExtendedBlockAck, guardInterval]
    global technology
    global frequency
    global channelWidth
    global guardInterval

    tech = technology[random.randint(0, len(technology)-1)]
    freq = frequency[tech][random.randint(0, len(frequency[tech])-1)]

    indiv = [tech, freq, channelWidth[freq][random.randint(0, len(channelWidth[freq])-1)], random.randint(0,1), random.randint(0,1), random.randint(0,1), guardInterval[random.randint(0, len(guardInterval)-1)], sys.maxsize]
    return indiv

def heuristic (energy, real_throughput, minimum_throughput):
    if (real_throughput < minimum_throughput):
        return sys.maxsize
    return (energy/real_throughput)

def run_indiv (indiv, scen):
    global minimum_throughput
    command = cmd_str % tuple(scen + indiv[0:-1])
    result = subprocess.run(['./waf','--run', command])
    result.stdout
    print (result)
    #TODO parse stats
    energy=10
    real_throughput=15
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

def gen_new_population(parents, offspring, elite_per, random_per, current_scen = scenario[0]):
    #TODO
    size = len(parents)
    comp_elite = int(size * elite_per)
    comp_random = int(size * random_per)
    offspring.sort(key=lambda x: x[-1])
    parents.sort(key=lambda x: x[-1])
    new_population = copy.deepcopy(parents[:comp_elite]) + offspring[:size - comp_elite - comp_random] + gen_population (comp_random, current_scen)
    return new_population

def mutate(indiv, mutation_prob):
    #TODO adapt
    if random.random() < mutation_prob:
        i = j = 0
        
        

def mutate_all(population, mutation_prob):
    #TODO add run indiv
    for i in range (len(population)):
        population[i] = mutate(population[i], mutation_prob)
    

def main ():
    evo()

if __name__ == "__main__":
    main()