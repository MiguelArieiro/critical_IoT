from typing import List
import subprocess
import random
import sys
import copy
import numpy
import concurrent.futures
from math import sqrt

__author__ = 'Miguel Arieiro'

#directory = "/home/ubuntu/critical_iot/tests/"
directory = ""
verbose = True

#parameters
pop_size = 25
number_generations = 20
runs_per_scen = 5
elite_per = 0.3
random_per = 0.2
minimum_throughput = 0.0
mutation_prob = 0.3


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
worst_per_gen = list()

def hamming_distance(v1, v2):
    return sum([1 for i in range (len(v1)-1) if v1[i] != v2 [i]]) # heuristic = v[-1]

def euclidean_distance(v1, v2):
    pairs = list(zip(v1[:-1], v2[:-1]))
    return sqrt(sum([(pair[0] - pair[1])**2 for pair in pairs])) # heuristic = v[-1]

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
    global worst_per_gen

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
    worst_per_gen = list ()
 
def update_stats(population, metric=hamming_distance):

    global best_per_gen
    global avg_per_gen
    global diversity
    global worst_per_gen
    p_size = len(population)

    population.sort(key=lambda x: x[-1])
    best_per_gen.append(population[0][-1])
    worst_per_gen.append(population[-1])
    avg_per_gen.append(sum([f[-1] for f in population]) / p_size)
    
    count=0
    for i in range (p_size):
        for j in range (i+1, p_size):
            distance = metric(population[i], population[j])
            if distance !=0:
                count+=1
    
    div = (2.0*count)/(p_size*(p_size-1))
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
    global worst_per_gen
    global worst
    
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

    worst_per_gen.sort(key=lambda x: x[-1])


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
    
    string = '\nMédia: {}'.format(str(avg_per_gen))
    string += '\nDiversidade: {}'.format(str(diversity))
    string += '\nMáximo do melhor: {} -> geração: {}/{}'.format(maxFit, genMax, number_generations)
    string += '\nMínimo do melhor: {} -> geração: {}/{}'.format(minFit, genMin, number_generations)
    string += '\nMédia do melhor: {} +- {}'.format(avgBest, stdBest)
    string += '\nMédia geracional: {} +- {}'.format(avgGen, stdGen)
    string += '\nDiversidade média: {} +- {}'.format(avgDiversity, stdDiversity)

    string += '\nPior: {}'.format(str(worst_per_gen[-1]))
    return string
    
def main():
    global pop_size
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput
    #global scenario
    global runs_per_scen
    global directory
    global metric
    global mutation_op
    metric = hamming_distance
    mutation_op = "mutate_one"
    mutation_prob = 0.30


    all_pop=[
        [[0, 6, 160, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 5, 160, 0, 0, 1600, 0, 0, 3, 0.948224, 104911.0, 0.354138, 1.2239194659397013e-05], [0, 5, 80, 0, 0, 800, 1, 1, 0, 1.01681, 87103.4, 0.423407, 1.6616279865883538e-05], [0, 5, 80, 0, 0, 800, 1, 1, 0, 1.01681, 87103.4, 0.423407, 1.6616279865883538e-05], [1, 5, 160, 0, 0, 0, 1, 0, 4, 3.24472, 60769.4, 0.538524, 1.6616279865883538e-05], [0, 6, 80, 1, 0, 800, 1, 0, 4, 0.996953, 82898.5, 0.552111, 1.8665979695446843e-05], [1, 5, 20, 0, 0, 0, 0, 0, 20, 3.21689, 93775.7, 0.392413, 4.776546008795455e-05], [0, 6, 160, 1, 0, 1600, 1, 1, 11, 0.952689, 10413.3, 0.939599, 8.214791643952383e-05]],
        [[0, 6, 160, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 160, 0, 0, 3200, 0, 0, 5, 0.940847, 92034.5, 0.485545, 1.5186376376413194e-05], [0, 5, 80, 0, 0, 800, 1, 1, 5, 0.936604, 89668.7, 0.518153, 1.5857352369466712e-05], [0, 5, 80, 0, 0, 800, 1, 1, 5, 0.936604, 89668.7, 0.518153, 1.5857352369466712e-05], [0, 6, 80, 0, 1, 800, 1, 0, 4, 9223372036854775807, -1.0, 1.0, 1.5857352369466712e-05], [0, 5, 80, 1, 0, 1600, 1, 0, 11, 0.950957, 10400.0, 0.939982, 0.00017738840988211542], [0, 6, 160, 0, 0, 1600, 1, 1, 11, 9223372036854775807, -1.0, 1.0, 9223372036854775807], [0, 6, 80, 1, 0, 800, 0, 0, 4, 0.996953, 82898.5, 0.552111, 9223372036854775807]],
        [[0, 6, 160, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [1, 5, 20, 0, 0, 0, 1, 1, 5, 3.23947, 65530.6, 0.529591, 7.56145092028762e-05], [0, 6, 20, 1, 0, 800, 0, 1, 8, 0.926534, 10415.3, 0.939395, 0.00017252651454398817], [0, 6, 20, 0, 1, 800, 1, 0, 4, 9223372036854775807, -1.0, 1.0, 9223372036854775807], [0, 6, 20, 0, 1, 800, 1, 0, 4, 9223372036854775807, -1.0, 1.0, 9223372036854775807], [0, 6, 20, 0, 1, 800, 1, 0, 4, 9223372036854775807, -1.0, 1.0, 9223372036854775807], [0, 6, 20, 0, 1, 800, 1, 0, 4, 9223372036854775807, -1.0, 1.0, 9223372036854775807]],
        [[0, 6, 160, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 20, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 80, 0, 0, 800, 0, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 20, 0, 0, 800, 1, 0, 4, 0.942041, 115602.0, 0.284979, 1.047129722789398e-05], [0, 6, 160, 0, 0, 800, 1, 1, 4, 0.944431, 116517.0, 0.298624, 1.0526024210578712e-05], [1, 5, 20, 0, 0, 0, 0, 1, 5, 3.23947, 65530.6, 0.529591, 7.56145092028762e-05], [0, 6, 20, 1, 1, 800, 0, 1, 10, 0.963608, 10383.0, 0.939943, 0.00018003896699836272], [0, 6, 160, 0, 1, 800, 0, 1, 5, 9223372036854775807, -1.0, 1.0, 9223372036854775807]]
    ]
    reset_stats()
    count=0
    num_scen=0

    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, mutation_op, mutation_prob)
    file_path = directory + filename
    file = open(file_path, "w")
    file.write("[technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]\n")


    run = 0
    for population in all_pop:
        

        if (count == runs_per_scen):
            num_scen+=1
            #current_scen=scenario[num_scen]
            count=0
            print("Scenario: %d" % num_scen)

        if verbose:
            run += 1
            print("Run: %d\t Scenario: %d" % (run, num_scen))

        if verbose:
            print(population)

        update_stats(population, metric)
        file.write(str(population)+'\n')
        count+=1

    calculate_stats()

    if verbose:
        print(stats_string())
    
    file.write(stats_string()+'\n')

    file.close()
if __name__ == "__main__":
    main()
