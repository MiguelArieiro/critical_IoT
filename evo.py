from typing import List

from Individual import *
from RunStats import *


class TSP:
    indiv_matrix: List[List[int]]

    def __init__(self, num_indiv, max_weight, num_weight max_bias, pop_size, mutation_prob, crossover_prob, tournament_size, number_generations, elite_per, random_per):
        self.num_indiv = num_indiv
        self.max_weight = max_weight
        self.num_weight = num_weight
        self.max_bias = max_bias
        self.pop_size = pop_size
        self.mutation_prob = mutation_prob
        self.crossover_prob = crossover_prob
        self.tournament_size = tournament_size
        self.number_generations = number_generations
        self.elite_per = elite_per
        self.random_per = random_per
        self.gen_problem(num_indiv, max_weight)
        self.initial_population = [Individual(self.indiv_matrix) for _ in range(pop_size)]

    def gen_problem(self, num_indiv, max_weight):
        self.indiv_matrix = [[random.randdouble(-max_weight, max_weight) for i in range (num_weight)] for j in range(num_indiv)]
        for i in range(num_indiv):
            for j in range(num_indiv):
                self.indiv_matrix[j][i] = self.indiv_matrix[i][j]

    def __str__(self):
        return '\n'.join(str(i) for i in self.initial_population)

    def execute(self, verbose=False, method='pmx'):
        population = self.initial_population
        gen_stats = RunStats()
        for run in range(self.number_generations):
            if verbose:
                print("run: %d" % run)
            mate_pool = self.tournament(population)
            offspring = []
            # mutation
            for i in range(0, len(mate_pool), 2):
                crossover = mate_pool[i].crossover(mate_pool[i + 1], self.crossover_prob, method=method)
                offspring.extend(crossover())
            self.mutate_all(offspring)
            population = self.gen_new_population(population, offspring)
            gen_stats.update(population)
        return gen_stats

    def tournament(self, population):
        size_pop = len(population)
        mate_pool = []
        for i in range(size_pop):
            winner = random.sample(population, self.tournament_size)
            winner.sort(key=lambda x: x.fit)
            mate_pool.append(winner[0])
        return mate_pool

    def gen_new_population(self, parents, offspring):
        size = len(parents)
        comp_elite = int(size * self.elite_per)
        comp_random = int(size * self.random_per)
        offspring.sort(key=lambda x: x.fit)
        parents.sort(key=lambda x: x.fit)
        new_population = copy.deepcopy(parents[:comp_elite]) + offspring[:size - comp_elite - comp_random] + [Individual(self.indivMatrix) for _ in range(comp_random)]
        return new_population

    def mutate_all(self, pop):
        for i in pop:
            i.mutate(self.mutation_prob)
