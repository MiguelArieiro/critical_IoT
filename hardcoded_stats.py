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
number_generations = 5
runs_per_scen = 5
elite_per = 0.3
random_per = 0.2
minimum_throughput = 0.0
mutation_prob = 0.5


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
    #global scenario
    global runs_per_scen
    global directory
    metric = hamming_distance
    mutation_op = "mutate_prob"

    reset_stats()
    count=0
    num_scen=0

    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, mutation_op, mutation_prob)
    file_path = directory + filename
    file = open(file_path, "w")
    file.write("[technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]\n")


    all_pop=[[[0, 6, 20, 0, 0, 3200, 1, 1, 2, 2.424733994089638e-06], [0, 5, 160, 0, 0, 3200, 0, 1, 1, 2.425035058113279e-06], [0, 5, 160, 0, 0, 3200, 0, 1, 1, 2.425035058113279e-06], [0, 5, 80, 0, 0, 800, 0, 1, 0, 2.4646835583444254e-06], [0, 5, 80, 0, 0, 800, 0, 1, 0, 2.4646835583444254e-06], [0, 6, 80, 1, 0, 800, 0, 0, 4, 2.6968359682532927e-06], [0, 6, 80, 1, 0, 800, 0, 0, 4, 2.6968359682532927e-06], [0, 6, 20, 1, 0, 3200, 1, 0, 4, 2.7089345514427108e-06], [0, 6, 20, 1, 0, 3200, 1, 0, 4, 2.7089345514427108e-06], [0, 6, 20, 1, 0, 1600, 0, 0, 1, 2.7171902859161104e-06], [0, 6, 20, 1, 0, 1600, 0, 0, 1, 2.7171902859161104e-06], [0, 5, 80, 1, 1, 3200, 1, 0, 2, 2.9319161246134012e-06], [0, 5, 80, 1, 1, 3200, 1, 0, 2, 2.9319161246134012e-06], [0, 6, 20, 1, 1, 3200, 0, 1, 2, 2.9339940172504033e-06], [0, 6, 20, 1, 1, 3200, 0, 1, 2, 2.9339940172504033e-06], [0, 5, 40, 1, 1, 1600, 0, 0, 0, 3.0490784233636274e-06], [0, 6, 160, 1, 1, 800, 0, 1, 5, 4.47357597779863e-06], [1, 5, 80, 0, 0, 1, 1, 0, 13, 8.193175786479913e-06], [1, 5, 20, 0, 0, 1, 1, 1, 6, 8.217567653209963e-06], [1, 5, 20, 0, 1, 1, 1, 1, 13, 8.237817922510102e-06], [1, 5, 40, 0, 1, 1, 1, 1, 13, 8.237817922510102e-06], [1, 5, 20, 0, 1, 0, 0, 0, 14, 8.23900641787497e-06], [1, 5, 80, 0, 1, 1, 1, 1, 1, 8.447349655336344e-06], [1, 5, 40, 1, 1, 1, 1, 1, 5, 8.92164329936387e-06], [0, 5, 40, 0, 1, 1600, 1, 1, 6, 9.975300911431155e-06]],
            [[0, 5, 40, 0, 0, 800, 0, 1, 1, 2.409826259873399e-06], [0, 5, 40, 0, 0, 800, 0, 1, 1, 2.409826259873399e-06], [0, 6, 160, 0, 0, 1600, 1, 0, 1, 2.4170509103008984e-06], [0, 6, 160, 0, 0, 1600, 1, 0, 1, 2.4170509103008984e-06], [0, 6, 160, 0, 0, 3200, 0, 0, 1, 2.4239840908910847e-06], [0, 6, 160, 0, 0, 3200, 0, 0, 1, 2.4239840908910847e-06], [0, 5, 80, 0, 0, 1600, 0, 0, 3, 2.495463914112359e-06], [0, 6, 20, 0, 1, 800, 0, 0, 2, 2.5552813190818403e-06], [0, 6, 20, 0, 1, 800, 0, 0, 2, 2.5552813190818403e-06], [0, 6, 80, 0, 1, 3200, 1, 1, 2, 2.5743267333792873e-06], [0, 6, 80, 0, 1, 3200, 1, 1, 2, 2.5743267333792873e-06], [0, 6, 40, 0, 1, 3200, 1, 1, 0, 2.6733053365556042e-06], [0, 6, 40, 0, 1, 3200, 1, 1, 0, 2.6733053365556042e-06], [0, 6, 80, 1, 0, 1600, 1, 1, 4, 2.7006885223578277e-06], [0, 5, 80, 1, 0, 3200, 0, 0, 1, 2.7316567732601435e-06], [0, 5, 80, 1, 0, 3200, 0, 0, 1, 2.7316567732601435e-06], [0, 5, 40, 1, 0, 800, 0, 1, 0, 2.780840453415938e-06], [0, 6, 40, 0, 0, 1600, 1, 1, 5, 3.2363406919560653e-06], [0, 6, 160, 1, 0, 3200, 0, 0, 5, 3.6272946124367276e-06], [1, 5, 80, 0, 0, 0, 1, 1, 19, 8.207707116882293e-06], [1, 5, 40, 0, 0, 0, 0, 0, 9, 8.277016320709872e-06], [1, 5, 40, 0, 0, 0, 0, 0, 2, 8.295688617061731e-06], [1, 5, 20, 0, 1, 0, 0, 1, 1, 8.447349655336344e-06], [1, 5, 20, 1, 0, 0, 0, 1, 2, 8.935142633442267e-06], [0, 6, 80, 0, 0, 3200, 1, 1, 10, 1.0035813678424826e-05]],
            [[0, 6, 20, 0, 0, 800, 0, 0, 3, 2.3922202416320062e-06], [0, 6, 20, 0, 0, 800, 0, 0, 3, 2.3922202416320062e-06], [0, 6, 40, 0, 0, 1600, 0, 0, 2, 2.415460429887735e-06], [0, 6, 160, 0, 0, 800, 0, 0, 0, 2.4636380202349922e-06], [0, 6, 160, 0, 0, 800, 0, 0, 0, 2.4636380202349922e-06], [0, 5, 40, 0, 1, 800, 0, 1, 4, 2.5287420073053427e-06], [0, 5, 40, 0, 1, 800, 0, 1, 4, 2.5287420073053427e-06], [0, 5, 40, 1, 0, 800, 0, 1, 2, 2.711688786951656e-06], [0, 5, 40, 1, 0, 800, 0, 1, 2, 2.711688786951656e-06], [0, 6, 160, 1, 0, 800, 1, 0, 0, 2.779626469674597e-06], [0, 6, 160, 1, 0, 800, 1, 0, 0, 2.779626469674597e-06], [0, 6, 40, 1, 0, 1600, 0, 0, 0, 2.786783822309214e-06], [0, 6, 40, 1, 0, 1600, 0, 0, 0, 2.786783822309214e-06], [0, 6, 20, 1, 1, 800, 1, 1, 2, 2.9126973659018095e-06], [0, 6, 20, 1, 1, 800, 1, 1, 2, 2.9126973659018095e-06], [0, 5, 160, 1, 1, 1600, 1, 0, 1, 2.91825497129775e-06], [0, 5, 20, 1, 1, 1600, 0, 0, 2, 2.9218657577034207e-06], [0, 6, 40, 1, 1, 3200, 1, 0, 2, 2.9319161246134012e-06], [1, 5, 40, 0, 0, 1, 0, 1, 5, 8.217480173977802e-06], [1, 5, 40, 0, 0, 0, 0, 0, 5, 8.217480173977802e-06], [1, 5, 80, 0, 0, 0, 1, 0, 17, 8.251307241324672e-06], [1, 5, 160, 0, 1, 0, 0, 1, 19, 8.253399150608518e-06], [1, 5, 80, 0, 0, 0, 1, 1, 3, 8.27082442046553e-06], [0, 5, 80, 0, 0, 3200, 1, 0, 6, 9.822914492451124e-06], [0, 5, 80, 0, 1, 3200, 1, 0, 10, 1.0352536005009393e-05]],
            [[0, 5, 20, 0, 1, 800, 0, 0, 4, 2.5284842722446715e-06], [0, 5, 20, 0, 1, 800, 0, 0, 4, 2.5284842722446715e-06], [0, 6, 40, 0, 1, 1600, 0, 0, 3, 2.5289398621345376e-06], [0, 6, 40, 0, 1, 1600, 0, 0, 3, 2.5289398621345376e-06], [0, 5, 160, 0, 1, 1600, 0, 1, 0, 2.657697488313129e-06], [0, 5, 20, 1, 0, 1600, 0, 1, 0, 2.7884153061267908e-06], [0, 5, 20, 1, 0, 1600, 0, 1, 0, 2.7884153061267908e-06], [0, 5, 40, 1, 1, 3200, 1, 1, 3, 2.896026546413682e-06], [0, 5, 40, 1, 1, 3200, 1, 1, 3, 2.896026546413682e-06], [0, 6, 40, 1, 1, 800, 0, 1, 0, 3.041384592472821e-06], [0, 6, 40, 1, 1, 800, 0, 1, 0, 3.041384592472821e-06], [1, 5, 40, 0, 0, 1, 0, 1, 5, 8.217480173977802e-06], [1, 5, 40, 0, 0, 1, 0, 1, 5, 8.217480173977802e-06], [1, 5, 160, 0, 0, 1, 1, 0, 18, 8.231715046266954e-06], [1, 5, 160, 0, 0, 1, 1, 0, 18, 8.231715046266954e-06], [1, 5, 160, 0, 0, 0, 1, 1, 3, 8.27082442046553e-06], [1, 5, 80, 0, 1, 1, 0, 1, 0, 8.673803316878363e-06], [1, 5, 20, 1, 1, 1, 1, 1, 5, 8.92164329936387e-06], [1, 5, 160, 1, 0, 1, 1, 0, 2, 8.935142633442267e-06], [1, 5, 20, 1, 0, 0, 0, 1, 2, 8.935142633442267e-06], [1, 5, 20, 1, 1, 1, 1, 0, 2, 9.04733740399549e-06], [0, 5, 160, 0, 0, 1600, 1, 1, 9, 9.81680929520629e-06], [0, 5, 40, 1, 0, 800, 0, 0, 6, 1.0355324410823393e-05], [0, 6, 40, 1, 1, 3200, 1, 1, 6, 1.0582030113471051e-05], [1, 5, 40, 0, 0, 1, 1, 1, 22, 2.894971613949716e-05]],
            [[0, 6, 20, 0, 1, 3200, 0, 1, 3, 2.5446834640678233e-06], [0, 6, 20, 0, 1, 3200, 0, 1, 3, 2.5446834640678233e-06], [0, 6, 20, 0, 1, 800, 0, 1, 0, 2.654286506615958e-06], [0, 5, 40, 1, 0, 1600, 1, 1, 1, 2.7179560279644104e-06], [0, 6, 40, 1, 1, 3200, 0, 0, 3, 2.8960978723404255e-06], [0, 6, 40, 1, 1, 3200, 0, 0, 3, 2.8960978723404255e-06], [0, 6, 160, 1, 0, 1600, 1, 1, 5, 3.7929174134922855e-06], [0, 6, 160, 1, 0, 1600, 1, 1, 5, 3.7929174134922855e-06], [0, 6, 40, 1, 0, 800, 0, 1, 5, 3.994772703143248e-06], [0, 6, 40, 1, 0, 800, 0, 1, 5, 3.994772703143248e-06], [1, 5, 80, 0, 0, 1, 0, 1, 15, 8.193590556171764e-06], [1, 5, 80, 0, 0, 1, 0, 1, 15, 8.193590556171764e-06], [1, 5, 160, 0, 0, 0, 1, 1, 20, 8.198472617227421e-06], [1, 5, 160, 0, 0, 1, 0, 1, 17, 8.251307241324672e-06], [1, 5, 160, 0, 0, 1, 0, 1, 17, 8.251307241324672e-06], [1, 5, 160, 0, 1, 1, 1, 0, 12, 8.251685642524938e-06], [1, 5, 160, 0, 1, 1, 1, 0, 12, 8.251685642524938e-06], [1, 5, 40, 0, 0, 1, 0, 0, 3, 8.27082442046553e-06], [1, 5, 160, 0, 0, 0, 0, 0, 2, 8.295688617061731e-06], [1, 5, 20, 0, 0, 1, 0, 1, 8, 8.381029346517082e-06], [1, 5, 80, 0, 0, 0, 0, 0, 0, 8.564416448775851e-06], [1, 5, 40, 0, 1, 0, 0, 1, 0, 8.673803316878363e-06], [1, 5, 40, 1, 0, 0, 0, 1, 4, 8.86704538201315e-06], [1, 5, 80, 1, 0, 1, 0, 1, 8, 9.026547919441384e-06], [0, 6, 80, 0, 1, 800, 0, 0, 6, 9.96641271829124e-06]]]

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
    
    directory = "/mnt/d/Users/Miguel/Documents/Engenharia Informática/UC/Ano 5/IoT/cenario_IoT/experimentation/evo/"
    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (pop_size, number_generations, runs_per_scen, elite_per, random_per, "mutate_prob", mutation_prob)
    file_path = directory + filename
    with open(file_path, 'w+') as file:
        file.write(stats_string()+'\n')

if __name__ == "__main__":
    main()
