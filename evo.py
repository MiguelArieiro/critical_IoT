from concurrent.futures import thread
from typing import List
import subprocess
import random
import sys
import copy
import numpy
import concurrent.futures
import pickle
from math import sqrt

__author__ = 'Miguel Arieiro'

directory = "../../tests/"

verbose = True
seed = 1
restart = 0
max_threads = None

# parameters
pop_size = [25, 10, 10]
number_generations = 40
runs_per_scen = [25, 15, 5]
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

# {num_cen: [numAp, numSta, duration, dataRate]}
scenario = {0: [4, 4, 10, 100000], 1: [
    9, 16, 3, 100000], 2: [4, 32, 3, 100000]}

# [0 - 802.11ax, 1 - 802.11n]
technology = [0, 1]

# {technology: [2.4GHz, 5GHz, 6GHz]}
frequency = {0: [5, 6], 1: [5]}

# {frequency: [20MHz, 40MHz, 80MHz, 160 MHz]}
channelWidth = {2: [20, 40], 5: [20, 40, 80, 160], 6: [20, 40, 80, 160]}

# {technology: [800ns, 1600ns, 3200ns]}
guardInterval = {0: [800, 1600, 3200], 1: [0, 1]}

# {technology: [mcs#]}
mcs = {0: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], 1: [0, 1, 2, 3, 4, 5,
                                                      6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]}

#param = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]
param = [technology, frequency, channelWidth, [0, 1],
         [0, 1], guardInterval, [0, 1], [0, 1], mcs]

#indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs, energy, throughput, packet_loss, fitness]

cmd_str = "evoMCS -runs=3 -verbose=0 -tracing=0 -seed=%d -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -guardInterval=%d -enableObssPd=%d -useExtendedBlockAck=%d -mcs=%d"


def gen_indiv():
    global param

    # param[][random.randint(0, len(param[]) - 1)]
    tech = param[0][random.randint(0, len(param[0]) - 1)]
    freq = param[1][tech][random.randint(0, len(param[1][tech]) - 1)]

    indiv = [tech, freq, param[2][freq][random.randint(0, len(param[2][freq]) - 1)], param[3][random.randint(0, len(param[3]) - 1)], param[4][random.randint(0, len(param[4]) - 1)], param[5][tech][random.randint(
        0, len(param[5][tech]) - 1)], param[6][random.randint(0, len(param[6]) - 1)], param[7][random.randint(0, len(param[7]) - 1)], param[8][tech][random.randint(0, len(param[8][tech]) - 1)], -1, -1, -1, -1]
    return indiv


def heuristic_v1(energy, throughput, packetLoss=0.0, minimum_throughput=0.0):
    if (throughput <= minimum_throughput):
        return sys.maxsize
    return (energy/throughput)


def heuristic_v2(energy, throughput, packetLoss=0.0, minimum_throughput=0.0):
    if (throughput <= minimum_throughput):
        return sys.maxsize
    return (energy/throughput*(1.0+packetLoss))


def run_indiv(indiv, scen):
    global minimum_throughput
    global cmd_str
    command = cmd_str % tuple([1] + scen + indiv[0:-4])
    result = subprocess.run(
        ['./waf', '--run-no-build', command], capture_output=True, text=True)

    try:
        res = result.stdout.splitlines()[1].split()
        energy = float(res[0])
        throughput = float(res[1])
        packet_loss = float(res[2])
    except:
        command = cmd_str % tuple([3] + scen + indiv[0:-4])
        result = subprocess.run(
            ['./waf', '--run-no-build', command], capture_output=True, text=True)
        try:
            res = result.stdout.splitlines()[1].split()
            energy = float(res[0])
            throughput = float(res[1])
            packet_loss = float(res[2])
        except:
            print(
                "*********************************Errror*********************************:\n"+command)
            print(result)
            energy = sys.maxsize
            throughput = - 1.0
            packet_loss = 1.0

    indiv[-4] = energy
    indiv[-3] = throughput
    indiv[-2] = packet_loss

    if verbose:
        print("energy: " + str(energy) + "\tthroughput: " +
              str(throughput) + "\tpacket_loss: " + str(packet_loss))

    return heuristic_v2(energy, throughput, packet_loss, minimum_throughput)


def run_all(population, current_scen, all=False):

    global max_threads

    res = list()
    if all == True:
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
            for i in range(len(population)):
                f = executor.submit(run_indiv, population[i], current_scen)
                res.append(f)
        for i in range(len(population)):
            population[i][-1] = res[i].result()

    else:
        count = 0
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
            for i in range(len(population)):
                if (population[i][-1] == (-1)):
                    f = executor.submit(run_indiv, population[i], current_scen)
                    res.append(f)

        for i in range(len(population)):
            if (population[i][-1] == (-1)):
                population[i][-1] = res[count].result()
                count += 1

    return population


def gen_population(pop_size, current_scen):
    population = []
    for _ in range(pop_size):
        indiv = gen_indiv()
        population.append(indiv)

    population = run_all(population, current_scen)
    return population


def gen_new_population(parents, offspring, current_scen):
    global elite_per
    global random_per
    size = len(parents)
    comp_elite = int(size * elite_per)
    comp_random = int(size * random_per)
    offspring.sort(key=lambda x: x[-1])
    parents.sort(key=lambda x: x[-1])
    new_population = copy.deepcopy(parents[:comp_elite]) + offspring[:size -
                                                                     comp_elite - comp_random] + gen_population(comp_random, current_scen)
    return new_population


def mutate_prob(original_indiv):
    global param
    global mutation_prob
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs, energy, throughput, packet_loss, fitness]
    indiv = original_indiv
    indiv[-4:] = [-1, -1, -1, -1]

    for i in range(len(indiv) - 5):
        if random.random() < mutation_prob:
            # case tech=1 == 802.11n, and there's only 5GHz
            if ((i == 1) and (len(param[1][indiv[0]]) == 1)):
                continue

            # frequency and channel width
            if (i == 1) or (i == 2):
                temp = indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][indiv[i-1]
                                    ][random.randint(0, len(param[i][indiv[i-1]]) - 1)]
                indiv[i] = temp

            # guardInterval and mcs
            elif (i == 5) or (i == 8):
                temp = indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][indiv[0]][random.randint(
                        0, len(param[i][indiv[0]]) - 1)]
                indiv[i] = temp

            # other
            else:
                temp = indiv[i]
                while (temp == indiv[i]):
                    temp = param[i][random.randint(0, len(param[i]) - 1)]
                indiv[i] = temp

            # dependent parameters
            if indiv[1] not in param[1][indiv[0]]:
                indiv[1] = param[1][indiv[0]][random.randint(
                    0, len(param[1][indiv[0]]) - 1)]
            if indiv[2] not in param[2][indiv[1]]:
                indiv[2] = param[2][indiv[1]][random.randint(
                    0, len(param[2][indiv[1]]) - 1)]
            if indiv[5] not in param[5][indiv[0]]:
                indiv[5] = param[5][indiv[0]][random.randint(
                    0, len(param[5][indiv[0]]) - 1)]
            if indiv[8] not in param[8][indiv[0]]:
                indiv[8] = param[8][indiv[0]][random.randint(
                    0, len(param[8][indiv[0]]) - 1)]

    return indiv


def mutate_one(original_indiv):
    global param
    #indiv = [technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs, energy, throughput, packet_loss, fitness]
    indiv = original_indiv
    indiv[-4:] = [-1, -1, -1, -1]

    i = random.randint(0, len(indiv) - 5)

    # case tech=1 == 802.11n, and there's only 5GHz
    while ((i == 1) and (len(param[1][indiv[0]]) == 1)):
        i = random.randint(0, len(indiv) - 5)

    # frequency and channel width
    if (i == 1) or (i == 2):
        temp = indiv[i]
        while (temp == indiv[i]):
            temp = param[i][indiv[i-1]
                            ][random.randint(0, len(param[i][indiv[i-1]]) - 1)]
        indiv[i] = temp

    # guardInterval and mcs
    elif (i == 5) or (i == 8):
        temp = indiv[i]
        while (temp == indiv[i]):
            temp = param[i][indiv[0]][random.randint(
                0, len(param[i][indiv[0]]) - 1)]
        indiv[i] = temp

    # other
    else:
        temp = indiv[i]
        while (temp == indiv[i]):
            temp = param[i][random.randint(0, len(param[i]) - 1)]
        indiv[i] = temp

    # dependent parameters
    if indiv[1] not in param[1][indiv[0]]:
        indiv[1] = param[1][indiv[0]][random.randint(
            0, len(param[1][indiv[0]]) - 1)]
    if indiv[2] not in param[2][indiv[1]]:
        indiv[2] = param[2][indiv[1]][random.randint(
            0, len(param[2][indiv[1]]) - 1)]
    if indiv[5] not in param[5][indiv[0]]:
        indiv[5] = param[5][indiv[0]][random.randint(
            0, len(param[5][indiv[0]]) - 1)]
    if indiv[8] not in param[8][indiv[0]]:
        indiv[8] = param[8][indiv[0]][random.randint(
            0, len(param[8][indiv[0]]) - 1)]

    return indiv


def rank_pop(population, n):
    pop = copy.deepcopy(population)
    pop.sort(reverse=True, key=lambda x: x[-1])
    probs = [(2*i)/(len(pop)*(len(pop) + 1)) for i in range(1, len(pop) + 1)]
    parents = []
    for _ in range(n):
        value = random.uniform(0, 1)
        index = 0
        total = probs[index]
        while total < value:
            index += 1
            total += probs[index]
        parents.append(pop[index])
    return parents


def mutate_all(population, mutation_op=mutate_one):
    global random_per
    global elite_per
    offspring = []

    size = len(population)
    n = size-int(size * elite_per)-int(size * random_per)

    for i in rank_pop(population, n):
        offspring.append(mutation_op(i))

    return offspring


def hamming_distance(v1, v2):
    # heuristic = v[-1]
    return sum([1 for i in range(len(v1)-4) if v1[i] != v2[i]])


def euclidean_distance(v1, v2):
    pairs = list(zip(v1[:-4], v2[:-4]))
    # heuristic = v[-1]
    return sqrt(sum([(pair[0] - pair[1])**2 for pair in pairs]))


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
    p_size = len(population)

    population.sort(key=lambda x: x[-1])
    best_per_gen.append(population[0][-1])
    avg_per_gen.append(sum([f[-1] for f in population]) / p_size)

    count = 0
    for i in range(p_size):
        for j in range(i+1, p_size):
            distance = metric(population[i], population[j])
            if distance != 0:
                count += 1

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

    string = '\nMédia: {}'.format(str(avg_per_gen))
    string += '\nDiversidade: {}'.format(str(diversity))
    string += '\nMáximo do melhor: {} -> geração: {}/{}'.format(
        maxFit, genMax, number_generations)
    string += '\nMínimo do melhor: {} -> geração: {}/{}'.format(
        minFit, genMin, number_generations)
    string += '\nMédia do melhor: {} +- {}'.format(avgBest, stdBest)
    string += '\nMédia geracional: {} +- {}'.format(avgGen, stdGen)
    string += '\nDiversidade média: {} +- {}'.format(
        avgDiversity, stdDiversity)
    return string


def log_file(path, string):
    with open(path, "a+") as file:
        file.write(string+'\n')


def save_random_state(run):
    global directory
    file_path = directory+'random_state_'+str(run)
    with open(file_path, 'wb') as random_state_file:
        pickle.dump(random.getstate(), random_state_file)


def load_random_state(run):
    global directory
    file_path = directory+'random_state_'+str(run)
    with open(file_path, 'rb') as random_state_file:
        random_state = pickle.load(random_state_file)
        random.setstate(random_state)


def main():
    global pop_size
    global number_generations
    global elite_per
    global random_per
    global minimum_throughput
    global scenario
    global runs_per_scen
    global seed
    global restart
    global directory
    global max_threads
    metric = hamming_distance
    mutation_op = mutate_one

    filename = "%d_%d_%d_%.2f_%.2f_%s_%.2f.log" % (
        pop_size[0], number_generations, runs_per_scen[0], elite_per, random_per, mutation_op.__name__, mutation_prob)
    file_path = directory + filename
    reset_stats()

    start = 0

    if restart:
        # set values
        count = 25
        num_scen = 1
        current_scen = scenario[num_scen]
        start = 25
        load_random_state(start-1)
        # set last pop
        population = []
    else:
        random.seed(seed)
        count = 0
        num_scen = 0
        current_scen = scenario[num_scen]
        log_file(
            file_path, "[technology, frequency, channelWidth, useUDP, useRts, guardInterval, enableObssPd, useExtendedBlockAck, mcs]")
        # gen original population
        population = gen_population(pop_size[0], current_scen)

    update_stats(population, metric)

    if verbose:
        print(population)
    log_file(file_path, str(population))

    for run in range(start, number_generations):
        if (count == runs_per_scen[num_scen]):
            count = 0
            num_scen += 1
            current_scen = scenario[num_scen]
            max_threads = 1
            population = population[:pop_size[num_scen]]

            calculate_stats()

            if verbose:
                print(stats_string())
            log_file(file_path, stats_string())

            print("Scenario: %d" % (num_scen))
            log_file(file_path, str(num_scen))

            reset_stats()
            run_all(population, current_scen, all=True)

        if verbose:
            print("Run: %d\t Scenario: %d" % (run, num_scen))

        offspring = mutate_all(population, mutation_op)
        run_all(offspring, current_scen)
        population = gen_new_population(population, offspring, current_scen)
        run_all(population, current_scen)

        update_stats(population, metric)

        if verbose:
            print(population)
        log_file(file_path, str(population))

        save_random_state(run)

        count += 1

    calculate_stats()

    if verbose:
        print(stats_string())

    log_file(file_path, stats_string())


if __name__ == "__main__":
    main()
